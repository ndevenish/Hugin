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
#include <functional>
#include <utility>
#include <PT/Panorama.h>

// forward declarations
class CPImageCtrl;
class CPEvent;
class wxTabView;
class wxNotebook;
class wxNotebookEvent;
class wxListCtrl;

struct CorrelationResult;

namespace vigra {
    struct Diff2D;
}

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
    CPEditorPanel(wxWindow * parent, PT::Panorama * pano);

    /** dtor.
     */
    virtual ~CPEditorPanel();

    /// set left image
    void setLeftImage(unsigned int imgNr);
    /// set right image
    void setRightImage(unsigned int imgNr);

    void SetPano(PT::Panorama * panorama)
        { m_pano = panorama; };

    /** called when the panorama changes and we should
     *  update our display
     */
    void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);


    /** Select a point.
     *
     *  This should highlight it in the listview and on the pictures
     */
    void SelectGlobalPoint(unsigned int globalNr);

private:

    /** updates the display after another image has been selected.
     *  updates control points, and other widgets
     */
    void UpdateDisplay();

    /** select a local point.
     *
     *  @todo scroll windows so that the point is centred
     */
    void SelectLocalPoint(unsigned int LVpointNr);

    /// map a global point nr to a local one, if possible
    bool globalPNr2LocalPNr(unsigned int & localNr, unsigned int globalNr) const;
    // function called when a new point has been selected in the first or
    // second image
    void CreateNewPointLeft(wxPoint p);
    void CreateNewPointRight(wxPoint p);

    /// search for region in destImg
    bool FindTemplate(unsigned int tmplImgNr, const wxRect &region, unsigned int dstImgNr, CorrelationResult & res);

    bool CPEditorPanel::PointFineTune(unsigned int tmplImgNr,
                                      const vigra::Diff2D &tmplPoint,
                                      unsigned int subjImgNr,
                                      const vigra::Diff2D &subjPoint,
                                      vigra::Diff2D & tunedPos);

    // event handler functions
    void OnMyButtonClicked(wxCommandEvent &e);
    void OnCPEvent(CPEvent &ev);
    void OnLeftImgChange(wxNotebookEvent & e);
    void OnRightImgChange(wxNotebookEvent & e);
    void OnCPListSelect(wxListEvent & e);
    void OnZoom(wxCommandEvent & e);


    // GUI controls
    wxNotebook *m_leftTabs, *m_rightTabs;
    CPImageCtrl *m_leftImg, *m_rightImg;
    wxListCtrl *m_cpList;

    wxTextCtrl *m_x1Text, *m_y1Text, *m_x2Text, *m_y2Text, *m_errorText;
    wxChoice *m_cpModeChoice;

    // my data
    PT::Panorama * m_pano;
    // the current images
    unsigned int m_leftImageNr;
    unsigned int m_rightImageNr;
    std::string m_leftFile, m_rightFile;
    bool m_listenToPageChange;

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
