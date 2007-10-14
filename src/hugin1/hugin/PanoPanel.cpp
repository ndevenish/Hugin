// -*- c-basic-offset: 4 -*-

/** @file PanoPanel.cpp
 *
 *  @brief implementation of PanoPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoPanel.cpp 1994 2007-05-09 11:57:45Z dangelo $
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
#include <hugin/config_defaults.h>

#include "panoinc_WX.h"

#include "panoinc.h"

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
#include "hugin/MyProgressDialog.h"
#include "hugin/PTStitcherPanel.h"
#include "hugin/NonaStitcherPanel.h"
#include "hugin/config_defaults.h"
#include "hugin/MyExternalCmdExecDialog.h"

#define WX_BROKEN_SIZER_UNKNOWN

using namespace PT;
using namespace std;
using namespace utils;

BEGIN_EVENT_TABLE(PanoPanel, wxWindow)
    EVT_SIZE   ( PanoPanel::OnSize )
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
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_blended"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_stacks"), PanoPanel::OnOutputFilesChanged)
    EVT_CHECKBOX ( XRCID("pano_cb_hdr_output_layers"), PanoPanel::OnOutputFilesChanged)

    EVT_CHOICE ( XRCID("pano_choice_remapper"),PanoPanel::RemapperChanged )
    EVT_CHOICE ( XRCID("pano_choice_blender"),PanoPanel::BlenderChanged )
    EVT_CHOICE ( XRCID("pano_choice_interpolator"),PanoPanel::InterpolatorChanged)
    EVT_CHECKBOX( XRCID("pano_cb_remapper_cropped"), PanoPanel::OnRemapperCropped)



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

    m_InterpolatorChoice = XRCCTRL(*this, "pano_choice_interpolator",
                                    wxChoice);
    DEBUG_ASSERT(m_InterpolatorChoice);

#ifdef USE_WX253
    m_pano_ctrls = XRCCTRL(*this, "pano_controls_panel", wxScrolledWindow);
    DEBUG_ASSERT(m_pano_ctrls);
    m_pano_ctrls->SetSizeHints(20, 20);
    m_pano_ctrls->FitInside();
    m_pano_ctrls->SetScrollRate(10, 10);
#endif

    // observe the panorama
    pano->addObserver (this);


    // setup the stitcher
#if (defined NO_PTSTITCHER) || (defined __WXMAC__)
    // disable stitcher choice and select nona (default),
    // since PTStitcher is not available on OSX
    m_RemapperChoice->Disable();
#endif


/*
    // trigger creation of apropriate stitcher control, if
    // not already happend.
    if (! m_Stitcher) {
        wxCommandEvent dummy;
        StitcherChanged(dummy);
    }
*/
    DEBUG_TRACE("")
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
    pano.removeObserver(this);
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
    XRCCTRL(*this, "pano_cb_ldr_output_exposure_layers", wxCheckBox)->SetValue(opt.outputLDRExposureLayers);
    XRCCTRL(*this, "pano_cb_hdr_output_blended", wxCheckBox)->SetValue(opt.outputHDRBlended);
    XRCCTRL(*this, "pano_cb_hdr_output_stacks", wxCheckBox)->SetValue(opt.outputHDRStacks);
    XRCCTRL(*this, "pano_cb_hdr_output_layers", wxCheckBox)->SetValue(opt.outputHDRLayers);

    XRCCTRL(*this, "pano_cb_remapper_cropped", wxCheckBox)->SetValue(opt.tiff_saveROI);
}

void PanoPanel::ProjectionChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
//    PanoramaOptions::ProjectionFormat oldP = opt.getProjection();

    PanoramaOptions::ProjectionFormat newP = (PanoramaOptions::ProjectionFormat) m_ProjectionChoice->GetSelection();
//    int w = opt.getWidth();
//    int h = opt.getHeight();
    opt.setProjection(newP);

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Projection changed: "  << newP)
}

void PanoPanel::HFOVChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();


    wxString text = m_HFOVText->GetValue();
    DEBUG_INFO ("HFOV = " << text.mb_str() );
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
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO ( "new hfov: " << hfov )
}

void PanoPanel::VFOVChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();

    wxString text = m_VFOVText->GetValue();
    DEBUG_INFO ("VFOV = " << text.mb_str() );
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
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO ( "new vfov: " << vfov )
}

/*
void PanoPanel::VFOVChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE("")
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
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
    PanoramaOptions opt = pano.getOptions();
    long nWidth;
    if (m_WidthTxt->GetValue().ToLong(&nWidth)) {
        if (nWidth <= 0) return;
        opt.setWidth((unsigned int) nWidth, m_keepViewOnResize);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
        DEBUG_INFO(nWidth );
    } else {
        wxLogError(_("width needs to be an integer bigger than 0"));
    }
}

void PanoPanel::HeightChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    long nHeight;
    if (m_HeightTxt->GetValue().ToLong(&nHeight)) {
        if(nHeight <= 0) return;
        opt.setHeight((unsigned int) nHeight);
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( pano, opt )
                                               );
        DEBUG_INFO(nHeight);
    } else {
        wxLogError(_("height needs to be an integer bigger than 0"));
    }
}

void PanoPanel::ROIChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
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
            new PT::SetPanoOptionsCmd( pano, opt )
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
    // TODO: change panorama options..

    PanoramaOptions opt = pano.getOptions();
    if (remapper == 1) {
        opt.remapper = PanoramaOptions::PTMENDER;
    } else {
        opt.remapper = PanoramaOptions::NONA;
    }

    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
}

void PanoPanel::BlenderChanged(wxCommandEvent & e)
{
    int blender = m_BlenderChoice->GetSelection();
    DEBUG_DEBUG("changing stitcher to " << blender);
    // TODO: change panorama options.

    PanoramaOptions opt = pano.getOptions();
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
            new PT::SetPanoOptionsCmd( pano, opt )
            );
}

// Stitcher options
void PanoPanel::InterpolatorChanged(wxCommandEvent & e)
{
    // TODO: check panotools interpolators if PTmender is
    // is used
    PanoramaOptions opt = pano.getOptions();
    //Interpolator from PanoramaMemento.h
    int lt = m_InterpolatorChoice->GetSelection();

    opt.interpolator = (vigra_ext::Interpolator) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Interpolator changed to: " << lt )
}

void PanoPanel::OnRemapperCropped(wxCommandEvent & e)
{
    PanoramaOptions opt = pano.getOptions();
    //Interpolator from PanoramaMemento.h
    opt.tiff_saveROI = e.IsChecked();
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
}



void PanoPanel::DoCalcFOV(wxCommandEvent & e)
{
    DEBUG_TRACE("");

    double hfov, height;
    pano.fitPano(hfov, height);
    PanoramaOptions opt = pano.getOptions();
    opt.setHFOV(hfov);
    opt.setHeight(roundi(height));

    DEBUG_INFO ( "hfov: " << opt.getHFOV() << "  w: " << opt.getWidth() << " h: " << opt.getHeight() << "  => vfov: " << opt.getVFOV()  << "  before update");

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    PanoramaOptions opt2 = pano.getOptions();
    DEBUG_INFO ( "hfov: " << opt2.getHFOV() << "  w: " << opt2.getWidth() << " h: " << opt2.getHeight() << "  => vfov: " << opt2.getVFOV()  << "  after update");

}

void PanoPanel::DoCalcOptimalWidth(wxCommandEvent & e)
{
    PanoramaOptions opt = pano.getOptions();
    unsigned width = pano.calcOptimalWidth();
    if (width > 0) {
        opt.setWidth( width );
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
    }
    DEBUG_INFO ( "new optimal width: " << opt.getWidth() );
}


void PanoPanel::DoStitch()
{
    if (pano.getNrOfImages() == 0) {
        return;
    }

    wxCommandEvent dummy;
    MainFrame::Get()->OnSaveProject(dummy);

    // TODO: save project to temporary file and stitch from there.
#ifdef __WXGTK__
    wxString terminal = wxConfigBase::Get()->Read(wxT("terminalEmulator"), wxT(HUGIN_STITCHER_TERMINAL));
    wxString filename = MainFrame::Get()->getProjectName();
    // cd to project directory
    wxString oldCWD = wxFileName::GetCwd();
    wxFileName::SetCwd(wxFileName(filename).GetPath());
    wxString command = terminal + wxString(wxT("\"make -f ")) + wxQuoteString(filename + wxT(".mk")) + wxString(wxT(" all clean || read dummy\""));
    // execute commands..
    cout << "Executing stitching command: " << command.mb_str() << endl;
    wxExecute(command);
    wxFileName::SetCwd(oldCWD);
#elif defined __WXMAC__
    wxString filename = MainFrame::Get()->getProjectName();
    // cd to project directory
    wxString oldCWD = wxFileName::GetCwd();
    wxFileName::SetCwd(wxFileName(filename).GetPath());
    wxString args = wxString(wxT("-f ")) + wxQuoteString(filename + wxT(".mk")) + wxString(wxT(" all clean || read dummy"));
    MyExecuteCommandOnDialog(wxT("make"), args, 0);
    wxFileName::SetCwd(oldCWD);
#else
    wxMessageBox(_("Makefile execution not yet implemented"));
#endif

#if 0
    PanoramaOptions opt = pano.getOptions();
    // select output file
    // FIXME put in right output extension for selected
    // file format
    wxString ext(opt.getOutputExtension().c_str(), *wxConvCurrent);
    // create filename
    wxString filename =  getDefaultProjectName(pano) + wxT(".") + ext;
    wxString wildcard = wxT("*.") + ext;
    wildcard = wildcard + wxT("|") + wildcard;

    wxFileDialog dlg(this,_("Create panorama image"),
                     wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")),
                     filename, wildcard,
                     wxSAVE, wxDefaultPosition);
    dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
    if (dlg.ShowModal() == wxID_OK) {
        // print as optimizer script..
        wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
        opt.outfile = dlg.GetPath().mb_str();
        std::string outfile = stripExtension(opt.outfile) + std::string(".") + opt.getOutputExtension();
        wxString wxfn(outfile.c_str(), *wxConvCurrent);
        if (! (opt.outputFormat == PanoramaOptions::TIFF_m 
               || opt.outputFormat == PanoramaOptions::TIFF_mask) )
        {
            if (wxFile::Exists(wxfn)) {
                int answer = wxMessageBox(wxString::Format(_("File %s already exists\n\nOverwrite?"), wxfn.c_str() ),
                                        _("Overwrite file?"),
                                        (wxYES_NO | wxICON_EXCLAMATION),
                                        this);
                if (answer != wxYES) {
                    return;
                }
            }
        } else {
            // TODO: add check for tiff_m output images
        }

        // TODO: save project to temp folder and run makefile (in the background)
        wxMessageBox(_("TODO: save and execute makefile"));
        // save project and create panorama
        wxCommandEvent dummy;
        MainFrame::Get()->SaveProjectAs(dummy);
        
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
                    quoted = utils::wxQuoteFilename(wxString(pano.getImage(0).getFilename().c_str(), *wxConvCurrent));
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
#endif
}

void PanoPanel::OnDoStitch ( wxCommandEvent & e )
{
    DoStitch();
}

void PanoPanel::OnOutputFilesChanged(wxCommandEvent & e)
{
    int id = e.GetId();
    PanoramaOptions opts = pano.getOptions();

    if (id == XRCID("pano_cb_ldr_output_blended") ) {
        opts.outputLDRBlended = e.IsChecked();
    } else if (id == XRCID("pano_cb_ldr_output_layers") ) {
        opts.outputLDRLayers = e.IsChecked();
    } else if (id == XRCID("pano_cb_ldr_output_exposure_layers") ) {
        opts.outputLDRExposureLayers = e.IsChecked();
    } else if (id == XRCID("pano_cb_hdr_output_blended") ) {
        opts.outputHDRBlended = e.IsChecked();
    } else if (id == XRCID("pano_cb_hdr_output_stacks") ) {
        opts.outputHDRStacks = e.IsChecked();
    } else if (id == XRCID("pano_cb_hdr_output_layers") ) {
        opts.outputHDRLayers = e.IsChecked();
    }

    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opts )
        );
}

// We need to override the default handling of size events because the
// sizers set the virtual size but not the actual size. We reverse
// the standard handling and fit the child to the parent rather than
// fitting the parent around the child

void PanoPanel::OnSize( wxSizeEvent & e )
{
    DEBUG_TRACE("");
    wxSize new_size = e.GetSize();
    XRCCTRL(*this, "panorama_panel", wxPanel)->SetSize ( new_size );
    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
	e.Skip();
}
