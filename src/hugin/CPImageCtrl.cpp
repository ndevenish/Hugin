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


#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "wx/image.h"
#include "wx/config.h"

#include "common/utils.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"

using namespace std;

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

CPEvent::CPEvent(wxWindow* win, wxPoint & p)
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

CPEvent::CPEvent(wxWindow* win, unsigned int cpNr, const wxPoint & p)
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

CPEvent::CPEvent(wxWindow* win, CPEventMode evt_mode, const wxPoint & p)
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

IMPLEMENT_DYNAMIC_CLASS(CPImageCtrl, wxScrolledWindow);

BEGIN_EVENT_TABLE(CPImageCtrl, wxScrolledWindow)
//    EVT_PAINT(CPImageCtrl::OnPaint)
    EVT_LEFT_DOWN(CPImageCtrl::mousePressLMBEvent)
    EVT_MOTION(CPImageCtrl::mouseMoveEvent)
    EVT_LEFT_UP(CPImageCtrl::mouseReleaseLMBEvent)
    EVT_RIGHT_UP(CPImageCtrl::mouseReleaseRMBEvent)
    EVT_MIDDLE_DOWN(CPImageCtrl::mousePressMMBEvent)
    EVT_MIDDLE_UP(CPImageCtrl::mouseReleaseMMBEvent)
    EVT_SIZE(CPImageCtrl::OnSize)
    EVT_CHAR(CPImageCtrl::OnKey)
    EVT_KEY_UP(CPImageCtrl::OnKeyUp)
    EVT_KEY_DOWN(CPImageCtrl::OnKeyDown)
    EVT_LEAVE_WINDOW(CPImageCtrl::OnMouseLeave)
    EVT_ENTER_WINDOW(CPImageCtrl::OnMouseEnter)
END_EVENT_TABLE()

CPImageCtrl::CPImageCtrl(CPEditorPanel* parent, wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& name)
    : wxScrolledWindow(parent, id, pos, size, style, name),
      editState(NO_IMAGE),
      scaleFactor(1), fitToWindow(false),
      m_showSearchArea(false), m_searchRectWidth(0),
      m_showTemplateArea(false), m_templateRectWidth(0),
      m_tempZoom(false),m_savedScale(1), m_editPanel(parent)
{
    SetCursor(wxCursor(wxCURSOR_BULLSEYE));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("BLUE")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("GREEN")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("CYAN")));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour("MAGENTA")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("GOLD")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("ORANGE")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("NAVY")));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour("FIREBRICK")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("SIENNA")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("DARK TURQUOISE")));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour("SALMON")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("MAROON")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("KHAKI")));

    m_searchRectWidth = 120;
}

CPImageCtrl::~CPImageCtrl()
{
    DEBUG_TRACE("dtor");
    DEBUG_TRACE("dtor end");
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
    vector<wxPoint>::const_iterator it;
    for (it = points.begin(); it != points.end(); ++it) {
        if (i==selectedPointNr) {
            if (editState != NEW_POINT_SELECTED) {
                drawPoint(dc,*it,*wxTheColourDatabase->FindColour("RED"));
            } else {
                drawPoint(dc,*it,*wxTheColourDatabase->FindColour("YELLOW"));
            }
        } else {
            drawPoint(dc,*it,pointColors[i%pointColors.size()]);
        }
        i++;
    }
/*
    if (drawNewPoint) {
        drawPoint(dc, newPoint, *wxTheColourDatabase->FindColour("RED"));
    }
*/
    switch(editState) {
    case SELECT_REGION:
        dc.SetLogicalFunction(wxINVERT);
        dc.SetPen(wxPen("WHITE", 1, wxSOLID));
        dc.SetBrush(wxBrush("WHITE",wxTRANSPARENT));
        dc.DrawRectangle(scale(region.GetLeft()),
                    scale(region.GetTop()),
                    scale(region.GetWidth()),
                    scale(region.GetHeight()));
        break;
    case NEW_POINT_SELECTED:
        drawPoint(dc, newPoint, *wxTheColourDatabase->FindColour("RED"));
        if (m_showTemplateArea) {
            dc.SetLogicalFunction(wxINVERT);
            dc.SetPen(wxPen("RED", 1, wxSOLID));
            dc.SetBrush(wxBrush("WHITE",wxTRANSPARENT));
            wxPoint upperLeft = scale(newPoint - wxPoint(m_templateRectWidth, m_templateRectWidth));
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
        dc.SetPen(wxPen("WHITE", 1, wxSOLID));
        dc.SetBrush(wxBrush("WHITE",wxTRANSPARENT));

        wxPoint upperLeft = scale(m_mousePos - wxPoint(m_searchRectWidth, m_searchRectWidth));
        int width = scale(m_searchRectWidth);
        DEBUG_DEBUG("drawing rect with width " << 2*width << " orig: " << m_searchRectWidth*2  << " scale factor: " << getScaleFactor());

        dc.DrawRectangle(upperLeft.x, upperLeft.y, 2*width, 2*width);
        dc.SetLogicalFunction(wxCOPY);
    }


}


void CPImageCtrl::drawPoint(wxDC & dc, const wxPoint & point, const wxColor & color) const
{
    dc.SetBrush(wxBrush("WHITE",wxTRANSPARENT));
    dc.SetPen(wxPen(color, 3, wxSOLID));
    dc.DrawCircle(scale(point), 6);
    dc.SetPen(wxPen("BLACK", 1, wxSOLID));
    dc.DrawCircle(scale(point), 7);
    dc.SetPen(wxPen("WHITE", 1, wxSOLID));
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
        bitmap = img->ConvertToBitmap();
    } else {
        imageSize.SetWidth( scale(imageSize.GetWidth()) );
        imageSize.SetHeight( scale(imageSize.GetHeight()) );
        DEBUG_DEBUG("rescaling to " << imageSize.GetWidth() << "x"
                    << imageSize.GetHeight() );
        bitmap = img->Scale(imageSize.GetWidth(),
                            imageSize.GetHeight()).ConvertToBitmap();
        DEBUG_DEBUG("rescaling finished");
    }

    SetVirtualSize(imageSize.GetWidth(), imageSize.GetHeight());
    SetScrollRate(1,1);
//    SetSizeHints(-1,-1,imageSize.GetWidth(), imageSize.GetHeight(),1,1);
//    SetScrollbars(16,16,bitmap.GetWidth()/16, bitmap.GetHeight()/16);
}

void CPImageCtrl::setCtrlPoints(const std::vector<wxPoint> & cps)
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
    showPosition(points[nr].x, points[nr].y);
    update();
}

void CPImageCtrl::showPosition(int x, int y, bool warpPointer)
{
    DEBUG_DEBUG("x: " << x  << " y: " << y);
    wxSize sz = GetClientSize();
    int scrollx = scale(x)- sz.GetWidth()/2;
//    if (x<0) x = 0;
    int scrolly = scale(y)- sz.GetHeight()/2;
//    if (y<0) x = 0;
//    Scroll(x/16, y/16);
    Scroll(scrollx, scrolly);
    if (warpPointer) {
        int sx,sy;
        GetViewStart(&sx, &sy);
        DEBUG_DEBUG("relative coordinages: " << scale(x)-sx << "," << scale(y)-sy);
        WarpPointer(scale(x)-sx,scale(y)-sy);
    }
}

CPImageCtrl::EditorState CPImageCtrl::isOccupied(const wxPoint &p, unsigned int & pointNr) const
{
    // check if mouse is over a known point
    vector<wxPoint>::const_iterator it;
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


void CPImageCtrl::mouseMoveEvent(wxMouseEvent *mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse->GetPosition().x, mouse->GetPosition().y,
                           &mpos.x, & mpos.y);
    bool doUpdate = false;
    mpos = invScale(mpos);
//    DEBUG_DEBUG(" pos:" << mpos.x << ", " << mpos.y);
    if (mouse->LeftIsDown()) {
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
            region.SetWidth(mpos.x - region.x);
            region.SetHeight(mpos.y - region.y);
            // do more intelligent updating here?
            doUpdate = true;
            break;
        case NO_IMAGE:
            break;
        }
    }
    if (mouse->MiddleIsDown() ) {  // scrolling with the mouse
      if (m_mouseScrollPos != mouse->GetPosition()) {
          wxPoint delta = mouse->GetPosition() - m_mouseScrollPos;
          double speed = (double)GetVirtualSize().GetHeight() / GetClientSize().GetHeight();
//          int speed = wxConfigBase::Get()->Read("/CPEditorPanel/scrollSpeed",5);
	  delta.x = (int) (delta.x * speed);
	  delta.y = (int) (delta.y * speed);
	  ScrollDelta(delta);
	  if (mouse->ShiftDown()) {
              // emit scroll event, so that other window can be scrolled
              // as well.
	      CPEvent e(this, CPEvent::SCROLLED, delta);
	      emit(e);
	  }
	  m_mouseScrollPos = mouse->GetPosition();
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


void CPImageCtrl::mousePressLMBEvent(wxMouseEvent *mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse->GetPosition().x, mouse->GetPosition().y,
                           &mpos.x, & mpos.y);
    mpos = invScale(mpos);
    DEBUG_DEBUG("mousePressEvent, pos:" << mpos.x
                << ", " << mpos.y);
    unsigned int selPointNr = 0;
    EditorState oldstate = editState;
    EditorState clickState = isOccupied(mpos, selPointNr);
    if (mouse->LeftDown() && editState != NO_IMAGE) {
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
            region.x = mpos.x;
            region.y = mpos.y;
        } else {
            DEBUG_ERROR("invalid state " << clickState << " on mouse down");
        }
        DEBUG_DEBUG("ImageDisplay: mouse down, state change: " << oldstate
                    << " -> " << editState);
    }
    m_mousePos = mpos;
}

void CPImageCtrl::mouseReleaseLMBEvent(wxMouseEvent *mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse->GetPosition().x, mouse->GetPosition().y,
                           &mpos.x, & mpos.y);
    mpos = invScale(mpos);
    DEBUG_DEBUG("mouseReleaseEvent, pos:" << mpos.x
                << ", " << mpos.y);
    EditorState oldState = editState;
    if (mouse->LeftUp()) {
        switch(editState) {
        case NO_SELECTION:
            DEBUG_WARN("mouse release without selection");
            break;
        case KNOWN_POINT_SELECTED:
        {
            DEBUG_DEBUG("mouse release with known point " << selectedPointNr);
            if (point != points[selectedPointNr]) {
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
            if (region.GetPosition() == mpos) {
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
        DEBUG_DEBUG("ImageDisplay: mouse release, state change: " << oldState
                    << " -> " << editState);
    }

}


void CPImageCtrl::mouseReleaseMMBEvent(wxMouseEvent *mouse)
{
    DEBUG_DEBUG("middle mouse button released, leaving scroll mode")
    SetCursor(wxCursor(wxCURSOR_BULLSEYE));
}


void CPImageCtrl::mousePressMMBEvent(wxMouseEvent *mouse)
{
    DEBUG_DEBUG("middle mouse button pressed, entering scroll mode")
    m_mouseScrollPos = mouse->GetPosition();
    SetCursor(wxCursor(wxCURSOR_HAND));
}


void CPImageCtrl::mouseReleaseRMBEvent(wxMouseEvent *mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse->GetPosition().x, mouse->GetPosition().y,
                           &mpos.x, & mpos.y);
    mpos = invScale(mpos);
    DEBUG_DEBUG("mouseReleaseEvent, pos:" << mpos.x
                << ", " << mpos.y);

    if (mouse->RightUp()) {
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
}

bool CPImageCtrl::emit(CPEvent & ev)
{
    if ( ProcessEvent( ev ) == FALSE ) {
        wxLogWarning( "Could not process event!" );
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
    DEBUG_DEBUG("forwarding key " << e.m_keyCode
                << " origin: id:" << e.m_id << " obj: "
                << e.GetEventObject());
    // forward all keys to our parent
    //GetParent()->GetEventHandler()->ProcessEvent(e);
    m_editPanel->GetEventHandler()->ProcessEvent(e);
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
    m_mousePos = wxPoint(-1,-1);
    SetCursor(wxCursor(wxCURSOR_BULLSEYE));
}

void CPImageCtrl::OnMouseEnter(wxMouseEvent & e)
{
    DEBUG_TRACE("");
    SetFocus();
}

wxPoint CPImageCtrl::getNewPoint()
{
    // only possible if a new point is actually selected
    DEBUG_ASSERT(editState == NEW_POINT_SELECTED);
    return newPoint;
}

void CPImageCtrl::setNewPoint(const wxPoint & p)
{
    DEBUG_DEBUG("setting new point " << p.x << "," << p.y);
    // should we need to check for some precondition?
    newPoint = p;
    editState = NEW_POINT_SELECTED;

    // show new point.
    showPosition(p.x, p.y);

    // we do not send an event, since CPEditorPanel
    // caused the change.. so it doesn't need to filter
    // out its own change messages.
}

void CPImageCtrl::showSearchArea(bool show)
{
    m_showSearchArea = show;
    if (show)
    {
        int templSearchAreaPercent = wxConfigBase::Get()->Read("/CPEditorPanel/templateSearchAreaPercent",10);
        wxImage * img = ImageCache::getInstance().getImage(imageFilename);

        m_searchRectWidth = (img->GetWidth() * templSearchAreaPercent) / 200;
        DEBUG_DEBUG("Setting new search area: w in %:" << templSearchAreaPercent << " bitmap width: " << bitmap.GetWidth() << "  resulting size: " << m_searchRectWidth);
        m_mousePos = wxPoint(-1,-1);
    }
}

void CPImageCtrl::showTemplateArea(bool show)
{
    m_showTemplateArea = show;
    if (show)
    {
        m_templateRectWidth = wxConfigBase::Get()->Read("/CPEditorPanel/templateSize",14l) / 2;
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


