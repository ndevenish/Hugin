// -*- c-basic-offset: 4 -*-
/** @file CPEditorPanel.h
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

#ifndef _CPEDITORPANEL_H
#define _CPEDITORPANEL_H



//-----------------------------------------------------------------------------
// Headers
//-----------------------------------------------------------------------------

#include <vector>
#include <set>
#include <wx/panel.h>
#include <PT/Panorama.h>

// forward declarations
class CPImageCtrl;
class CPEvent;
class wxTabView;
class wxNotebookEvent;

/** control point editor panel.
 *
 *  This panel is used to create/change/edit control points
 *
 *  @todo support control lines
 */
class CPEditorPanel : public wxPanel, public PT::PanoramaObserver
{
public:

    /** ctor.
     */
    CPEditorPanel(wxWindow * parent);

    /** dtor.
     */
    virtual ~CPEditorPanel();

    void setLeftImage(wxImage & img);
    void setRightImage(wxImage & img);
    
    /** called when the panorama changes and we should
     *  update our display
     */
    void panoramaChanged(PT::Panorama &pano);

private:

    // event handler functions
    void OnMyButtonClicked(wxCommandEvent &e);
    void OnCPEvent(CPEvent &ev);
    void OnLeftImgChange(wxNotebookEvent & e);
    void OnRightImgChange(wxNotebookEvent & e);


    // GUI controls
    CPImageCtrl *m_leftImg, *m_rightImg;
//    wxTabView *m_leftTabs, *m_rightTabs;
    
    // my data
    
    // the current images
    unsigned int firstImage;
    unsigned int secondImage;
    
    wxPoint newPoint;
    enum CPCreationState { NO_POINT, FIRST_POINT, SECOND_POINT};
    CPCreationState cpCreationState;

    typedef std::pair<unsigned int, PT::ControlPoint> CPoint;
    std::vector<CPoint> currentPoints;
    // this set contains all points that are mirrored (point 1 in right window,
    // point 2 in left window), in local point numbers
    std::set<unsigned int> mirroredPoints;

    
    // needed for receiving events.
    DECLARE_EVENT_TABLE();
};

#endif // _CPEDITORPANEL_H
