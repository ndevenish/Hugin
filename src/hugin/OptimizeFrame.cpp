// -*- c-basic-offset: 4 -*-

/** @file OptimizeFrame.cpp
 *
 *  @brief implementation of OptimizeFrame
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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


//-----------------------------------------------------------------------------
// Standard wxWindows headers
//-----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/xrc/xmlres.h"              // XRC XML resouces
#include "wx/notebook.h"
#include "wx/listctrl.h"
#include "wx/process.h"
#include "wx/txtstrm.h"
#include <wx/config.h>

#include "hugin/OptimizeFrame.h"

#include <string>
#include <iostream>
#include <fstream>

#include "common/stl_utils.h"
#include "PT/PanoCommand.h"
#include "hugin/CommandHistory.h"
#include "hugin/RunOptimizerFrame.h"

using namespace std;
using namespace PT;


//============================================================================
//============================================================================
//============================================================================

BEGIN_EVENT_TABLE(OptimizeFrame, wxFrame)
    EVT_CLOSE(OptimizeFrame::OnClose)
    EVT_BUTTON(XRCID("optimize_frame_optimize"), OptimizeFrame::OnOptimizeButton)
    EVT_BUTTON(XRCID("opt_yaw_select"), OptimizeFrame::OnSelYaw)
    EVT_BUTTON(XRCID("opt_yaw_clear"), OptimizeFrame::OnDelYaw)
    EVT_BUTTON(XRCID("opt_yaw_equalize"), OptimizeFrame::OnEqYaw)
    EVT_BUTTON(XRCID("opt_pitch_select"), OptimizeFrame::OnSelPitch)
    EVT_BUTTON(XRCID("opt_pitch_clear"), OptimizeFrame::OnDelPitch)
//    EVT_BUTTON(XRCID("opt_pitch_equalize"), OptimizeFrame::OnEqPitch)
    EVT_BUTTON(XRCID("opt_roll_select"), OptimizeFrame::OnSelRoll)
    EVT_BUTTON(XRCID("opt_roll_clear"), OptimizeFrame::OnDelRoll)
//    EVT_BUTTON(XRCID("opt_roll_equalize"), OptimizeFrame::OnEqRoll)
END_EVENT_TABLE()

OptimizeFrame::OptimizeFrame(wxWindow * parent, PT::Panorama * pano)
    : m_pano(pano)
{
    DEBUG_TRACE("");
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("optimize_frame"));

    m_yaw_list = XRCCTRL(*this, "optimizer_yaw_list", wxCheckListBox);
    m_pitch_list = XRCCTRL(*this, "optimizer_pitch_list", wxCheckListBox);
    m_roll_list = XRCCTRL(*this, "optimizer_roll_list", wxCheckListBox);
    m_lens_list = XRCCTRL(*this, "optimizer_lens_list", wxCheckListBox);

    wxConfigBase * config = wxConfigBase::Get();
    long w = config->Read("/OptimizerFrame/width",-1);
    long h = config->Read("/OptimizerFrame/height",-1);
    if (w != -1) {
        SetClientSize(w,h);
    }

    // observe the panorama
    m_pano->addObserver(this);
}

OptimizeFrame::~OptimizeFrame()
{
    DEBUG_TRACE("dtor, writing config");
    wxSize sz = GetClientSize();
    wxConfigBase * config = wxConfigBase::Get();
    config->Write("/OptimizerFrame/width",sz.GetWidth());
    config->Write("/OptimizerFrame/height",sz.GetWidth());
    m_pano->removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void OptimizeFrame::OnOptimizeButton(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // run optimizer
    // take the OptimizeVector from somewhere...

    OptimizeVector optvars = getOptimizeSettings();
    PanoramaOptions opts;
    runOptimizer(optvars, opts);
}

// FIXME this should be on the preview panel, once we have one :)
void OptimizeFrame::OnEqYaw(wxCommandEvent & e)
{
    VariableMapVector vars = m_pano->getVariables();
    VariableMapVector::iterator it;
    double min = 1000;
    double max = -1000;
    for(it = vars.begin(); it != vars.end(); it++) {
        double val = map_get(*it,"y").getValue();
        if (val < min) min = val;
        if (val > max) max = val;
    }


    double shift = min + (max-min)/2;
    for(it = vars.begin(); it != vars.end(); it++) {
        map_get(*it, "y").setValue( map_get(*it, "y").getValue() - shift);
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::UpdateVariablesCmd(*m_pano, vars)
        );
}

void OptimizeFrame::SetCheckMark(wxCheckListBox * l, int check)
{
    int n = l->GetCount();
    for (int i=0; i < n; i++) {
        l->Check(i, check);
    }
}


OptimizeVector OptimizeFrame::getOptimizeSettings()
{

    int nrLI = m_yaw_list->GetCount();
    assert(nrLI >=0);
    unsigned int nr = (unsigned int) nrLI;
    unsigned int nImages = m_pano->getNrOfImages();
    assert(nr == nImages);

    OptimizeVector optvars;

    // possibly linked parameters.
    set<string> linked;
    if (m_lens_list->IsChecked(0)) {
        linked.insert("v");
    }
    if (m_lens_list->IsChecked(1)) {
        linked.insert("a");
    }
    if (m_lens_list->IsChecked(2)) {
        linked.insert("b");
    }
    if (m_lens_list->IsChecked(3)) {
        linked.insert("c");
    }
    if (m_lens_list->IsChecked(4)) {
        linked.insert("d");
    }
    if (m_lens_list->IsChecked(5)) {
        linked.insert("e");
    }
    for (unsigned int i=0; i < nImages; i++) {
        set<string> imgopt = linked;
        if (m_roll_list->IsChecked(i)) {
            imgopt.insert("r");
        }
        if (m_pitch_list->IsChecked(i)) {
            imgopt.insert("p");
        }
        if (m_yaw_list->IsChecked(i)) {
            imgopt.insert("y");
        }
        optvars.push_back(imgopt);
    }

    return optvars;
}

void OptimizeFrame::panoramaImagesChanged(PT::Panorama &pano,
                                          const PT::UIntSet & imgNr)
{
    DEBUG_TRACE("");
    // add/remove items
    int nrLI = m_yaw_list->GetCount();
    assert(nrLI >=0);
    unsigned int nr = (unsigned int) nrLI;
    unsigned int nImages = m_pano->getNrOfImages();
    while (nr < nImages) {
        // add checkboxes.
        m_yaw_list->Append(wxString::Format("%d",nr));
        m_pitch_list->Append(wxString::Format("%d",nr));
        m_roll_list->Append(wxString::Format("%d",nr));
        nr++;
    }

    while (nr > nImages) {
        if (nr == 0)
            break;
        m_yaw_list->Delete(nr-1);
        m_pitch_list->Delete(nr-1);
        m_roll_list->Delete(nr-1);
        nr--;
    }

    // display values of the variables.
    UIntSet::iterator it;
    for (it = imgNr.begin(); it != imgNr.end(); it++) {
        const VariableMap & vars = pano.getImageVariables(*it);
        // keep selections
        bool sel = m_yaw_list->IsChecked(*it);
        m_yaw_list->SetString(*it, wxString::Format("%d (%5f)",
                                *it, const_map_get(vars,"y").getValue()));
        m_yaw_list->Check(*it,sel);

        sel = m_pitch_list->IsChecked(*it);
        m_pitch_list->SetString(*it, wxString::Format("%d (%5f)",
                                *it, const_map_get(vars,"p").getValue()));
        m_pitch_list->Check(*it,sel);

        sel = m_roll_list->IsChecked(*it);
        m_roll_list->SetString(*it, wxString::Format("%d (%5f)",
                                *it, const_map_get(vars,"r").getValue()));
        m_roll_list->Check(*it,sel);
    }
}


void OptimizeFrame::runOptimizer(const OptimizeVector & optvars, const PanoramaOptions & options)
{
    DEBUG_TRACE("");
    // open window that shows a status dialog, and allows to
    // apply the results
    new RunOptimizerFrame(this, m_pano, options, optvars);
}


void OptimizeFrame::OnClose(wxCloseEvent& event)
{
    DEBUG_TRACE("OnClose");
    // do not close, just hide if we're not forced
    if (event.CanVeto()) {
        event.Veto();
        Hide();
        DEBUG_DEBUG("Hiding");
    } else {
        DEBUG_DEBUG("Closing");
    }
}
