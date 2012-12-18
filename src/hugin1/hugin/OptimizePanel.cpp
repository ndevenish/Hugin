// -*- c-basic-offset: 4 -*-

/** @file OptimizePanel.cpp
 *
 *  @brief implementation of OptimizePanel
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 */

/*  This program is free software; you can redistribute it and/or
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

#include "PT/PTOptimise.h"

#include "hugin/OptimizePanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/MainFrame.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/PTWXDlg.h"
#include "hugin/config_defaults.h"
#include "hugin/ImagesTree.h"
#include "panodata/OptimizerSwitches.h"
#include "hugin/PanoOperation.h"

using namespace std;
using namespace PT;
using namespace hugin_utils;

//============================================================================
//============================================================================
//============================================================================

BEGIN_EVENT_TABLE(OptimizePanel, wxPanel)
    EVT_CLOSE(OptimizePanel::OnClose)
    EVT_BUTTON(XRCID("optimize_panel_optimize"), OptimizePanel::OnOptimizeButton)
    EVT_BUTTON(XRCID("optimize_panel_reset"), OptimizePanel::OnReset)
END_EVENT_TABLE()


OptimizePanel::OptimizePanel()
{
    DEBUG_TRACE("");
}

bool OptimizePanel::Create(wxWindow* parent, wxWindowID id , const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
    DEBUG_TRACE("");
    // Not needed here, wxPanel::Create is called by LoadPanel below
    if (! wxPanel::Create(parent, id, pos, size, style, name) ) {
        return false;
    }

    // create a sub-panel and load class into it!

    // wxPanel::Create is called in here!
    wxXmlResource::Get()->LoadPanel(this, wxT("optimize_panel"));
    wxPanel * panel = XRCCTRL(*this, "optimize_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer( topsizer );

    m_only_active_images_cb = XRCCTRL(*this, "optimizer_panel_only_active_images", wxCheckBox);
    DEBUG_ASSERT(m_only_active_images_cb);
    m_only_active_images_cb->SetValue(wxConfigBase::Get()->Read(wxT("/OptimizePanel/OnlyActiveImages"),1l) != 0);

    m_images_tree_list = XRCCTRL(*this, "optimize_panel_images", ImagesTreeCtrl);
    DEBUG_ASSERT(m_images_tree_list);
    m_lens_tree_list = XRCCTRL(*this, "optimize_panel_lenses", ImagesTreeCtrl);
    DEBUG_ASSERT(m_lens_tree_list);

    m_edit_cb = XRCCTRL(*this, "optimizer_panel_edit_script", wxCheckBox);
    DEBUG_ASSERT(m_edit_cb);

    XRCCTRL(*this, "optimizer_panel_splitter", wxSplitterWindow)->SetSashGravity(0.66);

    wxString text(_("Any variables below which are bold and underlined will be optimized."));
    text.Append(wxT(" "));
#if defined __WXMAC__ || defined __WXOSX_COCOA__
    text.Append(_("Use command + left mouse click to toggle state of variables."));
#else
    text.Append(_("Use control + left mouse click to toggle state of variables."));
#endif
    text.Append(wxT(" "));
    text.Append(_("Variables which shown in normal font will act as references or anchors."));
    XRCCTRL(*this, "optimizer_panel_information_text", wxStaticText)->SetLabel(text);

    return true;
}

void OptimizePanel::Init(PT::Panorama * pano)
{
    DEBUG_TRACE("");
    m_pano = pano;

    // observe the panorama
    m_pano->addObserver(this);
    m_images_tree_list->Init(m_pano);
    m_images_tree_list->SetOptimizerMode();
    m_images_tree_list->SetDisplayMode(ImagesTreeCtrl::DISPLAY_POSITION);

    m_lens_tree_list->Init(m_pano);
    m_lens_tree_list->SetOptimizerMode();
    m_lens_tree_list->SetGroupMode(ImagesTreeCtrl::GROUP_LENS);
    m_lens_tree_list->SetDisplayMode(ImagesTreeCtrl::DISPLAY_LENS);
}

void OptimizePanel::SetGuiLevel(GuiLevel newGuiLevel)
{
    m_images_tree_list->SetGuiLevel(newGuiLevel);
    m_lens_tree_list->SetGuiLevel(newGuiLevel);
};

OptimizePanel::~OptimizePanel()
{
    DEBUG_TRACE("dtor, writing config");
    wxConfigBase::Get()->Write(wxT("/OptimizePanel/OnlyActiveImages"),m_only_active_images_cb->IsChecked() ? 1l : 0l);

    m_pano->removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void OptimizePanel::panoramaChanged(PT::Panorama & pano)
{
    //Show(m_pano->getOptimizerSwitch()==0);
    m_images_tree_list->Enable(m_pano->getOptimizerSwitch()==0);
    m_lens_tree_list->Enable(m_pano->getOptimizerSwitch()==0);
    m_edit_cb->Enable(m_pano->getOptimizerSwitch()==0);
}

void OptimizePanel::panoramaImagesChanged(PT::Panorama &pano,
                                          const PT::UIntSet & imgNr)
{
    XRCCTRL(*this, "optimize_panel_optimize", wxButton)->Enable(pano.getNrOfImages()>0);
    XRCCTRL(*this, "optimize_panel_reset", wxButton)->Enable(pano.getNrOfImages()>0);    
};

void OptimizePanel::OnOptimizeButton(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // run optimizer

    UIntSet imgs;
    if (m_only_active_images_cb->IsChecked() || m_pano->getOptimizerSwitch()!=0)
    {
        // use only selected images.
        imgs = m_pano->getActiveImages();
        if (imgs.size() == 0)
        {
            wxMessageBox(_("The project does not contain any active images.\nPlease activate at least one image in the (fast) preview window.\nOptimization canceled."),
#ifdef _WINDOWS
                _("Hugin"),
#else
                wxT(""),
#endif
                wxICON_ERROR | wxOK);
            return;
        }
    }
    else
    {
        fill_set(imgs, 0, m_pano->getNrOfImages()-1);
    }
    runOptimizer(imgs);
}

void OptimizePanel::runOptimizer(const UIntSet & imgs)
{
    DEBUG_TRACE("");
    // open window that shows a status dialog, and allows to
    // apply the results
    int mode=m_pano->getOptimizerSwitch();

    Panorama optPano = m_pano->getSubset(imgs);
    PanoramaOptions opts = optPano.getOptions();
    switch(opts.getProjection())
    {
        case PanoramaOptions::RECTILINEAR:
        case PanoramaOptions::CYLINDRICAL:
        case PanoramaOptions::EQUIRECTANGULAR:
            break;
        default:
            // temporarily change to equirectangular
            opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
            optPano.setOptions(opts);
            break;
    }
    UIntSet allImg;
    fill_set(allImg,0, imgs.size()-1);

    char *p = setlocale(LC_ALL,NULL);
    char *oldlocale = strdup(p);
    setlocale(LC_ALL,"C");

    if (mode & HuginBase::OPT_PAIR )
    {
        // remove vertical and horizontal control points
        CPVector cps = optPano.getCtrlPoints();
        CPVector newCP;
        for (CPVector::const_iterator it = cps.begin(); it != cps.end(); it++)
        {
            if (it->mode == ControlPoint::X_Y)
            {
                newCP.push_back(*it);
            }
        }
        optPano.setCtrlPoints(newCP);

        // temporarily disable PT progress dialog..
        deregisterPTWXDlgFcn();
        {
            wxBusyCursor bc;
            // run pairwise optimizer
            PTools::autoOptimise(optPano);
        }
#ifdef DEBUG
        // print optimized script to cout
        DEBUG_DEBUG("panorama after autoOptimise():");
        optPano.printPanoramaScript(std::cerr, optPano.getOptimizeVector(), optPano.getOptions(), allImg, false);
#endif

        registerPTWXDlgFcn(MainFrame::Get());
        // do global optimisation
        optPano.setCtrlPoints(cps);
        PTools::optimize(optPano);
#ifdef DEBUG
        // print optimized script to cout
        DEBUG_DEBUG("panorama after optimise():");
        optPano.printPanoramaScript(std::cerr, optPano.getOptimizeVector(), optPano.getOptions(), allImg, false);
#endif

    }
    else
    {
        if (m_edit_cb->IsChecked() && mode==0)
        {
            // show and edit script..
            ostringstream scriptbuf;
            optPano.printPanoramaScript(scriptbuf, optPano.getOptimizeVector(), optPano.getOptions(), allImg, true);
            // open a text dialog with an editor inside
            wxDialog * edit_dlg = wxXmlResource::Get()->LoadDialog(this, wxT("edit_script_dialog"));
            wxTextCtrl *txtCtrl=XRCCTRL(*edit_dlg,"script_edit_text",wxTextCtrl);
            txtCtrl->SetValue(wxString(scriptbuf.str().c_str(), *wxConvCurrent));

            char * script = 0;
            if (edit_dlg->ShowModal() == wxID_OK)
            {
                script = strdup(txtCtrl->GetValue().mb_str(*wxConvCurrent));
            }
            else
            {
                return;
            }
            PTools::optimize(optPano, script);
            free(script);
        }
        else
        {
            PTools::optimize(optPano);
        }
#ifdef DEBUG
        // print optimized script to cout
        DEBUG_DEBUG("panorama after optimise():");
        optPano.printPanoramaScript(std::cerr, optPano.getOptimizeVector(), optPano.getOptions(), allImg, false);
#endif
    }

    setlocale(LC_ALL,oldlocale);
    free(oldlocale);
#ifdef __WXMSW__
    // the progress window of the optimizer is marked for deletion
    // but the final deletion happens only in the idle event
    // so we need to process the event to really close the progress window
    wxTheApp->ProcessIdle();
#endif
    // calculate control point errors and display text.
    if (AskApplyResult(optPano))
    {
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateVariablesCPSetCmd(*m_pano, imgs, optPano.getVariables(), optPano.getCtrlPoints())
        );
    }
}

bool OptimizePanel::AskApplyResult(const Panorama & pano)
{
    double min;
    double max;
    double mean;
    double var;
    pano.calcCtrlPntsErrorStats( min, max, mean, var);
    // check for HFOV lines. if smaller than 1 report a warning;
    // also check for high distortion coefficients.
    bool smallHFOV=false;
    bool highDist = false;
    const VariableMapVector & vars = pano.getVariables();
    for (VariableMapVector::const_iterator it = vars.begin() ; it != vars.end(); it++)
    {
        if (const_map_get(*it,"v").getValue() < 1.0) smallHFOV = true;
        if (fabs(const_map_get(*it,"a").getValue()) > 0.8) highDist = true;
        if (fabs(const_map_get(*it,"b").getValue()) > 0.8) highDist = true;
        if (fabs(const_map_get(*it,"c").getValue()) > 0.8) highDist = true;
    }

    wxString msg;
    int style=0;
    if (smallHFOV)
    {
        msg.Printf( _("Optimizer run finished.\nWARNING: a very small Field of View (v) has been estimated\n\nThe results are probably invalid.\n\nOptimization of the Field of View (v) of partial panoramas can lead to bad results.\nTry adding more images and control points.\n\nApply the changes anyway?"));
        style = wxYES_NO;
    }
    else
    {
        if (highDist)
        {
            msg.Printf(_("Optimizer run finished.\nResults:\n average control point distance: %f\n standard deviation: %f\n maximum: %f\n\n*WARNING*: very high distortion coefficients (a,b,c) have been estimated.\nThe results are probably invalid.\nOnly optimize all distortion parameters when many, well spread control points are used.\nPlease reset the a,b and c parameters to zero and add more control points\n\nApply the changes anyway?"),
                   mean, sqrt(var), max);
            style = wxYES_NO | wxICON_EXCLAMATION;
        }
        else
        {
            msg.Printf(_("Optimizer run finished.\nResults:\n average control point distance: %f\n standard deviation: %f\n maximum: %f\n\nApply the changes?"),
                   mean, sqrt(var), max);
            style = wxYES_NO | wxICON_EXCLAMATION;
        }
    };

    int id = wxMessageBox(msg,_("Optimization result"),style,this);

    return id == wxYES;
}

void OptimizePanel::OnClose(wxCloseEvent& event)
{
    DEBUG_TRACE("OnClose");
    // do not close, just hide if we're not forced
    if (event.CanVeto())
    {
        event.Veto();
        Hide();
        DEBUG_DEBUG("Hiding");
    }
    else
    {
        DEBUG_DEBUG("Closing");
        Destroy();
    }
}

void OptimizePanel::OnReset(wxCommandEvent& e)
{
    PanoOperation::ResetOperation op(PanoOperation::ResetOperation::RESET_DIALOG_LENS);
    PT::PanoCommand* cmd=op.GetCommand(this,*m_pano, m_images_tree_list->GetSelectedImages());
    if(cmd!=NULL)
    {
        GlobalCmdHist::getInstance().addCommand(cmd);
    };

};

IMPLEMENT_DYNAMIC_CLASS(OptimizePanel, wxPanel)


OptimizePanelXmlHandler::OptimizePanelXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *OptimizePanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, OptimizePanel)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow( cp);

    return cp;
}

bool OptimizePanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("OptimizePanel"));
}

IMPLEMENT_DYNAMIC_CLASS(OptimizePanelXmlHandler, wxXmlResourceHandler)
