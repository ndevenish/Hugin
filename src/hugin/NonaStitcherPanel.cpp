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

#include "panoinc_WX.h"

#include "panoinc.h"

#include "PT/Stitcher.h"

//#include "hugin/config.h"
#include "hugin/RunStitcherFrame.h"
#include "hugin/CommandHistory.h"
//#include "hugin/ImageCache.h"
//#include "hugin/CPEditorPanel.h"
//#include "hugin/List.h"
//#include "hugin/LensPanel.h"
//#include "hugin/ImagesPanel.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/NonaStitcherPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/MyProgressDialog.h"

using namespace PT;
using namespace std;
using namespace utils;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(NonaStitcherPanel, wxWindow)
    EVT_SIZE   ( NonaStitcherPanel::FitParent )

    EVT_CHOICE ( XRCID("nona_choice_interpolator"),NonaStitcherPanel::InterpolatorChanged)
    EVT_SPINCTRL(XRCID("nona_jpeg_quality"), NonaStitcherPanel::OnSetQuality)

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
    PanoramaOptions opt = pano.getOptions();
    // update all options for dialog and notebook tab
    UpdateDisplay(opt);
    m_oldOpt = opt;
}

void NonaStitcherPanel::UpdateDisplay(const PanoramaOptions & opt)
{
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
        DEBUG_ERROR("NONA: Unknown output format, switching to JPG");
        format = 0;
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
    } else {
        m_EnblendCheckBox->Disable();
    }
}


void NonaStitcherPanel::InterpolatorChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    //Interpolator from PanoramaMemento.h
    int lt = m_InterpolatorChoice->GetSelection();

    opt.interpolator = (PanoramaOptions::Interpolator) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Interpolator changed to: " << lt )
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
    MyProgressDialog pdisp(_("Stitching Panorama"), "", NULL, wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );

    PanoramaOptions opts = options;

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
                           pdisp, opts.outfile);

        string output = stripExtension(opts.outfile);
        if (enblend) {
            wxConfigBase* config = wxConfigBase::Get();
#ifdef __WXMSW__
            wxString enblendExe = config->Read("/Enblend/EnblendExe","enblend.exe");
            if (!wxFile::Exists(enblendExe)){
                wxFileDialog dlg(this,_("Select enblend.exe"),
                                 "", "enblend.exe",
                                 "Executables (*.exe)|*.exe",
                                 wxOPEN, wxDefaultPosition);
                if (dlg.ShowModal() == wxID_OK) {
                    enblendExe = dlg.GetPath();
                    config->Write("/Enblend/EnblendExe",enblendExe);
                } else {
                    wxLogError(_("No enblend.exe selected"));
                }
            }
#else
            wxString enblendExe = config->Read("/Enblend/EnblendExe","enblend");
#endif

            // call enblend, and create the right output file
            // I hope this works correctly with filenames that contain
            // spaces

            string cmd(enblendExe.c_str());
            if (opts.HFOV == 360.0) {
                // blend over the border
                cmd.append(" -v -w -o ");
            } else {
                cmd.append(" -v -o ");
            }

            cmd.append(output).append(".tif ");

            unsigned int nImg = pano.getNrOfImages();
            char imgname[256];
            for(unsigned int i = 0; i < nImg; i++)
            {
                snprintf(imgname,256,"%s%04d.tif", output.c_str(), i);
                cmd.append(" ").append(imgname);
            }
            DEBUG_DEBUG("enblend cmdline: " << cmd);
            wxShell(cmd.c_str());
        }
    } catch (std::exception & e) {
        DEBUG_FATAL(_("error during stitching:") << e.what());
        return;
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

void NonaStitcherPanel::FitParent( wxSizeEvent & e )
{
    DEBUG_TRACE("");
//    Layout();
    wxSize new_size = e.GetSize();
//    this->SetSize(new_size);
    XRCCTRL(*this, "nona_panel", wxPanel)->SetSize ( new_size );
    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}
