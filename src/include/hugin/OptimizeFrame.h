// -*- c-basic-offset: 4 -*-
/** @file OptimizeFrame.h
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

#ifndef _WXPANOCOMMAND_H
#define _WXPANOCOMMAND_H

#include <PT/Panorama.h>

class wxCheckListBox;

/** run the optimizer. this is tied into the wxWindows hugin
 *
 *  it will emit an event that
 */
class OptimizeFrame : public wxFrame, public PT::PanoramaObserver
{
public:
    OptimizeFrame(wxWindow * parent, PT::Panorama * pano);
    virtual ~OptimizeFrame();

    /** receives notification about panorama changes */
    virtual void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);
protected:
    void OnOptimizeButton(wxCommandEvent & e);

    void OnClose(wxCloseEvent& e);
    
    // shortcuts to check/uncheck
    void OnSelYaw(wxCommandEvent & e)
        { SetCheckMark(m_yaw_list,true); }
    void OnDelYaw(wxCommandEvent & e)
        { SetCheckMark(m_yaw_list,false); }
    void OnSelPitch(wxCommandEvent & e)
        { SetCheckMark(m_pitch_list,true); }
    void OnDelPitch(wxCommandEvent & e)
        { SetCheckMark(m_pitch_list,false); }
    void OnSelRoll(wxCommandEvent & e)
        { SetCheckMark(m_roll_list,true); }
    void OnDelRoll(wxCommandEvent & e)
        { SetCheckMark(m_roll_list,false); }

    void OnEqYaw(wxCommandEvent & e);
//    void OnEqPitch(wxCommandEvent & e);
//    void OnEqRoll(wxCommandEvent & e);

    // helper function for wxCheckListBox
    void SetCheckMark(wxCheckListBox * l, int check);


    /** return currently selected OptimizeVector
     *
     *  @return OptimizeVector that contains the settings from
     *          the GUI
     */
    PT::OptimizeVector getOptimizeSettings();


    void runOptimizer(const PT::OptimizeVector & optvars, const PT::PanoramaOptions & options);

    wxCheckListBox * m_yaw_list;
    wxCheckListBox * m_pitch_list;
    wxCheckListBox * m_roll_list;
    wxCheckListBox * m_lens_list;

    PT::Panorama * m_pano;
private:
    DECLARE_EVENT_TABLE()
};


#endif // _WXPANOCOMMAND_H
