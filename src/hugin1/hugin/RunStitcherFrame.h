// -*- c-basic-offset: 4 -*-
/** @file RunStitcherFrame.h
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

#ifndef _RUNSTITCHERFRAME_H
#define _RUNSTITCHERFRAME_H

namespace PT {
    class Panorama;
}

class wxProcessEvent;
class wxProcess;
class wxButton;
class wxGauge;
class wxTextInputStream;

// ----------------------------------------------------------------------------
// RunStitcherFrame: allows the user to communicate with the child process
// ----------------------------------------------------------------------------

class RunStitcherFrame : public wxFrame
{
public:
    RunStitcherFrame(wxWindow *parent,
                     const PT::Panorama * pano,
                     const PT::PanoramaOptions & options,
                     const PT::UIntSet & imgs,
                     bool editScript=false
                     );

    ~RunStitcherFrame();

    void OnProcessTerm(wxProcessEvent& event);

protected:
    void OnAbort(wxCommandEvent& event);
    void OnClose(wxCloseEvent & event);
    void OnTimer(wxTimerEvent& event);

private:
    wxProcess *m_process;
    long m_pid;

    wxTextInputStream *m_in;

    wxButton * m_abort;
    wxButton * m_cancel;
    wxGauge * m_stitcherProgress;
    wxStaticText * m_stitcherStatus;

    const PT::Panorama * m_pano;
    wxTimer m_timer;

    // the current parser state.
    wxString m_description;
    long m_percent;

    DECLARE_EVENT_TABLE()
};


#endif // _RUNSTITCHERFRAME_H
