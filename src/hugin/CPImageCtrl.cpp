// -*- c-basic-offset: 4 -*-

/** @file CPImageCtrl.cpp
 *
 *  @brief implementation of CPImageCtrl Class
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

// standard wx include
#include <config.h>
#include "panoinc_WX.h"

// standard hugin include
#include "panoinc.h"

#include "hugin/config_defaults.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/MainFrame.h"

#if 0
#include "hugin/UniversalCursor.h"
#endif

#include "hugin/CPZoomDisplayPanel.h"

using namespace std;
using namespace utils;

// event stuff


// definition of the control point event

IMPLEMENT_DYNAMIC_CLASS( CPEvent, wxEvent )
DEFINE_EVENT_TYPE( EVT_CPEVENT )

CPEvent::CPEvent( )
{
    SetEventType( EVT_CPEVENT );
    SetEventObject( (wxWindow *) NULL );
    mode = NONE;
}

CPEvent::CPEvent(wxWindow* win, FDiff2D & p)
{
    SetEventType( EVT_CPEVENT );
    SetEventObject( win );
    mode = NEW_POINT_CHANGED;
    point = p;
}

CPEvent::CPEvent(wxWindow *win, unsigned int cpNr)
{
    SetEventType( EVT_CPEVENT );
    SetEventObject( win );
    mode = POINT_SELECTED;
    pointNr = cpNr;
}

CPEvent::CPEvent(wxWindow* win, unsigned int cpNr, const FDiff2D & p)
{
    SetEventType( EVT_CPEVENT );
    SetEventObject( win );
    mode = POINT_CHANGED;
    pointNr = cpNr;
    point = p;
}

CPEvent::CPEvent(wxWindow* win, wxRect & reg)
{
    SetEventType( EVT_CPEVENT );
    SetEventObject( win );
    mode = REGION_SELECTED;
    region = reg;
}

CPEvent::CPEvent(wxWindow* win, CPEventMode evt_mode, const FDiff2D & p)
{
    SetEventType(EVT_CPEVENT);
    SetEventObject(win);
    mode = evt_mode;
    point = p;
}

wxEvent * CPEvent::Clone() const
{
    return new CPEvent(*this);
}


// our image control

BEGIN_EVENT_TABLE(CPImageCtrl, wxScrolledWindow)
    EVT_SIZE(CPImageCtrl::OnSize)
    EVT_CHAR(CPImageCtrl::OnKey)
    EVT_KEY_UP(CPImageCtrl::OnKeyUp)
    EVT_KEY_DOWN(CPImageCtrl::OnKeyDown)
    EVT_LEAVE_WINDOW(CPImageCtrl::OnMouseLeave)
    EVT_ENTER_WINDOW(CPImageCtrl::OnMouseEnter)
    EVT_MOTION(CPImageCtrl::mouseMoveEvent)
    EVT_LEFT_DOWN(CPImageCtrl::mousePressLMBEvent)
    EVT_LEFT_UP(CPImageCtrl::mouseReleaseLMBEvent)
    EVT_RIGHT_UP(CPImageCtrl::mouseReleaseRMBEvent)
    EVT_MIDDLE_DOWN(CPImageCtrl::mousePressMMBEvent)
    EVT_MIDDLE_UP(CPImageCtrl::mouseReleaseMMBEvent)
END_EVENT_TABLE()

CPImageCtrl::CPImageCtrl(CPEditorPanel* parent, wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& name)
    : wxScrolledWindow(parent, id, pos, size, style, name),
      selectedPointNr(0),
      editState(NO_IMAGE),
      scaleFactor(1), fitToWindow(false),
      m_showSearchArea(false), m_searchRectWidth(0),
      m_showTemplateArea(false), m_templateRectWidth(0),
      m_tempZoom(false),m_savedScale(1), m_editPanel(parent),
      m_zoomDisplay(0)
{

    wxString filename;
//#if defined(__WXMSW__) || defined(__WXMAC__)
#if 1
    m_CPSelectCursor = new wxCursor(wxCURSOR_CROSS);
#else
    int cursorType = wxConfigBase::Get()->Read(wxT("/CPImageCtrl/CursorType"),HUGIN_CP_CURSOR);
    if (cursorType == 0) {
        m_CPSelectCursor = new wxCursor(wxCURSOR_CROSS);
    } else {
        filename.Printf(wxT("%sdata/CPCursor%d.png"),MainFrame::Get()->GetXRCPath().c_str(),
                        cursorType);
        wxImage cImg(filename);
        if (cImg.Ok()) {
            m_CPSelectCursor = new UniversalCursor(filename);
        } else {
            DEBUG_ERROR("Cursor file:" << filename << " not found");
            m_CPSelectCursor = new wxCursor(wxCURSOR_CROSS);
        }
    }
#endif
    // scroll cursor not used right now.
//    m_ScrollCursor = new wxCursor(wxCURSOR_HAND);
    SetCursor(*m_CPSelectCursor);

    // functions were renamed in 2.5 :(
#if (wxMAJOR_VERSION == 2 && wxMINOR_VERSION == 4)
    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("BLUE"))));
    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("GREEN"))));
    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("CYAN"))));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("MAGENTA")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("GOLD"))));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour("ORANGE")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("NAVY"))));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("FIREBRICK"))));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("SIENNA"))));
    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("DARK TURQUOISE"))));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("SALMON"))));
    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("MAROON"))));
    pointColors.push_back(*(wxTheColourDatabase->FindColour(wxT("KHAKI"))));
#else
    pointColors.push_back(wxTheColourDatabase->Find(wxT("BLUE")));
    pointColors.push_back(wxTheColourDatabase->Find(wxT("GREEN")));
    pointColors.push_back(wxTheColourDatabase->Find(wxT("CYAN")));
//    pointColors.push_back(wxTheColourDatabase->Find(wxT("MAGENTA")));
    pointColors.push_back(wxTheColourDatabase->Find(wxT("GOLD")));
//    pointColors.push_back(wxTheColourDatabase->Find(wxT("ORANGE"));
    pointColors.push_back(wxTheColourDatabase->Find(wxT("NAVY")));
//    pointColors.push_back(wxTheColourDatabase->Find(wxT("FIREBRICK")));
//    pointColors.push_back(wxTheColourDatabase->Find(wxT("SIENNA")));
    pointColors.push_back(wxTheColourDatabase->Find(wxT("DARK TURQUOISE")));
//    pointColors.push_back(wxTheColourDatabase->Find(wxT("SALMON")));
    pointColors.push_back(wxTheColourDatabase->Find(wxT("MAROON")));
    pointColors.push_back(wxTheColourDatabase->Find(wxT("KHAKI")));
#endif

    m_searchRectWidth = 120;
}

CPImageCtrl::~CPImageCtrl()
{
    DEBUG_TRACE("dtor");
    this->SetCursor(wxNullCursor);
    delete m_CPSelectCursor;
//    delete m_ScrollCursor;
    DEBUG_TRACE("dtor end");
}

void CPImageCtrl::SetZoomView(CPZoomDisplayPanel * d)
{
    m_zoomDisplay = d;
}


void CPImageCtrl::OnDraw(wxDC & dc)
{
    wxSize vSize = GetVirtualSize();
    // draw image (FIXME, redraw only visible regions.)
    if (editState != NO_IMAGE) {
        if (bitmap.GetWidth() < vSize.GetWidth()) {
            dc.SetPen(wxPen(GetBackgroundColour(), 1, wxSOLID));
            dc.SetBrush(wxBrush(GetBackgroundColour(),wxSOLID));
            dc.DrawRectangle(bitmap.GetWidth(), 0,
                             vSize.GetWidth() - bitmap.GetWidth(),vSize.GetHeight());
        }
        if (bitmap.GetHeight() < vSize.GetHeight()) {
            dc.SetPen(wxPen(GetBackgroundColour(), 1, wxSOLID));
            dc.SetBrush(wxBrush(GetBackgroundColour(),wxSOLID));
            dc.DrawRectangle(0, bitmap.GetHeight(),
                             vSize.GetWidth(), vSize.GetHeight() - bitmap.GetHeight());
        }
        dc.DrawBitmap(bitmap,0,0);
    }

    // draw known points.
    unsigned int i=0;
    vector<FDiff2D>::const_iterator it;
    for (it = points.begin(); it != points.end(); ++it) {
        if (editState == KNOWN_POINT_SELECTED && i==selectedPointNr) {

#if (wxMAJOR_VERSION == 2 && wxMINOR_VERSION == 4)
                drawHighlightPoint(dc,*it,*wxTheColourDatabase->FindColour(wxT("RED")));
#else
                drawHighlightPoint(dc,*it,wxTheColourDatabase->Find(wxT("RED")));
#endif

        } else {
            drawPoint(dc,*it,pointColors[i%pointColors.size()]);
        }
        i++;
    }

    switch(editState) {
    case SELECT_REGION:
        dc.SetLogicalFunction(wxINVERT);
        dc.SetPen(wxPen(wxT("WHITE"), 1, wxSOLID));
        dc.SetBrush(wxBrush(wxT("WHITE"),wxTRANSPARENT));
        dc.DrawRectangle(scale(region.GetLeft()),
                    scale(region.GetTop()),
                    scale(region.GetWidth()),
                    scale(region.GetHeight()));
        break;
    case NEW_POINT_SELECTED:
#if (wxMAJOR_VERSION == 2 && wxMINOR_VERSION == 4)
        drawHighlightPoint(dc, newPoint, *wxTheColourDatabase->FindColour(wxT("YELLOW")));
#else
        drawHighlightPoint(dc, newPoint, wxTheColourDatabase->Find(wxT("YELLOW")));
#endif
        if (m_showTemplateArea) {
            dc.SetLogicalFunction(wxINVERT);
            dc.SetPen(wxPen(wxT("RED"), 1, wxSOLID));
            dc.SetBrush(wxBrush(wxT("WHITE"),wxTRANSPARENT));
            wxPoint upperLeft = roundP(scale(newPoint - FDiff2D(m_templateRectWidth, m_templateRectWidth)));
            int width = scale(m_templateRectWidth);

            dc.DrawRectangle(upperLeft.x, upperLeft.y, 2*width, 2*width);
            dc.SetLogicalFunction(wxCOPY);
        }

        break;
    case KNOWN_POINT_SELECTED:
        break;
    case NO_SELECTION:
    case NO_IMAGE:
        break;
    }

    if (m_showSearchArea && m_mousePos.x != -1){
        dc.SetLogicalFunction(wxINVERT);
        dc.SetPen(wxPen(wxT("WHITE"), 1, wxSOLID));
        dc.SetBrush(wxBrush(wxT("WHITE"),wxTRANSPARENT));

        FDiff2D upperLeft = scale(m_mousePos - FDiff2D(m_searchRectWidth, m_searchRectWidth));
        int width = scale(m_searchRectWidth);
        DEBUG_DEBUG("drawing rect with width " << 2*width << " orig: " << m_searchRectWidth*2  << " scale factor: " << getScaleFactor());

        dc.DrawRectangle(roundi(upperLeft.x), roundi(upperLeft.y), 2*width, 2*width);
        dc.SetLogicalFunction(wxCOPY);
    }


}


void CPImageCtrl::drawPoint(wxDC & dc, const FDiff2D & point, const wxColor & color) const
{
    double f = getScaleFactor();
    if (f < 1) {
        f = 1;
    }

    dc.SetBrush(wxBrush(wxT("WHITE"),wxTRANSPARENT));
    dc.SetPen(wxPen(color, 2, wxSOLID));
    dc.DrawCircle(roundP(scale(point)), roundi(6*f));
    dc.SetPen(wxPen(wxT("BLACK"), roundi(1*f), wxSOLID));
    dc.DrawCircle(roundP(scale(point)), roundi(7*f));
    dc.SetPen(wxPen(wxT("WHITE"), 1, wxSOLID));
//    dc.DrawCircle(scale(point), 4);
}


void CPImageCtrl::drawHighlightPoint(wxDC & dc, const FDiff2D & point, const wxColor & color) const
{
    double f = getScaleFactor();
    if (f < 1) {
        f = 1;
    }

    dc.SetBrush(wxBrush(wxT("WHITE"),wxTRANSPARENT));
    dc.SetPen(wxPen(color, 3, wxSOLID));
    dc.DrawCircle(roundP(scale(point)), roundi(7*f));
    dc.SetPen(wxPen(wxT("BLACK"), roundi(1*f), wxSOLID));
    dc.DrawCircle(roundP(scale(point)), roundi(8*f));
    dc.SetPen(wxPen(wxT("WHITE"), 1, wxSOLID));
//    dc.DrawCircle(scale(point), 4);
}


wxSize CPImageCtrl::DoGetBestSize() const
{
    return wxSize(imageSize.GetWidth(),imageSize.GetHeight());
}


void CPImageCtrl::setImage(const std::string & file)
{
    DEBUG_TRACE("setting Image " << file);
    imageFilename = file;

    if (imageFilename != "") {
        editState = NO_SELECTION;
        rescaleImage();
    } else {
        editState = NO_IMAGE;
        bitmap = wxBitmap();
        SetSizeHints(0,0,0,0,1,1);
    }
}



void CPImageCtrl::rescaleImage()
{
    if (editState == NO_IMAGE) {
        return;
    }
    // rescale image
    wxImage * img = ImageCache::getInstance().getImage(imageFilename);
    imageSize = wxSize(img->GetWidth(), img->GetHeight());
    m_realSize = imageSize;
    if (fitToWindow) {
        scaleFactor = calcAutoScaleFactor(imageSize);
    }
    DEBUG_DEBUG("src image size "
                << imageSize.GetHeight() << "x" << imageSize.GetWidth());
    if (getScaleFactor() == 1.0) {
        bitmap = wxBitmap(img);
    } else {
        imageSize.SetWidth( scale(imageSize.GetWidth()) );
        imageSize.SetHeight( scale(imageSize.GetHeight()) );
        DEBUG_DEBUG("rescaling to " << imageSize.GetWidth() << "x"
                    << imageSize.GetHeight() );
        bitmap = wxBitmap(img->Scale(imageSize.GetWidth(), imageSize.GetHeight()));
        DEBUG_DEBUG("rescaling finished");
    }

    SetVirtualSize(imageSize.GetWidth(), imageSize.GetHeight());
    SetScrollRate(1,1);
    Refresh(FALSE);
//    SetSizeHints(-1,-1,imageSize.GetWidth(), imageSize.GetHeight(),1,1);
//    SetScrollbars(16,16,bitmap.GetWidth()/16, bitmap.GetHeight()/16);
}

void CPImageCtrl::setCtrlPoints(const std::vector<FDiff2D> & cps)
{
    points = cps;
    // update view
    update();
}



void CPImageCtrl::clearNewPoint()
{
    DEBUG_TRACE("clearNewPoint");
    if (editState != NO_IMAGE) {
        editState = NO_SELECTION;
    }
}


void CPImageCtrl::selectPoint(unsigned int nr)
{
    DEBUG_TRACE("nr: " << nr);
    assert(nr < points.size());
    selectedPointNr = nr;
    editState = KNOWN_POINT_SELECTED;
    showPosition(points[nr]);
    update();
}

void CPImageCtrl::deselect()
    {
        DEBUG_TRACE("deselecting points");
        if (editState == KNOWN_POINT_SELECTED) {
            editState = NO_SELECTION;
        }
        // update view
        update();
    }

void CPImageCtrl::showPosition(FDiff2D point, bool warpPointer)
{
    DEBUG_DEBUG("x: " << point.x  << " y: " << point.y);
    wxSize sz = GetClientSize();
    point = scale(point);
    int x = roundi(point.x);
    int y = roundi(point.y);
    int scrollx = x - sz.GetWidth()/2;
//    if (x<0) x = 0;
    int scrolly = y - sz.GetHeight()/2;
//    if (y<0) x = 0;
//    Scroll(x/16, y/16);
    Scroll(scrollx, scrolly);
    if (warpPointer) {
        int sx,sy;
        GetViewStart(&sx, &sy);
        DEBUG_DEBUG("relative coordinages: " << x-sx << "," << y-sy);
        WarpPointer(x-sx,y-sy);
    }
}

CPImageCtrl::EditorState CPImageCtrl::isOccupied(const FDiff2D &p, unsigned int & pointNr) const
{
    // check if mouse is over a known point
    vector<FDiff2D>::const_iterator it;
    for (it = points.begin(); it != points.end(); ++it) {
        if (p.x < it->x + invScale(4) &&
            p.x > it->x - invScale(4) &&
            p.y < it->y + invScale(4) &&
            p.y > it->y - invScale(4)
            )
        {
            pointNr = it - points.begin();
            return KNOWN_POINT_SELECTED;
            break;
        }
    }

    return NEW_POINT_SELECTED;
/*
    if (p.x < newPoint.x + 4 &&
        p.x > newPoint.x - 4 &&
        p.y < newPoint.y + 4 &&
        p.y > newPoint.y - 4)
    {
//    } else {
//        return SELECT_REGION;
    }
*/
}


void CPImageCtrl::mouseMoveEvent(wxMouseEvent& mouse)
{
    wxPoint mpos_;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos_.x, & mpos_.y);
    FDiff2D mpos(mpos_.x, mpos_.y);
    bool doUpdate = false;
    mpos = invScale(mpos);
    mpos_ = invScale(mpos_);
//    DEBUG_DEBUG(" pos:" << mpos.x << ", " << mpos.y);
    // only if the shift key is not pressed.
    if (mouse.LeftIsDown() && ! mouse.ShiftDown()) {
        switch(editState) {
        case NO_SELECTION:
            DEBUG_ERROR("mouse down movement without selection, in NO_SELECTION state!");
            break;
        case KNOWN_POINT_SELECTED:
            if (mpos.x >= 0 && mpos.x <= m_realSize.GetWidth()){
                points[selectedPointNr].x = mpos.x;
            } else if (mpos.x < 0) {
                points[selectedPointNr].x = 0;
            } else if (mpos.x > m_realSize.GetWidth()) {
                points[selectedPointNr].x = m_realSize.GetWidth();
            }

            if (mpos.y >= 0 && mpos.y <= m_realSize.GetHeight()){
                points[selectedPointNr].y = mpos.y;
            } else if (mpos.y < 0) {
                points[selectedPointNr].y = 0;
            } else if (mpos.y > m_realSize.GetHeight()) {
                points[selectedPointNr].y = m_realSize.GetHeight();
            }
            // emit a notify event here.
            //
            //emit(pointMoved(selectedPointNr, points[selectedPointNr]));
            // do more intelligent updating here?
            doUpdate = true;
            break;
            // not possible.
        case NEW_POINT_SELECTED:
            DEBUG_DEBUG("WARNING: mouse move in new point state")
            newPoint = mpos;
            //emit(newPointMoved(newPoint));
            doUpdate = true;
            break;
        case SELECT_REGION:
            DEBUG_FATAL("Select region not in use anymore");
            region.SetWidth(mpos_.x - region.x);
            region.SetHeight(mpos_.y - region.y);
            // do more intelligent updating here?
            doUpdate = true;
            break;
        case NO_IMAGE:
            break;
        }
    }

    if (mouse.MiddleIsDown() || mouse.ShiftDown() || mouse.m_controlDown ) {
        // scrolling with the mouse
        if (m_mouseScrollPos !=mouse.GetPosition()) {
            wxPoint delta_ = mouse.GetPosition() - m_mouseScrollPos;
            double speed = (double)GetVirtualSize().GetHeight() / GetClientSize().GetHeight();
//          int speed = wxConfigBase::Get()->Read(wxT("/CPEditorPanel/scrollSpeed"),5);
            wxPoint delta;
            delta.x = roundi(delta_.x * speed);
            delta.y =  roundi(delta_.y * speed);
            ScrollDelta(delta);
            if (mouse.ShiftDown()) {
                // emit scroll event, so that other window can be scrolled
                // as well.
                CPEvent e(this, CPEvent::SCROLLED, FDiff2D(delta.x, delta.y));
                emit(e);
            }
            m_mouseScrollPos = mouse.GetPosition();
        }
    }

//    DEBUG_DEBUG("ImageDisplay: mouse move, state: " << editState);

    // draw a rectangle
    if (m_showSearchArea) {
        doUpdate = true;
    }

    m_mousePos = mpos;
    // repaint
    if (doUpdate) {
        update();
    }
}


void CPImageCtrl::mousePressLMBEvent(wxMouseEvent& mouse)
{
    wxPoint mpos_;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos_.x, & mpos_.y);
    FDiff2D mpos(mpos_.x, mpos_.y);
    mpos = invScale(mpos);
    mpos_ = invScale(mpos_);
    DEBUG_DEBUG("mousePressEvent, pos:" << mpos.x
                << ", " << mpos.y);
    unsigned int selPointNr = 0;
//    EditorState oldstate = editState;
    EditorState clickState = isOccupied(mpos, selPointNr);
    if (mouse.LeftDown() && editState != NO_IMAGE
        && mpos.x < m_realSize.x && mpos.y < m_realSize.y)
    {
        // we can always select a new point
        if (clickState == KNOWN_POINT_SELECTED) {
            DEBUG_DEBUG("click on point: " << selPointNr);
            selectedPointNr = selPointNr;
            point = points[selectedPointNr];
            editState = clickState;
            CPEvent e( this, selectedPointNr);
            emit(e);
        } else if (clickState == NEW_POINT_SELECTED) {
            DEBUG_DEBUG("click on new space, select region/new point");
//            editState = SELECT_REGION;
            editState = NEW_POINT_SELECTED;
            newPoint = mpos;
            region.x = roundi(mpos.x);
            region.y = roundi(mpos.y);
        } else {
            DEBUG_ERROR("invalid state " << clickState << " on mouse down");
        }
//        DEBUG_DEBUG("ImageDisplay: mouse down, state change: " << oldstate
//                    << " -> " << editState);
    }
    m_mousePos = mpos;
}

void CPImageCtrl::mouseReleaseLMBEvent(wxMouseEvent& mouse)
{
    wxPoint mpos_;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos_.x, & mpos_.y);
    FDiff2D mpos(mpos_.x, mpos_.y);
    mpos = invScale(mpos);
    DEBUG_DEBUG("mouseReleaseEvent, pos:" << mpos.x
                << ", " << mpos.y);
//    EditorState oldState = editState;
    if (mouse.LeftUp()) {
        switch(editState) {
        case NO_SELECTION:
            DEBUG_WARN("mouse release without selection");
            break;
        case KNOWN_POINT_SELECTED:
        {
            DEBUG_DEBUG("mouse release with known point " << selectedPointNr);
            if (! (point == points[selectedPointNr]) ) {
                CPEvent e( this, selectedPointNr, points[selectedPointNr]);
                emit(e);
            //emit(pointChanged(selectedPointNr, points[selectedPointNr]));
            }
            break;
        }
        case NEW_POINT_SELECTED:
        {
//            assert(drawNewPoint);
            DEBUG_DEBUG("new Point changed (event fire): x:" << mpos.x << " y:" << mpos.y);
            // fire the wxWin event
            CPEvent e( this, newPoint);
            emit(e);
            //emit(newPointChanged(newPoint));
            break;
        }
        case SELECT_REGION:
        {
            DEBUG_FATAL("Select region not in use anymore");
            if (region.GetPosition() == roundP(mpos)) {
                // create a new point.
                editState = NEW_POINT_SELECTED;
                newPoint = mpos;
                DEBUG_DEBUG("new Point changed: x:" << mpos.x << " y:" << mpos.y);
                //emit(newPointChanged(newPoint));
                CPEvent e(this, newPoint);
                emit(e);
                update();
            } else {
                DEBUG_DEBUG("new Region selected " << region.GetLeft() << "," << region.GetTop() << " " << region.GetRight() << "," << region.GetBottom());
                editState = NO_SELECTION;
                // normalize region
                if (region.GetWidth() < 0) {
                    region.SetX(region.GetRight());
                }
                if (region.GetHeight() < 0) {
                    region.SetY(region.GetBottom());
                }
                //emit(regionSelected(region));
                CPEvent e(this, region);
                emit(e);
                update();
            }
            break;
        }
        case NO_IMAGE:
            break;

        }
//        DEBUG_DEBUG("ImageDisplay: mouse release, state change: " << oldState
//                    << " -> " << editState);
    }

}


void CPImageCtrl::mouseReleaseMMBEvent(wxMouseEvent& mouse)
{
    DEBUG_DEBUG("middle mouse button released, leaving scroll mode")
//    SetCursor(wxCursor(wxCURSOR_BULLSEYE));
}


void CPImageCtrl::mousePressMMBEvent(wxMouseEvent& mouse)
{
    DEBUG_DEBUG("middle mouse button pressed, entering scroll mode")
    m_mouseScrollPos = mouse.GetPosition();
//    SetCursor(wxCursor(wxCURSOR_HAND));
}


void CPImageCtrl::mouseReleaseRMBEvent(wxMouseEvent& mouse)
{
    wxPoint mpos_;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos_.x, & mpos_.y);
    FDiff2D mpos(mpos_.x, mpos_.y);
    mpos = invScale(mpos);
    DEBUG_DEBUG("mouseReleaseEvent, pos:" << mpos.x
                << ", " << mpos.y);

    if (mouse.RightUp()) {
        // set right up event
        DEBUG_DEBUG("Emitting right click (rmb release)");
        CPEvent e(this, CPEvent::RIGHT_CLICK, mpos);
        emit(e);
    }
}

void CPImageCtrl::update()
{
    if (editState == NO_IMAGE) return;
    DEBUG_TRACE("edit state:" << editState);
    wxClientDC dc(this);
    PrepareDC(dc);
    OnDraw(dc);

    updateZoomed();
}

void CPImageCtrl::updateZoomed()
{
    if (!m_zoomDisplay) return;

    // update zoom view
    switch(editState) {
    case KNOWN_POINT_SELECTED:
        // update known point
        m_zoomDisplay->SetPoint(points[selectedPointNr]);
        break;
    case NEW_POINT_SELECTED:
        m_zoomDisplay->SetPoint(newPoint);
        break;
    default:
        break;
    }
}

bool CPImageCtrl::emit(CPEvent & ev)
{
    if ( ProcessEvent( ev ) == FALSE ) {
        wxLogWarning( _("Could not process event!") );
        return false;
    } else {
        return true;
    }
}

void CPImageCtrl::setScale(double factor)
{
    if (factor == 0) {
        fitToWindow = true;
        factor = calcAutoScaleFactor(imageSize);
    } else {
        fitToWindow = false;
    }
    DEBUG_DEBUG("new scale factor:" << factor);
    // update if factor changed
    if (factor != scaleFactor) {
        scaleFactor = factor;
        rescaleImage();
    }
}

double CPImageCtrl::calcAutoScaleFactor(wxSize size)
{
//    wxSize csize = GetClientSize();
    wxSize csize = GetSize();
    double s1 = (double)csize.GetWidth()/size.GetWidth();
    double s2 = (double)csize.GetHeight()/size.GetHeight();
    return s1 < s2 ? s1 : s2;
}

double CPImageCtrl::getScaleFactor() const
{
    return scaleFactor;
}

void CPImageCtrl::OnSize(wxSizeEvent &e)
{
    DEBUG_TRACE("");
    // rescale bitmap if needed.
    // on the fly rescaling in OnDraw is terribly slow. I wonder
    // how anybody can use it..
    if (imageFilename != "") {
        if (fitToWindow) {
            setScale(0);
        }
    }
}

void CPImageCtrl::OnKey(wxKeyEvent & e)
{
    wxPoint delta(0,0);
    switch (e.m_keyCode) {
    case WXK_LEFT:
        delta.x = -1;
        break;
    case WXK_UP:
        delta.y = -1;
        break;
    case WXK_RIGHT:
        delta.x = 1;
        break;
    case WXK_DOWN:
        delta.y = 1;
        break;
    default:
        break;
    }
    if (delta.x != 0 || delta.y != 0) {
        // move to the left
        double speed = (double) GetClientSize().GetWidth()/10;
        delta.x = (int) (delta.x * speed);
        delta.y = (int) (delta.y * speed);
        ScrollDelta(delta);
        if (e.ShiftDown()) {
            // emit scroll event, so that other window can be scrolled
            // as well.
            CPEvent e(this, CPEvent::SCROLLED, FDiff2D(delta.x, delta.y));
            emit(e);
        }
    } else if (e.m_keyCode == 'a') {
        DEBUG_DEBUG("adding point with a key, faking right click");
        // faking right mouse button with "a"
        // set right up event
        CPEvent ev(this, CPEvent::RIGHT_CLICK, FDiff2D(0,0));
        emit(ev);
    } else {
        DEBUG_DEBUG("forwarding key " << e.m_keyCode
                    << " origin: id:" << e.m_id << " obj: "
                    << e.GetEventObject());
        // forward all keys to our parent
        //GetParent()->GetEventHandler()->ProcessEvent(e);
        m_editPanel->GetEventHandler()->ProcessEvent(e);
    }
}

void CPImageCtrl::OnKeyUp(wxKeyEvent & e)
{
    DEBUG_TRACE("key:" << e.m_keyCode);
    e.Skip();
}

void CPImageCtrl::OnKeyDown(wxKeyEvent & e)
{
#if 0
    DEBUG_TRACE("key:" << e.m_keyCode);
    if (e.m_keyCode == WXK_SHIFT) {
        DEBUG_DEBUG("shift down");
        double scale = getScale();
        if ((scale != 1) && (!m_tempZoom)) {
            wxPoint mpos;
            CalcUnscrolledPosition(e.m_x, e.m_y,
                                   &mpos.x, & mpos.y);
            mpos = invScale(mpos);
            m_tempZoom = true;
            m_savedScale = scale;
            DEBUG_DEBUG("zoom into");
            setScale(1);
            showPosition(mpos.x, mpos.y);
        }
    } else {
        e.Skip();
    }
#endif
    if (e.m_keyCode == WXK_SHIFT || e.m_keyCode == WXK_CONTROL) {
        DEBUG_DEBUG("shift or control down, reseting scoll position");
        m_mouseScrollPos = e.GetPosition();
    }

    e.Skip();
}

void CPImageCtrl::OnMouseLeave(wxMouseEvent & e)
{
#if 0
    DEBUG_TRACE("");
    if (m_tempZoom) {
        setScale(m_savedScale);
        m_tempZoom = false;
    }
#endif
    m_mousePos = FDiff2D(-1,-1);
//    SetCursor(wxCursor(wxCURSOR_BULLSEYE));
}

void CPImageCtrl::OnMouseEnter(wxMouseEvent & e)
{
    DEBUG_TRACE("");
    SetFocus();
}

FDiff2D CPImageCtrl::getNewPoint()
{
    // only possible if a new point is actually selected
    DEBUG_ASSERT(editState == NEW_POINT_SELECTED);
    return newPoint;
}

void CPImageCtrl::setNewPoint(const FDiff2D & p)
{
    DEBUG_DEBUG("setting new point " << p.x << "," << p.y);
    // should we need to check for some precondition?
    newPoint = p;
    editState = NEW_POINT_SELECTED;

    // show new point.
    showPosition(p);

    // we do not send an event, since CPEditorPanel
    // caused the change.. so it doesn't need to filter
    // out its own change messages.
}

void CPImageCtrl::showSearchArea(bool show)
{
    m_showSearchArea = show;
    if (show)
    {
        int templSearchAreaPercent = wxConfigBase::Get()->Read(wxT("/Finetune/SearchAreaPercent"), HUGIN_FT_SEARCH_AREA_PERCENT);
        wxImage * img = ImageCache::getInstance().getImage(imageFilename);

        m_searchRectWidth = (img->GetWidth() * templSearchAreaPercent) / 200;
        DEBUG_DEBUG("Setting new search area: w in %:" << templSearchAreaPercent << " bitmap width: " << bitmap.GetWidth() << "  resulting size: " << m_searchRectWidth);
        m_mousePos = FDiff2D(-1,-1);
    }
}

void CPImageCtrl::showTemplateArea(bool show)
{
    m_showTemplateArea = show;
    if (show)
    {
        m_templateRectWidth = wxConfigBase::Get()->Read(wxT("/Finetune/TemplateSize"),HUGIN_FT_TEMPLATE_SIZE) / 2;
    }
}

void CPImageCtrl::ScrollDelta(const wxPoint & delta)
{
    int x,y;
    GetViewStart( &x, &y );
    x = x + delta.x;
    y = y + delta.y;
    if (x<0) x = 0;
    if (y<0) y = 0;
    Scroll( x, y);
}

