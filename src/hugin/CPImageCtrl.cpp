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

#include "common/utils.h"
#include "hugin/CPImageCtrl.h"

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

wxEvent * CPEvent::Clone() const
{
    return new CPEvent(*this);
}


// our image control

IMPLEMENT_DYNAMIC_CLASS(CPImageCtrl, wxScrolledWindow);

BEGIN_EVENT_TABLE(CPImageCtrl, wxScrolledWindow)
//    EVT_PAINT(CPImageCtrl::OnPaint)
    EVT_LEFT_DOWN(CPImageCtrl::mousePressEvent)
    EVT_MOTION(CPImageCtrl::mouseMoveEvent)
    EVT_LEFT_UP(CPImageCtrl::mouseReleaseEvent)
END_EVENT_TABLE()

CPImageCtrl::CPImageCtrl(wxWindow* parent, wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& name)
    : wxScrolledWindow(parent, id, pos, size, style, name),
      bitmap(0), drawNewPoint(false), scaleFactor(1)
{
    SetCursor(wxCursor(wxCURSOR_MAGNIFIER));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("BLUE")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("GREEN")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("CYAN")));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour("MAGENTA")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("YELLOW")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("ORANGE")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("NAVY")));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour("FIREBRICK")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("SIENNA")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("DARK TURQUOISE")));
//    pointColors.push_back(*(wxTheColourDatabase->FindColour("SALMON")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("MAROON")));
    pointColors.push_back(*(wxTheColourDatabase->FindColour("KHAKI")));
}

CPImageCtrl::~CPImageCtrl()
{
    delete bitmap;
}


void CPImageCtrl::OnDraw(wxDC & dc)
{
    if (scaleFactor != 1.0) {
        dc.SetUserScale(scaleFactor, scaleFactor);
    }
    
    // draw image (FIXME, redraw only visible regions.)
    if (bitmap) {
        dc.DrawBitmap(*bitmap,0,0);
    }
    dc.SetUserScale(1.0,1.0);

    // draw known points.
    int i=0;
    vector<wxPoint>::const_iterator it;
    for (it = points.begin(); it != points.end(); ++it) {
        drawPoint(dc,*it,pointColors[i%pointColors.size()]);
        i++;
    }

    if (drawNewPoint) {
        drawPoint(dc, newPoint, *wxTheColourDatabase->FindColour("RED"));
    }

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
    case KNOWN_POINT_SELECTED:
    case NO_SELECTION:
        break;
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
    dc.DrawCircle(scale(point), 4);
}

wxSize CPImageCtrl::DoGetBestSize() const
{
    if (bitmap) {
        return wxSize(bitmap->GetWidth(),bitmap->GetHeight());
    } else {
        return wxSize(0,0);
    }
}


void CPImageCtrl::setImage(const wxImage & img)
{
    DEBUG_TRACE("setting Image image w:" << img.GetWidth()
                << "h: " << img.GetHeight());
    delete bitmap;
    bitmap = new wxBitmap(img);
    // FIXME update size & relayout dialog
    SetSizeHints(-1,-1,img.GetWidth(), img.GetHeight(),1,1);
    //SetVirtualSizeHints(-1,-1,img.GetWidth(), img.GetHeight());
//    SetVirtualSizeHints(bitmap->GetWidth(),bitmap->GetHeight(),bitmap->GetWidth(), bitmap->GetHeight());
    SetScrollbars(16, 16, bitmap->GetWidth()/16 ,bitmap->GetHeight()/16 );
    // redraw
    Refresh();
}


void CPImageCtrl::setCtrlPoints(const std::vector<wxPoint> & cps)
{
    points = cps;
    // update view
    update();
}



void CPImageCtrl::clearNewPoint()
{
    drawNewPoint = false;
}


void CPImageCtrl::selectPoint(unsigned int nr)
{
    selectedPointNr = nr;
}


CPImageCtrl::EditorState CPImageCtrl::isOccupied(const wxPoint &p, unsigned int & pointNr) const
{
    // check if mouse is over a known point
    vector<wxPoint>::const_iterator it;
    for (it = points.begin(); it != points.end(); ++it) {
        if (p.x < it->x + 4 &&
            p.x > it->x - 4 &&
            p.y < it->y + 4 &&
            p.y > it->y - 4
            )
        {
            pointNr = it - points.begin();
            return KNOWN_POINT_SELECTED;
            break;
        }
    }

    if (drawNewPoint &&
        p.x < newPoint.x + 4 &&
        p.x > newPoint.x - 4 &&
        p.y < newPoint.y + 4 &&
        p.y > newPoint.y - 4)
    {
        return NEW_POINT_SELECTED;
    } else {
        return SELECT_REGION;
    }
}


void CPImageCtrl::mouseMoveEvent(wxMouseEvent *mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse->GetPosition().x, mouse->GetPosition().y,
                           &mpos.x, & mpos.y);
    mpos = invScale(mpos);
    if (mouse->LeftIsDown()) {
        switch(editState) {
        case NO_SELECTION:
            DEBUG_FATAL("mouse movement without selection!");
            assert(0);
            break;
        case KNOWN_POINT_SELECTED:
            if (mpos.x >= 0 && mpos.x <= bitmap->GetWidth()){
                points[selectedPointNr].x = mpos.x;
            } else if (mpos.x < 0) {
                points[selectedPointNr].x = 0;
            } else if (mpos.x > bitmap->GetWidth()) {
                points[selectedPointNr].x = bitmap->GetWidth();
            }

            if (mpos.y >= 0 && mpos.y <= bitmap->GetHeight()){
                points[selectedPointNr].y = mpos.y;
            } else if (mpos.y < 0) {
                points[selectedPointNr].y = 0;
            } else if (mpos.y > bitmap->GetHeight()) {
                points[selectedPointNr].y = bitmap->GetHeight();
            }
            // emit a notify event here.
            //
            //emit(pointMoved(selectedPointNr, points[selectedPointNr]));
            // do more intelligent updating here?
            update();
            break;
        case NEW_POINT_SELECTED:
            newPoint = mpos;
            //emit(newPointMoved(newPoint));
            update();
            break;
        case SELECT_REGION:
            region.SetWidth(mpos.x - region.x);
            region.SetHeight(mpos.y - region.y);
            // do more intelligent updating here?
            drawNewPoint = false;
            update();
        }
//        DEBUG_DEBUG("ImageDisplay: mouse move, state change: " << oldstate
//                    << " -> " << editState);
    }
}


void CPImageCtrl::mousePressEvent(wxMouseEvent *mouse)
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
    if (mouse->LeftDown()) {
        // we can always select a new point
        if (clickState == KNOWN_POINT_SELECTED) {
            if (selectedPointNr != selPointNr) {
                selectedPointNr = selPointNr;
                point = points[selectedPointNr];
                editState = clickState;
                // FIXME emit(pointSelected(selectedPointNr));
            }
        } else if (clickState == NEW_POINT_SELECTED) {
            drawNewPoint = true;
            newPoint = mpos;
            editState = NEW_POINT_SELECTED;
        } else if (clickState == SELECT_REGION) {
            editState = SELECT_REGION;
            region.x = mpos.x;
            region.y = mpos.y;
        }
        DEBUG_DEBUG("ImageDisplay: mouse down, state change: " << oldstate
                    << " -> " << editState);
    }
}


void CPImageCtrl::mouseReleaseEvent(wxMouseEvent *mouse)
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
            assert(0);
            break;
        case KNOWN_POINT_SELECTED:
            if (point != points[selectedPointNr]) {
                CPEvent e( this, selectedPointNr, points[selectedPointNr]);
                emit(e);
                //emit(pointChanged(selectedPointNr, points[selectedPointNr]));
            }
            break;
        case NEW_POINT_SELECTED:
        {
            assert(drawNewPoint);
            DEBUG_DEBUG("new Point changed (event fire): x:" << mpos.x << " y:" << mpos.y);
            // fire the wxWin event
            CPEvent e( this, newPoint);
            emit(e);
            //emit(newPointChanged(newPoint));
            break;
        }
        case SELECT_REGION:
            if (region.GetPosition() == mpos) {
                // create a new point.
                drawNewPoint = true;
                editState = KNOWN_POINT_SELECTED;
                newPoint = mpos;
                DEBUG_DEBUG("new Point changed: x:" << mpos.x << " y:" << mpos.y);
                //emit(newPointChanged(newPoint));
                CPEvent e(this, newPoint);
                emit(e);
                update();
            } else {
                editState = NO_SELECTION;
                DEBUG_DEBUG("new Region selected " << region.GetTop() << "," << region.GetLeft() << " " << region.GetRight() << "," << region.GetBottom());
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
        }
        DEBUG_DEBUG("ImageDisplay: mouse release, state change: " << oldState
                    << " -> " << editState);
    }
}

void CPImageCtrl::update()
{
    Refresh(FALSE);
//    DEBUG_DEBUG("redraw display");
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

