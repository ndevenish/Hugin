// -*- c-basic-offset: 4 -*-
/** @file RunOptimizerFrame.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _RUNOPTIMIZERFRAME_H
#define _RUNOPTIMIZERFRAME_H

namespace PT {
    class Panorama;
}

// ----------------------------------------------------------------------------
// RunOptimizerFrame: allows the user to communicate with the child process
// ----------------------------------------------------------------------------

class RunOptimizerFrame : public wxFrame
{
public:
    RunOptimizerFrame(wxFrame *parent,
                      PT::Panorama * pano,
                      const PT::PanoramaOptions & options,
                      const PT::OptimizeVector & optvars);
    
    void OnProcessTerm(wxProcessEvent& event);
    
protected:
    void OnCancel(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);

    void OnClose(wxCloseEvent& event);


private:
    wxProcess *m_process;
    long m_pid;

    wxTextInputStream *m_in;

    wxButton * m_apply;
    wxButton * m_cancel;
    wxStaticText * m_optimizer_status;
    wxStaticText * m_optimizer_result_text;

    PT::Panorama * m_pano;
    wxTimer m_timer;

    PT::VariablesVector m_vars;
    PT::CPVector m_cps;
    
    DECLARE_EVENT_TABLE()
};


#endif // _RUNOPTIMIZERFRAME_H
