// -*- c-basic-offset: 4 -*-

/** @file NonaStitcherPanel.cpp
 *
 *  @brief implementation of NonaStitcherPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo@mathematik.uni-ulm.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>
#include <errno.h>
#include "panoinc_WX.h"

#include "panoinc.h"

#include "PT/Stitcher.h"

#include "common/wxPlatform.h"
#include "hugin/config_defaults.h"
#include "hugin/RunStitcherFrame.h"
#include "hugin/CommandHistory.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/NonaStitcherPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/MyProgressDialog.h"

//#ifdef __WXMAC__
#include "hugin/MyExternalCmdExecDialog.h"
//#endif

using namespace PT;
using namespace std;
using namespace utils;

//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(NonaStitcherPanel, wxWindow)
    EVT_CHOICE ( XRCID("nona_choice_interpolator"),NonaStitcherPanel::InterpolatorChanged)
    EVT_SPINCTRL(XRCID("nona_jpeg_quality"), NonaStitcherPanel::OnSetQuality)
    EVT_CHECKBOX( XRCID("nona_check_enblend"), NonaStitcherPanel::OnEnblendChanged)
    EVT_CHOICE   ( XRCID("nona_choice_format_final"),NonaStitcherPanel::FileFormatChanged)
END_EVENT_TABLE()


// Define a constructor for the Pano Panel
NonaStitcherPanel::NonaStitcherPanel(wxWindow *parent, Panorama & pano)
    : StitcherPanel(parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(pano),
      updatesDisabled(false)
{

    // loading xrc resources in selfcreated this panel
    wxXmlResource::Get()->LoadPanel ( this, wxT("nona_panel"));

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    // get gui controls
    m_InterpolatorChoice = XRCCTRL(*this, "nona_choice_interpolator",
                                   wxChoice);
    DEBUG_ASSERT(m_InterpolatorChoice);
    m_FormatChoice = XRCCTRL(*this, "nona_choice_format_final", wxChoice);
    DEBUG_ASSERT(m_FormatChoice);
    m_JPEGQualitySpin = XRCCTRL(*this, "nona_jpeg_quality", wxSpinCtrl);
    DEBUG_ASSERT(m_JPEGQualitySpin);
    m_JPEGQualitySpin->PushEventHandler(new TextKillFocusHandler(this));

    m_EnblendCheckBox = XRCCTRL(*this, "nona_check_enblend", wxCheckBox);
    DEBUG_ASSERT(m_EnblendCheckBox);

    UpdateDisplay(pano.getOptions());

    // observe the panorama
    pano.addObserver (this);

    Fit();
    wxSize sz = GetSize();
    SetSizeHints(sz.GetWidth(), sz.GetHeight());

    DEBUG_DEBUG("setting minsize to:" << sz.GetWidth() << "x" << sz.GetHeight());
}


NonaStitcherPanel::~NonaStitcherPanel(void)
{
    DEBUG_TRACE("dtor");
    pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}


void NonaStitcherPanel::panoramaChanged (PT::Panorama &pano)
{
	DEBUG_TRACE("");
    PanoramaOptions opt = pano.getOptions();
    // update all options for dialog and notebook tab
    UpdateDisplay(opt);
    m_oldOpt = opt;
}

void NonaStitcherPanel::UpdateDisplay(const PanoramaOptions & opt)
{
    unsigned int nImages = pano.getNrOfImages();
	
	if (nImages == 0) {
		// disable controls
  		m_InterpolatorChoice->Disable();
  		m_FormatChoice->Disable();
  		m_JPEGQualitySpin->Disable();
  		m_EnblendCheckBox->Disable();
		//
	} else {
		// enable controls
  		if (m_InterpolatorChoice->Enable()) 
		{
  		  m_FormatChoice->Enable();
  		  m_JPEGQualitySpin->Enable();
  		  m_EnblendCheckBox->Enable();
		}
		//

  	  m_InterpolatorChoice->SetSelection(opt.interpolator);

      // translate format
      int format;
      switch (opt.outputFormat) {
      case PanoramaOptions::JPEG:
          format = 0;
          break;
      case PanoramaOptions::PNG:
          format = 1;
          break;
      case PanoramaOptions::TIFF:
          format = 2;
          break;
      case PanoramaOptions::TIFF_m:
          format = 3;
          break;
      case PanoramaOptions::TIFF_multilayer:
          format = 4;
          break;
      default:
          {
              PanoramaOptions opts = pano.getOptions();
              opts.outputFormat = PanoramaOptions::JPEG;
              GlobalCmdHist::getInstance().addCommand(
                  new PT::SetPanoOptionsCmd( pano, opts )
                  );
              format = 0;
          }
      }
      m_FormatChoice->SetSelection(format);

      if (opt.outputFormat == PanoramaOptions::JPEG) {
          m_JPEGQualitySpin->Enable();
      } else {
          m_JPEGQualitySpin->Disable();
      }
      m_JPEGQualitySpin->SetValue(opt.quality);

      if (opt.outputFormat == PanoramaOptions::TIFF) {
      // enable enblend
          m_EnblendCheckBox->Enable();
          m_EnblendCheckBox->SetValue(opt.blendMode == PanoramaOptions::SPLINE_BLEND);
      } else {
          m_EnblendCheckBox->SetValue(false);
          m_EnblendCheckBox->Disable();
      }
	}
}


void NonaStitcherPanel::InterpolatorChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    //Interpolator from PanoramaMemento.h
    int lt = m_InterpolatorChoice->GetSelection();

    opt.interpolator = (vigra_ext::Interpolator) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Interpolator changed to: " << lt )
}

void NonaStitcherPanel::OnEnblendChanged( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();


    if (m_EnblendCheckBox->IsChecked()) {
        opt.blendMode = PanoramaOptions::SPLINE_BLEND;
    } else {
        opt.blendMode = PanoramaOptions::WEIGHTED_BLEND;
    }

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
}

void NonaStitcherPanel::FileFormatChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int format = m_FormatChoice->GetSelection();

    // map to output format
    switch(format) {
    case 0:
        opt.outputFormat = PanoramaOptions::JPEG;
        break;
    case 1:
        opt.outputFormat = PanoramaOptions::PNG;
        break;
    case 2:
        opt.outputFormat = PanoramaOptions::TIFF;
        break;
    case 3:
        opt.outputFormat = PanoramaOptions::TIFF_m;
        break;
    case 4:
        opt.outputFormat = PanoramaOptions::TIFF_multilayer;
        break;
    default:
        DEBUG_ERROR("Unknown output format " << format);
        opt.outputFormat = PanoramaOptions::JPEG;
    }

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_INFO ( PanoramaOptions::getFormatName(opt.outputFormat));
}

void NonaStitcherPanel::Stitch( const Panorama & pano,
                                const PanoramaOptions & options)
{
	// work around a flaw in wxProgresDialog that results in incorrect layout  
	// by pre-allocting sufficient horizontal and vertical space 
    MyProgressDialog pdisp(_("Stitching Panorama"), (wxString((wxChar)' ', 60) + wxT("\n ")), NULL, 
						   wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );

    PanoramaOptions opts = options;
    DEBUG_DEBUG("Stitching to " << opts.outfile);

    UIntSet imgs;
    if (wxConfigBase::Get()->Read(wxT("/General/UseOnlySelectedImages"),
		                          HUGIN_USE_SELECTED_IMAGES))
    {
        // use only selected images.
        imgs = pano.getActiveImages();
    } else {
        fill_set(imgs, 0, pano.getNrOfImages()-1);
    }
    
    bool enblend = m_EnblendCheckBox->IsChecked();
    try {
        if (enblend) {
            // output must be set to TIFF
            DEBUG_ASSERT(opts.outputFormat == PanoramaOptions::TIFF);
            // set output to multiple tiff.
            // hope the next enblend will also contain multilayer support
            opts.outputFormat = PanoramaOptions::TIFF_m;
        }
        // stitch panorama
        PT::stitchPanorama(pano, opts,
                           pdisp, opts.outfile, imgs);
    } catch (std::exception & e) {
        DEBUG_FATAL(_("error during stitching:") << e.what());
        return;
    }

	wxString outpath;
	wxString outname;
	wxString output;
	wxFileName::SplitPath(wxString(opts.outfile.c_str(), *wxConvCurrent), &outpath, &outname, NULL);
    output = outpath + wxFileName::GetPathSeparator() + outname;
    if (enblend) {
        wxConfigBase* config = wxConfigBase::Get();
#ifdef __WXMSW__
        wxString enblendExe = config->Read(wxT("/Enblend/EnblendExe"),wxT(HUGIN_ENBLEND_EXE));
        if (!wxFile::Exists(enblendExe)){
            wxFileDialog dlg(this,_("Select enblend.exe"),
                             wxT(""), wxT("enblend.exe"),
                             wxT("Executables (*.exe)|*.exe"),
                              wxOPEN, wxDefaultPosition);
            if (dlg.ShowModal() == wxID_OK) {
                enblendExe = dlg.GetPath();
                config->Write(wxT("/Enblend/EnblendExe"),enblendExe);
            } else {
                wxLogError(_("No enblend.exe selected"));
                return;
            }
        }
#elif defined __WXMAC__
        wxString enblendExe = config->Read(wxT("/Enblend/EnblendExe"), wxT(HUGIN_ENBLEND_EXE));
        if (!wxFile::Exists(enblendExe)){
            wxFileDialog dlg(this,_("Select enblend commandline tool"),
                             wxT(""), wxT(""),
                             wxT("Any Files |*"),
                             wxOPEN, wxDefaultPosition);
            if (dlg.ShowModal() == wxID_OK) {
                enblendExe = dlg.GetPath();
                config->Write(wxT("/Enblend/EnblendExe"),enblendExe);
            } else {
                wxLogError(_("No enblend commandline tool selected"));
                return;
            }
        }
#else
        wxString enblendExe = config->Read(wxT("/Enblend/EnblendExe"), wxT(HUGIN_ENBLEND_EXE));
#endif
        // call enblend, and create the right output file
        // I hope this works correctly with filenames that contain
        // spaces

        wxString args = config->Read(wxT("/Enblend/EnblendArgs"));
        if (opts.HFOV == 360.0) {
            // blend over the border
            args.append(wxT(" -w"));
        }
        args.append(wxT(" -o "));
        wxString quoted = output + wxT(".tif");
        quoted = utils::wxQuoteFilename(quoted);
        args.append(quoted);

        unsigned int nImg = imgs.size();
        for (UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
        {
            quoted = output + wxString::Format(wxT("%04d.tif"), *it);
            quoted = utils::wxQuoteFilename(quoted);
            args.append(wxT(" "));
            args.append(quoted);
        }

#ifdef __WXMSW__
        if (args.size() > 1950) {
            wxMessageBox(_("Can not call enblend with a command line > 2000 characters.\nThis is a Windows limitation\nPlease use less images, or place the images in a folder with\na shorter pathname"),
                _("Too many images selected"));
            return;
        }
#endif
        int ret = -1;
        
        wxString cmdline = utils::wxQuoteFilename(enblendExe) + wxT(" ") + args;
        {
#ifdef unix
            wxProgressDialog progress(_("Running Enblend"),_("Enblend will take a while to finish processing the panorama\nYou can watch the enblend progress in the command window"));
            DEBUG_DEBUG("using system() to execute enblend with cmdline:" << cmdline.mb_str());
            ret = system(cmdline.mb_str());
            if (ret == -1) {
                wxLogError(_("Could not execute enblend, system() failed: \nCommand was :") + cmdline + wxT("\n") +
                    _("Error returned was :") + wxString(strerror(errno), *wxConvCurrent));
            } else {
                ret = WEXITSTATUS(ret);
            }
#elif __WXMSW__
            // using CreateProcess on windows
            /* CreateProcess API initialization */
            STARTUPINFO siStartupInfo;
            PROCESS_INFORMATION piProcessInfo;
            memset(&siStartupInfo, 0, sizeof(siStartupInfo));
            memset(&piProcessInfo, 0, sizeof(piProcessInfo));
            siStartupInfo.cb = sizeof(siStartupInfo);
#if wxUSE_UNICODE
            WCHAR * cmdline_c = (WCHAR *) cmdline.wc_str();
            WCHAR * exe_c = (WCHAR *) enblendExe.wc_str();
#else //ANSI
            char * cmdline_c = (char*) cmdline.mb_str();
            char * exe_c = (char*) enblendExe.mb_str();
#endif
            DEBUG_DEBUG("using CreateProcess() to execute enblend:" << enblendExe.mb_str());
            DEBUG_DEBUG("with cmdline:" << cmdline.mb_str());
            ret = CreateProcess(exe_c, cmdline_c, NULL, NULL, FALSE,
                                    IDLE_PRIORITY_CLASS | CREATE_NEW_CONSOLE, NULL,
                                    NULL, &siStartupInfo, &piProcessInfo);
            if (ret) {
                ret = 0;
            } else {
                ret = -1;
                wxLogError(_("Could not execute command: ") + cmdline  , _("CreateProcess Error"));
            }
#elif __WXMAC__
            // use MyExternalCmdExecDialog
            ret = MyExecuteCommandOnDialog(cmdline, this);
#else
            // use stock wxWindows wxExecute on other platforms.
            ret = wxExecute(cmdline, wxEXEC_SYNC);
#endif
        }
        DEBUG_NOTICE("enblend returned with: " << ret);

        if (ret == -1) {
            wxLogError( _("Could not execute command: ") + cmdline, _("wxExecute Error"));
            return;
        } else if (ret > 0) {
            wxLogError(_("command: ") + cmdline +
                _("\nfailed with error code: ") + wxString::Format(wxT("%d"),ret),
                _("enblend error"));
            return;
        }
        if (wxConfigBase::Get()->Read(wxT("/Enblend/DeleteRemappedFiles"), HUGIN_ENBLEND_DELETE_REMAPPED_FILES) != 0) {
            // delete remapped tiff files
            for (UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
            {
                wxString f = output + wxString::Format(wxT("%04d.tif"), *it);
                wxRemoveFile(f);
            }
        }
    }    
}

void NonaStitcherPanel::OnSetQuality(wxSpinEvent & e)
{
    PanoramaOptions opt = pano.getOptions();

    opt.quality = m_JPEGQualitySpin->GetValue();

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
}

