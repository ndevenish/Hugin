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

#include <panotools/PanoToolsInterface.h>
#include <base_wx/wxImageCache.h>

class CPEditorPanel;
class CPImageCtrl;

/** Events to notify about new point / region / point change
 *
 */
class CPEvent : public wxCommandEvent
{
    DECLARE_DYNAMIC_CLASS(CPEvent)

    enum CPEventMode { NONE, NEW_POINT_CHANGED, NEW_LINE_ADDED, POINT_SELECTED, POINT_CHANGED, RIGHT_CLICK, SCROLLED, DELETE_REGION_SELECTED };

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
    /** delete region selected */
    CPEvent(wxWindow* win, const hugin_utils::FDiff2D & p1, const hugin_utils::FDiff2D & p2);
    /// right mouse click
    CPEvent(wxWindow* win, CPEventMode mode, const hugin_utils::FDiff2D & p);
    /** new control point */
    CPEvent(wxWindow* win, CPEventMode mode, const HuginBase::ControlPoint cp);
    /** updated control point */
    CPEvent(wxWindow* win, CPEventMode mode, size_t cpNr, const HuginBase::ControlPoint cp);

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

    const HuginBase::ControlPoint& getControlPoint() 
        { return m_cp; };
private:
    CPEventMode mode;
    wxRect region;
    hugin_utils::FDiff2D point;
    HuginBase::ControlPoint m_cp;
    int pointNr;
};

typedef void (wxEvtHandler::*CPEventFunction)(CPEvent&);

BEGIN_DECLARE_EVENT_TYPES()
#if _WINDOWS && defined Hugin_shared
    DECLARE_LOCAL_EVENT_TYPE(EVT_CPEVENT,1)
#else
    DECLARE_EVENT_TYPE(EVT_CPEVENT,1)
#endif
END_DECLARE_EVENT_TYPES()

#define EVT_CPEVENT(func) \
    DECLARE_EVENT_TABLE_ENTRY( EVT_CPEVENT, \
                            -1,                       \
                            -1,                       \
                            (wxObjectEventFunction)   \
                            (CPEventFunction) & func, \
                            (wxObject *) NULL ),


/** helper class to display and manipulate cp in cp tab */
class DisplayedControlPoint
{
public:
    /** default constructor */
    DisplayedControlPoint() { m_mirrored=false; m_control=NULL; m_line=false; };
    /** constructor with full initialisation */
    DisplayedControlPoint(const HuginBase::ControlPoint& cp, CPImageCtrl* control, bool mirrored);
    /** set colours for drawing control points */
    void SetColour(wxColour pointColour, wxColour textColour);
    /** set label to given wxString */
    void SetLabel(wxString newLabel);
    /** remember the control, where the information should be drawn */
    void SetControl(CPImageCtrl* control);
    /** draw the control points to the given device context */
    void Draw(wxDC& dc, bool selected, bool newPoint=false);
    /** check if given point is over label of cp, using screen coordinates */
    const bool isOccupiedLabel(const wxPoint mousePos) const;
    /** check if the given point is over the drawn cp, using image coordinates */
    const bool isOccupiedPos(const hugin_utils::FDiff2D &p) const;
    /** used by manipulating line control points, remember if the selected point 
        given in screen coordinates @param mousePos and image coordinates @param p
        is over the first or second point, stores this information in m_mirrored */
    void CheckSelection(const wxPoint mousePos, const hugin_utils::FDiff2D& p);
    /** return true, if cp is used with mirrored coordinates by current CPImageCtrl */
    const bool IsMirrored() const { return m_mirrored; };
    /** return label */
    const wxString GetLabel() const { return m_label; };
    /** update x coordinate of selected cp coordinate */
    void UpdateControlPointX(double x);
    /** update y coordinate of selected cp coordinate */
    void UpdateControlPointY(double y);
    /** update selected cp coordinate */
    void UpdateControlPoint(hugin_utils::FDiff2D newPoint);
    /** shift selected cp coordinate by given @param shift */
    void ShiftControlPoint(hugin_utils::FDiff2D shift);
    /** starts a new line control point with given coodinates */
    void StartLineControlPoint(hugin_utils::FDiff2D newPoint);
    /** returns selected position */
    hugin_utils::FDiff2D GetPos() const;
    /** returns the control point */
    const HuginBase::ControlPoint GetControlPoint() const { return m_cp; };
    /** compare operator */
    bool operator==(const DisplayedControlPoint other);
private:
    /** draw magnified area */
    wxRect DrawTextMag(wxDC& dc, wxPoint p, hugin_utils::FDiff2D pointInput, bool drawMag, wxColour pointColour, wxColour textColour);
    /** draw line control point on same image */
    void DrawLine(wxDC& dc);
    /** draw line control point over different images */
    void DrawLineSegment(wxDC& dc);
    /** representation of underlying control point */
    HuginBase::ControlPoint m_cp;
    /** is first or second image in cp used */
    bool m_mirrored;
    /** pointer to control to access some functions */
    CPImageCtrl* m_control;
    /** colour of the point */
    wxColour m_pointColour;
    /** colour of the text background */
    wxColour m_textColour;
    /** label of displayed control point: number or new */
    wxString m_label;
    /** position of the point labels (in screen coordinates) */
    wxRect m_labelPos;
    wxRect m_labelPos2;
    /** true, if line control point on same image*/
    bool m_line;
};

/** brief description.
 *
 *  What this does
 */
class CPImageCtrl : public wxScrolledWindow
{
public:
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
     *        - SELECT_DELETE_REGION
     *          - mouse down on image
     *
     *    - KNOWN_POINT_SELECTED,
     *       a known point is selected and can be moved around.
     *       clients are notified about the movement, points can also
     *       be switched.
     *
     *        - KNOWN_POINT_SELECTED
     *          - selection of another point
     *        - SELECT_DELETE_REGION
     *          - mouse down on image
     *
     *    - NEW_POINT_SELECTED (can move new point), mouse up reports change,
     *           movement can be tracked
     *        - KNOWN_POINT_SELECTED
     *          - mouse down on known point
     *          - programatic change
     *        - SELECT_DELETE_REGION
     *          - mouse down on image
     *
     *    - NEW_LINE_CREATING mouse up reports new line
     *            movement can be tracked
     *
     *    - SELECT_DELETE_REGION user can draw rectangle inside which all cp should be removed
     *        - NO_SELECTION

     */
    enum EditorState {NO_IMAGE=0, NO_SELECTION, KNOWN_POINT_SELECTED, NEW_POINT_SELECTED, NEW_LINE_CREATING, SELECT_DELETE_REGION};
    
    /** ctor.
     */
    CPImageCtrl()
        : scaleFactor(1),fitToWindow(false)
        { }

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(CPEditorPanel * parent);


    /** dtor.
     */
    ~CPImageCtrl();

    /** image rotation.
     *  Useful to display images depending on their roll setting.
     *  rotation is clockwise
     */ 
    enum ImageRotation { ROT0=0, ROT90, ROT180, ROT270 };

    /// display img. every CPImageCtrl has a wxBitmap with
    /// its current image
    void setImage (const std::string & filename, ImageRotation rot);
    void setSameImage(bool sameImage);
    void setTransforms(HuginBase::PTools::Transform* firstTrans, HuginBase::PTools::Transform* firstInvTrans, HuginBase::PTools::Transform* secondInvTrans);
    HuginBase::PTools::Transform* getFirstTrans() const { return m_firstTrans; };
    HuginBase::PTools::Transform* getFirstInvTrans() const { return m_firstInvTrans; };
    HuginBase::PTools::Transform* getSecondInvTrans() const { return m_secondInvTrans; };

    /** add control piont to internal cp list */
    void setCtrlPoint(const HuginBase::ControlPoint& cp, const bool mirrored);
    /** clear internal control point list */
    void clearCtrlPointList();

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
    void mousePressRMBEvent(wxMouseEvent& mouse);
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
            return wxPoint(hugin_utils::roundi(p.x), hugin_utils::roundi(p.y));
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
    // some helper function for DisplayedControlPoint 
    const bool GetMouseInWindow() const { return m_mouseInWindow; };
    const bool GetForceMagnifier() const { return m_forceMagnifier; };
    /** get pointer to image, for DisplayedControlPoint */
    const HuginBase::ImageCache::ImageCacheRGB8Ptr GetImg() const { return m_img->image8; };
    /** draw the magnified view of a selected control point */
    wxBitmap generateMagBitmap(hugin_utils::FDiff2D point, wxPoint canvasPos) const;
    /** return the real size of the image in the control */
    const wxSize GetRealImageSize() const { return m_realSize; };
    /** return the size of the drawn bitmap (possible rotate is applied) */
    const wxSize GetBitmapSize() const;

protected:
    // display the image when loading finishes
    void OnImageLoaded(ImageCache::EntryPtr entry, std::string filename, bool load_small);
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

private:
    wxBitmap bitmap;
    std::string imageFilename;
    // size of displayed (probably scaled) image
    wxSize imageSize;
    // size of real image
    wxSize m_realSize;

    std::vector<DisplayedControlPoint> m_points;

    wxCursor * m_CPSelectCursor;
    wxCursor * m_ScrollCursor;

    // this is only valid during MOVE_POINT
    unsigned int selectedPointNr;
    // valid during MOVE_POINT and CREATE_POINT
    DisplayedControlPoint m_selectedPoint;
    hugin_utils::FDiff2D newPoint;
    /** true, if in control point tab the same image is selected 2 times
        in this case a special treatment for creating line control points
        is activated */
    bool m_sameImage;

    // only valid during SELECT_DELETE_REGION
    hugin_utils::FDiff2D rectStartPos;
    // draw a selection rectangle from pos1 to pos2
    void DrawSelectionRectangle(hugin_utils::FDiff2D pos1,hugin_utils::FDiff2D pos2);

    EditorState editState;
    // store pointer to transformation object to draw line control points
    HuginBase::PTools::Transform* m_firstTrans;
    HuginBase::PTools::Transform* m_firstInvTrans;
    HuginBase::PTools::Transform* m_secondInvTrans;

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

    /// check if p is over a known point, if it is, pointNr contains
    /// the point
    EditorState isOccupied(wxPoint mousePos, const hugin_utils::FDiff2D & point, unsigned int & pointNr) const;

    CPEditorPanel * m_editPanel;

    ImageRotation m_imgRotation;

    ImageCache::EntryPtr m_img;
    ImageCache::RequestPtr m_imgRequest;

    bool m_mouseInWindow;
    bool m_forceMagnifier;
    wxTimer m_timer;

    DECLARE_EVENT_TABLE();
    DECLARE_DYNAMIC_CLASS(CPImageCtrl)
};

/** xrc handler */
class CPImageCtrlXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(CPImageCtrlXmlHandler)

public:
    CPImageCtrlXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};


#endif // _CPIMAGECTRL_H
