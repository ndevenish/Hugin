// -*- c-basic-offset: 4 -*-
/** @file CPImageCtrl.h
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

#ifndef _CPIMAGECTRL_H
#define _CPIMAGECTRL_H

#include <string>
#include <vector>

#include <base_wx/ImageCache.h>

class CPEditorPanel;
class CPZoomDisplayPanel;
/** Events to notify about new point / region / point change
 *
 */

class CPEvent : public wxCommandEvent
{
    DECLARE_DYNAMIC_CLASS(CPEvent)

    enum CPEventMode { NONE, NEW_POINT_CHANGED, POINT_SELECTED, POINT_CHANGED, REGION_SELECTED, RIGHT_CLICK, SCROLLED };

public:
    CPEvent( );
    /// create a specific CPEvent
    CPEvent(wxWindow* win, CPEventMode mode);
    /// a new point has been created.
    CPEvent(wxWindow* win, hugin_utils::FDiff2D & p);
    /// a point has been selected
    CPEvent(wxWindow *win, unsigned int cpNr);
    /// a point has been moved
    CPEvent(wxWindow* win, unsigned int cpNr, const hugin_utils::FDiff2D & p);
    /// region selected
    CPEvent(wxWindow* win, wxRect & reg);
    /// right mouse click
    CPEvent(wxWindow* win, CPEventMode mode, const hugin_utils::FDiff2D & p);

    virtual wxEvent* Clone() const;

    /// accessor functions (they could check mode to see if a getXYZ() is
    /// allowed
    CPEventMode getMode()
        { return mode; };

    const wxRect & getRect()
        { return region; }

    const hugin_utils::FDiff2D & getPoint()
        { return point; }

    unsigned int getPointNr()
        { return pointNr; }
private:
    CPEventMode mode;
    wxRect region;
    hugin_utils::FDiff2D point;
    int pointNr;
};

typedef void (wxEvtHandler::*CPEventFunction)(CPEvent&);

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(EVT_CPEVENT,1)
END_DECLARE_EVENT_TYPES()

#define EVT_CPEVENT(func) \
    DECLARE_EVENT_TABLE_ENTRY( EVT_CPEVENT, \
                            -1,                       \
                            -1,                       \
                            (wxObjectEventFunction)   \
                            (CPEventFunction) & func, \
                            (wxObject *) NULL ),


/** brief description.
 *
 *  What this does
 */
class CPImageCtrl : public wxScrolledWindow
{
public:
    /** ctor.
     */

    CPImageCtrl();

    CPImageCtrl(wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxHSCROLL | wxVSCROLL, const wxString& name = wxT("scrolledWindow"));


    /** dtor.
     */
    ~CPImageCtrl();

    /** Delayed creation */
    bool Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name);

    /** proper initialisation */
    void Init(CPEditorPanel* parent);

    /** image rotation.
     *  Useful to display images depending on their roll setting.
     *  rotation is clockwise
     */ 
    enum ImageRotation { ROT0=0, ROT90, ROT180, ROT270 };

    /// associate a zoomed display with this image
    void SetZoomView(CPZoomDisplayPanel * display);

    /// display img. every CPImageCtrl has a wxBitmap with
    /// its current image
    void setImage (const std::string & filename, ImageRotation rot);

    /// control point inside this image
    void setCtrlPoints(const std::vector<hugin_utils::FDiff2D> & points);

    /// clear new point
    void clearNewPoint();

    /// set new point to a specific point
    void setNewPoint(const hugin_utils::FDiff2D & p);

    /// select a point for usage
    void selectPoint(unsigned int);

    /// remove selection.
    void deselect();

    void mousePressLMBEvent(wxMouseEvent& mouse);
    void mouseReleaseLMBEvent(wxMouseEvent& mouse);
    void mouseReleaseRMBEvent(wxMouseEvent& mouse);
    void mouseMoveEvent(wxMouseEvent& mouse);
    void mousePressMMBEvent(wxMouseEvent& mouse);
    void mouseReleaseMMBEvent(wxMouseEvent& mouse);

    wxSize DoGetBestSize() const;
//    virtual wxSize GetBestSize() const
//        { return DoGetBestSize(); }

    /** set the scaling factor for cp display.
     *
     *  @param factor zoom factor, 0 means fit to window.
     */
    void setScale(double factor);

    /// return scale factor, 0 for autoscale
    double getScale()
        { return fitToWindow ? 0 : scaleFactor; }

    /** Show point @p x, @p y
     *
     *  Scrolls the windows so that x,y is shown in the center
     *  if @p warpPointer is true, the mouse pointer is moved
     *  to that position as well
     */
    void showPosition(hugin_utils::FDiff2D point, bool warpPointer=false);

    /** show/hide the search area rectangle
     *
     */
    void showSearchArea(bool show=true);

    void showTemplateArea(bool show=true);

    /// get the new point
    hugin_utils::FDiff2D getNewPoint();

    /// initiate redraw
    void update();

    /// scroll the window by @p delta pixels
    void ScrollDelta(const wxPoint & delta);

    /// calculate maximum delta that is allowed when scrolling
    wxPoint MaxScrollDelta(wxPoint delta);

protected:
    wxRect drawPoint(wxDC & p, const hugin_utils::FDiff2D & point, int i, bool selected = false) const;
    // draw the magnified view of a selected control point
    wxBitmap generateMagBitmap(hugin_utils::FDiff2D point, wxPoint canvasPos) const;
    void OnDraw(wxDC& dc);
    void OnSize(wxSizeEvent & e);
    void OnKey(wxKeyEvent & e);
    void OnKeyDown(wxKeyEvent & e);
    void OnMouseLeave(wxMouseEvent & e);
    void OnMouseEnter(wxMouseEvent & e);
    void OnTimer(wxTimerEvent & e);

    /// helper func to emit a region
    bool emit(CPEvent & ev);

    /// get scale factor (calculates factor when fit to window is active)
    double getScaleFactor() const;

    /// calculate new scale factor for this image
    double calcAutoScaleFactor(wxSize size);

    // rescale image
    void rescaleImage();

    /// update display of zoomed point
//    void updateZoomed();



private:

    wxBitmap bitmap;
    std::string imageFilename;
    // size of displayed (probably scaled) image
    wxSize imageSize;
    // size of real image
    wxSize m_realSize;

    std::vector<hugin_utils::FDiff2D> points;

    // position of the point labels (in screen coordinates)
    std::vector<wxRect> m_labelPos;

    wxCursor * m_CPSelectCursor;
    wxCursor * m_ScrollCursor;

    int scale(int x) const
        {  return (int) (x * getScaleFactor() + 0.5); }

    double scale(double x) const
        {  return x * getScaleFactor(); }

    hugin_utils::FDiff2D scale(const hugin_utils::FDiff2D & p) const
        {
            hugin_utils::FDiff2D r;
            r.x = scale(p.x);
            r.y = scale(p.y);
            return r;
        }

    wxPoint scale(const wxPoint & p) const
        {
            wxPoint r;
            r.x = scale(p.x);
            r.y = scale(p.y);
            return r;
        }

    int invScale(int x) const
        {  return (int) (x / getScaleFactor() + 0.5); }

    double invScale(double x) const
        {  return x / getScaleFactor(); }

    hugin_utils::FDiff2D invScale(const hugin_utils::FDiff2D & p) const
        {
            hugin_utils::FDiff2D r;
            r.x = invScale(p.x);
            r.y = invScale(p.y);
            return r;
        }

    wxPoint invScale(const wxPoint & p) const
        {
            wxPoint r;
            r.x = invScale(p.x);
            r.y = invScale(p.y);
            return r;
        }

    wxPoint roundP(const hugin_utils::FDiff2D & p) const
        {
            return wxPoint(utils::roundi(p.x), utils::roundi(p.y));
        }

    // rotate coordinate to fit possibly rotated image display
    // useful for drawing something on the rotated display
    template <class T>
    T applyRot(const T & p) const
    {
        switch (m_imgRotation) {
            case ROT0:
                return p;
                break;
            case ROT90:
                return T(m_realSize.GetHeight()-1 - p.y, p.x);
                break;
            case ROT180:
                return T(m_realSize.GetWidth()-1 - p.x, m_realSize.GetHeight()-1 - p.y);
                break;
            case ROT270:
                return T(p.y, m_realSize.GetWidth()-1 - p.x);
                break;
            default:
                return p;
                break;
        }
    }

    // rotate coordinate to fit possibly rotated image display
    // useful for converting rotated display coordinates to image coordinates
    template <class T>
    T applyRotInv(const T & p) const
    {
        switch (m_imgRotation) {
            case ROT90:
                return T(p.y, m_realSize.GetHeight()-1 - p.x);
                break;
            case ROT180:
                return T(m_realSize.GetWidth()-1 - p.x, m_realSize.GetHeight()-1 - p.y);
                break;
            case ROT270:
                return T(m_realSize.GetWidth()-1 - p.y, p.x);
                break;
            case ROT0:
            default:
                return p;
                break;
        }
    }

    // this is only valid during MOVE_POINT
    unsigned int selectedPointNr;
    // valid during MOVE_POINT and CREATE_POINT
    hugin_utils::FDiff2D point;
    hugin_utils::FDiff2D newPoint;

    // only valid during SELECT_REGION
    wxRect region;
    // state of widget (selection modes etc)
    // select region can also be used to just click...

    /**  state machine for selection process:
     *
     *   The select region is temporarily disabled.. maybe I find use
     *   for it later on..
     *
     *   format of this list:
     *    - current state name
     *        - possible next state
     *          - conditions for next state
     *
     *   states:
     *
     *    - NO_IMAGE
     *        - NO_SELECTION
     *          - an image has been inserted
     *
     *    - NO_SELECTION nothing selected
     *        - KNOWN_POINT_SELECTED
     *          - mouse down on known point
     *          - set from outside
     *        - REGION
     *          - mouse down on unidentifed field
     *
     *    - KNOWN_POINT_SELECTED,
     *       a known point is selected and can be moved around.
     *       clients are notified about the movement, points can also
     *       be switched.
     *
     *        - KNOWN_POINT_SELECTED
     *          - selection of another point
     *        - REGION
     *          - mouse down on unidentifed field
     *
     *    - REGION the user can draw a bounding box.
     *        - NEW_POINT_SELECTED
     *          - mouse up on same point as mouse down.
     *        - NO_SELECTION
     *          - mouse up on different point, report selection
     *
     *    - NEW_POINT_SELECTED (can move new point), mouse up reports change,
     *           movement can be tracked
     *        - KNOWN_POINT_SELECTED
     *          - mouse down on known point
     *          - programatic change
     *        - REGION
     *          - mouse down on free space
     *
     */
    enum EditorState {NO_IMAGE=0, NO_SELECTION, KNOWN_POINT_SELECTED, NEW_POINT_SELECTED, SELECT_REGION};
    EditorState editState;

    // colors for the different points
    std::vector<wxColour> pointColors;
    std::vector<wxColour> textColours;
    double scaleFactor;
    bool fitToWindow;

    bool m_showSearchArea;
    int m_searchRectWidth;

    hugin_utils::FDiff2D m_mousePos;
    wxPoint m_mouseScrollPos;

    bool m_showTemplateArea;
    int m_templateRectWidth;

    bool m_tempZoom;
    double m_savedScale;

    /// check if p is over a known point, if it is, pointNr contains
    /// the point
    EditorState isOccupied(wxPoint mousePos, const hugin_utils::FDiff2D & point, unsigned int & pointNr) const;

    CPEditorPanel * m_editPanel;

//    CPZoomDisplayPanel * m_zoomDisplay;

    ImageRotation m_imgRotation;

    ImageCache::EntryPtr m_img;

    bool m_mouseInWindow;
    bool m_forceMagnifier;
    wxTimer m_timer;

    DECLARE_EVENT_TABLE();
};



#endif // _CPIMAGECTRL_H
