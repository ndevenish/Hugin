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
#include "common/utils.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/ImageCache.h"

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
    EVT_SIZE(CPImageCtrl::OnSize)
END_EVENT_TABLE()

CPImageCtrl::CPImageCtrl(wxWindow* parent, wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& name)
    : wxScrolledWindow(parent, id, pos, size, style, name),
      scaleFactor(1), fitToWindow(false),
      m_showSearchArea(false), m_searchRectWidth(10)
{
    SetCursor(wxCursor(wxCURSOR_BULLSEYE));
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

    m_drawSearchRect = false;
    m_searchRectWidth = 120;
}

CPImageCtrl::~CPImageCtrl()
{
    DEBUG_TRACE("dtor");
    DEBUG_TRACE("dtor end");
}


void CPImageCtrl::OnDraw(wxDC & dc)
{
    // draw image (FIXME, redraw only visible regions.)
    if (editState != NO_IMAGE) {
        dc.DrawBitmap(bitmap,0,0);
    }

    // draw known points.
    unsigned int i=0;
    vector<wxPoint>::const_iterator it;
    for (it = points.begin(); it != points.end(); ++it) {
        if (i==selectedPointNr) {
            drawPoint(dc,*it,*wxTheColourDatabase->FindColour("RED"));
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
        break;
    case KNOWN_POINT_SELECTED:
        break;
    case NO_SELECTION:
    case NO_IMAGE:
        break;
    }

    if (m_drawSearchRect){
        dc.SetLogicalFunction(wxINVERT);
        dc.SetPen(wxPen("WHITE", 1, wxSOLID));
        dc.SetBrush(wxBrush("WHITE",wxTRANSPARENT));

//        DEBUG_DEBUG("drawing rect with width " << 2*width);
        wxPoint upperLeft = scale(m_mousePos - wxPoint(m_searchRectWidth, m_searchRectWidth));
        int width = scale(m_searchRectWidth);

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
        rescaleImage();
        editState = NO_SELECTION;
    } else {
        editState = NO_IMAGE;
        bitmap = wxBitmap();
        SetSizeHints(0,0,0,0,1,1);
    }
    // redraw
    update();
}



void CPImageCtrl::rescaleImage()
{
    // rescale image
    wxImage * img = ImageCache::getInstance().getImage(imageFilename);
    imageSize = wxSize(img->GetWidth(), img->GetHeight());
    if (fitToWindow) {
        scaleFactor = calcAutoScaleFactor(imageSize);
    }
    DEBUG_DEBUG("src image size "
                << imageSize.GetHeight() << "x" << imageSize.GetWidth());
    if (getScaleFactor() == 1.0) {
        bitmap = img->ConvertToBitmap();
    } else {
        DEBUG_DEBUG("rescaling to " << scale(imageSize.GetWidth()) << "x"
                    << scale(imageSize.GetHeight()) );
        bitmap = img->Scale(scale(imageSize.GetWidth()),
                            scale(imageSize.GetHeight())).ConvertToBitmap();
        DEBUG_DEBUG("rescaling finished");
    }
    SetSizeHints(-1,-1,imageSize.GetWidth(), imageSize.GetHeight(),1,1);
    SetScrollbars(16,16,bitmap.GetWidth()/16, bitmap.GetHeight()/16);
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
    editState = NO_SELECTION;
    update();
}


void CPImageCtrl::selectPoint(unsigned int nr)
{
    assert(nr < points.size());
    selectedPointNr = nr;
    showPosition(points[nr].x, points[nr].y);
    update();
}

void CPImageCtrl::showPosition(int x, int y)
{
    wxSize sz = GetClientSize();
    x = scale(x)- sz.GetWidth()/2;
//    if (x<0) x = 0;
    y = scale(y)- sz.GetHeight()/2;
//    if (y<0) x = 0;
    Scroll(x/16, y/16);
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
    if (mouse->LeftIsDown()) {
        switch(editState) {
        case NO_SELECTION:
            DEBUG_ERROR("mouse down movement without selection, in NO_SELECTION state!");
            break;
        case KNOWN_POINT_SELECTED:
            if (mpos.x >= 0 && mpos.x <= imageSize.GetWidth()){
                points[selectedPointNr].x = mpos.x;
            } else if (mpos.x < 0) {
                points[selectedPointNr].x = 0;
            } else if (mpos.x > imageSize.GetWidth()) {
                points[selectedPointNr].x = imageSize.GetWidth();
            }

            if (mpos.y >= 0 && mpos.y <= imageSize.GetHeight()){
                points[selectedPointNr].y = mpos.y;
            } else if (mpos.y < 0) {
                points[selectedPointNr].y = 0;
            } else if (mpos.y > imageSize.GetHeight()) {
                points[selectedPointNr].y = imageSize.GetHeight();
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
      int x,y;
      wxSize vs;
      GetVirtualSize( &vs.x, &vs.y );
      wxSize sz = GetClientSize();
      x = (int)((double)mouse->GetPosition().x/16.0/(double)sz.GetWidth()
          * (double)vs.x
          - (double)sz.x/32.0);
      if (x<0) x = 0;
      y = (int)((double)mouse->GetPosition().y/16.0/(double)sz.GetHeight()
          * (double)vs.y
          - (double)sz.y/32.0);
      if (y<0) x = 0;
      Scroll( x, y);
    }

//    DEBUG_DEBUG("ImageDisplay: mouse move, state: " << editState);

    // draw a rectangle

    if (editState == NO_SELECTION && m_showSearchArea)
    {
        DEBUG_DEBUG("enabled search area")
        doUpdate = true;
        m_drawSearchRect = true;
    } else {
        if (m_drawSearchRect) {
            DEBUG_DEBUG("disable search area display")
            m_drawSearchRect = false;
            doUpdate = true;
        }
    }

    m_mousePos = mpos;
    // repaint
    if (doUpdate) {
        update();
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
            DEBUG_DEBUG("click on point: " << selPointNr);
            selectedPointNr = selPointNr;
            point = points[selectedPointNr];
            editState = clickState;
            if (selectedPointNr != selPointNr) {
                CPEvent e( this, selectedPointNr);
                emit(e);
            }
        } else if (clickState == NEW_POINT_SELECTED) {
            DEBUG_DEBUG("click on new space, select region/new point");
            editState = SELECT_REGION;
            newPoint = mpos;
            region.x = mpos.x;
            region.y = mpos.y;
        } else {
            DEBUG_ERROR("invalid state " << clickState << " on mouse down");
        }
        DEBUG_DEBUG("ImageDisplay: mouse down, state change: " << oldstate
                    << " -> " << editState);
    }
    if (mouse->MiddleIsDown() ) {  // scrolling with the mouse
        int x,y;
        wxSize vs;
        GetVirtualSize( &vs.x, &vs.y );
        wxSize sz = GetClientSize();
        x = (int)((double)mouse->GetPosition().x/16.0/(double)sz.GetWidth()
                  * (double)vs.x
                  - (double)sz.x/32.0);
        if (x<0) x = 0;
        y = (int)((double)mouse->GetPosition().y/16.0/(double)sz.GetHeight()
                  * (double)vs.y
                  - (double)sz.y/32.0);
        if (y<0) x = 0;
        Scroll( x, y);
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
            DEBUG_WARN("mouse release without selection");
            break;
        case KNOWN_POINT_SELECTED:
            DEBUG_DEBUG("mouse release with known point " << selectedPointNr);
            if (point != points[selectedPointNr]) {
                CPEvent e( this, selectedPointNr, points[selectedPointNr]);
                emit(e);
                //emit(pointChanged(selectedPointNr, points[selectedPointNr]));
            }
            break;
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
        case NO_IMAGE:
            break;

        }
        DEBUG_DEBUG("ImageDisplay: mouse release, state change: " << oldState
                    << " -> " << editState);
    }
}

void CPImageCtrl::update()
{
    DEBUG_TRACE("");
    Refresh(FALSE);
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
        update();
    }
}

double CPImageCtrl::calcAutoScaleFactor(wxSize size)
{
    wxSize csize = GetClientSize();
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
