// -*- c-basic-offset: 4 -*-
/** @file OptimizePhotometricPanel.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: OptimizePhotometricPanel.h 1935 2007-04-15 20:25:06Z dangelo $
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

#ifndef _OPTIMIZE_PHOTO_PANEL_H
#define _OPTIMIZE_PHOTO_PANEL_H

#include <PT/Panorama.h>

class wxCheckListBox;

/** run the optimizer. this is tied into the wxWindows hugin
 *
 *  it will emit an event that
 */
class OptimizePhotometricPanel : public wxPanel, public PT::PanoramaObserver
{


public:
    OptimizePhotometricPanel(wxWindow * parent, PT::Panorama * pano);
    virtual ~OptimizePhotometricPanel();

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
    void OnSelExposure(wxCommandEvent & e);
    void OnDelExposure(wxCommandEvent & e);
    void OnSelWB(wxCommandEvent & e);
    void OnDelWB(wxCommandEvent & e);

    // helper function for wxCheckListBox
    void SetCheckMark(wxCheckListBox * l, int check, int anchor = -1);

    // called whenever the optimize mode changes
    void OnChangeMode(wxCommandEvent & e);

    void runOptimizer(const PT::UIntSet & img);

    wxCheckListBox * m_vig_list;
    wxCheckListBox * m_vigc_list;
    wxCheckListBox * m_exp_list;
    wxCheckListBox * m_wb_list;
    wxCheckListBox * m_resp_list;

    wxChoice * m_mode_cb;
	
#ifdef USE_WX253
	wxScrolledWindow *m_opt_ctrls;
#endif
	
    PT::Panorama * m_pano;
private:
	
    DECLARE_EVENT_TABLE()
};


#endif // _WXPANOCOMMAND_H
