// -*- c-basic-offset: 4 -*-
/** @file OptimizePanel.h
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

#ifndef _OPTIMIZE_PANEL_H
#define _OPTIMIZE_PANEL_H

#include <PT/Panorama.h>

class wxCheckListBox;

/** run the optimizer. this is tied into the wxWindows hugin
 *
 *  it will emit an event that
 */
class OptimizePanel : public wxPanel, public PT::PanoramaObserver
{


public:
    OptimizePanel(wxWindow * parent, PT::Panorama * pano);
    virtual ~OptimizePanel();

    /** receives notification about panorama changes */
    virtual void panoramaChanged(PT::Panorama & pano);
    /** receives notification about panorama changes */
    virtual void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** return currently selected OptimizeVector
     *
     *  @return OptimizeVector that contains the settings from
     *          the GUI
     */
    PT::OptimizeVector getOptimizeVector();

    /** updates the display to fit the settings in optimize vector */
    void setOptimizeVector(const PT::OptimizeVector & optvec);

    /** run the optimizer */
    void OnOptimizeButton(wxCommandEvent & e);

protected:

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

    // helper function for wxCheckListBox
    void SetCheckMark(wxCheckListBox * l, int check);

    // called whenever the optimize mode changes
    void OnChangeMode(wxCommandEvent & e);

    void runOptimizer(const PT::OptimizeVector & optvars,
		              const PT::PanoramaOptions & options, 
                      const PT::UIntSet & img);

    wxCheckListBox * m_yaw_list;
    wxCheckListBox * m_pitch_list;
    wxCheckListBox * m_roll_list;

    wxCheckListBox * m_v_list;
    wxCheckListBox * m_a_list;
    wxCheckListBox * m_b_list;
    wxCheckListBox * m_c_list;
    wxCheckListBox * m_d_list;
    wxCheckListBox * m_e_list;

    wxCheckBox * m_edit_cb;
    wxChoice * m_mode_cb;
	
#ifdef USE_WX253
	wxScrolledWindow *m_opt_ctrls;
#endif
	
    PT::Panorama * m_pano;
private:
	
    DECLARE_EVENT_TABLE()
};


#endif // _WXPANOCOMMAND_H
