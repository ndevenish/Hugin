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

#include <string>
#include <iostream>
#include <fstream>

#include "PT/PanoCommand.h"
#include "hugin/CommandHistory.h"
#include "hugin/RunOptimizerFrame.h"
#include "hugin/OptimizeFrame.h"

using namespace std;
using namespace PT;



//============================================================================
//============================================================================
//============================================================================

BEGIN_EVENT_TABLE(OptimizeFrame, wxFrame)
    EVT_BUTTON(XRCID("optimize_frame_optimize"), OptimizeFrame::OnOptimizeButton)
    EVT_BUTTON(XRCID("opt_yaw_select"), OptimizeFrame::OnSelYaw)
    EVT_BUTTON(XRCID("opt_yaw_clear"), OptimizeFrame::OnDelYaw)
    EVT_BUTTON(XRCID("opt_pitch_select"), OptimizeFrame::OnSelPitch)
    EVT_BUTTON(XRCID("opt_pitch_clear"), OptimizeFrame::OnDelPitch)
    EVT_BUTTON(XRCID("opt_roll_select"), OptimizeFrame::OnSelRoll)
    EVT_BUTTON(XRCID("opt_roll_clear"), OptimizeFrame::OnDelRoll)
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

    // observe the panorama
    m_pano->addObserver(this);
}

OptimizeFrame::~OptimizeFrame()
{
    DEBUG_TRACE("");
    m_pano->removeObserver(this);
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

    // "global" flags
    OptimizerSettings s;
    s.HFOV = m_lens_list->IsChecked(0);
    s.a = m_lens_list->IsChecked(1);
    s.b = m_lens_list->IsChecked(2);
    s.c = m_lens_list->IsChecked(3);
    s.d = m_lens_list->IsChecked(4);
    s.e = m_lens_list->IsChecked(5);
    for (unsigned int i=0; i < nImages; i++) {
        s.roll = m_roll_list->IsChecked(i);
        s.pitch = m_pitch_list->IsChecked(i);
        s.yaw = m_yaw_list->IsChecked(i);
        optvars.push_back(s);
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
        m_yaw_list->SetString(*it, wxString::Format("%d (%5f)",
                                *it, m_pano->getVariable(*it).yaw.getValue()));
        m_pitch_list->SetString(*it, wxString::Format("%d (%5f)",
                              *it, m_pano->getVariable(*it).pitch.getValue()));
        m_roll_list->SetString(*it, wxString::Format("%d (%5f)",
                               *it, m_pano->getVariable(*it).roll.getValue()));
    }
}

/*
bool OptimizeFrame::runStitcher(const Panorama & pano,
                           const OptimizeVector & optvars
                           const PanoramaOptions & target)
{
    std::ofstream script(PTScriptFile.c_str());
    pano.printStitcherScript(script, target);
    script.close();

    string cmd = stitcherExe + string(" -o \"") + target.outfile + "\" " + PTScriptFile;

    wxProcess *process = wxProcess::Open(cmd.c_str());

    if ( !process )
    {
        wxLogError(_T("Failed to launch the command."));
        return;
    }

    wxOutputStream *out = process->GetOutputStream();
    if ( !out )
    {
        wxLogError(_T("Failed to connect to child stdin"));
        return;
    }

#ifdef unix
    // open window that shows a status dialog
    new MyPipeFrame(this, cmd, process);
#endif
}

*/
void OptimizeFrame::runOptimizer(const OptimizeVector & optvars, const PanoramaOptions & options)
{
    DEBUG_TRACE("");
    // open window that shows a status dialog, and allows to
    // apply the results
    new RunOptimizerFrame(this, m_pano, options, optvars);
}


/*
        {
            // create a new process.

            Process process(false);
            pano.runOptimizer(process,optvars,outputOpts);
            process.wait();

            VariablesVector vars = pano.getVariables();
            CPVector cps = pano.getCtrlPoints();

            pano.readOptimizerOutput(vars, cps);
            pano.updateVariables(vars);
            pano.updateCtrlPointErrors(cps);
            pano.changeFinished();
        }
*/
