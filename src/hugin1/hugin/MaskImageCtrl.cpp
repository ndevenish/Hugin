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
#include "hugin/MainFrame.h"
#include "hugin/config_defaults.h"
#include "hugin/MaskImageCtrl.h"
#include "hugin/MaskEditorPanel.h"
#include "base_wx/wxImageCache.h"
#ifndef SUPPORTS_WXINVERT
#include <vigra/inspectimage.hxx>
#endif

using namespace hugin_utils;

/** half size of markers */
const int polygonPointSize=3;
/** maximal distance for selection of one point */
const int maxSelectionDistance=20;

// our image control

BEGIN_EVENT_TABLE(MaskImageCtrl, wxScrolledWindow)
    EVT_SIZE(MaskImageCtrl::OnSize)
    EVT_MOTION(MaskImageCtrl::OnMouseMove)
    EVT_LEFT_DOWN(MaskImageCtrl::OnLeftMouseDown)
    EVT_LEFT_UP(MaskImageCtrl::OnLeftMouseUp)
    EVT_LEFT_DCLICK(MaskImageCtrl::OnLeftMouseDblClick)
    EVT_RIGHT_DOWN(MaskImageCtrl::OnRightMouseDown)
    EVT_RIGHT_UP(MaskImageCtrl::OnRightMouseUp)
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
    m_maskEditState = NO_IMAGE;
    m_imgRotation = ROT0;
    m_scaleFactor = 1;
    m_fitToWindow = false;
    m_previewOnly = false;
    m_activeMask = UINT_MAX;
    m_showActiveMasks = false;
    m_maskMode = true;

    return true;
}

void MaskImageCtrl::Init(MaskEditorPanel * parent)
{
    m_editPanel = parent;
}

void MaskImageCtrl::SetMaskMode(bool newMaskMode)
{
    m_maskMode=newMaskMode;
    if(m_maskMode)
    {
        SetCursor(wxNullCursor);
        if(m_maskEditState!=NO_IMAGE)
        {
            m_maskEditState=NO_SELECTION;
            setActiveMask(UINT_MAX,false);
        };
    }
    else
    {
        if(m_maskEditState!=NO_IMAGE)
        {
            m_maskEditState=CROP_SHOWING;
        };
    };
};

void MaskImageCtrl::setImage(const std::string & file, HuginBase::MaskPolygonVector newMask, HuginBase::MaskPolygonVector masksToDraw, ImageRotation rot)
{
    DEBUG_TRACE("setting Image " << file);
    m_imageFilename = file;
    wxString fn(m_imageFilename.c_str(),HUGIN_CONV_FILENAME);
    if (wxFileName::FileExists(fn))
    {
        m_img = ImageCache::getInstance().getImage(m_imageFilename);
        if(m_maskMode)
        {
            m_maskEditState = NO_MASK;
        }
        else
        {
            m_maskEditState = CROP_SHOWING;
        };
        m_imageMask=newMask;
        m_masksToDraw=masksToDraw;
        m_imgRotation=rot;
        setActiveMask(UINT_MAX,false);
        rescaleImage();
    }
    else
    {
        m_maskEditState = NO_IMAGE;
        m_bitmap = wxBitmap();
        // delete the image (release shared_ptr)
        // create an empty image.
        m_img = ImageCache::EntryPtr(new ImageCache::Entry);
        HuginBase::MaskPolygonVector mask;
        m_imageMask=mask;
        m_masksToDraw=mask;
        m_imgRotation=ROT0;
        setActiveMask(UINT_MAX,false);
        SetVirtualSize(100,100);
        Refresh(true);
    }
}

void MaskImageCtrl::setNewMasks(HuginBase::MaskPolygonVector newMasks, HuginBase::MaskPolygonVector masksToDraw)
{
    m_imageMask=newMasks;
    m_masksToDraw=masksToDraw;
    if(m_activeMask>=m_imageMask.size())
        setActiveMask(UINT_MAX);
    Refresh(false);
};

void MaskImageCtrl::setCrop(HuginBase::SrcPanoImage::CropMode newCropMode, vigra::Rect2D newCropRect, bool isCentered, hugin_utils::FDiff2D center, bool isCircleCrop)
{
    m_cropMode=newCropMode;
    m_cropRect=newCropRect;
    m_cropCentered=isCentered;
    m_cropCenter=center;
    m_cropCircle=isCircleCrop;
};

void MaskImageCtrl::setActiveMask(unsigned int newMask, bool doUpdate)
{
    if(m_activeMask!=newMask)
    {
        m_activeMask=newMask;
        m_selectedPoints.clear();
    };
    if(newMask<UINT_MAX)
    {
        if(m_maskMode)
        {
            if(m_selectedPoints.empty())
            {
                m_maskEditState=NO_SELECTION;
            }
            else
            {
                m_maskEditState=POINTS_SELECTED;
            };
        }
        else
        {
            m_selectedPoints.clear();
            m_maskEditState=CROP_SHOWING;
        };
        m_editingMask=m_imageMask[m_activeMask];
    }
    else
    {
        if(!m_imageFilename.empty())
        {
            if(m_maskMode)
            {
                m_maskEditState=NO_MASK;
            }
            else
            {
                m_maskEditState=CROP_SHOWING;
            };
        };
        HuginBase::MaskPolygon mask;
        m_editingMask=mask;
    };
    if(doUpdate)
        Refresh(true);
};

void MaskImageCtrl::selectAllMarkers()
{
    m_selectedPoints.clear();
    if(m_activeMask<UINT_MAX)
        fill_set(m_selectedPoints,0,m_imageMask[m_activeMask].getMaskPolygon().size()-1);
};

// returns where the user clicked
MaskImageCtrl::ClickPos MaskImageCtrl::GetClickPos(vigra::Point2D pos)
{
    vigra::Rect2D testRect(pos.x-maxSelectionDistance, pos.y-maxSelectionDistance, pos.x+maxSelectionDistance, pos.y+maxSelectionDistance);
    if(m_cropMode==SrcPanoImage::CROP_CIRCLE)
    {
        double radius=std::min<int>(m_cropRect.width(), m_cropRect.height())/2.0;
        vigra::Point2D pos_center((m_cropRect.left()+m_cropRect.right())/2, (m_cropRect.top()+m_cropRect.bottom())/2);
        double dist=sqrt(double((pos-pos_center).squaredMagnitude()));
        if(dist-maxSelectionDistance<radius && radius<dist+maxSelectionDistance)
        {
            return CLICK_CIRCLE;
        };
    };
    if(m_cropRect.intersects(testRect))
    {
        if(abs(pos.x-m_cropRect.left())<maxSelectionDistance)
        {
            return CLICK_LEFT;
        };
        if(abs(pos.x-m_cropRect.right())<maxSelectionDistance)
        {
            return CLICK_RIGHT;
        };
        if(abs(pos.y-m_cropRect.top())<maxSelectionDistance)
        {
            return CLICK_TOP;
        };
        if(abs(pos.y-m_cropRect.bottom())<maxSelectionDistance)
        {
            return CLICK_BOTTOM;
        };
    };
    if(m_cropRect.contains(pos))
    {
        return CLICK_INSIDE;
    };
    return CLICK_OUTSIDE;
};

void MaskImageCtrl::UpdateCrop(hugin_utils::FDiff2D pos)
{
    FDiff2D delta=pos-applyRotInv(invtransform(m_currentPos));
    int newLeft, newRight, newTop, newBottom;
    bool needsUpdate=false;
    switch (m_maskEditState)
    {
        case CROP_MOVING:
            m_cropRect.moveBy(delta.x,delta.y);
            break;
        case CROP_LEFT_MOVING:
            if(m_cropCentered)
            {
                double newHalfWidth=m_cropRect.width()/2.0-delta.x;
                newLeft=hugin_utils::roundi(m_cropCenter.x-newHalfWidth);
                newRight=hugin_utils::roundi(m_cropCenter.x+newHalfWidth);
            }
            else
            {
                newLeft=m_cropRect.left()+delta.x;
                newRight=m_cropRect.right();
            };
            newTop=m_cropRect.top();
            newBottom=m_cropRect.bottom();
            needsUpdate=true;
            break;
        case CROP_RIGHT_MOVING:
            if(m_cropCentered)
            {
                double newHalfWidth=m_cropRect.width()/2.0+delta.x;
                newLeft=hugin_utils::roundi(m_cropCenter.x-newHalfWidth);
                newRight=hugin_utils::roundi(m_cropCenter.x+newHalfWidth);
            }
            else
            {
                newLeft=m_cropRect.left();
                newRight=m_cropRect.right()+delta.x;
            };
            newTop=m_cropRect.top();
            newBottom=m_cropRect.bottom();
            needsUpdate=true;
            break;
        case CROP_TOP_MOVING:
            if(m_cropCentered)
            {
                double newHalfHeight=m_cropRect.height()/2.0-delta.y;
                newTop=hugin_utils::roundi(m_cropCenter.y-newHalfHeight);
                newBottom=hugin_utils::roundi(m_cropCenter.y+newHalfHeight);
            }
            else
            {
                newTop=m_cropRect.top()+delta.y;
                newBottom=m_cropRect.bottom();
            };
            newLeft=m_cropRect.left();
            newRight=m_cropRect.right();
            needsUpdate=true;
            break;
        case CROP_BOTTOM_MOVING:
            if(m_cropCentered)
            {
                double newHalfHeight=m_cropRect.height()/2.0+delta.y;
                newTop=hugin_utils::roundi(m_cropCenter.y-newHalfHeight);
                newBottom=hugin_utils::roundi(m_cropCenter.y+newHalfHeight);
            }
            else
            {
                newTop=m_cropRect.top();
                newBottom=m_cropRect.bottom()+delta.y;
            };
            newLeft=m_cropRect.left();
            newRight=m_cropRect.right();
            needsUpdate=true;
            break;
        case CROP_CIRCLE_SCALING:
            {
                double radius=sqrt(sqr(pos.x-m_dragStartPos.x)+sqr(pos.y-m_dragStartPos.y));
                newLeft=m_dragStartPos.x-radius;
                newRight=m_dragStartPos.x+radius;
                newTop=m_dragStartPos.y-radius;
                newBottom=m_dragStartPos.y+radius;
                needsUpdate=true;
            };
            break;
    };
    if(needsUpdate)
    {
        // switch left/right or top/bottom if necessary
        if(newLeft>newRight)
        {
            int temp=newLeft;
            newLeft=newRight;
            newRight=temp;
        };
        if(newTop>newBottom)
        {
            int temp=newTop;
            newTop=newBottom;
            newBottom=temp;
        };
        m_cropRect.setUpperLeft(vigra::Point2D(newLeft, newTop));
        m_cropRect.setLowerRight(vigra::Point2D(newRight, newBottom));
    };
};

void MaskImageCtrl::OnMouseMove(wxMouseEvent& mouse)
{
    if(m_previewOnly)
        return;
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos.x, & mpos.y);
    FDiff2D currentPos=applyRotInv(invtransform(mpos));
    bool doUpdate = false;
    switch(m_maskEditState)
    {
        case NEW_POLYGON_CREATING:
            doUpdate=true;
            m_editingMask.movePointTo(m_editingMask.getMaskPolygon().size()-1,currentPos);
            break;
        case POLYGON_SELECTING:
        case REGION_SELECTING:
        case POINTS_DELETING:
#if defined SUPPORTS_WXINVERT
            DrawSelectionRectangle();
#endif
            m_currentPos=mpos;
            DrawSelectionRectangle();
            break;
        case POINTS_MOVING:
            doUpdate=true;
            m_editingMask=m_imageMask[m_activeMask];
            {
                FDiff2D delta=currentPos-applyRotInv(invtransform(m_dragStartPos));
                for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                    m_editingMask.movePointBy(*it,delta);
            };
            break;
        case POINTS_ADDING:
            doUpdate=true;
            for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                m_editingMask.movePointTo(*it,currentPos);
            break;
        case CROP_SHOWING:
            switch(GetClickPos(vigra::Point2D(currentPos.x, currentPos.y)))
            {
                case CLICK_INSIDE:
                    if(!m_cropCentered)
                    {
                        SetCursor(wxCURSOR_HAND);
                    }
                    else
                    {
                        SetCursor(wxNullCursor);
                    };
                    break;
                case CLICK_LEFT:
                case CLICK_RIGHT:
                    switch (m_imgRotation)
                    {
                        case ROT90:
                        case ROT270:
                            SetCursor(wxCURSOR_SIZENS);
                            break;
                        default:
                            SetCursor(wxCURSOR_SIZEWE);
                    };
                    break;
                case CLICK_TOP:
                case CLICK_BOTTOM:
                    switch (m_imgRotation)
                    {
                        case ROT90:
                        case ROT270:
                            SetCursor(wxCURSOR_SIZEWE);
                            break;
                        default:
                            SetCursor(wxCURSOR_SIZENS);
                    };
                    break;
                case CLICK_CIRCLE:
                    SetCursor(wxCURSOR_SIZING);
                    break;
                default:
                    SetCursor(wxNullCursor);
            };
            break;
        case CROP_MOVING:
        case CROP_LEFT_MOVING:
        case CROP_RIGHT_MOVING:
        case CROP_TOP_MOVING:
        case CROP_BOTTOM_MOVING:
        case CROP_CIRCLE_SCALING:
#if defined SUPPORTS_WXINVERT
            DrawCrop();
#else
            doUpdate=true;
#endif
            UpdateCrop(currentPos);
            m_currentPos=mpos;
#if defined SUPPORTS_WXINVERT
            DrawCrop();
#endif
            m_editPanel->UpdateCropFromImage();
            break;
    };
    if(doUpdate)
        update();
}

void MaskImageCtrl::OnLeftMouseDown(wxMouseEvent& mouse)
{
    if(m_previewOnly)
        return;
    DEBUG_DEBUG("LEFT MOUSE DOWN");
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &m_dragStartPos.x, & m_dragStartPos.y);
    FDiff2D currentPos=applyRotInv(invtransform(m_dragStartPos));
    m_currentPos=m_dragStartPos;
    if(!HasCapture())
        CaptureMouse();
    SetFocus();
    switch(m_maskEditState)
    {
        case NEW_POLYGON_STARTED:
            //starting polygon creating
            m_editingMask.addPoint(currentPos);
            m_selectedPoints.insert(m_editingMask.getMaskPolygon().size()-1);
            break;
        case NO_MASK:
            if(m_maskMode)
            {
                m_maskEditState=POLYGON_SELECTING;
                DrawSelectionRectangle();
            };
            break;
        case NO_SELECTION:
            if(mouse.CmdDown())
            {
                // check if mouse clicks happens near one line of active polygon
                unsigned int index=m_editingMask.FindPointNearPos(currentPos,5*maxSelectionDistance);
                if(index<UINT_MAX)
                {
                    m_selectedPoints.clear();
                    m_editingMask.insertPoint(index,currentPos);
                    m_selectedPoints.insert(index);
                    m_maskEditState=POINTS_ADDING;
                };
            }
            else
            {
                HuginBase::UIntSet points;
                if(SelectPointsInsideMouseRect(points,false))
                {
                    for(HuginBase::UIntSet::const_iterator it=points.begin();it!=points.end();it++)
                        m_selectedPoints.insert(*it);
                    m_maskEditState=POINTS_MOVING;
                }
                else
                {
                    m_maskEditState=REGION_SELECTING;
                    DrawSelectionRectangle();
                }
            };
            break;
        case POINTS_SELECTED:
            if(mouse.CmdDown())
            {
                // check if mouse clicks happens near one line of active polygon
                unsigned int index=m_editingMask.FindPointNearPos(currentPos, 5*maxSelectionDistance);
                if(index<UINT_MAX)
                {
                    m_selectedPoints.clear();
                    m_editingMask.insertPoint(index,currentPos);
                    m_selectedPoints.insert(index);
                    m_maskEditState=POINTS_ADDING;
                };
            }
            else
            {
                HuginBase::UIntSet points;
                if(SelectPointsInsideMouseRect(points,true))
                {
                    //selected point clicked, starting moving
                    m_maskEditState=POINTS_MOVING;
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
                        m_maskEditState=POINTS_MOVING;
                    }
                    else
                    {
                        m_maskEditState=REGION_SELECTING;
                        DrawSelectionRectangle();
                    };
                }
            };
            break;
        case CROP_SHOWING:
            switch(GetClickPos(vigra::Point2D(currentPos.x,currentPos.y)))
            {
                case CLICK_INSIDE:
                    if(!m_cropCentered)
                    {
                        m_maskEditState=CROP_MOVING;
                    };
                    break;
                case CLICK_LEFT:
                    m_maskEditState=CROP_LEFT_MOVING;
                    break;
                case CLICK_RIGHT:
                    m_maskEditState=CROP_RIGHT_MOVING;
                    break;
                case CLICK_TOP:
                    m_maskEditState=CROP_TOP_MOVING;
                    break;
                case CLICK_BOTTOM:
                    m_maskEditState=CROP_BOTTOM_MOVING;
                    break;
                case CLICK_CIRCLE:
                    m_dragStartPos.x=(m_cropRect.left()+m_cropRect.right())/2;
                    m_dragStartPos.y=(m_cropRect.top()+m_cropRect.bottom())/2;
                    m_maskEditState=CROP_CIRCLE_SCALING;
                    break;
            };
            if(m_maskEditState!=CROP_SHOWING)
            {
                if(m_cropMode==SrcPanoImage::NO_CROP && m_cropCircle)
                {
#if defined SUPPORTS_WXINVERT
                    DrawCrop();
                    m_cropMode=SrcPanoImage::CROP_CIRCLE;
                    DrawCrop();
#else
                    m_cropMode=SrcPanoImage::CROP_CIRCLE;
                    update();
#endif
                };
            };
            break;
    };
};

void MaskImageCtrl::OnLeftMouseUp(wxMouseEvent& mouse)
{
    if(m_previewOnly)
        return;
    DEBUG_DEBUG("LEFT MOUSE UP");
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos.x, & mpos.y);
    FDiff2D currentPos=applyRotInv(invtransform(mpos));
    bool doUpdate=false;
    switch(m_maskEditState)
    {
        case NEW_POLYGON_STARTED:
            doUpdate=true;
            m_editingMask.addPoint(currentPos);
            m_selectedPoints.insert(m_editingMask.getMaskPolygon().size()-1);
            m_maskEditState=NEW_POLYGON_CREATING;
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
                FDiff2D delta=currentPos-applyRotInv(invtransform(m_dragStartPos));
                if(sqr(delta.x)+sqr(delta.y)>sqr(maxSelectionDistance))
                {
                    for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                        m_imageMask[m_activeMask].movePointBy(*it,delta);
                    m_maskEditState=POINTS_SELECTED;
                    m_editPanel->UpdateMask();
                }
                else
                {
                    m_editingMask=m_imageMask[m_activeMask];
                    m_maskEditState=POINTS_SELECTED;
                    doUpdate=true;
                };
            };
            break;
        case POLYGON_SELECTING:
            if(HasCapture())
                ReleaseMouse();
#if defined SUPPORTS_WXINVERT
            DrawSelectionRectangle();
#else
            doUpdate=true;
#endif
            m_currentPos=mpos;
            m_maskEditState=NO_SELECTION;
            {
                hugin_utils::FDiff2D p;
                p.x=invtransform(m_dragStartPos.x+(m_currentPos.x-m_dragStartPos.x)/2);
                p.y=invtransform(m_dragStartPos.y+(m_currentPos.y-m_dragStartPos.y)/2);
                p=applyRotInv(p);
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
                        m_maskEditState=NO_SELECTION;
                    else
                        m_maskEditState=POINTS_SELECTED;
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
                        p=applyRotInv(p);
                        FindPolygon(p);
                    };
                    m_maskEditState=NO_SELECTION;
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
            m_maskEditState=POINTS_SELECTED;
            break;
        case CROP_MOVING:
        case CROP_LEFT_MOVING:
        case CROP_RIGHT_MOVING:
        case CROP_TOP_MOVING:
        case CROP_BOTTOM_MOVING:
        case CROP_CIRCLE_SCALING:
            if(HasCapture())
                ReleaseMouse();
#if defined SUPPORTS_WXINVERT
            DrawCrop();
#else
            doUpdate=true;
#endif
            UpdateCrop(currentPos);
#if defined SUPPORTS_WXINVERT
            DrawCrop();
#endif
            m_maskEditState=CROP_SHOWING;
            SetCursor(wxNullCursor);
            m_editPanel->UpdateCrop(true);
            break;
        default:
            if(HasCapture())
                ReleaseMouse();
    };
    if(doUpdate)
        update();
}

void MaskImageCtrl::OnLeftMouseDblClick(wxMouseEvent &mouse)
{
    if(m_previewOnly)
        return;
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos.x, & mpos.y);
    FDiff2D currentPos=applyRotInv(invtransform(mpos));
    switch(m_maskEditState)
    {
        case NEW_POLYGON_STARTED:
            {
                m_maskEditState=NO_SELECTION;
                HuginBase::MaskPolygon mask;
                m_editingMask=mask;
                m_selectedPoints.clear();
                MainFrame::Get()->SetStatusText(wxT(""),0);
                break;
            };
        case NEW_POLYGON_CREATING:
            {
                //close newly generated polygon
                m_maskEditState=NO_SELECTION;
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
                MainFrame::Get()->SetStatusText(wxT(""),0);
                break;
            };
    };
};

void MaskImageCtrl::OnRightMouseDown(wxMouseEvent& mouse)
{
    if(m_previewOnly)
        return;
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &m_dragStartPos.x, & m_dragStartPos.y);
    FDiff2D currentPos=applyRotInv(invtransform(m_dragStartPos));
    m_currentPos=m_dragStartPos;
    if(!HasCapture())
        CaptureMouse();
    SetFocus();
    switch(m_maskEditState)
    {
        case NO_SELECTION:
        case POINTS_SELECTED:
            if(mouse.CmdDown())
            {
                m_maskEditState=POINTS_DELETING;
                DrawSelectionRectangle();
            }
            else
            {
                if (m_editingMask.isInside(currentPos))
                {
                    fill_set(m_selectedPoints,0,m_editingMask.getMaskPolygon().size()-1);
                    m_maskEditState=POINTS_MOVING;
                    update();
                };
            };
            break;
    };
};

void MaskImageCtrl::OnRightMouseUp(wxMouseEvent& mouse)
{
    if(m_previewOnly)
        return;
    wxPoint mpos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos.x, & mpos.y);
    FDiff2D currentPos=applyRotInv(invtransform(mpos));
    if(HasCapture())
        ReleaseMouse();
    switch(m_maskEditState)
    {
        case NEW_POLYGON_STARTED:
            {
                m_maskEditState=NO_SELECTION;
                HuginBase::MaskPolygon mask;
                m_editingMask=mask;
                m_selectedPoints.clear();
                MainFrame::Get()->SetStatusText(wxT(""),0);
                break;
            };
        case NEW_POLYGON_CREATING:
            {
                //close newly generated polygon
                m_maskEditState=NO_SELECTION;
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
                MainFrame::Get()->SetStatusText(wxT(""),0);
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
                    m_maskEditState=NO_SELECTION;
                else
                    m_maskEditState=POINTS_SELECTED;
                break;
            };
        case POINTS_MOVING:
            {
                FDiff2D delta=currentPos-applyRotInv(invtransform(m_dragStartPos));
                if(sqr(delta.x)+sqr(delta.y)>sqr(maxSelectionDistance))
                {
                    for(HuginBase::UIntSet::const_iterator it=m_selectedPoints.begin();it!=m_selectedPoints.end();it++)
                        m_imageMask[m_activeMask].movePointBy(*it,delta);
                    m_maskEditState=POINTS_SELECTED;
                    m_editPanel->UpdateMask();
                }
                else
                {
                    m_editingMask=m_imageMask[m_activeMask];
                    m_maskEditState=POINTS_SELECTED;
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
            switch(m_maskEditState)
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
    switch(m_maskEditState)
    {
        case NEW_POLYGON_CREATING:
        case NEW_POLYGON_STARTED:
            {
                wxBell();
                m_maskEditState=NO_SELECTION;
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
    m_maskEditState=NEW_POLYGON_STARTED;
    HuginBase::MaskPolygon newMask;
    m_editingMask=newMask;
    m_selectedPoints.clear();
};

wxSize MaskImageCtrl::DoGetBestSize() const
{
    return wxSize(m_imageSize.GetWidth(),m_imageSize.GetHeight());
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
        polygonPoints[j]=transform(applyRot(poly.getMaskPolygon()[j]));
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

void MaskImageCtrl::DrawCrop()
{
    wxClientDC dc(this);
    PrepareDC(dc);
    DrawCrop(dc);
};

void MaskImageCtrl::DrawCrop(wxDC & dc)
{
    // draw crop rectangle/circle
    if(!m_maskMode)
    {
        // draw all areas without fillings
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
#if defined SUPPORTS_WXINVERT
        dc.SetLogicalFunction (wxINVERT);
#else
        dc.SetPen(wxPen(m_color_selection, 1, wxPENSTYLE_SOLID));
#endif
        wxPoint middle=transform(applyRot((m_cropRect.lowerRight()+m_cropRect.upperLeft())/2));

        int c = 8; // size of midpoint cross
        dc.DrawLine( middle.x + c, middle.y + c, middle.x - c, middle.y - c);
        dc.DrawLine( middle.x - c, middle.y + c, middle.x + c, middle.y - c);
        dc.DrawRectangle(wxRect(transform(applyRot(hugin_utils::FDiff2D(m_cropRect.left(), m_cropRect.top()))),
                                transform(applyRot(hugin_utils::FDiff2D(m_cropRect.right(), m_cropRect.bottom())))));

        // draw crop circle as well, if requested.
        if (m_cropMode==HuginBase::BaseSrcPanoImage::CROP_CIRCLE)
        {
            double radius=std::min<int>(m_cropRect.width(),m_cropRect.height())/2.0;
            dc.DrawCircle(middle.x, middle.y, scale(radius));
        }
    };
};

void MaskImageCtrl::OnDraw(wxDC & dc)
{
    if(m_maskEditState!=NO_IMAGE)
    {
        int offset=scale(HuginBase::maskOffset);
        //draw border around image to allow drawing mask over boudaries of image
        //don't draw as one complete rectangle to prevent flickering
        dc.SetPen(wxPen(GetBackgroundColour(), 1, wxSOLID));
        dc.SetBrush(wxBrush(GetBackgroundColour(),wxSOLID));
        dc.DrawRectangle(0,0,offset,m_bitmap.GetHeight()+2*offset);
        dc.DrawRectangle(0,0,m_bitmap.GetWidth()+2*offset,offset);
        dc.DrawRectangle(m_bitmap.GetWidth()+offset,0,m_bitmap.GetWidth()+2*offset,m_bitmap.GetHeight()+2*offset);
        dc.DrawRectangle(0,m_bitmap.GetHeight()+offset,m_bitmap.GetWidth()+2*offset,m_bitmap.GetHeight()+2*offset);
        dc.DrawBitmap(m_bitmap,offset,offset);
        if(m_fitToWindow)
        {
            //draw border when image is fit to window, otherwise the border (without image) is not updated
            wxSize clientSize=GetClientSize();
            if(m_bitmap.GetWidth()+2*offset<clientSize.GetWidth())
            {
                dc.DrawRectangle(m_bitmap.GetWidth()+2*offset,0,clientSize.GetWidth()-m_bitmap.GetWidth()+2*offset,clientSize.GetHeight());
            };
            if(m_bitmap.GetHeight()+2*offset<clientSize.GetHeight())
            {
                dc.DrawRectangle(0,m_bitmap.GetHeight()+2*offset,clientSize.GetWidth(),clientSize.GetHeight()-m_bitmap.GetHeight()+2*offset);
            };
        };
        if(m_maskMode && m_showActiveMasks && (m_cropMode!=SrcPanoImage::NO_CROP || m_masksToDraw.size()>0))
        {
            //whole image, we need it several times
            wxRegion wholeImage(transform(applyRot(hugin_utils::FDiff2D(0,0))),
                                transform(applyRot(hugin_utils::FDiff2D(m_realSize.GetWidth(),m_realSize.GetHeight()))));
            wxRegion region;
            if(m_cropMode!=SrcPanoImage::NO_CROP)
            {
                region.Union(wholeImage);
                //now the crop
                switch(m_cropMode)
                {
                    case HuginBase::SrcPanoImage::CROP_RECTANGLE:
                        region.Subtract(wxRegion(transform(applyRot(m_cropRect.upperLeft())),
                            transform(applyRot(m_cropRect.lowerRight()))));
                        break;
                    case HuginBase::SrcPanoImage::CROP_CIRCLE:
                        unsigned int nrOfPoints=dc.GetSize().GetWidth()*2;
                        wxPoint* circlePoints=new wxPoint[nrOfPoints];
                        vigra::Point2D middle=(m_cropRect.lowerRight()+m_cropRect.upperLeft())/2;
                        double radius=std::min<int>(m_cropRect.width(),m_cropRect.height())/2;
                        double interval=2*PI/nrOfPoints;
                        for(unsigned int i=0;i<nrOfPoints;i++)
                        {
                            circlePoints[i]=transform(applyRot(hugin_utils::FDiff2D(middle.x+radius*cos(i*interval),middle.y+radius*sin(i*interval))));
                        };
                        region.Subtract(wxRegion(nrOfPoints,circlePoints));
                        delete []circlePoints;
                        break;
                };
            };
            if(m_masksToDraw.size()>0)
            {
                for(unsigned int i=0;i<m_masksToDraw.size();i++)
                {
                    HuginBase::VectorPolygon poly=m_masksToDraw[i].getMaskPolygon();
                    wxPoint *polygonPoints=new wxPoint[poly.size()];
                    for(unsigned int j=0;j<poly.size();j++)
                    {
                        polygonPoints[j]=transform(applyRot(poly[j]));
                    };
                    wxRegion singleRegion(poly.size(),polygonPoints,wxWINDING_RULE);
                    if(m_masksToDraw[i].isInverted())
                    {
                        wxRegion newRegion(wholeImage);
                        newRegion.Subtract(singleRegion);
                        region.Union(newRegion);
                    }
                    else
                    {
                        region.Union(singleRegion);
                    };
                    delete []polygonPoints;
                };
            };
#ifndef __WXMAC__
            // on Windows and GTK we need to compensate to clipping region
            // by the scroll offset
            // this seems not to be necessary for wxMac
            int x;
            int y;
            GetViewStart(&x,&y);
            region.Offset(-x,-y);
#endif
            dc.SetDeviceClippingRegion(region);
            dc.DrawBitmap(m_disabledBitmap,offset,offset);
            dc.DestroyClippingRegion();
        };
        DrawCrop(dc);
        if(m_maskMode && m_imageMask.size()>0)
        {
            //now draw all polygons
            HuginBase::MaskPolygonVector maskList=m_imageMask;
            bool drawSelected=(m_maskEditState!=POINTS_ADDING && m_maskEditState!=POINTS_MOVING);
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
        if(m_maskEditState==POINTS_ADDING || m_maskEditState==POINTS_MOVING || m_maskEditState==NEW_POLYGON_CREATING)
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
    // rescale m_bitmap if needed.
    if (m_imageFilename != "") {
        if (m_fitToWindow) {
            setScale(0);
        }
    }
};

void MaskImageCtrl::rescaleImage()
{
    if (m_maskEditState == NO_IMAGE)
    {
        return;
    }
#ifndef SUPPORTS_WXINVERT
    //determine average colour and set selection colour corresponding
    vigra::FindAverage<vigra::RGBValue<vigra::UInt8> > average;
    vigra::inspectImage(vigra::srcImageRange(*(m_img->get8BitImage())), average);
    vigra::RGBValue<vigra::UInt8> RGBaverage=average.average();
    if(RGBaverage[0]<180 && RGBaverage[1]<180 && RGBaverage[2]<180)
    {
        m_color_selection=*wxWHITE;
    }
    else
    {
        m_color_selection=*wxBLACK;
    };
#endif
    wxImage img = imageCacheEntry2wxImage(m_img);
    if (img.GetWidth() == 0)
    {
        return;
    }
    m_imageSize = wxSize(img.GetWidth(), img.GetHeight());
    m_realSize = m_imageSize;
    m_imageSize.IncBy(2*HuginBase::maskOffset);
    if (m_fitToWindow)
        m_scaleFactor = calcAutoScaleFactor(m_imageSize);

    //scaling image to screen size
    if (getScaleFactor()!=1.0)
    {
        m_imageSize.SetWidth(scale(m_imageSize.GetWidth()));
        m_imageSize.SetHeight(scale(m_imageSize.GetHeight()));
        img=img.Scale(scale(m_realSize.GetWidth()), scale(m_realSize.GetHeight()));
    }
    else
    {
        //the conversion to disabled m_bitmap would work on the original cached image file
        //therefore we need to create a copy to work on it
        img=img.Copy();
    };
    //and now rotating
    switch(m_imgRotation)
    {
        case ROT90:
            img = img.Rotate90(true);
            break;
        case ROT180:
                // this is slower than it needs to be...
            img = img.Rotate90(true);
            img = img.Rotate90(true);
            break;
        case ROT270:
            img = img.Rotate90(false);
            break;
        default:
            break;
    }
    m_bitmap=wxBitmap(img);

    //create disabled m_bitmap for drawing active masks
#if wxCHECK_VERSION(2,9,0)
    img.ConvertToDisabled(192);
#else
    {
        int width = img.GetWidth();
        int height = img.GetHeight();
        for (int y = height-1; y >= 0; --y)
        {
            for (int x = width-1; x >= 0; --x)
            {
                unsigned char* data = img.GetData() + (y*(width*3))+(x*3);
                unsigned char* r = data;
                unsigned char* g = data+1;
                unsigned char* b = data+2;
                *r=(unsigned char)wxMin(0.6*(*r)+77,255);
                *b=(unsigned char)wxMin(0.6*(*b)+77,255);
                *g=(unsigned char)wxMin(0.6*(*g)+77,255);
            }
        }
    }
#endif
    m_disabledBitmap=wxBitmap(img);
    if (m_imgRotation == ROT90 || m_imgRotation == ROT270)
    {
        SetVirtualSize(m_imageSize.GetHeight(), m_imageSize.GetWidth());
    }
    else
    {
        SetVirtualSize(m_imageSize.GetWidth(), m_imageSize.GetHeight());
    };
    SetScrollRate(1,1);
    Refresh(true);
};

void MaskImageCtrl::DrawSelectionRectangle()
{
    wxClientDC dc(this);
    PrepareDC(dc);
#if defined SUPPORTS_WXINVERT
    dc.SetLogicalFunction(wxINVERT);
    dc.SetPen(wxPen(*wxWHITE,1,wxDOT));
#else
    OnDraw(dc);
    dc.SetPen(wxPen(m_color_selection, scale(1), wxDOT));
#endif
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
    bool found=false;
    hugin_utils::FDiff2D p1=applyRotInv(invtransform(m_dragStartPos));
    hugin_utils::FDiff2D p2=applyRotInv(invtransform(m_currentPos));
    double xmin=std::min(p1.x,p2.x)-maxSelectionDistance;
    double xmax=std::max(p1.x,p2.x)+maxSelectionDistance;
    double ymin=std::min(p1.y,p2.y)-maxSelectionDistance;
    double ymax=std::max(p1.y,p2.y)+maxSelectionDistance;
    const HuginBase::VectorPolygon poly=m_editingMask.getMaskPolygon();
    for(unsigned int i=0;i<poly.size();i++)
    {
        bool activePoints=true;
        if(considerSelectedOnly)
            activePoints=set_contains(m_selectedPoints,i);
        if(activePoints && xmin<=poly[i].x && poly[i].x<=xmax && ymin<=poly[i].y && poly[i].y<=ymax)
        {
            points.insert(i);
            found=true;
        };
    };
    return found;
};

void MaskImageCtrl::setScale(double factor)
{
    if (factor == 0)
    {
        m_fitToWindow = true;
        factor = calcAutoScaleFactor(m_imageSize);
    }
    else
    {
        m_fitToWindow = false;
    }
    DEBUG_DEBUG("new scale factor:" << factor);
    // update if factor changed
    if (factor != m_scaleFactor)
    {
        m_scaleFactor = factor;
        // keep existing scale focussed.
        rescaleImage();
    }
};

double MaskImageCtrl::calcAutoScaleFactor(wxSize size)
{
    int w = size.GetWidth();
    int h = size.GetHeight();
    if (m_imgRotation ==  ROT90 || m_imgRotation == ROT270)
    {
        int t = w;
        w = h;
        h = t;
    }

    wxSize csize = GetSize();
    DEBUG_DEBUG("csize: " << csize.GetWidth() << "x" << csize.GetHeight() << "image: " << w << "x" << h);
    double s1 = (double)csize.GetWidth()/w;
    double s2 = (double)csize.GetHeight()/h;
    DEBUG_DEBUG("s1: " << s1 << "  s2:" << s2);
    return s1 < s2 ? s1 : s2;
};

double MaskImageCtrl::getScaleFactor() const
{
    return m_scaleFactor;
};

void MaskImageCtrl::setDrawingActiveMasks(bool newDrawActiveMasks)
{
    m_showActiveMasks=newDrawActiveMasks;
    update();
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
