// -*- c-basic-offset: 4 -*-

/** @file PTStitcherPanel.cpp
 *
 *  @brief implementation of PTStitcherPanel Class
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

#include "panoinc_WX.h"

#include "panoinc.h"

#include "PT/Stitcher.h"

#include "hugin/RunStitcherFrame.h"
#include "hugin/CommandHistory.h"
//#include "hugin/ImageCache.h"
//#include "hugin/CPEditorPanel.h"
//#include "hugin/List.h"
//#include "hugin/LensPanel.h"
//#include "hugin/ImagesPanel.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/PTStitcherPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/MyProgressDialog.h"
#include "hugin/config_defaults.h"

using namespace PT;
using namespace std;
using namespace utils;

// image preview
extern wxBitmap *p_img;
extern ImgPreview *canvas;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PTStitcherPanel, wxWindow)
    EVT_CHOICE ( XRCID("pano_choice_interpolator"),PTStitcherPanel::InterpolatorChanged)
    EVT_TEXT_ENTER ( XRCID("pano_val_gamma"),PTStitcherPanel::GammaChanged )
    EVT_CHOICE ( XRCID("pano_choice_color_corr_mode"),PTStitcherPanel::ColourModeChanged)
    EVT_CHOICE ( XRCID("pano_choice_speedup"),PTStitcherPanel::OnSpeedupChanged)
    EVT_SPINCTRL(XRCID("pano_spin_color_corr_reference"),PTStitcherPanel::ColourModeChangedSpin)
    EVT_SPINCTRL(XRCID("pano_spin_feather_width"), PTStitcherPanel::OnFeatherWidthChanged)
    EVT_SPINCTRL(XRCID("pano_jpeg_quality"), PTStitcherPanel::OnSetQuality)

    EVT_CHOICE   ( XRCID("pano_choice_format_final"),PTStitcherPanel::FileFormatChanged)
END_EVENT_TABLE()


// Define a constructor for the Pano Panel
PTStitcherPanel::PTStitcherPanel(wxWindow *parent, Panorama & pano)
    : StitcherPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(pano),
      updatesDisabled(false)
{
//    opt = new PanoramaOptions();

    // loading xrc resources in selfcreated this panel
    wxXmlResource::Get()->LoadPanel ( this, wxT("ptstitcher_panel")); //);

    // get gui controls
    m_FormatChoice = XRCCTRL(*this, "pano_choice_format_final", wxChoice);
    DEBUG_ASSERT(m_FormatChoice);
    m_InterpolatorChoice = XRCCTRL(*this, "pano_choice_interpolator",
                                   wxChoice);
    DEBUG_ASSERT(m_InterpolatorChoice);
    m_GammaText = XRCCTRL(*this, "pano_val_gamma" ,wxTextCtrl);
    DEBUG_ASSERT(m_GammaText);
    m_GammaText->PushEventHandler(new TextKillFocusHandler(this));

    m_ColorCorrModeChoice = XRCCTRL(*this, "pano_choice_color_corr_mode",
                                    wxChoice);
    DEBUG_ASSERT(m_ColorCorrModeChoice);
    m_ColorCorrRefSpin = XRCCTRL(*this, "pano_spin_color_corr_reference",
                                 wxSpinCtrl);
    DEBUG_ASSERT(m_ColorCorrRefSpin);
    m_ColorCorrRefSpin->Disable();
    m_ColorCorrRefSpin->PushEventHandler(new TextKillFocusHandler(this));

    m_FeatherWidthSpin = XRCCTRL(*this, "pano_spin_feather_width",
                                 wxSpinCtrl);
    DEBUG_ASSERT(m_FeatherWidthSpin);
    m_FeatherWidthSpin->PushEventHandler(new TextKillFocusHandler(this));

    m_JPEGQualitySpin = XRCCTRL(*this, "pano_jpeg_quality", wxSpinCtrl);
    DEBUG_ASSERT(m_JPEGQualitySpin);
    m_JPEGQualitySpin->PushEventHandler(new TextKillFocusHandler(this));
    m_editScriptCB = XRCCTRL(*this, "pano_edit_script", wxCheckBox);
    DEBUG_ASSERT(m_editScriptCB);

    m_SpeedupChoice = XRCCTRL(*this, "pano_choice_speedup",
                              wxChoice);
    DEBUG_ASSERT(m_SpeedupChoice);

    UpdateDisplay(pano.getOptions());

    // observe the panorama
    pano.addObserver (this);

    wxSize size = GetSize();
    DEBUG_INFO( "before layout:" << size.GetWidth() <<"x"<< size.GetHeight());
//    Layout();
    size = GetSize();
    DEBUG_INFO( "after layout:" << size.GetWidth() <<"x"<< size.GetHeight());
    Fit();
    size = GetSize();
    DEBUG_INFO( "after fit:" << size.GetWidth() <<"x"<< size.GetHeight());
    DEBUG_TRACE("")
}


PTStitcherPanel::~PTStitcherPanel(void)
{
    DEBUG_TRACE("dtor");
    // FIXME. why does the crash at exit?
    m_GammaText->PopEventHandler(true);
    m_ColorCorrRefSpin->PopEventHandler(true);
    m_FeatherWidthSpin->PopEventHandler(true);
    m_JPEGQualitySpin->PopEventHandler(true);
    
    pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}


void PTStitcherPanel::panoramaChanged (PT::Panorama &pano)
{
	DEBUG_TRACE("");
    PanoramaOptions opt = pano.getOptions();
    // update all options for dialog and notebook tab
    UpdateDisplay(opt);
    m_oldOpt = opt;
}

void PTStitcherPanel::UpdateDisplay(const PanoramaOptions & opt)
{
    unsigned int nImages = pano.getNrOfImages();

    if (nImages == 0) {
        // disable some controls
        m_ColorCorrModeChoice->Disable();
        m_ColorCorrRefSpin->Disable();
  		m_FormatChoice->Disable();
    	m_InterpolatorChoice->Disable();
    	m_GammaText->Disable();
    	m_FeatherWidthSpin->Disable();
    	m_JPEGQualitySpin->Disable();
    	m_editScriptCB->Disable();
    	m_SpeedupChoice->Disable();
    } else {
        m_ColorCorrRefSpin->Enable();
        m_ColorCorrModeChoice->Enable();
  		m_FormatChoice->Enable();
    	m_InterpolatorChoice->Enable();
    	m_GammaText->Enable();
    	m_FeatherWidthSpin->Enable();
    	m_JPEGQualitySpin->Enable();
    	m_editScriptCB->Enable();
    	m_SpeedupChoice->Enable();
   }

    int maximg = ((int)nImages) -1;
    // set spinctrl limits
    if (opt.colorCorrection != PanoramaOptions::NONE) {
        m_ColorCorrRefSpin->Enable();
        m_ColorCorrRefSpin->SetRange(0, maximg);
    } else {
        m_ColorCorrRefSpin->Disable();
    }

    m_InterpolatorChoice->SetSelection(opt.interpolator);
    m_GammaText->SetValue(wxString::Format (wxT("%.2f"), opt.gamma));
    m_ColorCorrModeChoice->SetSelection(opt.colorCorrection);
    m_ColorCorrRefSpin->SetValue(opt.colorReferenceImage);

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
    case PanoramaOptions::TIFF_mask:
        format = 4;
        break;
    case PanoramaOptions::PICT:
        format = 5;
        break;
    case PanoramaOptions::PSD:
        format = 6;
        break;
    case PanoramaOptions::PSD_m:
        format = 7;
        break;
    case PanoramaOptions::PSD_mask:
        format = 8;
        break;
    case PanoramaOptions::PAN:
        format = 9;
        break;
    case PanoramaOptions::IVR:
        format = 10;
        break;
    case PanoramaOptions::IVR_java:
        format = 11;
        break;
    case PanoramaOptions::VRML:
        format = 12;
        break;
    case PanoramaOptions::QTVR:
        format = 13;
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

    m_FeatherWidthSpin->SetValue(opt.featherWidth);

    m_SpeedupChoice->SetSelection(opt.remapAcceleration);
}

void PTStitcherPanel::InterpolatorChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    //Interpolator from PanoramaMemento.h
    int lt = m_InterpolatorChoice->GetSelection();
    wxString Ip;
    switch ( lt ) {
    case vigra_ext::INTERP_CUBIC:             Ip = _("Bicubic"); break;
    case vigra_ext::INTERP_SPLINE_16:         Ip = _("Spline 16"); break;
    case vigra_ext::INTERP_SPLINE_36:         Ip = _("Spline 36"); break;
    case vigra_ext::INTERP_SINC_256:          Ip = _("Sinc 256"); break;
    case vigra_ext::INTERP_SPLINE_64:         Ip = _("Spline 64"); break;
    case vigra_ext::INTERP_BILINEAR:          Ip = _("Bilinear"); break;
    case vigra_ext::INTERP_NEAREST_NEIGHBOUR: Ip = _("Nearest neighbour"); break;
    case vigra_ext::INTERP_SINC_1024:         Ip = _("Sinc 1024"); break;
    }

    opt.interpolator = (vigra_ext::Interpolator) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Interpolator changed to: " << Ip.mb_str() )
}


void PTStitcherPanel::GammaChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    double val;
    wxString text = m_GammaText->GetValue();
    if (str2double(text, val)) {
        opt.gamma = val;
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
        DEBUG_DEBUG( val );
    } else {
        wxLogError(_("gamma must be a number"));
    }
}


void PTStitcherPanel::ColourModeChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int colorCorrection = m_ColorCorrModeChoice->GetSelection();
    wxString text = m_ColorCorrModeChoice->GetString(colorCorrection);

    int refImage = m_ColorCorrRefSpin->GetValue();
    opt.colorCorrection = (PanoramaOptions::ColorCorrection) colorCorrection;
    DEBUG_ASSERT(refImage >= 0 && refImage < (int)pano.getNrOfImages());
    opt.colorReferenceImage = (unsigned int) refImage;

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO(text.mb_str() << "(" << colorCorrection << ") with: " << refImage);
}


void PTStitcherPanel::ColourModeChangedSpin ( wxSpinEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int colorCorrection = m_ColorCorrModeChoice->GetSelection();
    wxString text = m_ColorCorrModeChoice->GetString(colorCorrection);

    int refImage = m_ColorCorrRefSpin->GetValue();
	DEBUG_INFO("old color correction mode:" << opt.colorCorrection);
    opt.colorCorrection = (PanoramaOptions::ColorCorrection) colorCorrection;
    DEBUG_ASSERT(refImage >= 0 && refImage < (int)pano.getNrOfImages());
    opt.colorReferenceImage = (unsigned int) refImage;

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO(text.mb_str() <<" with: " << refImage);
}


void PTStitcherPanel::FileFormatChanged ( wxCommandEvent & e )
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
        opt.outputFormat = PanoramaOptions::TIFF_mask;
        break;
    case 5:
        opt.outputFormat = PanoramaOptions::PICT;
        break;
    case 6:
        opt.outputFormat = PanoramaOptions::PSD;
        break;
    case 7:
        opt.outputFormat = PanoramaOptions::PSD_m;
        break;
    case 8:
        opt.outputFormat = PanoramaOptions::PSD_mask;
        break;
    case 9:
        opt.outputFormat = PanoramaOptions::PAN;
        break;
    case 10:
        opt.outputFormat = PanoramaOptions::IVR;
        break;
    case 11:
        opt.outputFormat = PanoramaOptions::IVR_java;
        break;
    case 12:
        opt.outputFormat = PanoramaOptions::VRML;
        break;
    case 13:
        opt.outputFormat = PanoramaOptions::QTVR;
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

void PTStitcherPanel::OnSpeedupChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int speedup = m_SpeedupChoice->GetSelection();
    wxString text = m_SpeedupChoice->GetString(speedup);

    opt.remapAcceleration = (PanoramaOptions::PTStitcherAcceleration) speedup;

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
}


void PTStitcherPanel::Stitch(const Panorama & pano,
                             PanoramaOptions opts_)
{
    PanoramaOptions opts(opts_);
    // work around a bug in PTStitcher, which doesn't
    // allow multilayer tif files to end with .tif
    if ( opts.outputFormat == PanoramaOptions::TIFF_m
         || opts.outputFormat == PanoramaOptions::TIFF_mask )
    {
        opts.outfile = stripExtension(opts.outfile);
    }
    
#if __unix__ || WIN32
    if ( opts.outputFormat == PanoramaOptions::QTVR ) {
        wxMessageBox(_("PTStitcher.exe does not support QTVR output on Windows and Linux"), _("PTStitcher note"));
        return;
    }

#endif
	UIntSet imgs;
	if (wxConfigBase::Get()->Read(wxT("/General/UseOnlySelectedImages"),
		                          HUGIN_USE_SELECTED_IMAGES))
	{
		// use only selected images.
		imgs = pano.getActiveImages();
	} else {
        fill_set(imgs, 0, pano.getNrOfImages()-1);
	}

    new RunStitcherFrame(this, &pano, opts, imgs, m_editScriptCB->IsChecked());
}

void PTStitcherPanel::OnSetQuality(wxSpinEvent & e)
{
    PanoramaOptions opt = pano.getOptions();

    opt.quality = m_JPEGQualitySpin->GetValue();

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
}


void PTStitcherPanel::OnFeatherWidthChanged(wxSpinEvent & e)
{
    PanoramaOptions opt = pano.getOptions();

    opt.featherWidth = m_FeatherWidthSpin->GetValue();

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
}
