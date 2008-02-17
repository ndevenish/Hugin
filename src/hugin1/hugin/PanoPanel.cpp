// -*- c-basic-offset: 4 -*-

/** @file PanoPanel.cpp
 *
 *  @brief implementation of PanoPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de>
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
#include "base_wx/platform.h"


#include <hugin/config_defaults.h>

#include "PT/Stitcher.h"
#include "common/wxPlatform.h"

extern "C" {
#ifdef HasPANO13
#include <pano13/queryfeature.h>
#else
#include <pano12/queryfeature.h>
#endif
}

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
#include "base_wx/MyProgressDialog.h"
#include "hugin/PTStitcherPanel.h"
#include "hugin/NonaStitcherPanel.h"
#include "hugin/config_defaults.h"
#include "base_wx/platform.h"

#define WX_BROKEN_SIZER_UNKNOWN

using namespace PT;
using namespace std;
using namespace utils;

BEGIN_EVENT_TABLE(PanoPanel, wxPanel)
    EVT_CHOICE ( XRCID("pano_choice_pano_type"),PanoPanel::ProjectionChanged )
    EVT_TEXT_ENTER( XRCID("pano_text_hfov"),PanoPanel::HFOVChanged )
    EVT_TEXT_ENTER( XRCID("pano_text_vfov"),PanoPanel::VFOVChanged )
    EVT_BUTTON ( XRCID("pano_button_calc_fov"), PanoPanel::DoCalcFOV)
    EVT_TEXT_ENTER ( XRCID("pano_val_width"),PanoPanel::WidthChanged )
    EVT_TEXT_ENTER ( XRCID("pano_val_height"),PanoPanel::HeightChanged )
    EVT_TEXT_ENTER ( XRCID("pano_val_roi_top"),PanoPanel::ROIChanged )
    EVT_TEXT_ENTER ( XRCID("pano_val_roi_bottom"),PanoPanel::ROIChanged )
    EVT_TEXT_ENTER ( XRCID("pano_val_roi_left"),PanoPanel::ROIChanged )
    EVT_TEXT_ENTER ( XRCID("pano_val_roi_right"),PanoPanel::ROIChanged )
    EVT_BUTTON ( XRCID("pano_button_opt_width"), PanoPanel::DoCalcOptimalWidth)
    EVT_BUTTON ( XRCID("pano_button_stitch"),PanoPanel::OnDoStitch )

    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_blended"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_layers"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_exposure_layers"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_exposure_blended"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_exposure_remapped"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_blended"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_stacks"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_layers"), PanoPanel::OnOutputFilesChanged)

    EVT_CHOICE ( XRCID("pano_choice_remapper"),PanoPanel::RemapperChanged )
    EVT_BUTTON ( XRCID("pano_button_remapper_opts"),PanoPanel::OnRemapperOptions )
    EVT_CHOICE ( XRCID("pano_choice_blender"),PanoPanel::BlenderChanged )
    EVT_BUTTON ( XRCID("pano_button_blender_opts"),PanoPanel::OnBlenderOptions )
    EVT_CHOICE ( XRCID("pano_choice_file_format"),PanoPanel::FileFormatChanged )
    EVT_CHOICE ( XRCID("pano_choice_hdr_file_format"),PanoPanel::HDRFileFormatChanged )
//    EVT_SPINCTRL ( XRCID("pano_output_normal_opts_jpeg_quality"),PanoPanel::OnJPEGQualitySpin )
    EVT_TEXT_ENTER ( XRCID("pano_output_normal_opts_jpeg_quality"),PanoPanel::OnJPEGQualityText )
    EVT_CHOICE ( XRCID("pano_output_normal_opts_tiff_compression"),PanoPanel::OnNormalTIFFCompression)
    EVT_CHOICE ( XRCID("pano_output_hdr_opts_tiff_compression"),PanoPanel::OnHDRTIFFCompression)

END_EVENT_TABLE()

PanoPanel::PanoPanel()
    : pano(0), updatesDisabled(false)
{

}

bool PanoPanel::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                      long style, const wxString& name)
{
    if (! wxPanel::Create(parent, id, pos, size, style, name)) {
        return false;
    }

    wxXmlResource::Get()->LoadPanel(this, wxT("panorama_panel"));
    wxPanel * panel = XRCCTRL(*this, "panorama_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer(topsizer);

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    // get gui controls
    m_ProjectionChoice = XRCCTRL(*this, "pano_choice_pano_type" ,wxChoice);
    DEBUG_ASSERT(m_ProjectionChoice);

    m_keepViewOnResize = true;

    /* populate with all available projection types */
#ifdef HasPANO13
    int nP = panoProjectionFormatCount();
    for(int n=0; n < nP; n++) {
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(n, &proj)) {
            wxString str2(proj.name, wxConvLocal);
            m_ProjectionChoice->Append(wxGetTranslation(str2));
        }
    }
#else
    bool ok = true;
    int n=0;
    while(ok) {
        char name[20];
        char str[255];
        sprintf(name,"PanoType%d",n);
        n++;
        int len = queryFeatureString(name,str,255);
        if (len > 0) {
            wxString str2(str, wxConvLocal);
            m_ProjectionChoice->Append(wxGetTranslation(str2));
        } else {
            ok = false;
        }
    }
#endif
    m_HFOVText = XRCCTRL(*this, "pano_text_hfov" ,wxTextCtrl);
    DEBUG_ASSERT(m_HFOVText);
    m_CalcHFOVButton = XRCCTRL(*this, "pano_button_calc_fov" ,wxButton);
    DEBUG_ASSERT(m_CalcHFOVButton);
    m_HFOVText->PushEventHandler(new TextKillFocusHandler(this));
    m_VFOVText = XRCCTRL(*this, "pano_text_vfov" ,wxTextCtrl);
    DEBUG_ASSERT(m_VFOVText);
    m_VFOVText->PushEventHandler(new TextKillFocusHandler(this));


    m_WidthTxt = XRCCTRL(*this, "pano_val_width", wxTextCtrl);
    DEBUG_ASSERT(m_WidthTxt);
    m_WidthTxt->PushEventHandler(new TextKillFocusHandler(this));
    m_CalcOptWidthButton = XRCCTRL(*this, "pano_button_opt_width" ,wxButton);
    DEBUG_ASSERT(m_CalcOptWidthButton);

    m_HeightTxt = XRCCTRL(*this, "pano_val_height", wxTextCtrl);
    DEBUG_ASSERT(m_HeightTxt);
    m_HeightTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_ROILeftTxt = XRCCTRL(*this, "pano_val_roi_left", wxTextCtrl);
    DEBUG_ASSERT(m_ROILeftTxt);
    m_ROILeftTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_ROIRightTxt = XRCCTRL(*this, "pano_val_roi_right", wxTextCtrl);
    DEBUG_ASSERT(m_ROIRightTxt);
    m_ROIRightTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_ROITopTxt = XRCCTRL(*this, "pano_val_roi_top", wxTextCtrl);
    DEBUG_ASSERT(m_ROITopTxt);
    m_ROITopTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_ROIBottomTxt = XRCCTRL(*this, "pano_val_roi_bottom", wxTextCtrl);
    DEBUG_ASSERT(m_ROIBottomTxt);
    m_ROIBottomTxt->PushEventHandler(new TextKillFocusHandler(this));

    m_RemapperChoice = XRCCTRL(*this, "pano_choice_remapper", wxChoice);
    DEBUG_ASSERT(m_RemapperChoice);
    m_BlenderChoice = XRCCTRL(*this, "pano_choice_blender", wxChoice);
    DEBUG_ASSERT(m_BlenderChoice);
    m_StitchButton = XRCCTRL(*this, "pano_button_stitch", wxButton);
    DEBUG_ASSERT(m_StitchButton);

    m_FileFormatChoice = XRCCTRL(*this, "pano_choice_file_format", wxChoice);
    DEBUG_ASSERT(m_FileFormatChoice);
    m_FileFormatPanelJPEG = XRCCTRL(*this, "pano_output_normal_opts_jpeg", wxPanel);
    DEBUG_ASSERT(m_FileFormatPanelJPEG);
    m_FileFormatJPEGQualityText = XRCCTRL(*this, "pano_output_normal_opts_jpeg_quality", wxTextCtrl);
    DEBUG_ASSERT(m_FileFormatJPEGQualityText);
    m_FileFormatJPEGQualityText->PushEventHandler(new TextKillFocusHandler(this));

    m_FileFormatPanelTIFF = XRCCTRL(*this, "pano_output_normal_opts_tiff", wxPanel);
    DEBUG_ASSERT(m_FileFormatPanelTIFF);
    m_FileFormatTIFFCompChoice = XRCCTRL(*this, "pano_output_normal_opts_tiff_compression", wxChoice);
    DEBUG_ASSERT(m_FileFormatTIFFCompChoice);

    m_HDRFileFormatChoice = XRCCTRL(*this, "pano_choice_hdr_file_format", wxChoice);
    DEBUG_ASSERT(m_HDRFileFormatChoice);
    m_HDRFileFormatPanelTIFF = XRCCTRL(*this, "pano_output_hdr_opts_tiff", wxPanel);
    DEBUG_ASSERT(m_HDRFileFormatPanelTIFF);
    m_FileFormatHDRTIFFCompChoice = XRCCTRL(*this, "pano_output_hdr_opts_tiff_compression", wxChoice);
    DEBUG_ASSERT(m_FileFormatHDRTIFFCompChoice);

    m_pano_ctrls = XRCCTRL(*this, "pano_controls_panel", wxScrolledWindow);
    DEBUG_ASSERT(m_pano_ctrls);
    m_pano_ctrls->SetSizeHints(20, 20);
    m_pano_ctrls->FitInside();
    m_pano_ctrls->SetScrollRate(10, 10);


/*
    // trigger creation of apropriate stitcher control, if
    // not already happend.
    if (! m_Stitcher) {
        wxCommandEvent dummy;
        StitcherChanged(dummy);
    }
*/
    DEBUG_TRACE("")
    return true;
}

void PanoPanel::Init(Panorama * panorama)
{
    pano = panorama;
    // observe the panorama
    pano->addObserver(this);
    panoramaChanged(*panorama);
}

PanoPanel::~PanoPanel(void)
{
    DEBUG_TRACE("dtor");
    wxConfigBase::Get()->Write(wxT("Stitcher/DefaultRemapper"),m_RemapperChoice->GetSelection());

    m_HFOVText->PopEventHandler(true);
    m_VFOVText->PopEventHandler(true);
    m_WidthTxt->PopEventHandler(true);
    m_ROILeftTxt->PopEventHandler(true);
    m_ROIRightTxt->PopEventHandler(true);
    m_ROITopTxt->PopEventHandler(true);
    m_ROIBottomTxt->PopEventHandler(true);
    m_FileFormatJPEGQualityText->PopEventHandler(true);
    pano->removeObserver(this);
    DEBUG_TRACE("dtor end");
}


void PanoPanel::panoramaChanged (PT::Panorama &pano)
{
    DEBUG_TRACE("");
    if (pano.getNrOfImages() == 0) {
        //m_ProjectionChoice->Disable();
        //m_HFOVSpin->Disable();
        m_CalcHFOVButton->Disable();
        //m_VFOVSpin->Disable();
        //m_WidthTxt->Disable();
        m_CalcOptWidthButton->Disable();
        //m_HeightStaticText->Disable();
        //m_StitcherChoice->Disable();
        m_StitchButton->Disable();
    } else {
        //m_ProjectionChoice->Enable();
        //m_HFOVSpin->Enable();
        m_CalcHFOVButton->Enable();
        //m_VFOVSpin->Enable();
        //m_WidthTxt->Enable();
        m_CalcOptWidthButton->Enable();
        //m_HeightStaticText->Enable();
        m_StitchButton->Enable();
    }
    PanoramaOptions opt = pano.getOptions();
    // update all options for dialog and notebook tab
    UpdateDisplay(opt);
    m_oldOpt = opt;
}

void PanoPanel::UpdateDisplay(const PanoramaOptions & opt)
{

//    m_HFOVSpin->SetRange(1,opt.getMaxHFOV());
//    m_VFOVSpin->SetRange(1,opt.getMaxVFOV());

    m_ProjectionChoice->SetSelection(opt.getProjection());
    m_keepViewOnResize = opt.fovCalcSupported(opt.getProjection());

    std::string val;
    val = doubleToString(opt.getHFOV(),1);
    m_HFOVText->SetValue(wxString(val.c_str(), *wxConvCurrent));
    val = doubleToString(opt.getVFOV(),1);
    m_VFOVText->SetValue(wxString(val.c_str(), *wxConvCurrent));

    // disable VFOV edit field, due to bugs in setHeight(), setWidth()
    m_VFOVText->Enable(m_keepViewOnResize);
    m_CalcOptWidthButton->Enable(m_keepViewOnResize);
    m_CalcHFOVButton->Enable(m_keepViewOnResize);

    m_WidthTxt->SetValue(wxString::Format(wxT("%d"), opt.getWidth()));
    m_HeightTxt->SetValue(wxString::Format(wxT("%d"), opt.getHeight()));

    m_ROILeftTxt->SetValue(wxString::Format(wxT("%d"), opt.getROI().left() ));
    m_ROIRightTxt->SetValue(wxString::Format(wxT("%d"), opt.getROI().right() ));
    m_ROITopTxt->SetValue(wxString::Format(wxT("%d"), opt.getROI().top() ));
    m_ROIBottomTxt->SetValue(wxString::Format(wxT("%d"), opt.getROI().bottom() ));

    // output types
    XRCCTRL(*this, "pano_cb_ldr_output_blended", wxCheckBox)->SetValue(opt.outputLDRBlended);
    XRCCTRL(*this, "pano_cb_ldr_output_layers", wxCheckBox)->SetValue(opt.outputLDRLayers);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_blended", wxCheckBox)->SetValue(opt.outputLDRExposureBlended);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_layers", wxCheckBox)->SetValue(opt.outputLDRExposureLayers);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_remapped", wxCheckBox)->SetValue(opt.outputLDRExposureRemapped);
    XRCCTRL(*this, "pano_cb_hdr_output_blended", wxCheckBox)->SetValue(opt.outputHDRBlended);
    XRCCTRL(*this, "pano_cb_hdr_output_stacks", wxCheckBox)->SetValue(opt.outputHDRStacks);
    XRCCTRL(*this, "pano_cb_hdr_output_layers", wxCheckBox)->SetValue(opt.outputHDRLayers);

    // output file mode
    long i=0;
    if (opt.outputImageType == "tif") {
        i = 0;
        m_FileFormatPanelJPEG->Hide();
        m_FileFormatPanelTIFF->Show();
        if (opt.outputImageTypeCompression  == "DEFLATE") {
            m_FileFormatTIFFCompChoice->SetSelection(1);
        } else if (opt.outputImageTypeCompression == "LZW") {
            m_FileFormatTIFFCompChoice->SetSelection(2);
        } else {
            m_FileFormatTIFFCompChoice->SetSelection(0);
        }
    } else if (opt.outputImageType == "jpg") {
        i = 1;
        m_FileFormatPanelJPEG->Show();
        m_FileFormatPanelTIFF->Hide();
        m_FileFormatJPEGQualityText->SetValue(wxString(opt.outputImageTypeCompression.c_str(), *wxConvCurrent));
    } else if (opt.outputImageType == "png") {
        m_FileFormatPanelJPEG->Hide();
        m_FileFormatPanelTIFF->Hide();
        i = 2;
    } else if (opt.outputImageType == "exr") {
        m_FileFormatPanelJPEG->Hide();
        m_FileFormatPanelTIFF->Hide();
        i = 3;
    } else
        wxLogError(wxT("INTERNAL error: unknown output image type"));

    m_FileFormatChoice->SetSelection(i);

    i=0;
    if (opt.outputImageTypeHDR == "exr") {
        i = 0;
        m_HDRFileFormatPanelTIFF->Hide();
    } else if (opt.outputImageTypeHDR == "tif") {
        i = 1;
        m_HDRFileFormatPanelTIFF->Show();
        if (opt.outputImageTypeHDRCompression  == "DEFLATE") {
            m_FileFormatHDRTIFFCompChoice->SetSelection(1);
        } else if (opt.outputImageTypeHDRCompression == "LZW") {
            m_FileFormatHDRTIFFCompChoice->SetSelection(2);
        } else {
            m_FileFormatHDRTIFFCompChoice->SetSelection(0);
        }
    } else
        wxLogError(wxT("INTERNAL error: unknown hdr output image type"));

    m_HDRFileFormatChoice->SetSelection(i);

    Layout();

}

void PanoPanel::ProjectionChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano->getOptions();
//    PanoramaOptions::ProjectionFormat oldP = opt.getProjection();

    PanoramaOptions::ProjectionFormat newP = (PanoramaOptions::ProjectionFormat) m_ProjectionChoice->GetSelection();
//    int w = opt.getWidth();
//    int h = opt.getHeight();
    opt.setProjection(newP);

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( *pano, opt )
        );
    DEBUG_DEBUG ("Projection changed: "  << newP)
}

void PanoPanel::HFOVChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano->getOptions();


    wxString text = m_HFOVText->GetValue();
    DEBUG_INFO ("HFOV = " << text.mb_str(*wxConvCurrent) );
    if (text == wxT("")) {
        return;
    }

    double hfov;
    if (!str2double(text, hfov)) {
        wxLogError(_("Value must be numeric."));
        return;
    }

    if ( hfov <=0 || hfov > opt.getMaxHFOV()) {
        wxLogError(wxString::Format(
            _("Invalid HFOV value. Maximum HFOV for this projection is %lf."),
            opt.getMaxHFOV()));
    }
    opt.setHFOV(hfov);
    // recalculate panorama height...
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( *pano, opt )
        );

    DEBUG_INFO ( "new hfov: " << hfov )
}

void PanoPanel::VFOVChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano->getOptions();

    wxString text = m_VFOVText->GetValue();
    DEBUG_INFO ("VFOV = " << text.mb_str(*wxConvCurrent) );
    if (text == wxT("")) {
        return;
    }

    double vfov;
    if (!str2double(text, vfov)) {
        wxLogError(_("Value must be numeric."));
        return;
    }

    if ( vfov <=0 || vfov > opt.getMaxVFOV()) {
        wxLogError(wxString::Format(
            _("Invalid VFOV value. Maximum VFOV for this projection is %lf."),
            opt.getMaxVFOV()));
        vfov = opt.getMaxVFOV();
    }
    opt.setVFOV(vfov);
    // recalculate panorama height...
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( *pano, opt )
        );

    DEBUG_INFO ( "new vfov: " << vfov )
}

/*
void PanoPanel::VFOVChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE("")
    if (updatesDisabled) return;
    PanoramaOptions opt = pano->getOptions();
    int vfov = m_VFOVSpin->GetValue() ;

    if (vfov != opt.getVFOV()) {
        opt.setVFOV(vfov);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
        DEBUG_INFO ( "new vfov: " << vfov << " => height: " << opt.getHeight() );
    } else {
        DEBUG_DEBUG("not setting same fov");
    }
}
*/

void PanoPanel::WidthChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano->getOptions();
    long nWidth;
    if (m_WidthTxt->GetValue().ToLong(&nWidth)) {
        if (nWidth <= 0) return;
        opt.setWidth((unsigned int) nWidth, m_keepViewOnResize);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
        DEBUG_INFO(nWidth );
    } else {
        wxLogError(_("width needs to be an integer bigger than 0"));
    }
}

void PanoPanel::HeightChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano->getOptions();
    long nHeight;
    if (m_HeightTxt->GetValue().ToLong(&nHeight)) {
        if(nHeight <= 0) return;
        opt.setHeight((unsigned int) nHeight);
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( *pano, opt )
                                               );
        DEBUG_INFO(nHeight);
    } else {
        wxLogError(_("height needs to be an integer bigger than 0"));
    }
}

void PanoPanel::ROIChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano->getOptions();
    long left, right, top, bottom;
    if (!m_ROITopTxt->GetValue().ToLong(&top)) {
        wxLogError(_("Top needs to be an integer bigger than 0"));
        return;
    }
    if (!m_ROILeftTxt->GetValue().ToLong(&left)) {
        wxLogError(_("left needs to be an integer bigger than 0"));
        return;
    }
    if (!m_ROIRightTxt->GetValue().ToLong(&right)) {
        wxLogError(_("right needs to be an integer bigger than 0"));
        return;
    }
    if (!m_ROIBottomTxt->GetValue().ToLong(&bottom)) {
        wxLogError(_("bottom needs to be an integer bigger than 0"));
        return;
    }
    opt.setROI(vigra::Rect2D(left, top, right, bottom));
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
                                           );
}


void PanoPanel::EnableControls(bool enable)
{
//    m_HFOVSpin->Enable(enable);
//    m_VFOVSpin->Enable(enable);
    m_WidthTxt->Enable(enable);
    m_RemapperChoice->Enable(enable);
    m_BlenderChoice->Enable(enable);
//    m_CalcHFOVButton->Enable(enable);
    m_CalcOptWidthButton->Enable(enable);
}

void PanoPanel::RemapperChanged(wxCommandEvent & e)
{
    int remapper = m_RemapperChoice->GetSelection();
    DEBUG_DEBUG("changing remapper to " << remapper);

    PanoramaOptions opt = pano->getOptions();
    if (remapper == 1) {
        opt.remapper = PanoramaOptions::PTMENDER;
    } else {
        opt.remapper = PanoramaOptions::NONA;
    }

    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
}

void PanoPanel::OnRemapperOptions(wxCommandEvent & e)
{
    PanoramaOptions opt = pano->getOptions();
    if (opt.remapper == PanoramaOptions::NONA) {
        wxLogError(_("Not yet implemented"));
        // wxDialog dlg;
        // dlg.Load
    } else {
        wxLogError(_(" PTmender options not yet implemented"));
    }
}


void PanoPanel::BlenderChanged(wxCommandEvent & e)
{
    int blender = m_BlenderChoice->GetSelection();
    DEBUG_DEBUG("changing stitcher to " << blender);
    // TODO: change panorama options.

    PanoramaOptions opt = pano->getOptions();
    switch (blender) {
        case 1:
            opt.blendMode = PanoramaOptions::NO_BLEND;
            break;
        case 2:
            opt.blendMode = PanoramaOptions::PTMASKER_BLEND;
            break;
        case 3:
            opt.blendMode = PanoramaOptions::PTBLENDER_BLEND;
            break;
        default:
        case 0:
            opt.blendMode = PanoramaOptions::ENBLEND_BLEND;
            break;
    }

    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
}

void PanoPanel::OnBlenderOptions(wxCommandEvent & e)
{
    wxLogError(_("Not yet implemented"));
}


void PanoPanel::DoCalcFOV(wxCommandEvent & e)
{
    DEBUG_TRACE("");

    double hfov, height;
    pano->fitPano(hfov, height);
    PanoramaOptions opt = pano->getOptions();
    opt.setHFOV(hfov);
    opt.setHeight(roundi(height));

    DEBUG_INFO ( "hfov: " << opt.getHFOV() << "  w: " << opt.getWidth() << " h: " << opt.getHeight() << "  => vfov: " << opt.getVFOV()  << "  before update");

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( *pano, opt )
        );

    PanoramaOptions opt2 = pano->getOptions();
    DEBUG_INFO ( "hfov: " << opt2.getHFOV() << "  w: " << opt2.getWidth() << " h: " << opt2.getHeight() << "  => vfov: " << opt2.getVFOV()  << "  after update");

}

void PanoPanel::DoCalcOptimalWidth(wxCommandEvent & e)
{
    PanoramaOptions opt = pano->getOptions();
    unsigned width = pano->calcOptimalWidth();
    if (width > 0) {
        opt.setWidth( width );
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
    }
    DEBUG_INFO ( "new optimal width: " << opt.getWidth() );
}


void PanoPanel::DoStitch()
{
    if (pano->getNrOfImages() == 0) {
        return;
    }

    // save project
    wxCommandEvent dummy;
    MainFrame::Get()->OnSaveProject(dummy);

    // run stitching in a separate process
    wxString ptofile = MainFrame::Get()->getProjectName();
    
    
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    // HuginStitchProject inside main bundle
    wxString hugin_stitch_project = MacGetPathToBundledAppMainExecutableFile(CFSTR("HuginStitchProject.app"));
    if(hugin_stitch_project == wxT(""))
    {
        DEBUG_ERROR("hugin_stitch_project could not be found in the bundle.");
        return;
    }
#elif defined __WXMAC__
    // HuginStitchProject installed in INSTALL_OSX_BUNDLE_DIR
    wxFileName hugin_stitch_project_app(wxT(INSTALL_OSX_BUNDLE_DIR), wxEmptyString);
    hugin_stitch_project_app.AppendDir(wxT("HuginStitchProject.app"));
    CFStringRef stitchProjectAppPath = MacCreateCFStringWithWxString(hugin_stitch_project_app.GetFullPath());
    wxString hugin_stitch_project = MacGetPathToMainExecutableFileOfBundle(stitchProjectAppPath);
    CFRelease(stitchProjectAppPath);
#else
    wxString hugin_stitch_project = wxT("hugin_stitch_project");
#endif
    wxFileName prefixFN(ptofile);
    wxString outputPrefix;
    if (prefixFN.GetExt() == wxT("pto")) {
        outputPrefix = prefixFN.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR, wxPATH_NATIVE) + prefixFN.GetName();
    } else {
        outputPrefix = ptofile;
    }
    
    //TODO: don't overwrite files without warning!
    // wxString command = hugin_stitch_project + wxT(" -o ") + wxQuoteFilename(outputPrefix) + wxT(" ") + wxQuoteFilename(ptofile);
    wxString command = hugin_stitch_project + wxT(" ") + wxQuoteFilename(ptofile);
    
#ifdef __WXGTK__
    // work around a wxExecute bug/problem
    // (endless polling of fd 0 and 1 in hugin_stitch_project)
    wxProcess *my_process = new wxProcess(this);
    my_process->Redirect();

    // Delete itself once processes terminated.
    my_process->Detach();
    wxExecute(command,wxEXEC_ASYNC, my_process);
#else
    wxExecute(command);
#endif

    // hmm, what to do with opening the result????
#if 0
        if (m_Stitcher->Stitch(pano, opt)) {
            int runViewer = wxConfig::Get()->Read(wxT("/Stitcher/RunEditor"), HUGIN_STITCHER_RUN_EDITOR);
            if (runViewer) {
                // TODO: show image after it has been created
                wxString editor = wxConfig::Get()->Read(wxT("/Stitcher/Editor"), wxT(HUGIN_STITCHER_EDITOR));
                wxString args = wxConfig::Get()->Read(wxT("/Stitcher/EditorArgs"), wxT(HUGIN_STITCHER_EDITOR_ARGS));

                if (opt.outputFormat == PanoramaOptions::TIFF_m || opt.outputFormat == PanoramaOptions::TIFF_mask)
                {
                    // TODO: tiff_m case? Open all files?
                } else {
                    wxString quoted = utils::wxQuoteFilename(wxfn);
                    args.Replace(wxT("%f"), quoted);
                    quoted = utils::wxQuoteFilename(wxString(pano->getImage(0).getFilename().c_str(), *wxConvCurrent));
                    args.Replace(wxT("%i"), quoted);

                    wxString cmdline = utils::wxQuoteFilename(editor) + wxT(" ") + args;

                    DEBUG_DEBUG("editor command: " << cmdline.c_str());
                    if (editor != wxT("")) {
                        wxExecute(cmdline, wxEXEC_ASYNC);
                    } else {
#ifdef __WXMSW__
                        // use default viewer on windows
                        SHELLEXECUTEINFO seinfo;
                        memset(&seinfo, 0, sizeof(SHELLEXECUTEINFO));
                        seinfo.cbSize = sizeof(SHELLEXECUTEINFO);
                        seinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
                        seinfo.lpFile = args.c_str();
                        seinfo.lpParameters = args.c_str();
                        if (!ShellExecuteEx(&seinfo)) {
                            wxMessageBox(_("Could not execute command: ") + args, _("ShellExecuteEx failed"), wxCANCEL | wxICON_ERROR);
                        }
#endif
                    }
                }
            }
        }
#endif
}

void PanoPanel::OnDoStitch ( wxCommandEvent & e )
{
    DoStitch();
}

void PanoPanel::FileFormatChanged(wxCommandEvent & e)
{

    int fmt = m_FileFormatChoice->GetSelection();
    DEBUG_DEBUG("changing file format to " << fmt);

    PanoramaOptions opt = pano->getOptions();
    switch (fmt) {
        case 1:
            m_FileFormatPanelJPEG->Show();
            m_FileFormatPanelTIFF->Hide();
            opt.outputImageType ="jpg";
            opt.outputImageTypeCompression = "100";
            break;
        case 2:
            m_FileFormatPanelJPEG->Hide();
            m_FileFormatPanelTIFF->Hide();
            opt.outputImageType ="png";
            opt.outputImageTypeCompression = "";
            break;
        case 3:
            m_FileFormatPanelJPEG->Hide();
            m_FileFormatPanelTIFF->Hide();
            opt.outputImageType ="exr";
            opt.outputImageTypeCompression = "";
            break;
        default:
        case 0:
            m_FileFormatPanelJPEG->Hide();
            m_FileFormatPanelTIFF->Show();
            opt.outputImageType ="tif";
            opt.outputImageTypeCompression = "DEFLATE";
            break;
    }

    Layout();

    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
}

void PanoPanel::HDRFileFormatChanged(wxCommandEvent & e)
{

    int fmt = m_HDRFileFormatChoice->GetSelection();
    DEBUG_DEBUG("changing file format to " << fmt);

    PanoramaOptions opt = pano->getOptions();
    switch (fmt) {
        case 1:
            m_HDRFileFormatPanelTIFF->Show();
            opt.outputImageTypeHDR ="tif";
            opt.outputImageTypeHDRCompression = "DEFLATE";
            break;
        default:
        case 0:
            m_HDRFileFormatPanelTIFF->Hide();
            opt.outputImageTypeHDR ="exr";
            opt.outputImageTypeHDRCompression = "";
            break;
    }

    Layout();

    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
}

void PanoPanel::OnJPEGQualityText(wxCommandEvent & e)
{
    PanoramaOptions opt = pano->getOptions();
    long l = 100;
    m_FileFormatJPEGQualityText->GetValue().ToLong(&l);
    if (l < 0) l=1;
    if (l > 100) l=100;
    DEBUG_DEBUG("Setting jpeg quality to " << l);
    ostringstream s;
    s << l;
    opt.outputImageTypeCompression = s.str();
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
}

void PanoPanel::OnNormalTIFFCompression(wxCommandEvent & e)
{
    PanoramaOptions opt = pano->getOptions();
    switch(e.GetSelection()) {
        case 0:
        default:
            opt.outputImageTypeCompression = "NONE";
            opt.tiffCompression = "NONE";
            break;
        case 1:
            opt.outputImageTypeCompression = "DEFLATE";
            opt.tiffCompression = "DEFLATE";
            break;
        case 2:
            opt.outputImageTypeCompression = "LZW";
            opt.tiffCompression = "LZW";
            break;
    }
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
}

void PanoPanel::OnHDRTIFFCompression(wxCommandEvent & e)
{
    PanoramaOptions opt = pano->getOptions();
    switch(e.GetSelection()) {
        case 0:
        default:
            opt.outputImageTypeHDRCompression = "NONE";
            break;
        case 1:
            opt.outputImageTypeHDRCompression = "DEFLATE";
            break;
        case 2:
            opt.outputImageTypeHDRCompression = "LZW";
            break;
    }
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
}

void PanoPanel::OnOutputFilesChanged(wxCommandEvent & e)
{
    int id = e.GetId();
    PanoramaOptions opts = pano->getOptions();

    if (id == XRCID("pano_cb_ldr_output_blended") ) {
        opts.outputLDRBlended = e.IsChecked();
    } else if (id == XRCID("pano_cb_ldr_output_layers") ) {
        opts.outputLDRLayers = e.IsChecked();
    } else if (id == XRCID("pano_cb_ldr_output_exposure_layers") ) {
        opts.outputLDRExposureLayers = e.IsChecked();
    } else if (id == XRCID("pano_cb_ldr_output_exposure_blended") ) {
        opts.outputLDRExposureBlended = e.IsChecked();
    } else if (id == XRCID("pano_cb_ldr_output_exposure_remapped") ) {
        opts.outputLDRExposureRemapped = e.IsChecked();
    } else if (id == XRCID("pano_cb_hdr_output_blended") ) {
        opts.outputHDRBlended = e.IsChecked();
    } else if (id == XRCID("pano_cb_hdr_output_stacks") ) {
        opts.outputHDRStacks = e.IsChecked();
    } else if (id == XRCID("pano_cb_hdr_output_layers") ) {
        opts.outputHDRLayers = e.IsChecked();
    }

    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opts )
        );
}


IMPLEMENT_DYNAMIC_CLASS(PanoPanel, wxPanel)

PanoPanelXmlHandler::PanoPanelXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *PanoPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, PanoPanel)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow( cp);

    return cp;
}

bool PanoPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("PanoPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(PanoPanelXmlHandler, wxXmlResourceHandler)

