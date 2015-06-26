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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// standard wx include
#include <config.h>
#include "panoinc_WX.h"

// standard hugin include
#include "panoinc.h"
#include "base_wx/platform.h"

#include <vigra/inspectimage.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/basicimageview.hxx>
#ifdef HAVE_CXX11
#include <functional>  // std::bind 
#else
#include <boost/bind.hpp>
#endif

#include "hugin/config_defaults.h"
#include "hugin/CPImageCtrl.h"
#include "base_wx/wxImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "base_wx/wxcms.h"

#include "vigra_ext/ImageTransforms.h"

using namespace std;
using namespace hugin_utils;

#if !wxCHECK_VERSION(3,0,0)
#define wxPENSTYLE_SOLID wxSOLID
#define wxPENSTYLE_DOT wxDOT
#define wxBRUSHSTYLE_TRANSPARENT wxTRANSPARENT
#define wxBRUSHSTYLE_SOLID wxSOLID
#endif

// definition of the control point event

IMPLEMENT_DYNAMIC_CLASS( CPEvent, wxEvent )
#if _WINDOWS && defined Hugin_shared
DEFINE_LOCAL_EVENT_TYPE( EVT_CPEVENT )
#else
DEFINE_EVENT_TYPE( EVT_CPEVENT )
#endif

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

CPEvent::CPEvent(wxWindow* win, const hugin_utils::FDiff2D & p1, const hugin_utils::FDiff2D & p2)
{
    SetEventType(EVT_CPEVENT);
    SetEventObject(win);
    mode=DELETE_REGION_SELECTED;
    region=wxRect(roundi(min(p1.x,p2.x)),roundi(min(p1.y,p2.y)),abs(roundi(p2.x-p1.x)),abs(roundi(p2.y-p1.y)));
};

CPEvent::CPEvent(wxWindow* win, CPEventMode evt_mode, const FDiff2D & p)
{
    SetEventType(EVT_CPEVENT);
    SetEventObject(win);
    mode = evt_mode;
    point = p;
}

CPEvent::CPEvent(wxWindow* win, CPEventMode evt_mode, const HuginBase::ControlPoint cp)
{
    SetEventType(EVT_CPEVENT);
    SetEventObject(win);
    mode=evt_mode;
    m_cp=cp;
};

CPEvent::CPEvent(wxWindow* win, CPEventMode evt_mode, size_t cpNr, const HuginBase::ControlPoint cp)
{
    SetEventType(EVT_CPEVENT);
    SetEventObject(win);
    mode=evt_mode;
    pointNr=cpNr;
    m_cp=cp;
};

wxEvent * CPEvent::Clone() const
{
    return new CPEvent(*this);
}

DisplayedControlPoint::DisplayedControlPoint(const HuginBase::ControlPoint& cp, CPImageCtrl* control, bool mirrored)
{
    m_cp=cp;
    m_control=control;
    m_mirrored=mirrored;
    m_line=(m_cp.mode!=HuginBase::ControlPoint::X_Y) && (m_cp.image1Nr==m_cp.image2Nr);
};

void DisplayedControlPoint::SetColour(wxColour pointColour, wxColour textColour)
{
    m_pointColour=pointColour;
    m_textColour=textColour;
};

void DisplayedControlPoint::SetLabel(wxString newLabel)
{
    m_label=newLabel;
};

void DisplayedControlPoint::SetControl(CPImageCtrl* control)
{
    m_control=control;
};

void DrawCross(wxDC& dc, wxPoint p, int l)
{
    dc.DrawLine(p + wxPoint(-l, 0),
                p + wxPoint(-1, 0));
    dc.DrawLine(p + wxPoint(2, 0),
                p + wxPoint(l+1, 0));
    dc.DrawLine(p + wxPoint(0, -l),
                p + wxPoint(0, -1));
    dc.DrawLine(p + wxPoint(0, 2),
                p + wxPoint(0, l+1));
};

void DisplayedControlPoint::Draw(wxDC& dc, bool selected, bool newPoint)
{
    if(m_control==NULL)
    {
        return;
    };
    // select color
    wxColour bgColor = m_pointColour;
    wxColour textColor = m_textColour;
    bool drawMag = false;
    if (selected)
    {
        bgColor = wxTheColourDatabase->Find(wxT("RED"));
        textColor = wxTheColourDatabase->Find(wxT("WHITE"));
        drawMag = !m_control->GetMouseInWindow() || m_control->GetForceMagnifier();
    }
    if (newPoint)
    {
        bgColor = wxTheColourDatabase->Find(wxT("YELLOW"));
        textColor = wxTheColourDatabase->Find(wxT("BLACK"));
        drawMag = true;
    }

    dc.SetPen(wxPen(wxT("WHITE"), 1, wxPENSTYLE_SOLID));
    dc.SetBrush(wxBrush(wxT("BLACK"),wxBRUSHSTYLE_TRANSPARENT));

    FDiff2D pointInput=m_mirrored ? hugin_utils::FDiff2D(m_cp.x2, m_cp.y2) : hugin_utils::FDiff2D(m_cp.x1, m_cp.y1);
    FDiff2D point = m_control->applyRot(pointInput);
    wxPoint p = m_control->roundP(m_control->scale(point));
    FDiff2D pointInput2;
    wxPoint p2;
    if(m_line)
    {
        pointInput2=m_mirrored ? hugin_utils::FDiff2D(m_cp.x1, m_cp.y1) : hugin_utils::FDiff2D(m_cp.x2, m_cp.y2);
        FDiff2D point2 = m_control->applyRot(pointInput2);
        p2 = m_control->roundP(m_control->scale(point2));
    };
    int l = 6;
    // draw cursor line, choose white or black
    vigra::Rect2D box;
    if(m_line)
    {
        box.setUpperLeft(vigra::Point2D(roundi(std::min(m_cp.x1, m_cp.x2))-l, roundi(std::min(m_cp.y1, m_cp.y2))-l));
        box.setSize(roundi(abs(m_cp.x1-m_cp.x2)+2.0*l), roundi(abs(m_cp.y1-m_cp.y2)+2.0*l)); 
    }
    else
    {
        box.setUpperLeft(vigra::Point2D(roundi(pointInput.x-l), roundi(pointInput.y-l)));
        box.setSize(2*l, 2*l);
    };                
    // only use part inside.
    box &= vigra::Rect2D(m_control->GetImg()->size());
    if(box.width()<=0 || box.height()<=0)
    {
        return;
    };
    // calculate mean "luminance value"
    vigra::FindAverage<vigra::UInt8> average;   // init functor
    vigra::RGBToGrayAccessor<vigra::RGBValue<vigra::UInt8> > lumac;
    vigra::inspectImage(m_control->GetImg()->upperLeft()+ box.upperLeft(),
                        m_control->GetImg()->upperLeft()+ box.lowerRight(),
                        lumac, average);
    if (average() < 150)
    {
        dc.SetPen(wxPen(wxT("WHITE"), 1, wxPENSTYLE_SOLID));
    }
    else
    {
        dc.SetPen(wxPen(wxT("BLACK"), 1, wxPENSTYLE_SOLID));
    }

    if(m_line)
    {
        DrawLine(dc);
        DrawCross(dc, p, l);
        DrawCross(dc, p2, l);
    }
    else
    {
        if (m_cp.mode != HuginBase::ControlPoint::X_Y)
        {
            DrawLineSegment(dc);
        };
        DrawCross(dc, p, l);
    };
    // calculate distance to the image boundaries,
    // decide where to put the label and magnifier
    m_labelPos=DrawTextMag(dc, p, pointInput, drawMag, textColor, bgColor);
    if(m_line)
    {
        m_labelPos2=DrawTextMag(dc, p2, pointInput2, drawMag, textColor, bgColor);
    };
}

wxRect DisplayedControlPoint::DrawTextMag(wxDC& dc, wxPoint p, hugin_utils::FDiff2D pointInput, bool drawMag, wxColour textColour, wxColour bgColour)
{
    wxRect labelPos;
    int l = 6;
    wxSize clientSize = m_control->GetClientSize();
    int vx0, vy0;
    m_control->GetViewStart(&vx0, &vy0);
    wxFont font(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_LIGHT);
    dc.SetFont(font);
    wxPoint pClient(p.x - vx0, p.y - vy0);
    // space in upper left, upper right, lower left, lower right
    int maxDistUR = std::min(clientSize.x - pClient.x, pClient.y);
    int maxDistLL = std::min(pClient.x, clientSize.y - pClient.y);
    int maxDistLR = std::min(clientSize.x - pClient.x, clientSize.y - pClient.y);

    // text and magnifier offset
    int toff = l-1;
    // default to lower right
    wxPoint tul = p + wxPoint(toff,toff);

    // calculate text position and extend
    // width of border around text label
    int tB = 2;
    wxCoord tw, th;
    dc.GetTextExtent(m_label, &tw, &th);

    if (drawMag)
    {
        wxBitmap magBitmap = m_control->generateMagBitmap(pointInput, p);
        // TODO: select position depending on visible part of canvas
        wxPoint ulMag = tul;
        // choose placement of the magnifier
        int w = toff  + magBitmap.GetWidth()+3;
        int db = 5;
        if ( maxDistLR > w + db  )
        {
            ulMag = p + wxPoint(toff,toff);
        }
        else
        {
            if (maxDistLL > w + db)
            {
                ulMag = p + wxPoint(-w, toff);
            }
            else
            {
                if (maxDistUR > w + db)
                {
                    ulMag = p + wxPoint(toff, -w);
                }
                else
                {
                    ulMag = p + wxPoint(-w, -w);
                }
            }
        };

        dc.DrawBitmap(magBitmap, ulMag);
        dc.SetPen(wxPen(wxT("BLACK"), 1, wxPENSTYLE_SOLID));
        dc.SetBrush(wxBrush(wxT("WHITE"),wxBRUSHSTYLE_TRANSPARENT));

        // draw Bevel
        int bw = magBitmap.GetWidth();
        int bh = magBitmap.GetHeight();
        dc.DrawLine(ulMag.x-1, ulMag.y+bh, 
                    ulMag.x+bw+1, ulMag.y+bh);
        dc.DrawLine(ulMag.x+bw, ulMag.y+bh, 
                    ulMag.x+bw, ulMag.y-2);
        dc.SetPen(wxPen(wxT("WHITE"), 1, wxPENSTYLE_SOLID));
        dc.DrawLine(ulMag.x-1, ulMag.y-1, 
                    ulMag.x+bw+1, ulMag.y-1);
        dc.DrawLine(ulMag.x-1, ulMag.y+bh, 
                    ulMag.x-1, ulMag.y-2);
    }
    // choose placement of text.
    int db = 5;
    int w = toff+tw+2*tB;
    if ( maxDistLR > w + db && (!drawMag) )
    {
        tul = p + wxPoint(toff,toff);
    }
    else
    {
        if (maxDistLL > w + db)
        {
            tul = p + wxPoint(-w, toff);
        }
        else
        {
            if (maxDistUR > w + db)
            {
                tul = p + wxPoint(toff, -(toff) - (th+2*tB));
            }
            else
            {
                tul = p + wxPoint(-w, -(toff) - (th+2*tB));
            };
        };
    };

    // draw background
    dc.SetPen(wxPen(textColour, 1, wxPENSTYLE_SOLID));
    dc.SetBrush(wxBrush(bgColour, wxBRUSHSTYLE_SOLID));
    dc.DrawRectangle(tul.x, tul.y, tw+2*tB+1, th+2*tB);
    labelPos.SetLeft(tul.x);
    labelPos.SetTop(tul.y);
    labelPos.SetWidth(tw+2*tB+1);
    labelPos.SetHeight(th+2*tB);
    // draw number
    dc.SetTextForeground(textColour);
    dc.DrawText(m_label, tul + wxPoint(tB,tB));
    return labelPos;
};

void DisplayedControlPoint::DrawLine(wxDC& dc)
{
    if(m_control==NULL)
    {
        return;
    };
    FDiff2D p1, p2, dp;
    // transform end points to pano space
    if(!m_control->getFirstInvTrans()->transformImgCoord(p1, hugin_utils::FDiff2D(m_cp.x1, m_cp.y1)))
    {
        return;
    };
    if(!m_control->getFirstInvTrans()->transformImgCoord(p2, hugin_utils::FDiff2D(m_cp.x2, m_cp.y2)))
    {
        return;
    };
    dp=p2-p1;
    int len=roundi(sqrt((m_cp.x1-m_cp.x2)*(m_cp.x1-m_cp.x2)+(m_cp.y1-m_cp.y2)*(m_cp.y1-m_cp.y2)))+1;
    if(len<5)
    {
        //very short line, draw straight line
        dc.DrawLine(m_control->roundP(m_control->scale(m_control->applyRot(hugin_utils::FDiff2D(m_cp.x1, m_cp.y1)))),
            m_control->roundP(m_control->scale(m_control->applyRot(hugin_utils::FDiff2D(m_cp.x2, m_cp.y2)))));
    }
    else
    {
        //longer line, draw correct line, taking output projection into account
        wxPoint* points=new wxPoint[len+1];
        for(size_t i=0; i<len+1; i++)
        {
            FDiff2D p=p1+dp*((double)i/len);
            // transform line coordinates back to image space
            if(!m_control->getFirstTrans()->transformImgCoord(p2, p))
            {
                delete []points;
                //fall through, draw direct line, not exact, but better than no line
                dc.DrawLine(m_control->roundP(m_control->scale(m_control->applyRot(hugin_utils::FDiff2D(m_cp.x1, m_cp.y1)))),
                    m_control->roundP(m_control->scale(m_control->applyRot(hugin_utils::FDiff2D(m_cp.x2, m_cp.y2)))));
                return;
            };
            points[i]=m_control->roundP(m_control->scale(m_control->applyRot(p2)));
        };
        dc.DrawLines(len+1, points);
        delete []points;
    };
};

void DisplayedControlPoint::DrawLineSegment(wxDC& dc)
{
    if(m_control==NULL)
    {
        return;
    };
    // calculate line equation
    FDiff2D p1_image=m_mirrored ? hugin_utils::FDiff2D(m_cp.x2, m_cp.y2) : hugin_utils::FDiff2D(m_cp.x1, m_cp.y1);
    FDiff2D p2_image=m_mirrored ? hugin_utils::FDiff2D(m_cp.x1, m_cp.y1) : hugin_utils::FDiff2D(m_cp.x2, m_cp.y2);
    FDiff2D p1, p2, dp;
    if(!m_control->getFirstInvTrans()->transformImgCoord(p1, p1_image))
    {
        return;
    };
    if(!m_control->getSecondInvTrans()->transformImgCoord(p2, p2_image))
    {
        return;
    };
    dp=p2-p1;
    // now find the parameter to draw an appropriate long line segment
    double f=1.0;
    int image_width=m_control->GetRealImageSize().GetWidth();
    int image_height=m_control->GetRealImageSize().GetHeight();
    int image_dimension=std::min(image_width, image_height);
    int line_length=-1;
    while(f>1e-4)
    {
        p2=p1+dp*f;
        if(m_control->getFirstTrans()->transformImgCoord(p2_image, p2))
        {
            double length=sqrt(p1_image.squareDistance(p2_image));
            if(length > 0.05f * image_dimension && length < 0.75f * image_dimension)
            {
                line_length=hugin_utils::roundi(length);
                break;
            };
        };
        f*=0.9;
    };
    // found no suitable length, don't draw line
    if(line_length<1)
    {
        return;
    };
    // now calc line positions
    wxPoint* points=new wxPoint[2*line_length+1];
    for(size_t i=0; i<2*line_length+1; i++)
    {
        FDiff2D p=p1+dp*f*(((double)i-line_length)/(2.0f*line_length));
        if(!m_control->getFirstTrans()->transformImgCoord(p2, p))
        {
            delete []points;
            return;
        };
        points[i]=m_control->roundP(m_control->scale(m_control->applyRot(p2)));
    };
    //and finally draw line segment
    dc.SetClippingRegion(wxPoint(0,0), m_control->GetBitmapSize());
    dc.DrawLines(2*line_length+1, points);
    dc.DestroyClippingRegion();
    delete []points;
};

const bool DisplayedControlPoint::isOccupiedLabel(const wxPoint mousePos) const
{
    if(m_line)
    {
        return m_labelPos.Contains(mousePos) || m_labelPos2.Contains(mousePos);
    }
    else
    {
        return m_labelPos.Contains(mousePos);
    };
};

const bool DisplayedControlPoint::isOccupiedPos(const hugin_utils::FDiff2D &p) const
{
    double d=m_control->invScale(3.0);
    if(m_line)
    {
        return (p.x < m_cp.x1 + d && p.x > m_cp.x1 - d && p.y < m_cp.y1 + d && p.y > m_cp.y1 - d) ||
               (p.x < m_cp.x2 + d && p.x > m_cp.x2 - d && p.y < m_cp.y2 + d && p.y > m_cp.y2 - d);
    }
    else
    {
        if(m_mirrored)
        {
            return (p.x < m_cp.x2 + d && p.x > m_cp.x2 - d && p.y < m_cp.y2 + d && p.y > m_cp.y2 - d);
        }
        else
        {
            return (p.x < m_cp.x1 + d && p.x > m_cp.x1 - d && p.y < m_cp.y1 + d && p.y > m_cp.y1 - d);
        };
    };
};

void DisplayedControlPoint::CheckSelection(const wxPoint mousePos, const hugin_utils::FDiff2D& p)
{
    if(!m_line)
    {
        return;
    };
    double d=m_control->invScale(3.0);
    m_mirrored=m_labelPos2.Contains(mousePos) || 
        (p.x < m_cp.x2 + d && p.x > m_cp.x2 - d && p.y < m_cp.y2 + d && p.y > m_cp.y2 - d);
};

void DisplayedControlPoint::UpdateControlPointX(double x)
{
    if(m_mirrored)
    {
        m_cp.x2=x;
    }
    else
    {
        m_cp.x1=x;
    };
};

void DisplayedControlPoint::UpdateControlPointY(double y)
{
    if(m_mirrored)
    {
        m_cp.y2=y;
    }
    else
    {
        m_cp.y1=y;
    };
};

void DisplayedControlPoint::UpdateControlPoint(hugin_utils::FDiff2D newPoint)
{
    if(m_mirrored)
    {
        m_cp.x2=newPoint.x;
        m_cp.y2=newPoint.y;
    }
    else
    {
        m_cp.x1=newPoint.x;
        m_cp.y1=newPoint.y;
    };
};

void DisplayedControlPoint::ShiftControlPoint(hugin_utils::FDiff2D shift)
{
    if(m_mirrored)
    {
        m_cp.x2+=shift.x;
        m_cp.y2+=shift.y;
    }
    else
    {
        m_cp.x1+=shift.x;
        m_cp.y1+=shift.y;
    };
};

void DisplayedControlPoint::StartLineControlPoint(hugin_utils::FDiff2D newPoint)
{
    //start a new line control point
    m_line=true;
    m_mirrored=true;
    m_label=_("new");
    m_cp.image1Nr=UINT_MAX;
    m_cp.x1=newPoint.x;
    m_cp.y1=newPoint.y;
    m_cp.image2Nr=m_cp.image1Nr;
    m_cp.x2=m_cp.x1;
    m_cp.y2=m_cp.y1;
    m_cp.mode = HuginBase::ControlPoint::X;
};

hugin_utils::FDiff2D DisplayedControlPoint::GetPos() const
{
    return m_mirrored ? FDiff2D(m_cp.x2, m_cp.y2) : FDiff2D(m_cp.x1, m_cp.y1);
};

bool DisplayedControlPoint::operator==(const DisplayedControlPoint other)
{
    return m_cp==other.GetControlPoint() && m_mirrored == other.IsMirrored() && m_label == other.GetLabel();
};

// our image control
BEGIN_EVENT_TABLE(CPImageCtrl, wxScrolledWindow)
    EVT_SIZE(CPImageCtrl::OnSize)
    EVT_CHAR(CPImageCtrl::OnKey)
//    EVT_KEY_UP(CPImageCtrl::OnKeyUp)
    EVT_KEY_DOWN(CPImageCtrl::OnKeyDown)
    EVT_LEAVE_WINDOW(CPImageCtrl::OnMouseLeave)
    EVT_ENTER_WINDOW(CPImageCtrl::OnMouseEnter)
    EVT_MOTION(CPImageCtrl::mouseMoveEvent)
    EVT_LEFT_DOWN(CPImageCtrl::mousePressLMBEvent)
    EVT_LEFT_UP(CPImageCtrl::mouseReleaseLMBEvent)
    EVT_RIGHT_DOWN(CPImageCtrl::mousePressRMBEvent)
    EVT_RIGHT_UP(CPImageCtrl::mouseReleaseRMBEvent)
    EVT_MIDDLE_DOWN(CPImageCtrl::mousePressMMBEvent)
    EVT_MIDDLE_UP(CPImageCtrl::mouseReleaseMMBEvent)
    EVT_TIMER(-1, CPImageCtrl::OnTimer)
END_EVENT_TABLE()

bool CPImageCtrl::Create(wxWindow * parent, wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& name)
{
    wxScrolledWindow::Create(parent, id, pos, size, style, name);
    selectedPointNr = 0;
    editState = NO_IMAGE;
    scaleFactor = 1;
    fitToWindow = false;
    m_showSearchArea = false;
    m_searchRectWidth = 0;
    m_showTemplateArea = false;
    m_templateRectWidth = 0;
    m_editPanel = 0;
    m_imgRotation = ROT0;
    m_sameImage = false;

    wxString filename;

#if defined(__WXMSW__) 
    wxString cursorPath = huginApp::Get()->GetXRCPath() + wxT("/data/cursor_cp_pick.cur");
    m_CPSelectCursor = new wxCursor(cursorPath, wxBITMAP_TYPE_CUR);
#else
    m_CPSelectCursor = new wxCursor(wxCURSOR_CROSS);
#endif
    SetCursor(*m_CPSelectCursor);

    // TODO: define custom, light background colors.
    pointColors.push_back(wxTheColourDatabase->Find(wxT("BLUE")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("WHITE")));

    pointColors.push_back(wxTheColourDatabase->Find(wxT("GREEN")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("WHITE")));

    pointColors.push_back(wxTheColourDatabase->Find(wxT("CYAN")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("BLACK")));
    pointColors.push_back(wxTheColourDatabase->Find(wxT("GOLD")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("BLACK")));

    pointColors.push_back(wxTheColourDatabase->Find(wxT("NAVY")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("WHITE")));

    pointColors.push_back(wxTheColourDatabase->Find(wxT("DARK TURQUOISE")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("BLACK")));

    pointColors.push_back(wxTheColourDatabase->Find(wxT("SALMON")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("BLACK")));

    pointColors.push_back(wxTheColourDatabase->Find(wxT("MAROON")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("BLACK")));

    pointColors.push_back(wxTheColourDatabase->Find(wxT("KHAKI")));
    textColours.push_back(wxTheColourDatabase->Find(wxT("BLACK")));

    m_searchRectWidth = 120;
    m_mouseInWindow = false;
    m_forceMagnifier = false;
    m_timer.SetOwner(this);

    return true;
}

void CPImageCtrl::Init(CPEditorPanel * parent)
{
    m_editPanel = parent;
    m_sameImage = false;
}

CPImageCtrl::~CPImageCtrl()
{
    DEBUG_TRACE("dtor");
    this->SetCursor(wxNullCursor);
    delete m_CPSelectCursor;
    DEBUG_TRACE("dtor end");
}

void CPImageCtrl::OnDraw(wxDC & dc)
{
    wxSize vSize = GetClientSize();
    // draw image (FIXME, redraw only visible regions.)
    if (editState != NO_IMAGE && m_img.get()) {
		//clear the blank rectangle to the left of the image
        if (bitmap.GetWidth() < vSize.GetWidth()) {
            dc.SetPen(wxPen(GetBackgroundColour(), 1, wxPENSTYLE_SOLID));
            dc.SetBrush(wxBrush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID));
            dc.DrawRectangle(bitmap.GetWidth(), 0,
                             vSize.GetWidth() - bitmap.GetWidth(),vSize.GetHeight());
        }
		//clear the blank rectangle below the image
        if (bitmap.GetHeight() < vSize.GetHeight()) {
            dc.SetPen(wxPen(GetBackgroundColour(), 1, wxPENSTYLE_SOLID));
            dc.SetBrush(wxBrush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID));
			dc.DrawRectangle(0, bitmap.GetHeight(),
                             vSize.GetWidth(), vSize.GetHeight() - bitmap.GetHeight());
        }
        dc.DrawBitmap(bitmap,0,0);
	} else {
		// clear the rectangle and exit
        dc.SetPen(wxPen(GetBackgroundColour(), 1, wxPENSTYLE_SOLID));
        dc.SetBrush(wxBrush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID));
        dc.Clear();
		return;
	}

    // draw known points.
    for(size_t i=0; i<m_points.size(); i++)
    {
        if (!(editState == KNOWN_POINT_SELECTED && i==selectedPointNr))
        {
            m_points[i].Draw(dc, false);
        };
    }

    switch(editState) {
    case NEW_POINT_SELECTED:
        // Boundary check
        if ((newPoint.x < 0) || (newPoint.y < 0)) {
            // Tried to create a point outside of the canvas.  Ignore it.
            break;
        } 
        {
            DisplayedControlPoint dsp(HuginBase::ControlPoint(0, newPoint.x, newPoint.y, 0, 0, 0), this, false);
            dsp.SetLabel(_("new"));
            dsp.Draw(dc, false, true);
        }
        if (m_showTemplateArea) {
            dc.SetLogicalFunction(wxINVERT);
            dc.SetPen(wxPen(wxT("RED"), 1, wxPENSTYLE_SOLID));
            dc.SetBrush(wxBrush(wxT("WHITE"), wxBRUSHSTYLE_TRANSPARENT));
            wxPoint upperLeft = applyRot(roundP(newPoint));
            upperLeft = scale(upperLeft);

            int width = scale(m_templateRectWidth);

            dc.DrawRectangle(upperLeft.x-width, upperLeft.y-width, 2*width, 2*width);
            dc.SetLogicalFunction(wxCOPY);
        }

        break;
    case NEW_LINE_CREATING:
        m_selectedPoint.Draw(dc, false, true);
        break;
    case KNOWN_POINT_SELECTED:
        m_points[selectedPointNr].Draw(dc, true);
        break;
    case NO_SELECTION:
    case NO_IMAGE:
        break;
    }

    if (m_showSearchArea && m_mousePos.x != -1){
        dc.SetLogicalFunction(wxINVERT);
        dc.SetPen(wxPen(wxT("WHITE"), 1, wxPENSTYLE_SOLID));
        dc.SetBrush(wxBrush(wxT("WHITE"), wxBRUSHSTYLE_TRANSPARENT));

        FDiff2D upperLeft = applyRot(m_mousePos);
        upperLeft = scale(upperLeft);
        int width = scale(m_searchRectWidth);
        DEBUG_DEBUG("drawing rect " << upperLeft << " with width " << 2*width << " orig: " << m_searchRectWidth*2  << " scale factor: " << getScaleFactor());

        dc.DrawRectangle(roundi(upperLeft.x - width), roundi(upperLeft.y-width), 2*width, 2*width);
        dc.SetLogicalFunction(wxCOPY);
    }
}

class ScalingTransform
{
public:
    ScalingTransform(double scale)
    : m_scale(scale) {};

    bool transformImgCoord(double & sx, double & sy, double x, double y)
    {
        sx = m_scale*x;
        sy = m_scale*y;
        return true;
    }

    double m_scale;
};

wxBitmap CPImageCtrl::generateMagBitmap(FDiff2D point, wxPoint canvasPos) const
{
    typedef vigra::RGBValue<vigra::UInt8> VT;
    DEBUG_TRACE("")

    // draw magnified image (TODO: warp!)
    double magScale = 3.0;
    wxConfigBase::Get()->Read(wxT("/CPEditorPanel/MagnifierScale"), &magScale);
    // width (and height) of magnifier region (output), should be odd
    int magWidth = wxConfigBase::Get()->Read(wxT("/CPEditorPanel/MagnifierWidth"),61l);
    int hw = magWidth/2;
    magWidth = hw*2+1;

    // setup simple scaling transformation function.
    ScalingTransform transform(1.0/magScale);
    ScalingTransform invTransform(magScale);
    wxImage img(magWidth, magWidth);
    vigra::BasicImageView<VT> magImg((VT*)img.GetData(), magWidth,magWidth);
    vigra::BImage maskImg(magWidth, magWidth);
    vigra_ext::PassThroughFunctor<vigra::UInt8> ptf;


    // middle pixel
    double mx, my;
    invTransform.transformImgCoord(mx, my, point.x, point.y);

    // apply the transform
    AppBase::DummyProgressDisplay progDisp;
    vigra_ext::transformImageIntern(vigra::srcImageRange(*(m_img->image8)),
                         vigra::destImageRange(magImg),
                         vigra::destImage(maskImg),
                         transform,
                         ptf,
                         vigra::Diff2D(hugin_utils::roundi(mx - hw),
                                       hugin_utils::roundi(my - hw)),
                         vigra_ext::interp_cubic(),
                         false,
                         &progDisp,
                         false);

    // TODO: contrast enhancement
    vigra::FindMinMax<vigra::UInt8> minmax;
    vigra::inspectImage(vigra::srcImageRange(magImg), minmax);

    // transform to range 0...255
    vigra::transformImage(vigra::srcImageRange(magImg), vigra::destImage(magImg),
                          vigra::linearRangeMapping(
                            VT(minmax.min), VT(minmax.max),               // src range
                            VT(0, 0, 0), VT(255, 255, 255)) // dest range
                          );
//    vigra::transformImage(srcImageRange(magImg), destImage(magImg),
//       vigra::BrightnessContrastFunctor<float>(brightness, contrast, minmax.min, minmax.max));

    // draw cursor
    for(int x=0; x < magWidth; x++) {
        VT p =magImg(x,hw+1);
        vigra::UInt8 v = 0.3/255*p.red() + 0.6/255*p.green() + 0.1/255*p.blue() < 0.5 ? 255 : 0;
        p[0] = v;
        p[1] = v;
        p[2] = v;
        magImg(x,hw+1) = p;
        p = magImg(hw+1, x);
        v = 0.3/255*p.red() + 0.6/255*p.green() + 0.1/255*p.blue() < 0.5 ? 255 : 0;
        p[0] = v;
        p[1] = v;
        p[2] = v;
        magImg(hw+1, x) = p;
    }

    // rotate image according to current display
    switch(m_imgRotation) {
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
    return wxBitmap (img);
}

wxSize CPImageCtrl::DoGetBestSize() const
{
    return wxSize(imageSize.GetWidth(),imageSize.GetHeight());
}


void CPImageCtrl::setImage(const std::string & file, ImageRotation imgRot)
{
    DEBUG_TRACE("setting Image " << file);
    imageFilename = file;
    m_sameImage=false;
    wxString fn(imageFilename.c_str(),HUGIN_CONV_FILENAME);
    if (wxFileName::FileExists(fn)) {
        m_imgRotation = imgRot;
        m_img = ImageCache::getInstance().getImageIfAvailable(imageFilename);
        editState = NO_SELECTION;
        if (m_img.get()) {
            rescaleImage();
        } else {
            // load the image in the background.
            m_imgRequest = ImageCache::getInstance().requestAsyncImage(imageFilename);
            m_imgRequest->ready.push_back(
#ifdef HAVE_CXX11
                std::bind(&CPImageCtrl::OnImageLoaded, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
#else
                boost::bind(&CPImageCtrl::OnImageLoaded, this, _1, _2, _3)
#endif
                );
            // With m_img.get() 0, everything will act as normal except drawing.
        }
    } else {
        editState = NO_IMAGE;
        bitmap = wxBitmap();
        // delete the image (release shared_ptr)
        // create an empty image.
        m_img = ImageCache::EntryPtr(new ImageCache::Entry);
    }
}

void CPImageCtrl::setTransforms(HuginBase::PTools::Transform* firstTrans, HuginBase::PTools::Transform* firstInvTrans, HuginBase::PTools::Transform* secondInvTrans)
{
    m_firstTrans=firstTrans;
    m_firstInvTrans=firstInvTrans;
    m_secondInvTrans=secondInvTrans;
};

void CPImageCtrl::setSameImage(bool sameImage)
{
    m_sameImage=sameImage;
};

void CPImageCtrl::OnImageLoaded(ImageCache::EntryPtr entry, std::string filename, bool load_small)
{
    // check we are still displaying this image
    if (imageFilename == filename)
    {
        m_img = entry;
        rescaleImage();
    }
}

void CPImageCtrl::rescaleImage()
{
    if (editState == NO_IMAGE || !m_img.get()) {
        return;
    }
    wxImage img = imageCacheEntry2wxImage(m_img);
    if (img.GetWidth() == 0) {
        return;
    }
    imageSize = wxSize(img.GetWidth(), img.GetHeight());
    m_realSize = imageSize;
    if (fitToWindow) {
        scaleFactor = calcAutoScaleFactor(imageSize);
    }
    DEBUG_DEBUG("src image size "
                << imageSize.GetHeight() << "x" << imageSize.GetWidth());
    if (getScaleFactor() == 1.0) {
        //the icc correction would work on the original cached image file
        //therefore we need to create a copy to work on it
        img = img.Copy();
    }
    else
    {
        // rescale image
        imageSize.SetWidth(scale(imageSize.GetWidth()));
        imageSize.SetHeight(scale(imageSize.GetHeight()));
        DEBUG_DEBUG("rescaling to " << imageSize.GetWidth() << "x"
            << imageSize.GetHeight());
        img = img.Scale(imageSize.GetWidth(), imageSize.GetHeight());
    };
    // need to rotate full image. warning. this can be very memory intensive
    if (m_imgRotation != ROT0)
    {
        switch (m_imgRotation)
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
        };
    };
    // do color correction only if input image has icc profile or if we found a monitor profile
    if (!m_img->iccProfile->empty() || huginApp::Get()->HasMonitorProfile())
    {
        HuginBase::Color::CorrectImage(img, *(m_img->iccProfile), huginApp::Get()->GetMonitorProfile());
    };
    bitmap = wxBitmap(img);

    if (m_imgRotation == ROT90 || m_imgRotation == ROT270) {
        SetVirtualSize(imageSize.GetHeight(), imageSize.GetWidth());
    } else {
        SetVirtualSize(imageSize.GetWidth(), imageSize.GetHeight());
    }
    SetScrollRate(1,1);
    Refresh(FALSE);
}

void CPImageCtrl::setCtrlPoint(const HuginBase::ControlPoint& cp, const bool mirrored)
{
    DisplayedControlPoint dcp(cp, this, mirrored);
    dcp.SetColour(pointColors[m_points.size() % pointColors.size()], textColours[m_points.size() % textColours.size()]);
    dcp.SetLabel(wxString::Format(wxT("%lu"), (unsigned long int)m_points.size()));
    m_points.push_back(dcp);
}

void CPImageCtrl::clearCtrlPointList()
{
    m_points.clear();
    if(editState == KNOWN_POINT_SELECTED)
    {
        editState = NO_SELECTION;
    };
    selectedPointNr = UINT_MAX;
};

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
    if (nr < m_points.size()) {
        selectedPointNr = nr;
        editState = KNOWN_POINT_SELECTED;
        showPosition(m_points[nr].GetPos());
        update();
    } else {
        DEBUG_DEBUG("trying to select invalid point nr: " << nr << ". Nr of points: " << m_points.size());
    }
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
    // transform and scale the co-ordinate to the screen.
    point = applyRot(point);
    point = scale(point);
    int x = roundi(point.x);
    int y = roundi(point.y);

    wxSize sz = GetClientSize();
    int scrollx = x - sz.GetWidth()/2;
    int scrolly = y - sz.GetHeight()/2;
    Scroll(scrollx, scrolly);
    if (warpPointer) {
        int sx,sy;
        GetViewStart(&sx, &sy);
        DEBUG_DEBUG("relative coordinages: " << x-sx << "," << y-sy);
        WarpPointer(x-sx,y-sy);
    }
}

CPImageCtrl::EditorState CPImageCtrl::isOccupied(wxPoint mousePos, const FDiff2D &p, unsigned int & pointNr) const
{
    // check if mouse is hovering over a label
    if(m_points.size()>0)
    {
        for(int i=m_points.size()-1; i>=0; i--)
        {
            if(m_points[i].isOccupiedLabel(mousePos))
            {
                pointNr = i;
                return KNOWN_POINT_SELECTED;
            }
        };
        // check if mouse is over a known point
        for(vector<DisplayedControlPoint>::const_iterator it=m_points.begin(); it!=m_points.end(); ++it)
        {
            if(it->isOccupiedPos(p))
            {
                pointNr = it - m_points.begin();
                return KNOWN_POINT_SELECTED;
            }
        };
    };

    return NEW_POINT_SELECTED;
}

void CPImageCtrl::DrawSelectionRectangle(hugin_utils::FDiff2D pos1,hugin_utils::FDiff2D pos2)
{
    wxClientDC dc(this);
    PrepareDC(dc);
    dc.SetLogicalFunction(wxINVERT);
    dc.SetPen(wxPen(*wxWHITE,1, wxPENSTYLE_DOT));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    wxPoint p1=roundP(scale(applyRot(pos1)));
    wxPoint p2=roundP(scale(applyRot(pos2)));
    dc.DrawRectangle(p1.x,p1.y,p2.x-p1.x,p2.y-p1.y);
};

void CPImageCtrl::mouseMoveEvent(wxMouseEvent& mouse)
{
    if (!m_img.get()) return; // ignore events if no image loaded.
    wxPoint unScrolledMousePos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &unScrolledMousePos.x, & unScrolledMousePos.y);
    FDiff2D mpos(unScrolledMousePos.x, unScrolledMousePos.y);
    bool doUpdate = false;
    mpos = applyRotInv(invScale(mpos));
    wxPoint mpos_ = applyRotInv(invScale(unScrolledMousePos));
    // if mouseclick is out of image, ignore
    if ((mpos.x >= m_realSize.GetWidth() || mpos.y >= m_realSize.GetHeight()) && editState!=SELECT_DELETE_REGION)
    {
        return;
    }

//    DEBUG_DEBUG(" pos:" << mpos.x << ", " << mpos.y);
    // only if the shift key is not pressed.
    if (mouse.LeftIsDown() && ! mouse.ShiftDown()) {
        switch(editState) {
        case NO_SELECTION:
            DEBUG_DEBUG("mouse down movement without selection, in NO_SELECTION state!");
            break;
        case KNOWN_POINT_SELECTED:
            if (mpos.x >= 0 && mpos.x <= m_realSize.GetWidth()){
                m_points[selectedPointNr].UpdateControlPointX(mpos.x);
            } else if (mpos.x < 0) {
                m_points[selectedPointNr].UpdateControlPointX(0);
            } else if (mpos.x > m_realSize.GetWidth()) {
                m_points[selectedPointNr].UpdateControlPointX(m_realSize.GetWidth());
            }

            if (mpos.y >= 0 && mpos.y <= m_realSize.GetHeight()){
                m_points[selectedPointNr].UpdateControlPointY(mpos.y);
            } else if (mpos.y < 0) {
                m_points[selectedPointNr].UpdateControlPointY(0);
            } else if (mpos.y > m_realSize.GetHeight()) {
                m_points[selectedPointNr].UpdateControlPointY(m_realSize.GetHeight());
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
            doUpdate = true;
            break;
        case NEW_LINE_CREATING:
            m_selectedPoint.UpdateControlPoint(mpos);
            doUpdate = true;
            break;
        case NO_IMAGE:
            break;
        }
    }

    if ((mouse.MiddleIsDown() || mouse.ShiftDown() || mouse.m_controlDown ) && editState!=SELECT_DELETE_REGION) {
        // scrolling with the mouse
        if (m_mouseScrollPos !=mouse.GetPosition()) {
            wxPoint delta_ = mouse.GetPosition() - m_mouseScrollPos;
            double speed = (double)GetVirtualSize().GetHeight() / GetClientSize().GetHeight();
//          int speed = wxConfigBase::Get()->Read(wxT("/CPEditorPanel/scrollSpeed"),5);
            wxPoint delta;
            delta.x = roundi(delta_.x * speed);
            delta.y =  roundi(delta_.y * speed);
            // scrolling is done later
            if (mouse.ShiftDown()) {
                // emit scroll event, so that other window can be scrolled
                // as well.
                CPEvent e(this, CPEvent::SCROLLED, FDiff2D(delta.x, delta.y));
                emit(e);
            } else {
                // scroll only our window
                ScrollDelta(delta);
            }
            m_mouseScrollPos = mouse.GetPosition();
        }
    }

    if(mouse.RightIsDown() && editState==SELECT_DELETE_REGION)
    {
        //update selection rectangle
        DrawSelectionRectangle(rectStartPos,m_mousePos);
        DrawSelectionRectangle(rectStartPos,mpos);
    }
//    DEBUG_DEBUG("ImageDisplay: mouse move, state: " << editState);

    // draw a rectangle
    if (m_showSearchArea) {
        doUpdate = true;
    }

    unsigned int selPointNr;
    if (isOccupied(unScrolledMousePos, mpos, selPointNr) == KNOWN_POINT_SELECTED &&
        (! (editState == KNOWN_POINT_SELECTED && selectedPointNr == selPointNr) ) ) {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    } else {
        SetCursor(*m_CPSelectCursor);
    }

    m_mousePos = mpos;
    // repaint
    if (doUpdate) {
        update();
    }
}


void CPImageCtrl::mousePressLMBEvent(wxMouseEvent& mouse)
{
    DEBUG_DEBUG("LEFT MOUSE DOWN");
    if (!m_img.get()) return; // ignore events if no image loaded.
    //ignore left mouse button if selecting region with right mouse button
    if(editState==SELECT_DELETE_REGION) 
        return;
    wxPoint unScrolledMousePos;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &unScrolledMousePos.x, & unScrolledMousePos.y);
    FDiff2D mpos(unScrolledMousePos.x, unScrolledMousePos.y);
    mpos = applyRotInv(invScale(mpos));
    wxPoint mpos_ = applyRotInv(invScale(unScrolledMousePos));
    DEBUG_DEBUG("mousePressEvent, pos:" << mpos.x
                << ", " << mpos.y);
    // if mouseclick is out of image, ignore
    if (mpos.x >= m_realSize.GetWidth() || mpos.y >= m_realSize.GetHeight()) {
        return;
    }
    unsigned int selPointNr = 0;
    EditorState clickState = isOccupied(unScrolledMousePos, mpos, selPointNr);
    if (mouse.LeftDown() && editState != NO_IMAGE
        && mpos.x < m_realSize.x && mpos.y < m_realSize.y)
    {
        // we can always select a new point
        if (clickState == KNOWN_POINT_SELECTED) {
            DEBUG_DEBUG("click on point: " << selPointNr);
            selectedPointNr = selPointNr;
            m_points[selectedPointNr].CheckSelection(unScrolledMousePos, mpos);
            m_selectedPoint = m_points[selectedPointNr];
            editState = clickState;
            CPEvent e( this, selectedPointNr);
            m_forceMagnifier = true;
            emit(e);
        } else if (clickState == NEW_POINT_SELECTED) {
            DEBUG_DEBUG("click on new space, select new point");
            if(m_sameImage && mouse.AltDown())
            {
                editState = NEW_LINE_CREATING;
                m_selectedPoint.StartLineControlPoint(mpos);
                m_selectedPoint.SetControl(this);
            }
            else
            {
                editState = NEW_POINT_SELECTED;
            };
            newPoint = mpos;
        } else {
            DEBUG_ERROR("invalid state " << clickState << " on mouse down");
        }
//        DEBUG_DEBUG("ImageDisplay: mouse down, state change: " << oldstate
//                    << " -> " << editState);
    }
    m_mousePos = mpos;
}

void CPImageCtrl::OnTimer(wxTimerEvent & e)
{
    if (!m_img.get()) return; // ignore events if no image loaded.
    m_forceMagnifier = false;
    update();
}

void CPImageCtrl::mouseReleaseLMBEvent(wxMouseEvent& mouse)
{
    DEBUG_DEBUG("LEFT MOUSE UP");
    if (!m_img.get()) return; // ignore events if no image loaded.
    //ignore left mouse button if selecting region with right mouse button
    if(editState==SELECT_DELETE_REGION) 
        return;

    m_timer.Start(2000, true);

    wxPoint mpos_;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos_.x, & mpos_.y);
    FDiff2D mpos(mpos_.x, mpos_.y);
    mpos = applyRotInv(invScale(mpos));
    DEBUG_DEBUG("mouseReleaseEvent, pos:" << mpos.x
                << ", " << mpos.y);
    // if mouseclick is out of image, ignore
    if (mpos.x >= m_realSize.GetWidth() || mpos.y >= m_realSize.GetHeight()) {
        return;
    }
//    EditorState oldState = editState;
    if (mouse.LeftUp()) {
        switch(editState) {
        case NO_SELECTION:
            DEBUG_DEBUG("mouse release without selection");
            break;
        case KNOWN_POINT_SELECTED:
        {
            DEBUG_DEBUG("mouse release with known point " << selectedPointNr);
            if (! (m_selectedPoint == m_points[selectedPointNr]) ) {
                CPEvent e( this, CPEvent::POINT_CHANGED, selectedPointNr, m_points[selectedPointNr].GetControlPoint());
                emit(e);
            }
            break;
        }
        case NEW_POINT_SELECTED:
        {
            DEBUG_DEBUG("new Point changed (event fire): x:" << mpos.x << " y:" << mpos.y);
            // fire the wxWin event
            CPEvent e( this, newPoint);
            emit(e);
            break;
        }
        case NEW_LINE_CREATING:
        {
            //notify parent
            CPEvent e(this, CPEvent::NEW_LINE_ADDED, m_selectedPoint.GetControlPoint());
            emit(e);
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
    if (!m_img.get()) return; // ignore events if no image loaded.
    m_mouseScrollPos = mouse.GetPosition();
//    SetCursor(wxCursor(wxCURSOR_HAND));
}

void CPImageCtrl::mousePressRMBEvent(wxMouseEvent& mouse)
{
    //ignore event if no image loaded
    if(!m_img.get()) 
        return;
    wxPoint mpos_;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y, &mpos_.x, & mpos_.y);
    FDiff2D mpos(mpos_.x, mpos_.y);
    mpos = applyRotInv(invScale(mpos));
    // if mouseclick is out of image, ignore
    if (mpos.x >= m_realSize.GetWidth() || mpos.y >= m_realSize.GetHeight())
    {
        return;
    }
    if(mouse.CmdDown() && (editState==NO_SELECTION || editState==KNOWN_POINT_SELECTED || editState==NEW_POINT_SELECTED))
    {
        rectStartPos=mpos;
        editState=SELECT_DELETE_REGION;
        DrawSelectionRectangle(mpos,mpos);
    };
};

void CPImageCtrl::mouseReleaseRMBEvent(wxMouseEvent& mouse)
{
    if (!m_img.get()) return; // ignore events if no image loaded.
    wxPoint mpos_;
    CalcUnscrolledPosition(mouse.GetPosition().x, mouse.GetPosition().y,
                           &mpos_.x, & mpos_.y);
    FDiff2D mpos(mpos_.x, mpos_.y);
    mpos = applyRotInv(invScale(mpos));
    DEBUG_DEBUG("mouseReleaseEvent, pos:" << mpos.x
                << ", " << mpos.y);

    if (mouse.RightUp())
    {
        if(editState==SELECT_DELETE_REGION)
        {
            DrawSelectionRectangle(rectStartPos,mpos);
            editState=NO_SELECTION;
            CPEvent e(this,rectStartPos,mpos);
            emit(e);
        }
        else
        {
            // if mouseclick is out of image, ignore
            if (mpos.x >= m_realSize.GetWidth() || mpos.y >= m_realSize.GetHeight()) {
                return;
            }
            // set right up event
            DEBUG_DEBUG("Emitting right click (rmb release)");
            CPEvent e(this, CPEvent::RIGHT_CLICK, mpos);
            emit(e);
        }
    }
}

void CPImageCtrl::update()
{
    DEBUG_TRACE("edit state:" << editState);
    wxClientDC dc(this);
    PrepareDC(dc);
    OnDraw(dc);
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
        // keep existing scale focussed.
        rescaleImage();
    }
}

double CPImageCtrl::calcAutoScaleFactor(wxSize size)
{
    // TODO correctly autoscale rotated iamges
    int w = size.GetWidth();
    int h = size.GetHeight();
    if (m_imgRotation ==  ROT90 || m_imgRotation == ROT270) {
        int t = w;
        w = h;
        h = t;
    }

//    wxSize csize = GetClientSize();
    wxSize csize = GetSize();
    DEBUG_DEBUG("csize: " << csize.GetWidth() << "x" << csize.GetHeight() << "image: " << w << "x" << h);
    double s1 = (double)csize.GetWidth()/w;
    double s2 = (double)csize.GetHeight()/h;
    DEBUG_DEBUG("s1: " << s1 << "  s2:" << s2);
    return s1 < s2 ? s1 : s2;
}

double CPImageCtrl::getScaleFactor() const
{
    return scaleFactor;
}

void CPImageCtrl::OnSize(wxSizeEvent &e)
{
    DEBUG_TRACE("size: " << e.GetSize().GetWidth() << "x" << e.GetSize().GetHeight());
    // rescale bitmap if needed.
    if (imageFilename != "") {
        if (fitToWindow) {
            setScale(0);
        }
    }
}

void CPImageCtrl::OnKey(wxKeyEvent & e)
{
    if (!m_img.get()) return; // ignore events if no image loaded.
    DEBUG_TRACE(" OnKey, key:" << e.m_keyCode);
    wxPoint delta(0,0);
    // check for cursor keys, if control is not pressed
    if ((!e.CmdDown()) && e.GetKeyCode() == WXK_LEFT ) delta.x = -1;
    if ((!e.CmdDown()) && e.GetKeyCode() == WXK_RIGHT ) delta.x = 1;
    if ((!e.CmdDown()) && e.GetKeyCode() == WXK_UP ) delta.y = -1;
    if ((!e.CmdDown()) && e.GetKeyCode() == WXK_DOWN ) delta.y = 1;
    if ( (delta.x != 0 || delta.y != 0 ) && (e.ShiftDown() || e.CmdDown())) {
        // move to the left
        double speed = (double) GetClientSize().GetWidth()/10;
        delta.x = (int) (delta.x * speed);
        delta.y = (int) (delta.y * speed);
        if (e.ShiftDown()) {
            // emit scroll event, so that other window can be scrolled
            // as well.
            CPEvent e(this, CPEvent::SCROLLED, FDiff2D(delta.x, delta.y));
            emit(e);
        } else if (e.CmdDown()) {
            ScrollDelta(delta);
        }
    } else if (delta.x != 0 || delta.y != 0 ) {

        FDiff2D shift(delta.x/3.0, delta.y/3.0);
        // rotate shift according to current display
        double t;
        switch (m_imgRotation) {
            case ROT90:
                t = shift.x;
                shift.x = shift.y;
                shift.y = -t;
                break;
            case ROT180:
                shift.x = -shift.x;
                shift.y = -shift.y;
                break;
            case ROT270:
                t = shift.x;
                shift.x = -shift.y;
                shift.y = t;
            default:
                break;
        }
        // move control point by half a pixel, if a point is selected
        if (editState == KNOWN_POINT_SELECTED ) {
            DisplayedControlPoint updatedCp=m_points[selectedPointNr];
            updatedCp.ShiftControlPoint(shift);
            CPEvent e( this, CPEvent::POINT_CHANGED, selectedPointNr, updatedCp.GetControlPoint());
            emit(e);
            m_forceMagnifier = true;
            m_timer.Stop();
            m_timer.Start(2000, true);
        } else if (editState == NEW_POINT_SELECTED) {
            newPoint = newPoint + shift;
            // update display.
            update();
        }

    } else if (e.m_keyCode == 'a') {
        DEBUG_DEBUG("adding point with a key, faking right click");
        // faking right mouse button with "a"
        // set right up event
        CPEvent ev(this, CPEvent::RIGHT_CLICK, FDiff2D(0,0));
        emit(ev);
    } else {
        // forward some keys...
        bool forward = false;
        switch (e.GetKeyCode())
        {
            case 'g':
            case '0':
            case '1':
            case '2':
            case 'f':
            case WXK_RIGHT:
            case WXK_LEFT:
            case WXK_UP:
            case WXK_DOWN:
            case WXK_DELETE:
                forward = true;
                break;
            default:
                break;
        }

        if (forward) {
            // dangelo: I don't understand why some keys are forwarded and others are not..
            // Delete is forwared under wxGTK, and g not..
            // wxWidgets 2.6.1 using gtk 2 doesn't set the event object
            // properly.. do it here by hand
            e.SetEventObject(this);
            DEBUG_DEBUG("forwarding key " << e.GetKeyCode()
                        << " origin: id:" << e.GetId() << " obj: "
                        << e.GetEventObject());
            // forward all keys to our parent
            //GetParent()->GetEventHandler()->ProcessEvent(e);
            m_editPanel->GetEventHandler()->ProcessEvent(e);
        } else {
            e.Skip();
        }
    }
}

void CPImageCtrl::OnKeyDown(wxKeyEvent & e)
{
    DEBUG_TRACE("key:" << e.m_keyCode);
    if (!m_img.get()) return; // ignore events if no image loaded.
    if (e.m_keyCode == WXK_SHIFT || e.m_keyCode == WXK_CONTROL) {
        DEBUG_DEBUG("shift or control down, reseting scoll position");
        m_mouseScrollPos = e.GetPosition();
    }
    e.Skip();
}

void CPImageCtrl::OnMouseLeave(wxMouseEvent & e)
{
    DEBUG_TRACE("MOUSE LEAVE");
    m_mousePos = FDiff2D(-1,-1);
    m_mouseInWindow = false;
    update();
}

void CPImageCtrl::OnMouseEnter(wxMouseEvent & e)
{
    DEBUG_TRACE("MOUSE Enter, setting focus");
    m_mouseInWindow = true;
    SetFocus();
    update();
}

FDiff2D CPImageCtrl::getNewPoint()
{
    // only possible if a new point is actually selected
    // DEBUG_ASSERT(editState == NEW_POINT_SELECTED);
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
        m_searchRectWidth = (m_realSize.GetWidth() * templSearchAreaPercent) / 200;
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

wxPoint CPImageCtrl::MaxScrollDelta(wxPoint delta)
{
    int x,y;
    GetViewStart( &x, &y );

    wxSize winSize = GetClientSize();
    wxSize imgSize;
    imgSize.x = bitmap.GetWidth();
    imgSize.y = bitmap.GetHeight();
    // check for top and left border
    if (x + delta.x < 0) {
        delta.x = -x;
    }
    if (y + delta.y < 0) {
        delta.y = -y;
    }
    // check for right and bottom border
    int right = x + delta.x + winSize.x ;
    if (right > imgSize.x) {
        delta.x = imgSize.x - right;
        if (delta.x < 0) {
            delta.x = 0;
        }
    }
    int bottom = y + delta.y + winSize.y ;
    if (bottom > imgSize.y) {
        delta.y = imgSize.y - bottom;
        if (delta.y < 0) {
            delta.y = 0;
        }
    }
    return delta;
}

void CPImageCtrl::ScrollDelta(const wxPoint & delta)
{
    // TODO: adjust
    if (delta.x == 0 && delta.y == 0) {
        return;
    }
    int x,y;
    GetViewStart( &x, &y );
    x = x + delta.x;
    y = y + delta.y;
    if (x<0) x = 0;
    if (y<0) y = 0;
    Scroll( x, y);
}

const wxSize CPImageCtrl::GetBitmapSize() const
{
#if wxCHECK_VERSION(2,9,0)
    return bitmap.GetSize();
#else
    return wxSize(bitmap.GetWidth(), bitmap.GetHeight());
#endif
};

IMPLEMENT_DYNAMIC_CLASS(CPImageCtrl, wxScrolledWindow)

CPImageCtrlXmlHandler::CPImageCtrlXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *CPImageCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, CPImageCtrl)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow( cp);

    return cp;
}

bool CPImageCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("CPImageCtrl"));
}

IMPLEMENT_DYNAMIC_CLASS(CPImageCtrlXmlHandler, wxXmlResourceHandler)
