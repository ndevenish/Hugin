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
#include "hugin/PanoPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/MyProgressDialog.h"
#include "hugin/PTStitcherPanel.h"
#include "hugin/NonaStitcherPanel.h"

using namespace PT;
using namespace std;
using namespace utils;

BEGIN_EVENT_TABLE(PanoPanel, wxWindow)
    EVT_SIZE   ( PanoPanel::FitParent )
    EVT_CHOICE ( XRCID("stitch_quick_mode"),PanoPanel::QuickModeChanged )
    EVT_CHOICE ( XRCID("pano_choice_pano_type"),PanoPanel::ProjectionChanged )
    EVT_SPINCTRL ( XRCID("pano_val_hfov"),PanoPanel::HFOVChangedSpin )
    EVT_TEXT_ENTER( XRCID("pano_val_hfov"),PanoPanel::HFOVChanged )
    EVT_SPINCTRL ( XRCID("pano_val_vfov"),PanoPanel::VFOVChangedSpin )
    EVT_TEXT_ENTER( XRCID("pano_val_vfov"),PanoPanel::VFOVChanged )
    EVT_BUTTON ( XRCID("pano_button_calc_fov"), PanoPanel::DoCalcFOV)
    EVT_TEXT_ENTER ( XRCID("pano_val_width"),PanoPanel::WidthChanged )
    EVT_BUTTON ( XRCID("pano_button_opt_width"), PanoPanel::DoCalcOptimalWidth)
    EVT_BUTTON ( XRCID("pano_button_stitch"),PanoPanel::DoStitch )
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
    m_HFOVSpin = XRCCTRL(*this, "pano_val_hfov" ,wxSpinCtrl);
    DEBUG_ASSERT(m_HFOVSpin);
    m_CalcHFOVButton = XRCCTRL(*this, "pano_button_calc_fov" ,wxButton);
    DEBUG_ASSERT(m_CalcHFOVButton);
    m_HFOVSpin->PushEventHandler(new TextKillFocusHandler(this));
    m_VFOVSpin = XRCCTRL(*this, "pano_val_vfov" ,wxSpinCtrl);
    DEBUG_ASSERT(m_VFOVSpin);
    m_VFOVSpin->PushEventHandler(new TextKillFocusHandler(this));


    m_WidthTxt = XRCCTRL(*this, "pano_val_width", wxTextCtrl);
    DEBUG_ASSERT(m_WidthTxt);
    m_WidthTxt->PushEventHandler(new TextKillFocusHandler(this));
    m_CalcOptWidthButton = XRCCTRL(*this, "pano_button_opt_width" ,wxButton);
    DEBUG_ASSERT(m_CalcOptWidthButton);

    m_HeightStaticText = XRCCTRL(*this, "pano_static_height", wxStaticText);
    DEBUG_ASSERT(m_HeightStaticText);

    m_StitcherChoice = XRCCTRL(*this, "pano_choice_stitcher", wxChoice);
    DEBUG_ASSERT(m_StitcherChoice);
    m_QuickChoice = XRCCTRL(*this, "stitch_quick_mode", wxChoice);
    DEBUG_ASSERT(m_QuickChoice);
    m_StitchButton = XRCCTRL(*this, "pano_button_stitch", wxButton);
    DEBUG_ASSERT(m_StitchButton);

    // observe the panorama
    pano->addObserver (this);

    // setup the stitcher
    int t = wxConfigBase::Get()->Read("Stitcher/DefaultStitcher",0l);
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
    wxConfigBase::Get()->Write("Stitcher/DefaultStitcher",m_StitcherChoice->GetSelection());

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

    m_WidthTxt->SetValue(wxString::Format("%d", opt.width));
    m_HeightStaticText->SetLabel(wxString::Format("%d", opt.getHeight()));
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

void PanoPanel::EnableControls(bool enable)
{
    m_HFOVSpin->Enable(enable);
    m_VFOVSpin->Enable(enable);
    m_WidthTxt->Enable(enable);
    m_StitcherChoice->Enable(enable);
    m_Stitcher->Enable(enable);
    m_CalcHFOVButton->Enable(enable);
    m_CalcOptWidthButton->Enable(enable);
}

void PanoPanel::ApplyQuickMode(int preset)
{
    if (preset == 0)
        return;

    PanoramaOptions opts = pano.getOptions();

    // resize image for all but manual settings
    if (preset != 0) {

        // do not play with the panorama fov
//        FDiff2D fov = pano.calcFOV();
//        opts.HFOV = fov.x;
//        opts.VFOV = fov.y;

        // resize.
        if (preset == 3) {
            opts.width = 1024;
        } else {
            opts.width = CalcOptimalWidth();
        }
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
        opts.blendMode = PanoramaOptions::SPLINE_BLEND;
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
        opts.outputFormat = PanoramaOptions::TIFF_m;
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

void PanoPanel::QuickModeChanged(wxCommandEvent & e)
{
    int preset = m_QuickChoice->GetSelection();
    DEBUG_DEBUG("changing quick stitch preset to " << preset);

    ApplyQuickMode(preset);

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
        m_Stitcher = new PTStitcherPanel(this, pano);
        break;
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
}

void PanoPanel::DoCalcFOV(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    PanoramaOptions opt = pano.getOptions();

    FDiff2D fov = pano.calcFOV();
    opt.HFOV = roundi(fov.x);
    opt.VFOV = roundi(fov.y);

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO ( "new fov: [" << opt.HFOV << " "<< opt.VFOV << "] => height: " << opt.getHeight() );

}


void PanoPanel::DoCalcOptimalWidth(wxCommandEvent & e)
{
    PanoramaOptions opt = pano.getOptions();
    opt.width = CalcOptimalWidth();
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );

    DEBUG_INFO ( "new optimal width: " << opt.width );
}

unsigned int PanoPanel::CalcOptimalWidth()
{
    // calculate average pixel density of each image
    // and use the highest one to calculate the width
    PanoramaOptions opt = pano.getOptions();
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
    return roundi(pixelDensity * opt.HFOV);
}

void PanoPanel::DoStitch ( wxCommandEvent & e )
{
    int preset = m_QuickChoice->GetSelection();
    // apply preset mode. (recalculates width etc)
    ApplyQuickMode(preset);

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
        wxConfig::Get()->Write("actualPath", dlg.GetDirectory());  // remember for later
        opt.outfile = dlg.GetPath().c_str();
        m_Stitcher->Stitch(pano, opt);
    }
}


void PanoPanel::FitParent( wxSizeEvent & e )
{
    DEBUG_TRACE("");
    Layout();
    wxSize new_size = e.GetSize();
//    this->SetSize(new_size);
    XRCCTRL(*this, "panorama_panel", wxPanel)->SetSize ( new_size );
    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}
