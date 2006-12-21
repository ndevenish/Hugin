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

#include "PT/Stitcher.h"

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

#define WX_BROKEN_SIZER_UNKNOWN

using namespace PT;
using namespace std;
using namespace utils;

BEGIN_EVENT_TABLE(PanoPanel, wxWindow)
    EVT_SIZE   ( PanoPanel::OnSize )
    EVT_CHOICE ( XRCID("stitch_quick_mode"),PanoPanel::QuickModeChanged )
    EVT_CHOICE ( XRCID("pano_choice_pano_type"),PanoPanel::ProjectionChanged )
    EVT_TEXT_ENTER( XRCID("pano_text_hfov"),PanoPanel::HFOVChanged )
    EVT_TEXT_ENTER( XRCID("pano_text_vfov"),PanoPanel::VFOVChanged )
    EVT_BUTTON ( XRCID("pano_button_calc_fov"), PanoPanel::DoCalcFOV)
    EVT_TEXT_ENTER ( XRCID("pano_val_width"),PanoPanel::WidthChanged )
    EVT_TEXT_ENTER ( XRCID("pano_val_height"),PanoPanel::HeightChanged )
    EVT_BUTTON ( XRCID("pano_button_opt_width"), PanoPanel::DoCalcOptimalWidth)
    EVT_BUTTON ( XRCID("pano_button_stitch"),PanoPanel::OnDoStitch )
    EVT_CHOICE ( XRCID("pano_choice_stitcher"),PanoPanel::StitcherChanged )
END_EVENT_TABLE()


// Define a constructor for the Pano Panel
PanoPanel::PanoPanel(wxWindow *parent, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(*pano),
      updatesDisabled(false), m_Stitcher(0)
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

    m_StitcherChoice = XRCCTRL(*this, "pano_choice_stitcher", wxChoice);
    DEBUG_ASSERT(m_StitcherChoice);
    m_QuickChoice = XRCCTRL(*this, "stitch_quick_mode", wxChoice);
    DEBUG_ASSERT(m_QuickChoice);
    m_StitchButton = XRCCTRL(*this, "pano_button_stitch", wxButton);
    DEBUG_ASSERT(m_StitchButton);

#ifdef USE_WX253
    m_pano_ctrls = XRCCTRL(*this, "pano_controls_panel", wxScrolledWindow);
    DEBUG_ASSERT(m_pano_ctrls);
    m_pano_ctrls->SetSizeHints(20, 20);
    m_pano_ctrls->FitInside();
    m_pano_ctrls->SetScrollRate(10, 10);
    m_pano_ctrls_fixed = XRCCTRL(*this, "pano_ctrl_fixed", wxPanel);
    DEBUG_ASSERT(m_pano_ctrls_fixed);
#endif

    // observe the panorama
    pano->addObserver (this);

    // setup the stitcher
    int t = wxConfigBase::Get()->Read(wxT("Stitcher/DefaultStitcher"),1l);
#if (!defined NO_PTSTITCHER) && (defined __WXMAC__)
    wxString currentPTStitcherExe
        = wxConfigBase::Get()->Read(wxT("/Panotools/PTStitcherExe"),wxT(HUGIN_PT_STITCHER_EXE));
    // unless custom PTStitcher specified, disable PTStitcher
    if (currentPTStitcherExe == wxT(HUGIN_PT_STITCHER_EXE))
    {   //TODO: for now, default path triggers non-custom path but to be fixed
#endif
#if (defined NO_PTSTITCHER) || (defined __WXMAC__)
        // disable stitcher choice and select nona,
        // since PTStitcher is not available on OSX
        if(t == 0) t = 1;
        m_StitcherChoice->Disable();
#endif
#if (!defined NO_PTSTITCHER) && (defined __WXMAC__)
    }
#endif
    m_StitcherChoice->SetSelection(t);

    // trigger creation of apropriate stitcher control, if
    // not already happend.
    if (! m_Stitcher) {
        wxCommandEvent dummy;
        StitcherChanged(dummy);
    }

    DEBUG_TRACE("")
}


PanoPanel::~PanoPanel(void)
{
    DEBUG_TRACE("dtor");
    wxConfigBase::Get()->Write(wxT("Stitcher/DefaultStitcher"),m_StitcherChoice->GetSelection());

    m_HFOVText->PopEventHandler(true);
    m_VFOVText->PopEventHandler(true);
    m_WidthTxt->PopEventHandler(true);
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
        m_QuickChoice->Disable();
        m_StitchButton->Disable();
    } else {
        //m_ProjectionChoice->Enable();
        //m_HFOVSpin->Enable();
        m_CalcHFOVButton->Enable();
        //m_VFOVSpin->Enable();
        //m_WidthTxt->Enable();
        m_CalcOptWidthButton->Enable();
        //m_HeightStaticText->Enable();
#if (!defined NO_PTSTITCHER) && (defined __WXMAC__)
        wxString currentPTStitcherExe
            = wxConfigBase::Get()->Read(wxT("/Panotools/PTStitcherExe"),wxT(HUGIN_PT_STITCHER_EXE));
        // only if custom PTStitcher specified, enable PTStitcher
        if (currentPTStitcherExe != wxT(HUGIN_PT_STITCHER_EXE))
        {   //TODO: for now, default path triggers non-custom path but to be fixed
            m_StitcherChoice->Enable();
        }
#endif
        m_QuickChoice->Enable();
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
}

void PanoPanel::ProjectionChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    PanoramaOptions::ProjectionFormat oldP = opt.getProjection();

    PanoramaOptions::ProjectionFormat newP = (PanoramaOptions::ProjectionFormat) m_ProjectionChoice->GetSelection();
    int w = opt.getWidth();
    int h = opt.getHeight();
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


void PanoPanel::EnableControls(bool enable)
{
//    m_HFOVSpin->Enable(enable);
//    m_VFOVSpin->Enable(enable);
    m_WidthTxt->Enable(enable);
#ifndef NO_PTSTITCHER
    m_StitcherChoice->Enable(enable);
#ifdef __WXMAC__
    wxString currentPTStitcherExe
        = wxConfigBase::Get()->Read(wxT("/Panotools/PTStitcherExe"),wxT(HUGIN_PT_STITCHER_EXE));
    // unless custom PTStitcher specified, disable choice
    if (currentPTStitcherExe == wxT(HUGIN_PT_STITCHER_EXE))
    {   //TODO: for now, default path triggers non-custom path but to be fixed
        m_StitcherChoice->Enable(false);
    }
#endif
#endif
    m_Stitcher->Enable(enable);
//    m_CalcHFOVButton->Enable(enable);
    m_CalcOptWidthButton->Enable(enable);
}

void PanoPanel::ApplyQuickMode(int preset)
{
    PanoramaOptions opts = pano.getOptions();

    // resize image for all but manual settings
    if (preset != 0) {

		// do not play with the panorama fov
		//        FDiff2D fov = pano.calcFOV();
		//        opts.HFOV = fov.x;
		//        opts.VFOV = fov.y;

        // resize.
        if (preset == 3) {
            opts.setWidth(1024);
        } else {
            opts.setWidth(pano.calcOptimalWidth());
        }

		switch (preset) {
		case 1:
			// high quality tiff file
			// nona + enblend
			opts.outputFormat = PanoramaOptions::TIFF;
			opts.interpolator = vigra_ext::INTERP_CUBIC;
			opts.colorCorrection = PanoramaOptions::NONE;
			opts.gamma = 1.0;
			opts.featherWidth = 10;
			opts.remapAcceleration = PanoramaOptions::MAX_SPEEDUP;
			opts.blendMode = PanoramaOptions::ENBLEND_BLEND;
			m_StitcherChoice->SetSelection(1);
			break;
		case 2:
			// high quality jpeg file
			// nona + jpg output + cubic interpolator
			// fixme: this should be an enblended pano...
			opts.outputFormat = PanoramaOptions::JPEG;
			opts.interpolator = vigra_ext::INTERP_CUBIC;
			opts.colorCorrection = PanoramaOptions::NONE;
			opts.gamma = 1.0;
			opts.featherWidth = 10;
			opts.remapAcceleration = PanoramaOptions::MAX_SPEEDUP;
            opts.blendMode = PanoramaOptions::NO_BLEND;
            m_StitcherChoice->SetSelection(1);
			break;
		case 3:
			// draft quality jpeg file
			// nona + jpg output
			opts.outputFormat = PanoramaOptions::JPEG;
			opts.interpolator = vigra_ext::INTERP_CUBIC;
			opts.colorCorrection = PanoramaOptions::NONE;
			opts.gamma = 1.0;
			opts.featherWidth = 10;
			opts.remapAcceleration = PanoramaOptions::MAX_SPEEDUP;
			m_StitcherChoice->SetSelection(1);
			break;
		case 4:
			// multilayer TIFF file
			opts.outputFormat = PanoramaOptions::TIFF_multilayer;
			opts.interpolator = vigra_ext::INTERP_CUBIC;
			opts.colorCorrection = PanoramaOptions::NONE;
			opts.gamma = 1.0;
			opts.featherWidth = 10;
			opts.remapAcceleration = PanoramaOptions::MAX_SPEEDUP;
			m_StitcherChoice->SetSelection(1);
			break;
		case 5:
			// multilayer PSD file
			opts.outputFormat = PanoramaOptions::PSD_mask;
			opts.interpolator = vigra_ext::INTERP_CUBIC;
			opts.colorCorrection = PanoramaOptions::NONE;
			opts.gamma = 1.0;
			opts.featherWidth = 10;
			opts.remapAcceleration = PanoramaOptions::MAX_SPEEDUP;
			m_StitcherChoice->SetSelection(0);
            break;
		default:
		    DEBUG_ERROR("unknown stitcher preset selected");
		    break;
		}

        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opts )
            );
        wxCommandEvent dummy;
        StitcherChanged(dummy);
    }
}

void PanoPanel::QuickModeChanged(wxCommandEvent & e)
{
    int preset = m_QuickChoice->GetSelection();
    DEBUG_DEBUG("changing quick stitch preset to " << preset);
    
#if (!defined NO_PTSTITCHER) && (defined __WXMAC__)
    wxString currentPTStitcherExe
        = wxConfigBase::Get()->Read(wxT("/Panotools/PTStitcherExe"),wxT(HUGIN_PT_STITCHER_EXE));
    // unless custom PTStitcher specified, disable PTStitcher
    if (currentPTStitcherExe == wxT(HUGIN_PT_STITCHER_EXE))
    {   //TODO: for now, default path triggers non-custom path but to be fixed
#endif
#if (defined NO_PTSTITCHER) || (defined __WXMAC__)
        if(preset == 5) // photoshop output uses PTStitcher
        {
            wxMessageBox(wxT("This option is not available without PTStitcher program."));
            preset = 0;
            m_QuickChoice->SetSelection(preset);
        }
#endif
#if (!defined NO_PTSTITCHER) && (defined __WXMAC__)
    }
#endif
    
    if(preset != 0) ApplyQuickMode(preset);
    
    switch (preset) {
        case 0:
            // custom
            EnableControls(true);
            break;
        default:
            EnableControls(false);
    }
}

void PanoPanel::StitcherChanged(wxCommandEvent & e)
{
    int stitcher = m_StitcherChoice->GetSelection();
    DEBUG_DEBUG("changing stitcher to " << stitcher);

    // PTStitcher
    if (m_Stitcher) {
        m_Stitcher->Destroy();
    }
    switch (stitcher) {
    case 0:
#ifndef NO_PTSTITCHER
        m_Stitcher = new PTStitcherPanel(this, pano);
        break;
#else
        m_StitcherChoice->SetSelection(1);
#endif
    case 1:
        m_Stitcher = new NonaStitcherPanel(this, pano);
        break;
    default:
        DEBUG_FATAL("Unknown Stitcher selected, XRC file might be invalid");
        return;
    }
    // show the new stitcher
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("pano_stitcher_unknown"),
               m_Stitcher );
    // redo layout.
//    Layout();
#ifdef USE_WX253
// the sizer system doesn't seem to work after AttachUnknownControl...
// the attached control is not included in the size calculations.
    m_Stitcher->FitInside();
    m_pano_ctrls->FitInside();
#ifdef WX_BROKEN_SIZER_UNKNOWN
    int w,h,w2,h2;
    m_pano_ctrls_fixed->GetVirtualSize(&w, &h);
    m_Stitcher->GetVirtualSize(&w2, &h2);
    h+=h2;
    w = std::max(w,w2);
    m_pano_ctrls->SetVirtualSize(w,h);
    m_pano_ctrls->SetVirtualSizeHints(w,h,-1,-1);
#endif
#endif
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
    PanoramaOptions opt = pano.getOptions();
    // select output file
    // FIXME put in right output extension for selected
    // file format
    wxString ext(opt.getOutputExtension().c_str(), *wxConvCurrent);
    // create filename
    wxString filename =  getDefaultProjectName(pano) + wxT(".") + ext;
    wxString wildcard = wxT("*.") + ext;

    wxFileDialog dlg(this,_("Create panorama image"),
                     wxConfigBase::Get()->Read(wxT("actualPath"),wxT("")),
                     filename, ext,
                     wxSAVE, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        // print as optimizer script..
        wxConfig::Get()->Write(wxT("actualPath"), dlg.GetDirectory());  // remember for later
        opt.outfile = dlg.GetPath().mb_str();
        m_Stitcher->Stitch(pano, opt);
    }
    // TODO: show image after it has been created
}

void PanoPanel::OnDoStitch ( wxCommandEvent & e )
{
    int preset = m_QuickChoice->GetSelection();
    // apply preset mode. (recalculates width etc)
    ApplyQuickMode(preset);
    DoStitch();
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
