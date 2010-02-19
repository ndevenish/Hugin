// -*- c-basic-offset: 4 -*-

/** @file MaskImageCtrl.cpp
 *
 *  @brief implementation of MaskImageCtrl Class
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */

/*  This program is free software; you can redistribute it and/or
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

#include "panoinc_WX.h"
#include "panoinc.h"
#include "base_wx/platform.h"
#include "hugin/config_defaults.h"
#include "hugin/MaskImageCtrl.h"
#include "hugin/MaskEditorPanel.h"
#include "base_wx/ImageCache.h"

using namespace hugin_utils;

/** half size of markers */
const int polygonPointSize=3;
/** maximal distance for selection of one point */
const int maxSelectionDistance=20;

// our image control

BEGIN_EVENT_TABLE(MaskImageCtrl, wxScrolledWindow)
    EVT_SIZE(MaskImageCtrl::OnSize)
    EVT_MOTION(MaskImageCtrl::mouseMoveEvent)
    EVT_LEFT_DOWN(MaskImageCtrl::mousePressLMBEvent)
    EVT_LEFT_UP(MaskImageCtrl::mouseReleaseLMBEvent)
    EVT_LEFT_DCLICK(MaskImageCtrl::mouseDblClickLeftEvent)
    EVT_RIGHT_DOWN(MaskImageCtrl::mousePressRMBEvent)
    EVT_RIGHT_UP(MaskImageCtrl::mouseReleaseRMBEvent)
    EVT_KEY_UP(MaskImageCtrl::OnKeyUp)
    EVT_MOUSE_CAPTURE_LOST(MaskImageCtrl::OnCaptureLost)
    EVT_KILL_FOCUS(MaskImageCtrl::OnKillFocus)
END_EVENT_TABLE()

bool MaskImageCtrl::Create(wxWindow * parent, wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& name)
{
    wxScrolledWindow::Create(parent, id, pos, size, style, name);
    maskEditState = NO_IMAGE;
    scaleFactor = 1;
    fitToWindow = false;

    return true;
}

void MaskImageCtrl::Init(MaskEditorPanel * parent)
{
    m_editPanel = parent;
}

void MaskImageCtrl::setImage(const std::string & file, HuginBase::MaskPolygonVector newMask)
{
    DEBUG_TRACE("setting Image " << file);
    imageFilename = file;
    wxString fn(imageFilename.c_str(),HUGIN_CONV_FILENAME);
    if (wxFileName::FileExists(fn)) 
    {
        m_img = ImageCache::getInstance().getImage(imageFilename);
        maskEditState = NO_MASK;
        m_imageMask=newMask;
        setActiveMask(UINT_MAX);
        rescaleImage();
    } 
    else 
    {
        maskEditState = NO_IMAGE;
        bitmap = wxBitmap();
        // delete the image (release shared_ptr)
        // create an empty image.
        m_img = ImageCache::EntryPtr(new ImageCache::Entry);
        HuginBase::MaskPolygonVector mask;
        m_imageMask=mask;
        setActiveMask(UINT_MAX);
        Refresh(true);
    }
}

void MaskImageCtrl::setNewMasks(HuginBase::MaskPolygonVector newMasks)
{
    m_imageMask=newMasks;
    if(m_activeMask>=m_imageMask.size())
        setActiveMask(UINT_MAX);
    Refresh(false);
};

void MaskImageCtrl::setActiveMask(unsigned int newMask)
{
    if(m_activeMask!=newMask)
    {
        m_activeMask=newMask;
        m_selectedPoints.clear();
    };
    if(newMask<UINT_MAX)
    {
        if(m_selectedPoints.empty())
            maskEditState=NO_SELECTION;
        else
            maskEditState=POINTS_SELECTED;
        m_editingMask=m_imageMask[m_activeMask];
    }
    else
    {
        if(!imageFilename.empty())
            maskEditState=NO_MASK;
        HuginBase::MaskPolygon mask;
        m_editingMask=mask;
    };
    Refresh(true);
};

void MaskImageCtrl::selectAllMarkers()
{
    m_selectedPoints.clear();
    if(m_activeMask<UINT_MAX)
        fill_set(m_selectedPoints,0,m_imageMask[m_activeMask].getMaskPolygon().size()-1);
};

void MaskImageCtrl::mouseMoveEvent(wxMouseEvent& mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos.x, & mpos.y);
    FDiff2D currentPos=invtransform(mpos);
    bool doUpdate = false;
    switch(maskEditState)
    {
        case NEW_POLYGON_CREATING:
            doUpdate=true;
            m_editingMask.movePointTo(m_editingMask.getMaskPolygon().size()-1,currentPos);
            break;
        case POLYGON_SELECTING:
        case REGION_SELECTING:
        case POINTS_DELETING:
            DrawSelectionRectangle();
            m_currentPos=mpos;
            DrawSelectionRectangle();
            break;
        case POINTS_MOVING:
            doUpdate=true;
            m_editingMask=m_imageMask[m_activeMask];
            {
                FDiff2D delta=currentPos-invtransform(m_dragStartPos);
                for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                    m_editingMask.movePointBy(*it,delta);
            };
            break;
        case POINTS_ADDING:
            doUpdate=true;
            for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                m_editingMask.movePointTo(*it,currentPos);
            break;
    };
    if(doUpdate)
        update();
}

void MaskImageCtrl::mousePressLMBEvent(wxMouseEvent& mouse)
{
    DEBUG_DEBUG("LEFT MOUSE DOWN");
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &m_dragStartPos.x, & m_dragStartPos.y);
    FDiff2D currentPos=invtransform(m_dragStartPos);
    m_currentPos=m_dragStartPos;
    if(!HasCapture())
        CaptureMouse();
    SetFocus();
    switch(maskEditState)
    {
        case NEW_POLYGON_STARTED:
            //starting polygon creating
            m_editingMask.addPoint(currentPos);
            m_selectedPoints.insert(m_editingMask.getMaskPolygon().size()-1);
            break;
        case NO_MASK:
            maskEditState=POLYGON_SELECTING;
            DrawSelectionRectangle();
            break;
        case NO_SELECTION:
            if(mouse.ControlDown())
            {
                // check if mouse clicks happens near one line of active polygon
                unsigned int index=m_editingMask.FindPointNearPos(currentPos,5*maxSelectionDistance);
                if(index<UINT_MAX)
                {
                    m_selectedPoints.clear();
                    m_editingMask.insertPoint(index,currentPos);
                    m_selectedPoints.insert(index);
                    maskEditState=POINTS_ADDING;
                };
            }
            else
            {
                HuginBase::UIntSet points;
                if(SelectPointsInsideMouseRect(points,false))
                {
                    for(HuginBase::UIntSet::const_iterator it=points.begin();it!=points.end();it++)
                        m_selectedPoints.insert(*it);
                    maskEditState=POINTS_MOVING;
                }
                else
                {
                    maskEditState=REGION_SELECTING;
                    DrawSelectionRectangle();
                }
            };
            break;
        case POINTS_SELECTED:
            if(mouse.ControlDown())
            {
                // check if mouse clicks happens near one line of active polygon
                unsigned int index=m_editingMask.FindPointNearPos(currentPos, 5*maxSelectionDistance);
                if(index<UINT_MAX)
                {
                    m_selectedPoints.clear();
                    m_editingMask.insertPoint(index,currentPos);
                    m_selectedPoints.insert(index);
                    maskEditState=POINTS_ADDING;
                };
            }
            else
            {
                HuginBase::UIntSet points;
                if(SelectPointsInsideMouseRect(points,true))
                {
                    //selected point clicked, starting moving
                    maskEditState=POINTS_MOVING;
                }
                else
                {
                    //unselected point clicked
                    if(SelectPointsInsideMouseRect(points,false))
                    {
                        //clicked near other point
                        if(!mouse.ShiftDown())
                            m_selectedPoints.clear();
                        for(HuginBase::UIntSet::const_iterator it=points.begin();it!=points.end();it++)
                            m_selectedPoints.insert(*it);
                        maskEditState=POINTS_MOVING;
                    }
                    else
                    {
                        maskEditState=REGION_SELECTING;
                        DrawSelectionRectangle();
                    };
                }
            };
            break;
    };
}

void MaskImageCtrl::mouseReleaseLMBEvent(wxMouseEvent& mouse)
{
    DEBUG_DEBUG("LEFT MOUSE UP");
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos.x, & mpos.y);
    FDiff2D currentPos=invtransform(mpos);
    bool doUpdate=false;
    switch(maskEditState)
    {
        case NEW_POLYGON_STARTED:
            doUpdate=true;
            m_editingMask.addPoint(currentPos);
            m_selectedPoints.insert(m_editingMask.getMaskPolygon().size()-1);
            maskEditState=NEW_POLYGON_CREATING;
            break;
        case NEW_POLYGON_CREATING:
            //next point of polygen selected
            doUpdate=true;
            m_editingMask.addPoint(currentPos);
            m_selectedPoints.insert(m_editingMask.getMaskPolygon().size()-1);
            break;
        case POINTS_MOVING:
            if(HasCapture())
                ReleaseMouse();
            {
                FDiff2D delta=currentPos-invtransform(m_dragStartPos);
                if(sqr(delta.x)+sqr(delta.y)>sqr(maxSelectionDistance))
                {
                    for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                        m_imageMask[m_activeMask].movePointBy(*it,delta);
                    maskEditState=POINTS_SELECTED;
                    m_editPanel->UpdateMask();
                }
                else
                {
                    m_editingMask=m_imageMask[m_activeMask];
                    maskEditState=POINTS_SELECTED;
                    doUpdate=true;
                };
            };
            break;
        case POLYGON_SELECTING:
            if(HasCapture())
                ReleaseMouse();
            DrawSelectionRectangle();
            m_currentPos=mpos;
            maskEditState=NO_SELECTION;
            {
                hugin_utils::FDiff2D p;
                p.x=invtransform(m_dragStartPos.x+(m_currentPos.x-m_dragStartPos.x)/2);
                p.y=invtransform(m_dragStartPos.y+(m_currentPos.y-m_dragStartPos.y)/2);
                FindPolygon(p);
            };
            break;
        case REGION_SELECTING:
            {
                if(HasCapture())
                    ReleaseMouse();
                DrawSelectionRectangle();
                m_currentPos=mpos;
                bool selectedPoints=!m_selectedPoints.empty();
                if(!mouse.ShiftDown())
                    m_selectedPoints.clear();
                if(SelectPointsInsideMouseRect(m_selectedPoints,false))
                {
                    //new points selected
                    if(m_selectedPoints.empty())
                        maskEditState=NO_SELECTION;
                    else
                        maskEditState=POINTS_SELECTED;
                }
                else
                {
                    //there were no points selected
                    if(!selectedPoints)
                    {
                        //if there where no points selected before, we searching for an other polygon
                        hugin_utils::FDiff2D p;
                        p.x=invtransform(m_dragStartPos.x+(m_currentPos.x-m_dragStartPos.x)/2);
                        p.y=invtransform(m_dragStartPos.y+(m_currentPos.y-m_dragStartPos.y)/2);
                        FindPolygon(p);
                    };
                    maskEditState=NO_SELECTION;
                };
                doUpdate=true;
                break;
            };
        case POINTS_ADDING:
            if(HasCapture())
                ReleaseMouse();
            for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                m_editingMask.movePointTo(*it,currentPos);
            m_imageMask[m_activeMask]=m_editingMask;
            m_editPanel->UpdateMask();
            maskEditState=POINTS_SELECTED;
            break;
        default:
            if(HasCapture())
                ReleaseMouse();
    };
    if(doUpdate)
        update();
}

void MaskImageCtrl::mouseDblClickLeftEvent(wxMouseEvent &mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos.x, & mpos.y);
    FDiff2D currentPos=invtransform(mpos);
    switch(maskEditState)
    {
        case NEW_POLYGON_STARTED:
            {
                maskEditState=NO_SELECTION;
                HuginBase::MaskPolygon mask;
                m_editingMask=mask;
                m_selectedPoints.clear();
                break;
            };
        case NEW_POLYGON_CREATING:
            {
                //close newly generated polygon
                maskEditState=NO_SELECTION;
                //delete last point otherwise it would be added twice, because we added it 
                //already in release left mouse button
                m_editingMask.removePoint(m_editingMask.getMaskPolygon().size()-1);
                if(m_editingMask.getMaskPolygon().size()>2)
                {
                    m_imageMask.push_back(m_editingMask);
                    m_activeMask=m_imageMask.size()-1;
                    m_editPanel->AddMask();
                }
                else
                {
                    HuginBase::MaskPolygon mask;
                    m_editingMask=mask;
                    m_selectedPoints.clear();
                    update();
                };
                break;
            };
    };
};

void MaskImageCtrl::mousePressRMBEvent(wxMouseEvent& mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &m_dragStartPos.x, & m_dragStartPos.y);
    FDiff2D currentPos=invtransform(m_dragStartPos);
    m_currentPos=m_dragStartPos;
    if(!HasCapture())
        CaptureMouse();
    SetFocus();
    switch(maskEditState)
    {
        case NO_SELECTION:
        case POINTS_SELECTED:
            if(mouse.ControlDown())
            {
                maskEditState=POINTS_DELETING;
                DrawSelectionRectangle();
            }
            else
            {
                if (m_editingMask.isInside(currentPos))
                {
                    fill_set(m_selectedPoints,0,m_editingMask.getMaskPolygon().size()-1);
                    maskEditState=POINTS_MOVING;
                };
            };
            break;
    };
};

void MaskImageCtrl::mouseReleaseRMBEvent(wxMouseEvent& mouse)
{
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos.x, & mpos.y);
    FDiff2D currentPos=invtransform(mpos);
    if(HasCapture())
        ReleaseMouse();
    switch(maskEditState)
    {
        case NEW_POLYGON_STARTED:
            {
                maskEditState=NO_SELECTION;
                HuginBase::MaskPolygon mask;
                m_editingMask=mask;
                m_selectedPoints.clear();
                break;
            };
        case NEW_POLYGON_CREATING:
            {
                //close newly generated polygon
                maskEditState=NO_SELECTION;
                m_editingMask.movePointTo(m_editingMask.getMaskPolygon().size()-1,currentPos);
                if(m_editingMask.getMaskPolygon().size()>2)
                {
                    m_imageMask.push_back(m_editingMask);
                    m_activeMask=m_imageMask.size()-1;
                    m_editPanel->AddMask();
                }
                else
                {
                    HuginBase::MaskPolygon mask;
                    m_editingMask=mask;
                    m_selectedPoints.clear();
                    update();
                };
                break;
            };
        case POINTS_DELETING:
            {
                DrawSelectionRectangle();
                HuginBase::UIntSet points;
                m_currentPos=mpos;
                if(SelectPointsInsideMouseRect(points,false))
                {
                    if(m_editingMask.getMaskPolygon().size()-points.size()>2)
                    {
                        // clear all selected points
                        for(HuginBase::UIntSet::const_reverse_iterator it=points.rbegin();it!=points.rend();it++)
                            m_editingMask.removePoint(*it);
                        // now update set of selected points
                        if(m_selectedPoints.size()>0)
                        {
                            std::vector<unsigned int> mappedSelectedPoints(m_imageMask[m_activeMask].getMaskPolygon().size());
                            for(unsigned int i=0;i<mappedSelectedPoints.size();i++)
                                mappedSelectedPoints[i]=i;
                            HuginBase::UIntSet temp=m_selectedPoints;
                            m_selectedPoints.clear();
                            for(HuginBase::UIntSet::const_iterator it=points.begin();it!=points.end();it++)
                            {
                                if((*it)<mappedSelectedPoints.size()-1)
                                    for(unsigned int i=(*it)+1;i<mappedSelectedPoints.size();i++)
                                        mappedSelectedPoints[i]--;
                            };
                            for(HuginBase::UIntSet::const_iterator it=temp.begin();it!=temp.end();it++)
                                if(!set_contains(points,*it))
                                    m_selectedPoints.insert(mappedSelectedPoints[*it]);
                        };
                        //now update the saved mask
                        m_imageMask[m_activeMask]=m_editingMask;
                        m_editPanel->UpdateMask();
                    }
                    else
                        wxBell();
                };
                if(m_selectedPoints.size()==0)
                    maskEditState=NO_SELECTION;
                else
                    maskEditState=POINTS_SELECTED;
                break;
            };
        case POINTS_MOVING:
            {
                FDiff2D delta=currentPos-invtransform(m_dragStartPos);
                if(sqr(delta.x)+sqr(delta.y)>sqr(maxSelectionDistance))
                {
                    for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                        m_imageMask[m_activeMask].movePointBy(*it,delta);
                    maskEditState=POINTS_SELECTED;
                    m_editPanel->UpdateMask();
                }
                else
                {
                    m_editingMask=m_imageMask[m_activeMask];
                    maskEditState=POINTS_SELECTED;
                };
                break;
            };
    };
};

void MaskImageCtrl::OnKeyUp(wxKeyEvent &e)
{
    int key=e.GetKeyCode();
    bool processed=false;
    if((key==WXK_DELETE) || (key==WXK_NUMPAD_DELETE))
    {
        if(m_activeMask<UINT_MAX)
        {
            switch(maskEditState)
            {
                case POINTS_SELECTED:
                    if((m_selectedPoints.size()>0) && (m_editingMask.getMaskPolygon().size()-m_selectedPoints.size()>2))
                    {
                        for(HuginBase::UIntSet::const_reverse_iterator it=m_selectedPoints.rbegin();it!=m_selectedPoints.rend();it++)
                            m_editingMask.removePoint(*it);
                        m_imageMask[m_activeMask]=m_editingMask;
                        processed=true;
                        m_editPanel->UpdateMask();
                    }
                    else
                    {
                        if(m_editingMask.getMaskPolygon().size()==m_selectedPoints.size())
                        {
                            wxCommandEvent dummy;
                            processed=true;
                            m_editPanel->OnMaskDelete(dummy);
                        }
                        else
                            wxBell();
                    };
                    break;
                case NO_SELECTION:
                    {
                        wxCommandEvent dummy;
                        processed=true;
                        m_editPanel->OnMaskDelete(dummy);
                    };
                    break;
            };
        };
    };
    if(!processed)
        e.Skip();
};

void MaskImageCtrl::OnCaptureLost(wxMouseCaptureLostEvent &e)
{
    wxFocusEvent dummy;
    OnKillFocus(dummy);
};

void MaskImageCtrl::OnKillFocus(wxFocusEvent &e)
{
    if(HasCapture())
        ReleaseMouse();
    switch(maskEditState)
    {
        case NEW_POLYGON_CREATING:
        case NEW_POLYGON_STARTED:
            {
                wxBell();
                maskEditState=NO_SELECTION;
                HuginBase::MaskPolygon mask;
                m_editingMask=mask;
                m_selectedPoints.clear();
                update();
                break;
            };
    };
};

void MaskImageCtrl::startNewPolygon()
{
    maskEditState=NEW_POLYGON_STARTED; 
    HuginBase::MaskPolygon newMask;
    m_editingMask=newMask;
    m_selectedPoints.clear();
};

wxSize MaskImageCtrl::DoGetBestSize() const
{
    return wxSize(imageSize.GetWidth(),imageSize.GetHeight());
};

void MaskImageCtrl::update()
{
    wxClientDC dc(this);
    PrepareDC(dc);
    OnDraw(dc);
};

void MaskImageCtrl::DrawPolygon(wxDC &dc, HuginBase::MaskPolygon poly, bool isSelected, bool drawMarker)
{
    unsigned int nrOfPoints=poly.getMaskPolygon().size();
    if (nrOfPoints<2)
        return;
    wxPoint *polygonPoints=new wxPoint[nrOfPoints];
    for(unsigned int j=0;j<nrOfPoints;j++)
    {
        polygonPoints[j].x=transform(poly.getMaskPolygon()[j].x);
        polygonPoints[j].y=transform(poly.getMaskPolygon()[j].y);
    };
    if(isSelected)
        dc.SetPen(wxPen(m_colour_point_unselected,1,wxSOLID));
    else
        switch(poly.getMaskType())
        {
            case HuginBase::MaskPolygon::Mask_negative:
                dc.SetPen(wxPen(m_colour_polygon_negative,1,wxSOLID));
                break;
            case HuginBase::MaskPolygon::Mask_positive:
                dc.SetPen(wxPen(m_colour_polygon_positive,1,wxSOLID));
                 break;
        };
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    if(nrOfPoints>2)
        dc.DrawPolygon(nrOfPoints,polygonPoints);
    else
        dc.DrawLine(polygonPoints[0],polygonPoints[1]);
    if(drawMarker)
    {
        wxPen penSelected(m_colour_point_selected);
        wxPen penUnselected(m_colour_point_unselected);
        wxBrush brushSelected(m_colour_point_selected);
        wxBrush brushUnselected(m_colour_point_unselected);
        for(unsigned int j=0;j<nrOfPoints;j++)
        {
            if(set_contains(m_selectedPoints,j))
            {
                dc.SetPen(penSelected);
                dc.SetBrush(brushSelected);
            }
            else
            {
                dc.SetPen(penUnselected);
                dc.SetBrush(brushUnselected);
            };
            dc.DrawRectangle(polygonPoints[j].x-polygonPointSize,polygonPoints[j].y-polygonPointSize,
                            2*polygonPointSize,2*polygonPointSize);
        };
    };
    delete []polygonPoints;
};
 
void MaskImageCtrl::OnDraw(wxDC & dc)
{
    if(maskEditState!=NO_IMAGE)
    {
        dc.DrawBitmap(bitmap,0,0);
        if(fitToWindow)
        {
            //draw border when image is fit to window, otherwise the border (without image) is not updated
            wxSize clientSize=GetClientSize();
            if(bitmap.GetWidth()<clientSize.GetWidth())
            {
                dc.SetPen(wxPen(GetBackgroundColour(),1));
                dc.SetBrush(wxBrush(GetBackgroundColour()));
                dc.DrawRectangle(bitmap.GetWidth(),0,clientSize.GetWidth()-bitmap.GetWidth(),clientSize.GetHeight());
            };
            if(bitmap.GetHeight()<clientSize.GetHeight())
            {
                dc.SetPen(wxPen(GetBackgroundColour(),1));
                dc.SetBrush(wxBrush(GetBackgroundColour()));
                dc.DrawRectangle(0,bitmap.GetHeight(),clientSize.GetWidth(),clientSize.GetHeight()-bitmap.GetHeight());
            };
        };
        if(m_imageMask.size()>0)
        {
            //now draw all polygons
            HuginBase::MaskPolygonVector maskList=m_imageMask;
            bool drawSelected=(maskEditState!=POINTS_ADDING && maskEditState!=POINTS_MOVING);
            for(unsigned int i=0;i<maskList.size();i++)
            {
                if(i!=m_activeMask)
                    DrawPolygon(dc,maskList[i],false,false);
                else
                    if(drawSelected)
                        DrawPolygon(dc,maskList[i],true,true);
            };
        };
        //and now the actual polygon
        if(maskEditState==POINTS_ADDING || maskEditState==POINTS_MOVING || maskEditState==NEW_POLYGON_CREATING)
            DrawPolygon(dc,m_editingMask,true,true);
	}
    else
    {
		// clear the rectangle and exit
        dc.SetPen(wxPen(GetBackgroundColour(), 1, wxSOLID));
        dc.SetBrush(wxBrush(GetBackgroundColour(),wxSOLID));
        dc.Clear();
		return;
	};
}

void MaskImageCtrl::OnSize(wxSizeEvent &e)
{
    DEBUG_TRACE("size: " << e.GetSize().GetWidth() << "x" << e.GetSize().GetHeight());
    // rescale bitmap if needed.
    if (imageFilename != "") {
        if (fitToWindow) {
            setScale(0);
        }
    }
};

void MaskImageCtrl::rescaleImage()
{
    if (maskEditState == NO_IMAGE) {
        return;
    }
    wxImage img = imageCacheEntry2wxImage(m_img);
    if (img.GetWidth() == 0) {
        return;
    }
    imageSize = wxSize(img.GetWidth(), img.GetHeight());
    imageSize.IncBy(2*HuginBase::maskOffset);
    m_realSize = imageSize;
    if (fitToWindow)
        scaleFactor = calcAutoScaleFactor(imageSize);
    //draw border around image to allow selection of position outside of image
    wxBitmap cachedBitmap=wxBitmap(img);
    bitmap=wxBitmap(imageSize.GetWidth(),imageSize.GetHeight());
    wxMemoryDC dc(bitmap);
    dc.SetPen(wxPen(GetBackgroundColour(), 1, wxSOLID));
    dc.SetBrush(wxBrush(GetBackgroundColour(),wxSOLID));
    dc.DrawRectangle(wxPoint(0,0),imageSize);
    dc.DrawBitmap(cachedBitmap,HuginBase::maskOffset,HuginBase::maskOffset);
    dc.SelectObject(wxNullBitmap);

    if (getScaleFactor()!=1.0) 
    {
        imageSize.SetWidth(scale(imageSize.GetWidth()));
        imageSize.SetHeight(scale(imageSize.GetHeight()));
        wxImage tmp=bitmap.ConvertToImage();
        bitmap=wxBitmap(tmp.Scale(imageSize.GetWidth(), imageSize.GetHeight()));
        DEBUG_DEBUG("rescaling finished");
    }

    SetVirtualSize(imageSize.GetWidth(), imageSize.GetHeight());
    SetScrollRate(1,1);
    Refresh(true);
};

void MaskImageCtrl::DrawSelectionRectangle()
{
    wxClientDC dc(this);
    PrepareDC(dc);
    dc.SetLogicalFunction(wxINVERT);
    dc.SetPen(wxPen(*wxWHITE,1,wxDOT));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(m_dragStartPos.x,m_dragStartPos.y,
        (m_currentPos.x-m_dragStartPos.x),(m_currentPos.y-m_dragStartPos.y));
};

void MaskImageCtrl::FindPolygon(hugin_utils::FDiff2D p)
{
    unsigned int selectedPolygon=UINT_MAX;
    unsigned int i=0;
    while(selectedPolygon==UINT_MAX && i<m_imageMask.size())
    {
        if(m_imageMask[i].isInside(p))
            selectedPolygon=i;
        i++;
    };
    if(selectedPolygon<UINT_MAX)
        m_editPanel->SelectMask(selectedPolygon);
};

bool MaskImageCtrl::SelectPointsInsideMouseRect(HuginBase::UIntSet &points,const bool considerSelectedOnly)
{
    bool founded=false;
    double xmin=invtransform(std::min(m_dragStartPos.x,m_currentPos.x))-maxSelectionDistance;
    double xmax=invtransform(std::max(m_dragStartPos.x,m_currentPos.x))+maxSelectionDistance;
    double ymin=invtransform(std::min(m_dragStartPos.y,m_currentPos.y))-maxSelectionDistance;
    double ymax=invtransform(std::max(m_dragStartPos.y,m_currentPos.y))+maxSelectionDistance;
    const HuginBase::VectorPolygon poly=m_editingMask.getMaskPolygon();
    for(unsigned int i=0;i<poly.size();i++)
    {
        bool activePoints=true;
        if(considerSelectedOnly)
            activePoints=set_contains(m_selectedPoints,i);
        if(activePoints && xmin<=poly[i].x && poly[i].x<=xmax && ymin<=poly[i].y && poly[i].y<=ymax)
        {
            points.insert(i);
            founded=true;
        };
    };
    return founded;
};

void MaskImageCtrl::setScale(double factor)
{
    if (factor == 0) 
    {
        fitToWindow = true;
        factor = calcAutoScaleFactor(imageSize);
    } 
    else 
    {
        fitToWindow = false;
    }
    DEBUG_DEBUG("new scale factor:" << factor);
    // update if factor changed
    if (factor != scaleFactor) 
    {
        scaleFactor = factor;
        // keep existing scale focussed.
        rescaleImage();
    }
};

double MaskImageCtrl::calcAutoScaleFactor(wxSize size)
{
    int w = size.GetWidth();
    int h = size.GetHeight();

    wxSize csize = GetSize();
    DEBUG_DEBUG("csize: " << csize.GetWidth() << "x" << csize.GetHeight() << "image: " << w << "x" << h);
    double s1 = (double)csize.GetWidth()/w;
    double s2 = (double)csize.GetHeight()/h;
    DEBUG_DEBUG("s1: " << s1 << "  s2:" << s2);
    return s1 < s2 ? s1 : s2;
};

double MaskImageCtrl::getScaleFactor() const
{
    return scaleFactor;
};

IMPLEMENT_DYNAMIC_CLASS(MaskImageCtrl, wxScrolledWindow)

MaskImageCtrlXmlHandler::MaskImageCtrlXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
};

wxObject *MaskImageCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, MaskImageCtrl)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow(cp);

    return cp;
};

bool MaskImageCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("MaskImageCtrl"));
};

IMPLEMENT_DYNAMIC_CLASS(MaskImageCtrlXmlHandler, wxXmlResourceHandler)
