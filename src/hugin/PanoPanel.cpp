// -*- c-basic-offset: 4 -*-

/** @file PanoPanel.cpp
 *
 *  @brief implementation of PanoPanel Class
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

#include "panoinc.h"
#include "panoinc_WX.h"
#include <wx/imaglist.h>
#include <wx/image.h>
#include <wx/spinctrl.h>
#include <wx/config.h>

//#include "hugin/config.h"
#include "hugin/RunStitcherFrame.h"
#include "hugin/CommandHistory.h"
//#include "hugin/ImageCache.h"
//#include "hugin/CPEditorPanel.h"
//#include "hugin/List.h"
//#include "hugin/LensPanel.h"
//#include "hugin/ImagesPanel.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/PanoPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/TextKillFocusHandler.h"

using namespace PT;

// image preview
extern wxBitmap *p_img;
extern ImgPreview *canvas;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PanoPanel, wxWindow)
    EVT_SIZE   ( PanoPanel::FitParent )

    EVT_CHOICE ( XRCID("pano_choice_pano_type"),PanoPanel::ProjectionChanged )
    EVT_CHOICE ( XRCID("pano_choice_interpolator"),PanoPanel::InterpolatorChanged)
    EVT_SPINCTRL ( XRCID("pano_val_hfov"),PanoPanel::HFOVChangedSpin )
    EVT_TEXT_ENTER( XRCID("pano_val_hfov"),PanoPanel::HFOVChanged )
    EVT_SPINCTRL ( XRCID("pano_val_vfov"),PanoPanel::VFOVChangedSpin )
    EVT_TEXT_ENTER( XRCID("pano_val_vfov"),PanoPanel::VFOVChanged )
    EVT_BUTTON ( XRCID("pano_button_calc_fov"), PanoPanel::DoCalcFOV)
    EVT_TEXT_ENTER ( XRCID("pano_val_gamma"),PanoPanel::GammaChanged )
    EVT_CHOICE ( XRCID("pano_choice_color_corr_mode"),PanoPanel::ColourModeChanged)
    EVT_SPINCTRL(XRCID("pano_spin_color_corr_reference"),PanoPanel::ColourModeChangedSpin)
    EVT_SPINCTRL(XRCID("pano_spin_feather_width"), PanoPanel::OnFeatherWidthChanged)
    EVT_SPINCTRL(XRCID("pano_jpeg_quality"), PanoPanel::OnSetQuality)

// TODO remove
//    EVT_COMBOBOX ( XRCID("pano_val_preview_width"),PanoPanel::PreviewWidthChanged )
//    EVT_BUTTON   ( XRCID("pano_button_preview"),PanoPanel::DoPreview )
//    EVT_CHECKBOX ( XRCID("pano_cb_auto_preview"),PanoPanel::AutoPreviewChanged )
//  EVT_CHECKBOX ( XRCID("pano_cb_auto_optimize"),PanoPanel::autoOptimize )
//    EVT_CHECKBOX ( XRCID("pano_cb_panoviewer_enabled"),
//                                               PanoPanel::PanoviewerEnabled )
    EVT_CHOICE   ( XRCID("pano_choice_format_final"),PanoPanel::FileFormatChanged)
    EVT_TEXT_ENTER ( XRCID("pano_val_width"),PanoPanel::WidthChanged )
    EVT_BUTTON ( XRCID("pano_button_opt_width"), PanoPanel::DoCalcOptimalWidth)
    EVT_BUTTON   ( XRCID("pano_button_stitch"),PanoPanel::DoStitch )
END_EVENT_TABLE()


// Define a constructor for the Pano Panel
PanoPanel::PanoPanel(wxWindow *parent, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(*pano),
      updatesDisabled(false)
{
//    opt = new PanoramaOptions();

    // loading xrc resources in selfcreated this panel
    wxXmlResource::Get()->LoadPanel ( this, wxT("panorama_panel")); //);

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    // get gui controls
    m_ProjectionChoice = XRCCTRL(*this, "pano_choice_pano_type" ,wxChoice);
    DEBUG_ASSERT(m_ProjectionChoice);
    m_HFOVSpin = XRCCTRL(*this, "pano_val_hfov" ,wxSpinCtrl);
    DEBUG_ASSERT(m_HFOVSpin);
    m_HFOVSpin->PushEventHandler(new TextKillFocusHandler(this));
    m_VFOVSpin = XRCCTRL(*this, "pano_val_vfov" ,wxSpinCtrl);
    DEBUG_ASSERT(m_VFOVSpin);
    m_VFOVSpin->PushEventHandler(new TextKillFocusHandler(this));

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

// TODO remove
#if 0
    m_PreviewWidthCombo = XRCCTRL(*this, "pano_val_preview_width", wxComboBox);
    DEBUG_ASSERT(m_PreviewWidthCombo);
    m_AutoPreviewCB = XRCCTRL(*this, "pano_cb_auto_preview", wxCheckBox);
    DEBUG_ASSERT(m_AutoPreviewCB);
    m_PreviewPanoviewerCB = XRCCTRL(*this, "pano_cb_panoviewer_enabled", wxCheckBox);
    DEBUG_ASSERT(m_PreviewPanoviewerCB);
    m_PreviewButton = XRCCTRL(*this, "pano_button_preview", wxButton);
    DEBUG_ASSERT(m_PreviewButton);
#endif

    m_WidthTxt = XRCCTRL(*this, "pano_val_width", wxTextCtrl);
    DEBUG_ASSERT(m_WidthTxt);
    m_WidthTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_HeightStaticText = XRCCTRL(*this, "pano_static_height", wxStaticText);
    m_FormatChoice = XRCCTRL(*this, "pano_choice_format_final", wxChoice);
    DEBUG_ASSERT(m_FormatChoice);
    m_JPEGQualitySpin = XRCCTRL(*this, "pano_jpeg_quality", wxSpinCtrl);
    DEBUG_ASSERT(m_JPEGQualitySpin);
    m_JPEGQualitySpin->PushEventHandler(new TextKillFocusHandler(this));
    m_editScriptCB = XRCCTRL(*this, "pano_edit_script", wxCheckBox);
    DEBUG_ASSERT(m_editScriptCB);
    m_StitchButton = XRCCTRL(*this, "pano_button_stitch", wxButton);
    DEBUG_ASSERT(m_StitchButton);

    // observe the panorama
    pano->addObserver (this);

    DEBUG_TRACE("")
}


PanoPanel::~PanoPanel(void)
{
    DEBUG_TRACE("dtor");
    // FIXME. why does the crash at exit?
//    m_HFOVSpin->PopEventHandler(false);
//    m_VFOVSpin->PopEventHandler(false);
//    m_GammaText->PopEventHandler(false);
//    m_WidthTxt->PopEventHandler(false);
    pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}


void PanoPanel::panoramaChanged (PT::Panorama &pano)
{
    PanoramaOptions opt = pano.getOptions();
    // update all options for dialog and notebook tab
    UpdateDisplay(opt);
    m_oldOpt = opt;
}

void PanoPanel::UpdateDisplay(const PanoramaOptions & opt)
{
    unsigned int nImages = pano.getNrOfImages();

    if (nImages < 1) {
        // disable some controls
        m_ColorCorrModeChoice->Disable();
// TODO remove
//        m_PreviewButton->Disable();
        m_StitchButton->Disable();
        m_ColorCorrRefSpin->Disable();
    } else {
        m_ColorCorrModeChoice->Enable();
//        m_PreviewButton->Enable();
        m_StitchButton->Enable();
    }

    int maximg = ((int)nImages) -1;
    // set spinctrl limits
    if (opt.colorCorrection != PanoramaOptions::NONE) {
        m_ColorCorrRefSpin->Enable();
        m_ColorCorrRefSpin->SetRange(0, maximg);
    } else {
        m_ColorCorrRefSpin->Disable();
    }
    switch (opt.projectionFormat) {
    case PanoramaOptions::RECTILINEAR:
        m_HFOVSpin->SetRange(1,179);
        m_VFOVSpin->SetRange(1,179);
        break;
    case PanoramaOptions::CYLINDRICAL:
        m_HFOVSpin->SetRange(1,360);
        m_VFOVSpin->SetRange(1,179);
        break;
    case PanoramaOptions::EQUIRECTANGULAR:
        m_HFOVSpin->SetRange(1,360);
        m_VFOVSpin->SetRange(1,180);
        break;
    }

    m_ProjectionChoice->SetSelection(opt.projectionFormat);
    m_HFOVSpin->SetValue((int)opt.HFOV);
    m_VFOVSpin->SetValue((int)opt.VFOV);

    m_InterpolatorChoice->SetSelection(opt.interpolator);
    m_GammaText->SetValue(wxString::Format ("%.2f", opt.gamma));
    m_ColorCorrModeChoice->SetSelection(opt.colorCorrection);
    m_ColorCorrRefSpin->SetValue(opt.colorReferenceImage);

    m_WidthTxt->SetValue(wxString::Format("%d", opt.width));
    m_HeightStaticText->SetLabel(wxString::Format("%d", opt.getHeight()));
    m_FormatChoice->SetSelection((int)opt.outputFormat);

    if (opt.outputFormat == PanoramaOptions::JPEG) {
        m_JPEGQualitySpin->Enable();
    } else {
        m_JPEGQualitySpin->Disable();
    }
    m_JPEGQualitySpin->SetValue(opt.quality);

    m_FeatherWidthSpin->SetValue(opt.featherWidth);
}

void PanoPanel::ProjectionChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int lt = m_ProjectionChoice->GetSelection();
    wxString Ip;
    switch ( lt ) {
    case PanoramaOptions::RECTILINEAR:       Ip = _("Rectilinear"); break;
    case PanoramaOptions::CYLINDRICAL:       Ip = _("Cylindrical"); break;
    case PanoramaOptions::EQUIRECTANGULAR:   Ip = _("Equirectangular"); break;
    }
    opt.projectionFormat = (PanoramaOptions::ProjectionFormat) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Projection changed: "  << lt << ":" << Ip )
}

void PanoPanel::InterpolatorChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    //Interpolator from PanoramaMemento.h
    int lt = m_InterpolatorChoice->GetSelection();
    wxString Ip;
    switch ( lt ) {
    case PanoramaOptions::POLY_3:            Ip = _("Poly3 (Bicubic)"); break;
    case PanoramaOptions::SPLINE_16:         Ip = _("Spline 16"); break;
    case PanoramaOptions::SPLINE_36:         Ip = _("Spline 36"); break;
    case PanoramaOptions::SINC_256:          Ip = _("Sinc 256"); break;
    case PanoramaOptions::SPLINE_64:         Ip = _("Spline 64"); break;
    case PanoramaOptions::BILINEAR:          Ip = _("Bilinear"); break;
    case PanoramaOptions::NEAREST_NEIGHBOUR: Ip = _("Nearest neighbour"); break;
    case PanoramaOptions::SINC_1024:         Ip = _("Sinc 1024"); break;
    }

    opt.interpolator = (PanoramaOptions::Interpolator) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Interpolator changed to: " << Ip )
}

void PanoPanel::HFOVChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int hfov = m_HFOVSpin->GetValue() ;

    DEBUG_ASSERT(hfov >= 0 && hfov <= 360);

    opt.HFOV = (int) hfov;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO ( "new hfov: " << hfov )
}

void PanoPanel::HFOVChangedSpin ( wxSpinEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int hfov = m_HFOVSpin->GetValue() ;

    DEBUG_ASSERT(hfov >= 0 && hfov <= 360);

    opt.HFOV = (int) hfov;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO ( "new hfov: " << hfov )
}

void PanoPanel::VFOVChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE("")
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int vfov = m_VFOVSpin->GetValue() ;
    DEBUG_ASSERT(vfov >= 0 && vfov <= 180);

    if (vfov != m_oldVFOV) {
        opt.VFOV = vfov;
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
        DEBUG_INFO ( "new vfov: " << vfov << " => height: " << opt.getHeight() );
        m_oldVFOV = vfov;
    } else {
        DEBUG_DEBUG("not setting same fov");
    }
}

void PanoPanel::VFOVChangedSpin ( wxSpinEvent & e )
{
    DEBUG_TRACE("")
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int vfov = m_VFOVSpin->GetValue() ;
    DEBUG_ASSERT(vfov >= 0 && vfov <= 180);

    if (vfov != m_oldVFOV) {
        opt.VFOV = vfov;
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
        DEBUG_INFO ( "new vfov: " << vfov << " => height: " << opt.getHeight() );
        m_oldVFOV = vfov;
    } else {
        DEBUG_DEBUG("not setting same fov");
    }
}

void PanoPanel::GammaChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    double val;
    wxString text = m_GammaText->GetValue();
    if (text.ToDouble( &val )) {
        opt.gamma = val;
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
        DEBUG_DEBUG( val );
    } else {
        wxLogError(_("gamma must be a number"));
    }
}

void PanoPanel::ColourModeChanged ( wxCommandEvent & e )
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

    DEBUG_INFO(text <<" with: " << refImage);
}

void PanoPanel::ColourModeChangedSpin ( wxSpinEvent & e )
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

    DEBUG_INFO(text <<" with: " << refImage);
}

// TODO remove
#if 0
void PanoPanel::PreviewWidthChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    DEBUG_ERROR("preview not implemented");
    PanoramaOptions opt = pano.getOptions();
    long previewWidth;
    if (m_PreviewWidthCombo->GetValue().ToLong(&previewWidth)) {
        DEBUG_INFO ( ": " << previewWidth );
    } else {
        wxLogError(_("preview width must be a number"));
    }
}
#endif


void PanoPanel::FileFormatChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    int format = m_FormatChoice->GetSelection();

    opt.outputFormat = (PanoramaOptions::FileFormat) format;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_INFO ( PanoramaOptions::getFormatName(opt.outputFormat));
}

void PanoPanel::WidthChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    long nWidth;
    if (m_WidthTxt->GetValue().ToLong(&nWidth)) {
        DEBUG_ASSERT(nWidth>0);
        opt.width =  (unsigned int) nWidth;
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
        DEBUG_INFO(nWidth );
    } else {
        wxLogError(_("width needs to be an integer bigger than 0"));
    }
}

// TODO remove
#if 0
void PanoPanel::AutoPreviewChanged (wxCommandEvent & e)
{
    wxLogError("preview not implemented");
}

void PanoPanel::PanoviewerEnabled(wxCommandEvent & e)
{
    if (updatesDisabled) return;
    wxLogError("preview not implemented");
}
#endif

void PanoPanel::DoCalcFOV(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    PanoramaOptions opt = pano.getOptions();

    FDiff2D fov = pano.calcFOV();
    opt.HFOV = (int) ceil(fov.x);
    opt.VFOV = (int) ceil(fov.y);

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO ( "new fov: [" << opt.HFOV << " "<< opt.VFOV << "] => height: " << opt.getHeight() );

}

void PanoPanel::DoCalcOptimalWidth(wxCommandEvent & e)
{
    // calculate average pixel density of each image
    // and use the highest one to calculate the width
    int nImgs = pano.getNrOfImages();
    double pixelDensity=0;
    for (int i=0; i<nImgs; i++) {
        const PanoImage & img = pano.getImage(i);
        const VariableMap & var = pano.getImageVariables(i);
        double density = img.getWidth() / const_map_get(var,"v").getValue();
        if (density > pixelDensity) {
            pixelDensity = density;
        }
    }
    PanoramaOptions opt = pano.getOptions();
    opt.width = (int) (pixelDensity * opt.HFOV);

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO ( "new optimal width: " << opt.width );
}

void PanoPanel::DoStitch ( wxCommandEvent & e )
{
    PanoramaOptions opt = pano.getOptions();
    // select output file
    // FIXME put in right output extension for selected
    // file format
    wxFileDialog dlg(this,_("Create panorama image"),
                     wxConfigBase::Get()->Read("actualPath",""),
                     "", "",
                     wxSAVE, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        // print as optimizer script..
        std::ofstream script(dlg.GetPath());
        wxConfig::Get()->Write("actualPath", dlg.GetDirectory());  // remember for later
        opt.outfile = dlg.GetPath().c_str();
        new RunStitcherFrame(this, &pano, opt, m_editScriptCB->IsChecked());
    }
}

void PanoPanel::OnSetQuality(wxSpinEvent & e)
{
    PanoramaOptions opt = pano.getOptions();

    opt.quality = m_JPEGQualitySpin->GetValue();

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
}

void PanoPanel::OnFeatherWidthChanged(wxSpinEvent & e)
{
    PanoramaOptions opt = pano.getOptions();

    opt.featherWidth = m_FeatherWidthSpin->GetValue();

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
}

// TODO remove
#if 0
void PanoPanel::DoPreview (wxCommandEvent & e)
{
    wxLogError("preview not implemented");
}
#endif

void PanoPanel::FitParent( wxSizeEvent & e )
{
    DEBUG_TRACE("");
    Layout();
    wxSize new_size = e.GetSize();
//    this->SetSize(new_size);
//    XRCCTRL(*this, "images_panel", wxPanel)->SetSize ( new_size );
    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}
