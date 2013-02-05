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
#include <wx/stdpaths.h>

#include "panoinc_WX.h"
#include "panoinc.h"
#include "base_wx/platform.h"


#include <hugin/config_defaults.h>

#include "PT/Stitcher.h"
#include "base_wx/wxPlatform.h"

extern "C" {
#include <pano13/queryfeature.h>
}

#include "hugin/CommandHistory.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/CPImagesComboBox.h"
#include "hugin/PanoPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/HDRMergeOptionDialog.h"
#include "hugin/TextKillFocusHandler.h"
#include "base_wx/MyProgressDialog.h"
#include "hugin/config_defaults.h"
#include "base_wx/platform.h"
#include "base_wx/huginConfig.h"
#include "algorithms/basic/LayerStacks.h"

#define WX_BROKEN_SIZER_UNKNOWN

using namespace PT;
using namespace std;
using namespace hugin_utils;

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
    EVT_BUTTON ( XRCID("pano_button_opt_roi"), PanoPanel::DoCalcOptimalROI)
    EVT_BUTTON ( XRCID("pano_button_stitch"),PanoPanel::OnDoStitch )

    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_blended"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_layers"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_exposure_layers"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_exposure_blended"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_exposure_layers_fused"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_stacks"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_ldr_output_exposure_remapped"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_blended"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_stacks"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_layers"), PanoPanel::OnOutputFilesChanged)

    EVT_CHOICE ( XRCID("pano_choice_remapper"),PanoPanel::RemapperChanged )
    EVT_BUTTON ( XRCID("pano_button_remapper_opts"),PanoPanel::OnRemapperOptions )

    EVT_CHOICE ( XRCID("pano_choice_fusion"),PanoPanel::FusionChanged )
    EVT_BUTTON ( XRCID("pano_button_fusion_opts"),PanoPanel::OnFusionOptions )

    EVT_CHOICE ( XRCID("pano_choice_hdrmerge"),PanoPanel::HDRMergeChanged )
    EVT_BUTTON ( XRCID("pano_button_hdrmerge_opts"),PanoPanel::OnHDRMergeOptions )

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
    : pano(0), updatesDisabled(false), m_guiLevel(GUI_SIMPLE)
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
    m_hasStacks=false;

#ifdef ThisNeverHappens
// provide some translatable strings for the drop down menu
    wxLogMessage(_("Fisheye"));
    wxLogMessage(_("Stereographic"));
    wxLogMessage(_("Mercator"));
    wxLogMessage(_("Trans Mercator"));
    wxLogMessage(_("Sinusoidal"));
    wxLogMessage(_("Lambert Cylindrical Equal Area"));
    wxLogMessage(_("Lambert Equal Area Azimuthal"));
    wxLogMessage(_("Albers Equal Area Conic"));
    wxLogMessage(_("Miller Cylindrical"));
    wxLogMessage(_("Panini"));
    wxLogMessage(_("Architectural"));
    wxLogMessage(_("Orthographic"));
    wxLogMessage(_("Equisolid"));
    wxLogMessage(_("Equirectangular Panini"));
    wxLogMessage(_("Biplane"));
    wxLogMessage(_("Triplane"));
    wxLogMessage(_("Panini General"));
    wxLogMessage(_("Thoby Projection"));
#endif

    /* populate with all available projection types */
    int nP = panoProjectionFormatCount();
    for(int n=0; n < nP; n++) {
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(n, &proj)) {
            wxString str2(proj.name, wxConvLocal);
            m_ProjectionChoice->Append(wxGetTranslation(str2));
        }
    }
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
    
    m_CalcOptROIButton = XRCCTRL(*this, "pano_button_opt_roi" ,wxButton);
    DEBUG_ASSERT(m_CalcOptROIButton);    

    m_RemapperChoice = XRCCTRL(*this, "pano_choice_remapper", wxChoice);
    DEBUG_ASSERT(m_RemapperChoice);
    m_FusionChoice = XRCCTRL(*this, "pano_choice_fusion", wxChoice);
    DEBUG_ASSERT(m_FusionChoice);
    m_HDRMergeChoice = XRCCTRL(*this, "pano_choice_hdrmerge", wxChoice);
    DEBUG_ASSERT(m_HDRMergeChoice);
    m_BlenderChoice = XRCCTRL(*this, "pano_choice_blender", wxChoice);
    DEBUG_ASSERT(m_BlenderChoice);

    m_StitchButton = XRCCTRL(*this, "pano_button_stitch", wxButton);
    DEBUG_ASSERT(m_StitchButton);

    m_FileFormatChoice = XRCCTRL(*this, "pano_choice_file_format", wxChoice);
    DEBUG_ASSERT(m_FileFormatChoice);
    m_FileFormatOptionsLabel = XRCCTRL(*this, "pano_output_ldr_format_options_label", wxStaticText);
    
    m_FileFormatJPEGQualityText = XRCCTRL(*this, "pano_output_normal_opts_jpeg_quality", wxTextCtrl);
    DEBUG_ASSERT(m_FileFormatJPEGQualityText);
    m_FileFormatJPEGQualityText->PushEventHandler(new TextKillFocusHandler(this));

    m_FileFormatTIFFCompChoice = XRCCTRL(*this, "pano_output_normal_opts_tiff_compression", wxChoice);
    DEBUG_ASSERT(m_FileFormatTIFFCompChoice);

    m_HDRFileFormatChoice = XRCCTRL(*this, "pano_choice_hdr_file_format", wxChoice);
    DEBUG_ASSERT(m_HDRFileFormatChoice);
    m_HDRFileFormatLabelTIFFCompression = XRCCTRL(*this, "pano_output_hdr_opts_tiff_compression_label", wxStaticText);
    DEBUG_ASSERT(m_HDRFileFormatLabelTIFFCompression);
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
    m_HeightTxt->PopEventHandler(true);
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

#ifdef STACK_CHECK //Disabled for 0.7.0 release
    const bool hasStacks = StackCheck(pano);
#else
    const bool hasStacks = false;
#endif

    PanoramaOptions opt = pano.getOptions();

    // update all options for dialog and notebook tab
    UpdateDisplay(opt,hasStacks);

    m_oldOpt = opt;
}


bool PanoPanel::StackCheck(PT::Panorama &pano)
{
    DEBUG_TRACE("");
    PanoramaOptions opt = pano.getOptions();

    // Determine if there are stacks in the pano.
    UIntSet activeImages = pano.getActiveImages();
    UIntSet images = getImagesinROI(pano,activeImages);
    vector<UIntSet> hdrStacks = HuginBase::getHDRStacks(pano, images, pano.getOptions());
    DEBUG_DEBUG(hdrStacks.size() << ": HDR stacks detected");
    const bool hasStacks = (hdrStacks.size() != activeImages.size());

    // Only change the output types if the stack configuration has changed.
    bool isChanged = (hasStacks != m_hasStacks);
    if (isChanged) {
        if (hasStacks) {
            // Disable normal output formats
            opt.outputLDRBlended = false;
            opt.outputLDRLayers = false;
            // Ensure at least one fused output is enabled
            if (!(opt.outputLDRExposureBlended ||
                  opt.outputLDRExposureLayers ||
                  opt.outputLDRExposureRemapped ||
                  opt.outputHDRBlended ||
                  opt.outputHDRStacks ||
                  opt.outputHDRLayers)) {
                opt.outputLDRExposureBlended = true;
            }
        } else {
            // Disable fused output formats
            opt.outputLDRExposureBlended = false;
            opt.outputLDRExposureLayers = false;
            opt.outputLDRExposureRemapped = false;
            opt.outputHDRBlended = false;
            opt.outputHDRStacks = false;
            opt.outputHDRLayers = false;
            // Ensure at least one normal output is enabled
            if (!(opt.outputLDRBlended || opt.outputLDRLayers)) {
                opt.outputLDRBlended = true;
            }
        }
        pano.setOptions(opt);
    }
        
    m_hasStacks = hasStacks;

    return hasStacks;
}


void PanoPanel::UpdateDisplay(const PanoramaOptions & opt, const bool hasStacks)
{

//    m_HFOVSpin->SetRange(1,opt.getMaxHFOV());
//    m_VFOVSpin->SetRange(1,opt.getMaxVFOV());

    m_ProjectionChoice->SetSelection(opt.getProjection());
    m_keepViewOnResize = opt.fovCalcSupported(opt.getProjection());

    std::string val;
    val = doubleToString(opt.getHFOV(),1);
    m_HFOVText->SetValue(wxString(val.c_str(), wxConvLocal));
    val = doubleToString(opt.getVFOV(),1);
    m_VFOVText->SetValue(wxString(val.c_str(), wxConvLocal));

    // disable VFOV edit field, due to bugs in setHeight(), setWidth()
    bool hasImages = pano->getActiveImages().size() > 0;
    m_VFOVText->Enable(m_keepViewOnResize);
    m_CalcOptWidthButton->Enable(m_keepViewOnResize && hasImages);
    m_CalcHFOVButton->Enable(m_keepViewOnResize && hasImages);
    m_CalcOptROIButton->Enable(hasImages);

    m_WidthTxt->SetValue(wxString::Format(wxT("%d"), opt.getWidth()));
    m_HeightTxt->SetValue(wxString::Format(wxT("%d"), opt.getHeight()));

    m_ROILeftTxt->SetValue(wxString::Format(wxT("%d"), opt.getROI().left() ));
    m_ROIRightTxt->SetValue(wxString::Format(wxT("%d"), opt.getROI().right() ));
    m_ROITopTxt->SetValue(wxString::Format(wxT("%d"), opt.getROI().top() ));
    m_ROIBottomTxt->SetValue(wxString::Format(wxT("%d"), opt.getROI().bottom() ));

    // output types
    XRCCTRL(*this, "pano_cb_ldr_output_blended", wxCheckBox)->SetValue(opt.outputLDRBlended);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_blended", wxCheckBox)->SetValue(opt.outputLDRExposureBlended);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_layers_fused", wxCheckBox)->SetValue(opt.outputLDRExposureLayersFused);
    XRCCTRL(*this, "pano_cb_hdr_output_blended", wxCheckBox)->SetValue(opt.outputHDRBlended);
    XRCCTRL(*this, "pano_cb_hdr_output_blended", wxCheckBox)->Show(opt.outputHDRBlended || m_guiLevel>GUI_SIMPLE);

    //remapped images
    XRCCTRL(*this, "pano_text_remapped_images", wxStaticText)->Show(opt.outputLDRLayers || opt.outputLDRExposureRemapped || opt.outputHDRLayers || m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_cb_ldr_output_layers", wxCheckBox)->SetValue(opt.outputLDRLayers);
    XRCCTRL(*this, "pano_cb_ldr_output_layers", wxCheckBox)->Show(opt.outputLDRLayers || m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_remapped", wxCheckBox)->SetValue(opt.outputLDRExposureRemapped);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_remapped", wxCheckBox)->Show(opt.outputLDRExposureRemapped || m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_cb_hdr_output_layers", wxCheckBox)->SetValue(opt.outputHDRLayers);
    XRCCTRL(*this, "pano_cb_hdr_output_layers", wxCheckBox)->Show(opt.outputHDRLayers || m_guiLevel>GUI_SIMPLE);

    //stacks
    XRCCTRL(*this, "pano_text_stacks", wxStaticText)->Show(opt.outputHDRStacks || opt.outputLDRStacks || m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_cb_ldr_output_stacks", wxCheckBox)->SetValue(opt.outputLDRStacks);
    XRCCTRL(*this, "pano_cb_ldr_output_stacks", wxCheckBox)->Show(opt.outputLDRStacks || m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_cb_hdr_output_stacks", wxCheckBox)->SetValue(opt.outputHDRStacks);
    XRCCTRL(*this, "pano_cb_hdr_output_stacks", wxCheckBox)->Show(opt.outputHDRStacks || m_guiLevel>GUI_SIMPLE);

    //layers
    XRCCTRL(*this, "pano_text_layers", wxStaticText)->Show(opt.outputLDRExposureLayers || m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_layers", wxCheckBox)->SetValue(opt.outputLDRExposureLayers);
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_layers", wxCheckBox)->Show(opt.outputLDRExposureLayers || m_guiLevel>GUI_SIMPLE);

    bool anyOutputSelected = (opt.outputLDRBlended || 
                              opt.outputLDRLayers || 
                              opt.outputLDRExposureLayers || 
                              opt.outputLDRExposureBlended || 
                              opt.outputLDRExposureLayersFused || 
                              opt.outputLDRExposureRemapped || 
                              opt.outputLDRStacks ||
                              opt.outputHDRBlended || 
                              opt.outputHDRStacks || 
                              opt.outputHDRLayers);
    
    //do not let the user stitch unless there are active images and an output selected.
    bool any_output_possible = hasImages && anyOutputSelected;
    m_StitchButton->Enable(any_output_possible);

#ifdef STACK_CHECK //Disabled for 0.7.0 release
    if (hasStacks) {
        XRCCTRL(*this,"pano_cb_ldr_output_blended",wxCheckBox)->Disable();
        XRCCTRL(*this,"pano_cb_ldr_output_layers",wxCheckBox)->Disable();

        XRCCTRL(*this,"pano_cb_ldr_output_exposure_layers",wxCheckBox)->Enable();
        XRCCTRL(*this,"pano_cb_ldr_output_exposure_blended",wxCheckBox)->Enable();
        XRCCTRL(*this,"pano_cb_ldr_output_exposure_remapped",wxCheckBox)->Enable();
        XRCCTRL(*this,"pano_cb_hdr_output_blended",wxCheckBox)->Enable();
        XRCCTRL(*this,"pano_cb_hdr_output_stacks",wxCheckBox)->Enable();
        XRCCTRL(*this,"pano_cb_hdr_output_layers",wxCheckBox)->Enable();

    } else {
        XRCCTRL(*this,"pano_cb_ldr_output_blended",wxCheckBox)->Enable();
        XRCCTRL(*this,"pano_cb_ldr_output_layers",wxCheckBox)->Enable();

        XRCCTRL(*this,"pano_cb_ldr_output_exposure_layers",wxCheckBox)->Disable();
        XRCCTRL(*this,"pano_cb_ldr_output_exposure_blended",wxCheckBox)->Disable();
        XRCCTRL(*this,"pano_cb_ldr_output_exposure_remapped",wxCheckBox)->Disable();

        XRCCTRL(*this,"pano_cb_hdr_output_blended",wxCheckBox)->Disable();
        XRCCTRL(*this,"pano_cb_hdr_output_stacks",wxCheckBox)->Disable();
        XRCCTRL(*this,"pano_cb_hdr_output_layers",wxCheckBox)->Disable();
    }
#endif

    m_RemapperChoice->Show(m_guiLevel>GUI_SIMPLE);
    m_RemapperChoice->Enable(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_button_remapper_opts", wxButton)->Show(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_button_remapper_opts", wxButton)->Enable(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_text_remapper", wxStaticText)->Show(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_text_processing", wxStaticText)->Show(m_guiLevel>GUI_SIMPLE);

    bool blenderEnabled = (opt.outputLDRBlended || 
                          opt.outputLDRExposureBlended || 
                          opt.outputLDRExposureLayersFused || 
                          opt.outputLDRExposureLayers || 
                          opt.outputHDRBlended ) && m_guiLevel>GUI_SIMPLE;

    m_BlenderChoice->Enable(blenderEnabled);
    m_BlenderChoice->Show(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_button_blender_opts", wxButton)->Enable(blenderEnabled);
    XRCCTRL(*this, "pano_button_blender_opts", wxButton)->Show(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_text_blender", wxStaticText)->Enable(blenderEnabled);
    XRCCTRL(*this, "pano_text_blender", wxStaticText)->Show(m_guiLevel>GUI_SIMPLE);

    bool fusionEnabled = (opt.outputLDRExposureBlended || opt.outputLDRExposureLayersFused || opt.outputLDRStacks) && m_guiLevel>GUI_SIMPLE;
    m_FusionChoice->Enable(fusionEnabled);
    m_FusionChoice->Show(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_button_fusion_opts", wxButton)->Enable(fusionEnabled);
    XRCCTRL(*this, "pano_button_fusion_opts", wxButton)->Show(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_text_fusion", wxStaticText)->Enable(fusionEnabled);
    XRCCTRL(*this, "pano_text_fusion", wxStaticText)->Show(m_guiLevel>GUI_SIMPLE);

    bool hdrMergeEnabled = (opt.outputHDRBlended || opt.outputHDRStacks) && m_guiLevel>GUI_SIMPLE;
    m_HDRMergeChoice->Enable(hdrMergeEnabled);
    m_HDRMergeChoice->Show(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_button_hdrmerge_opts", wxButton)->Enable(hdrMergeEnabled);
    XRCCTRL(*this, "pano_button_hdrmerge_opts", wxButton)->Show(m_guiLevel>GUI_SIMPLE);
    XRCCTRL(*this, "pano_text_hdrmerge", wxStaticText)->Enable(hdrMergeEnabled);
    XRCCTRL(*this, "pano_text_hdrmerge", wxStaticText)->Show(m_guiLevel>GUI_SIMPLE);

    // output file mode
    bool ldr_pano_enabled = opt.outputLDRBlended ||
                            opt.outputLDRExposureBlended ||
                            opt.outputLDRExposureLayersFused;
    
    XRCCTRL(*this, "pano_output_ldr_format_label", wxStaticText)->Enable(ldr_pano_enabled);
    m_FileFormatOptionsLabel->Enable(ldr_pano_enabled);
    m_FileFormatChoice->Enable(ldr_pano_enabled);
    m_FileFormatJPEGQualityText->Enable(ldr_pano_enabled);
    m_FileFormatTIFFCompChoice->Enable(ldr_pano_enabled);
    
    long i=0;
    if (opt.outputImageType == "tif") {
        i = 0;
        m_FileFormatOptionsLabel->Show();
        m_FileFormatOptionsLabel->SetLabel(_("Compression:"));
        m_FileFormatJPEGQualityText->Hide();
        m_FileFormatTIFFCompChoice->Show();
        if (opt.outputImageTypeCompression  == "PACKBITS") {
            m_FileFormatTIFFCompChoice->SetSelection(1);
        } else if (opt.outputImageTypeCompression == "LZW") {
            m_FileFormatTIFFCompChoice->SetSelection(2);
        } else if (opt.outputImageTypeCompression  == "DEFLATE") {
            m_FileFormatTIFFCompChoice->SetSelection(3);
        } else {
            m_FileFormatTIFFCompChoice->SetSelection(0);
        }
    } else if (opt.outputImageType == "jpg") {
        i = 1;
        m_FileFormatOptionsLabel->Show();
        m_FileFormatOptionsLabel->SetLabel(_("Quality:"));
        m_FileFormatJPEGQualityText->Show();
        m_FileFormatTIFFCompChoice->Hide();
        m_FileFormatJPEGQualityText->SetValue(wxString::Format(wxT("%d"), opt.quality));
    } else if (opt.outputImageType == "png") {
        m_FileFormatOptionsLabel->Hide();
        m_FileFormatJPEGQualityText->Hide();
        m_FileFormatTIFFCompChoice->Hide();
        i = 2;
    } else if (opt.outputImageType == "exr") {
        /// @todo Is this right? I don't see a 4th item in the combo box, and exr is a confusing LDR format.
        m_FileFormatOptionsLabel->Hide();
        m_FileFormatJPEGQualityText->Hide();
        m_FileFormatTIFFCompChoice->Hide();
        i = 3;
    } else
        wxLogError(wxT("INTERNAL error: unknown output image type"));

    m_FileFormatChoice->SetSelection(i);

    bool hdr_pano_enabled = opt.outputHDRBlended;
    
    XRCCTRL(*this, "pano_output_hdr_format_label", wxStaticText)->Enable(hdr_pano_enabled);
    XRCCTRL(*this, "pano_output_hdr_format_label", wxStaticText)->Show(hdr_pano_enabled || m_guiLevel>GUI_SIMPLE);
    m_HDRFileFormatChoice->Enable(hdr_pano_enabled);
    m_HDRFileFormatChoice->Show(hdr_pano_enabled || m_guiLevel>GUI_SIMPLE);
    m_HDRFileFormatLabelTIFFCompression->Enable(hdr_pano_enabled);
    m_HDRFileFormatLabelTIFFCompression->Show(hdr_pano_enabled || m_guiLevel>GUI_SIMPLE);
    m_FileFormatHDRTIFFCompChoice->Enable(hdr_pano_enabled);
    m_FileFormatHDRTIFFCompChoice->Show(hdr_pano_enabled || m_guiLevel>GUI_SIMPLE);
    
    i=0;
    if (opt.outputImageTypeHDR == "exr") {
        i = 0;
        m_HDRFileFormatLabelTIFFCompression->Hide();
        m_FileFormatHDRTIFFCompChoice->Hide();
    } else if (opt.outputImageTypeHDR == "tif") {
        i = 1;
        m_HDRFileFormatLabelTIFFCompression->Show();
        m_FileFormatHDRTIFFCompChoice->Show();
        if (opt.outputImageTypeHDRCompression  == "PACKBITS") {
            m_FileFormatHDRTIFFCompChoice->SetSelection(1);
        } else if (opt.outputImageTypeHDRCompression == "LZW") {
            m_FileFormatHDRTIFFCompChoice->SetSelection(2);
        } else if (opt.outputImageTypeHDRCompression  == "DEFLATE") {
            m_FileFormatHDRTIFFCompChoice->SetSelection(3);
        } else {
            m_FileFormatHDRTIFFCompChoice->SetSelection(0);
        }
    } else
        wxLogError(wxT("INTERNAL error: unknown hdr output image type"));

    m_HDRFileFormatChoice->SetSelection(i);

    m_pano_ctrls->FitInside();
    Layout();

#ifdef __WXMSW__
    this->Refresh(false);
#endif

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
    DEBUG_INFO ("HFOV = " << text.mb_str(wxConvLocal) );
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
    DEBUG_INFO ("VFOV = " << text.mb_str(wxConvLocal) );
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

    // make sure that left is really to the left of right
    if(opt.getROI().width()<1) {
        wxLogError(_("Left boundary must be smaller than right."));
        UpdateDisplay(pano->getOptions(), false);
        return;
    }
    // make sure that top is really higher than bottom
    if(opt.getROI().height()<1) {
        wxLogError(_("Top boundary must be smaller than bottom."));
        UpdateDisplay(pano->getOptions(), false);
        return;
    }

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
//    m_CalcOptWidthButton->Enable(enable);
//    m_CalcOptROIButton->Enable(enable);
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
        wxDialog dlg;
        wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("nona_options_dialog"));
        wxChoice * interpol_choice = XRCCTRL(dlg, "nona_choice_interpolator", wxChoice);
        wxCheckBox * cropped_cb = XRCCTRL(dlg, "nona_save_cropped", wxCheckBox);
        interpol_choice->SetSelection(opt.interpolator);
        cropped_cb->SetValue(opt.tiff_saveROI);
        dlg.CentreOnParent();

        if (dlg.ShowModal() == wxID_OK) {
            int interpol = interpol_choice->GetSelection();
            if (interpol >= 0) {
                opt.interpolator = (vigra_ext::Interpolator) interpol;
            }
            opt.tiff_saveROI = cropped_cb->GetValue();
            GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( *pano, opt )
                );
        }
    } else {
        wxLogError(_(" PTmender options not yet implemented"));
    }
}

void PanoPanel::BlenderChanged(wxCommandEvent & e)
{
    int blender = m_BlenderChoice->GetSelection();
    DEBUG_DEBUG("changing stitcher to " << blender);

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
    PanoramaOptions opt = pano->getOptions();
    if (opt.blendMode == PanoramaOptions::ENBLEND_BLEND) {
        wxDialog dlg;
        wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("enblend_options_dialog"));
        wxTextCtrl * enblend_opts_text = XRCCTRL(dlg, "blender_arguments_text", wxTextCtrl);
        enblend_opts_text->SetValue(wxString(opt.enblendOptions.c_str(), wxConvLocal));
        dlg.CentreOnParent();

        if (dlg.ShowModal() == wxID_OK) {
            if (enblend_opts_text->GetValue().length() > 0) {
                opt.enblendOptions = enblend_opts_text->GetValue().mb_str(wxConvLocal);
            }
            else
            {
                opt.enblendOptions = wxConfigBase::Get()->Read(wxT("Enblend/Args"),wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
            };
            GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( *pano, opt )
                );
        }
    } else {
        wxLogError(_(" PTblender options not yet implemented"));
    }
}

void PanoPanel::FusionChanged(wxCommandEvent & e)
{
    int fusion = m_FusionChoice->GetSelection();
    DEBUG_DEBUG("changing stacking program to " << fusion);
}

void PanoPanel::OnFusionOptions(wxCommandEvent & e)
{
    PanoramaOptions opt = pano->getOptions();
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("enfuse_options_dialog"));
    wxTextCtrl * enfuse_opts_text = XRCCTRL(dlg, "enfuse_arguments_text", wxTextCtrl);
    enfuse_opts_text->SetValue(wxString(opt.enfuseOptions.c_str(), wxConvLocal));
    dlg.CentreOnParent();

    if (dlg.ShowModal() == wxID_OK) {
        if (enfuse_opts_text->GetValue().length() > 0) {
            opt.enfuseOptions = enfuse_opts_text->GetValue().mb_str(wxConvLocal);
        }
        else
        {
            opt.enfuseOptions = wxConfigBase::Get()->Read(wxT("Enfuse/Args"),wxT(HUGIN_ENFUSE_ARGS)).mb_str(wxConvLocal);
        };
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
    }
}


void PanoPanel::HDRMergeChanged(wxCommandEvent & e)
{
    int blender = m_HDRMergeChoice->GetSelection();
    DEBUG_DEBUG("changing HDR merger to " << blender);
}

void PanoPanel::OnHDRMergeOptions(wxCommandEvent & e)
{
    PanoramaOptions opt = pano->getOptions();
    if (opt.hdrMergeMode == PanoramaOptions::HDRMERGE_AVERAGE) {
        HDRMergeOptionsDialog dlg(this);
        dlg.SetCommandLineArgument(wxString(opt.hdrmergeOptions.c_str(), wxConvLocal));
        if (dlg.ShowModal() == wxOK) 
        {
            opt.hdrmergeOptions=dlg.GetCommandLineArgument().mb_str(wxConvLocal);
            GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( *pano, opt )
                );
        }
    } else {
        wxLogError(_(" Options for this HDRMerge program not yet implemented"));
    }
}



void PanoPanel::DoCalcFOV(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (pano->getActiveImages().size() == 0) return;

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
    if (pano->getActiveImages().size() == 0) return;

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


void PanoPanel::DoCalcOptimalROI(wxCommandEvent & e)
{
    DEBUG_INFO("Dirty ROI Calc\n");
    if (pano->getActiveImages().size() == 0)
    {
        return;
    };

    vigra::Rect2D newROI;
    vigra::Size2D newSize;
    {
        ProgressReporterDialog progress(2, _("Autocrop"), _("Calculating optimal crop"), this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME);
        progress.increaseProgress(1);
        progress.Pulse();
        pano->calcOptimalROI(newROI,newSize);
    };
#ifdef __WXMSW__
    //try to workaround an issue that the main window lost it focus after wxProgressDialog is destroyed
    MainFrame::Get()->Raise();
#endif

    PanoramaOptions opt = pano->getOptions();
    //set the ROI - fail if the right/bottom is zero, meaning all zero
    if(newROI.right() != 0 && newROI.bottom() != 0)
    {
        opt.setROI(newROI);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
    };
};

void PanoPanel::DoStitch()
{
    if (pano->getNrOfImages() == 0) {
        return;
    }
    
    if (!CheckGoodSize()) {
        // oversized pano and the user no longer wants to stitch.
        return;
    }

    // save project
    // copy pto file to temporary file
    wxString tempDir= wxConfigBase::Get()->Read(wxT("tempDir"),wxT(""));
    if(!tempDir.IsEmpty())
        if(tempDir.Last()!=wxFileName::GetPathSeparator())
            tempDir.Append(wxFileName::GetPathSeparator());
    wxString currentPTOfn = wxFileName::CreateTempFileName(tempDir+wxT("huginpto_"));
    if(currentPTOfn.size() == 0) {
        wxMessageBox(_("Could not create temporary project file"),_("Error"),
                wxCANCEL | wxICON_ERROR,this);
        return;
    }
    DEBUG_DEBUG("tmp PTO file: " << (const char *)currentPTOfn.mb_str(wxConvLocal));
    // copy is not enough, need to adjust image path names...
    ofstream script(currentPTOfn.mb_str(HUGIN_CONV_FILENAME));
    PT::UIntSet all;
    if (pano->getNrOfImages() > 0) {
        fill_set(all, 0, pano->getNrOfImages()-1);
    }
    pano->printPanoramaScript(script, pano->getOptimizeVector(), pano->getOptions(), all, false, "");
    script.close();

//    wxCommandEvent dummy;
//    MainFrame::Get()->OnSaveProject(dummy);

#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    // HuginStitchProject inside main bundle
    wxString hugin_stitch_project = MacGetPathToBundledAppMainExecutableFile(CFSTR("HuginStitchProject.app"));
    if(hugin_stitch_project == wxT(""))
    {
        DEBUG_ERROR("hugin_stitch_project could not be found in the bundle.");
        return;
    }
    hugin_stitch_project = wxQuoteFilename(hugin_stitch_project);
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

    // Derive a default output prefix from the project filename if set, otherwise default project filename
    wxFileName outputPrefix(getDefaultOutputName(MainFrame::Get()->getProjectName(), *pano));
    outputPrefix.Normalize();

    // Show a file save dialog so user can confirm/change the prefix.
    // (We don't have to worry about overwriting existing files, since hugin_switch_project checks this.)
    // TODO: The following code is similar to stitchApp::OnInit in hugin_switch_project.cpp. Should be refactored.
    // TODO: We should save the output prefix somewhere, so we can recall it as the default if the user stitches this project again.
    wxFileDialog dlg(this,_("Specify output prefix"),
                     outputPrefix.GetPath(), outputPrefix.GetName(), wxT(""),
                     wxFD_SAVE, wxDefaultPosition);
    if (dlg.ShowModal() != wxID_OK)
    {
        return;
    };
    while(containsInvalidCharacters(dlg.GetPath()))
    {
        wxArrayString list;
        list.Add(dlg.GetPath());
        ShowFilenameWarning(this, list);
        if(dlg.ShowModal()!=wxID_OK)
            return;
    };

    wxString switches(wxT(" --delete -o "));
    if(wxConfigBase::Get()->Read(wxT("/Processor/overwrite"), HUGIN_PROCESSOR_OVERWRITE) == 1)
        switches=wxT(" --overwrite")+switches;
    wxString command = hugin_stitch_project + switches + wxQuoteFilename(dlg.GetPath()) + wxT(" ") + wxQuoteFilename(currentPTOfn);
    
    wxConfigBase::Get()->Flush();
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

}

void PanoPanel::DoSendToBatch()
{
    if (pano->getNrOfImages() == 0)
    {
        return;
    }
    
    if (!CheckGoodSize())
    {
        // oversized pano and the user no longer wants to stitch.
        return;
    }
    wxString switches(wxT(" "));
    if (wxConfigBase::Get()->Read(wxT("/Processor/start"), HUGIN_PROCESSOR_START) != 0)
    {
        switches += wxT("-b ");
    }
    if (wxConfigBase::Get()->Read(wxT("/Processor/parallel"), HUGIN_PROCESSOR_PARALLEL) != 0)
    {
        switches += wxT("-p ");
    }
    if (wxConfigBase::Get()->Read(wxT("/Processor/overwrite"), HUGIN_PROCESSOR_OVERWRITE) != 0)
    {
        switches += wxT("-o ");
    }
    if (wxConfigBase::Get()->Read(wxT("/Processor/verbose"), HUGIN_PROCESSOR_VERBOSE) != 0)
    {
        switches += wxT("-v ");
    }
    if(pano->isDirty())
    {
        bool showDlg=wxConfigBase::Get()->Read(wxT("ShowSaveMessage"), 1l)==1;
        if(showDlg)
        {
            wxDialog dlg;
            wxXmlResource::Get()->LoadDialog(&dlg, NULL, wxT("stitch_message_dlg"));
            if(dlg.ShowModal())
            {
                if(XRCCTRL(dlg, "stitch_dont_show_checkbox", wxCheckBox)->IsChecked())
                {
                    wxConfigBase::Get()->Write(wxT("ShowSaveMessage"), 0l);
                };
            };
        };
        wxCommandEvent dummy;
        MainFrame::Get()->OnSaveProject(dummy);
        //test if save was sucessful
        if(pano->isDirty())
        {
            return;
        };
    };
    wxString projectFile = MainFrame::Get()->getProjectName();
    if(wxFileName::FileExists(projectFile))
    {
        wxFileName outputPrefix(getDefaultOutputName(projectFile, *pano));
        outputPrefix.Normalize();

        // Show a file save dialog so user can confirm/change the prefix.
        // (We don't have to worry about overwriting existing files, since PTBatcherGUI checks this, or the overwrite flag was set.)
        wxFileDialog dlg(this,_("Specify output prefix"),
                         outputPrefix.GetPath(), outputPrefix.GetName(), wxT(""),
                         wxFD_SAVE, wxDefaultPosition);
        if (dlg.ShowModal() != wxID_OK)
        {
            return;
        };
        while(containsInvalidCharacters(dlg.GetPath()))
        {
            wxArrayString list;
            list.Add(dlg.GetPath());
            ShowFilenameWarning(this, list);
            if(dlg.ShowModal()!=wxID_OK)
                return;
        };

#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
		wxString cmd = MacGetPathToMainExecutableFileOfRegisteredBundle(CFSTR("net.sourceforge.hugin.PTBatcherGUI"));
		if(cmd != wxT(""))
		{ 
			//Found PTBatcherGui inside the (registered) PTBatcherGui bundle. Call it directly.
			//We need to call the binary from it's own bundle and not from the hugin bundle otherwise we get no menu as OSX assumes that the hugin bundle
			//will provide the menu
			cmd = wxQuoteString(cmd); 
			cmd += wxT(" ")+switches+wxQuoteFilename(projectFile)+wxT(" ")+wxQuoteFilename(dlg.GetPath());	
			wxExecute(cmd);
		}
		else
		{ //Can't find PTBatcherGui.app bundle. Use the most straightforward call possible to the bundle but this should actually not work either.
				wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), wxT("open")), _("Error"));
				cmd = wxT("open -b net.sourceforge.hugin.PTBatcherGUI ")+wxQuoteFilename(projectFile);
				wxExecute(cmd);
		}
		
#else
#ifdef __WINDOWS__
        wxString huginPath = getExePath(wxGetApp().argv[0])+wxFileName::GetPathSeparator();
#else
        wxString huginPath = wxT(""); //we call the batch processor directly without path on linux
#endif
        wxExecute(huginPath+wxT("PTBatcherGUI")+switches+wxQuoteFilename(projectFile)+wxT(" ")+wxQuoteFilename(dlg.GetPath()));
#endif
    }
};

void PanoPanel::OnDoStitch ( wxCommandEvent & e )
{
    long t;
#if wxCHECK_VERSION(2,9,4)
    if(wxGetKeyState(WXK_COMMAND))
#else
    if(wxGetKeyState(WXK_CONTROL))
#endif
    {
        t=1;
    }
    else
    {
        wxConfigBase::Get()->Read(wxT("/Processor/gui"),&t,HUGIN_PROCESSOR_GUI);
    };
    switch (t)
    {
        // PTBatcher
        case 0:
            DoSendToBatch();
            break;
        // hugin_stitch_project
        case 1:
            DoStitch();
            break;
        // there is an error in the preferences
        default :
            // TODO: notify user and fix preferences misconfiguration
            break;
      }
}

void PanoPanel::FileFormatChanged(wxCommandEvent & e)
{

    int fmt = m_FileFormatChoice->GetSelection();
    DEBUG_DEBUG("changing file format to " << fmt);

    PanoramaOptions opt = pano->getOptions();
    switch (fmt) {
        case 1:
            opt.outputImageType ="jpg";
            break;
        case 2:
            opt.outputImageType ="png";
            break;
        case 3:
            opt.outputImageType ="exr";
            break;
        default:
        case 0:
            opt.outputImageType ="tif";
            break;
    }

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
            opt.outputImageTypeHDR ="tif";
            break;
        default:
        case 0:
            opt.outputImageTypeHDR ="exr";
            break;
    }

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
    opt.quality = l;
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
            opt.outputImageTypeCompression = "PACKBITS";
            opt.tiffCompression = "PACKBITS";
            break;
        case 2:
            opt.outputImageTypeCompression = "LZW";
            opt.tiffCompression = "LZW";
            break;
        case 3:
            opt.outputImageTypeCompression = "DEFLATE";
            opt.tiffCompression = "DEFLATE";
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
            opt.outputImageTypeHDRCompression = "PACKBITS";
            break;
        case 2:
            opt.outputImageTypeHDRCompression = "LZW";
            break;
        case 3:
            opt.outputImageTypeHDRCompression = "DEFLATE";
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
    } else if (id == XRCID("pano_cb_ldr_output_exposure_layers_fused") ) {
        opts.outputLDRExposureLayersFused = e.IsChecked();
    } else if (id == XRCID("pano_cb_ldr_output_exposure_remapped") ) {
        opts.outputLDRExposureRemapped = e.IsChecked();
    } else if (id == XRCID("pano_cb_ldr_output_stacks") ) {
        opts.outputLDRStacks = e.IsChecked();
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

bool PanoPanel::CheckGoodSize()
{
    vigra::Rect2D cropped_region = pano->getOptions().getROI();
    unsigned long long int area = ((unsigned long int) cropped_region.width()) * ((unsigned long int) cropped_region.height());
    // Argh, more than half a gigapixel!
    if (area > 500000000)
    {
        // Tell the user the stitch will be really big, and give them a
        // chance to reduce the size.
#if wxCHECK_VERSION(2,9,0)
        wxMessageDialog dialog(this,
                _("Are you sure you want to stitch such a large panorama?"),
#ifdef _WINDOWS
                _("Hugin"),
#else
                wxT(""),
#endif
                wxICON_EXCLAMATION | wxYES_NO);
        dialog.SetExtendedMessage(
                wxString::Format(_("The panorama you are trying to stitch is %.1f gigapixels.\nIf this is too big, reduce the panorama Canvas Size and the cropped region and stitch from the Stitcher tab. Stitching a panorama this size could take a long time and a large amount of memory."),
                        area / 1000000000.0));
        dialog.SetYesNoLabels(_("Stitch anyway"), _("Let me fix that"));
#else // replacement for old wxWidgets versions.
        // wxMessageDialog derives from wxDialog, but I don't understand
        // why because on most platforms wxMessageDialog uses the native
        // message box, and trying to make descriptive buttons through
        // wxDialog::CreateStdButtonSizer causes a crash on wxGTK.
        // Descriptive buttons are recommended by the Windows, Gnome, KDE,
        // and Apple user interface guidelines.
        // Due to this wxWidgets WTF, the buttons will are labeled Yes and
        // No on wxWidgets 2.8 and earlier. This makes it a little
        // confusing, and it is more likely someone will just click yes
        // without reading the message and then wonder why their computer
        // has ground to a halt.
        /** @todo (Possibly) make a dialog manually with properly labelled
         * buttons.
         */
        wxMessageDialog dialog(this,
                wxString::Format(_("Are you sure you want to stitch such a large panorama?\n\nThe panorama you are trying to stitch is %.1f gigapixels.\nIf this is too big, reduce the panorama Canvas Size and the cropped region and stitch from the Stitcher tab. Stitching a panorama this size could take a long time and a large amount of memory."),
                        area / 1000000000.0),
#ifdef _WINDOWS
                _("Hugin"),
#else
                wxT(""),
#endif
                wxICON_EXCLAMATION | wxYES_NO);
#endif
        switch (dialog.ShowModal())
        {
            case wxID_OK:
            case wxID_YES:
                // Continue stitch.
                return true;
                break;
            default:
                // bring the user towards the approptiate controls.
                MainFrame::Get()->ShowStitcherTab();
                return false;
        }
    }
    // I see nothing wrong with this...
    return true;
}

void PanoPanel::SetGuiLevel(GuiLevel newGuiLevel)
{
    m_guiLevel=newGuiLevel;
    UpdateDisplay(m_oldOpt, false);
};

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

