// -*- c-basic-offset: 4 -*-

/** @file PreviewPanel.cpp
 *
 *  @brief implementation of PreviewPanel Class
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

#include <config.h>

#include "panoinc_WX.h"
#include "panoinc.h"

#include <common/math.h>

#include "Plot2D.h"


#include <math.h>

using namespace std;
using namespace hugin_utils;

BEGIN_EVENT_TABLE(Plot2DWindow, wxWindow)
//    EVT_PAINT(CPImageCtrl::OnPaint)
//    EVT_LEFT_DOWN(CPImageCtrl::mousePressEvent)
//    EVT_MOTION(CPImageCtrl::mouseMoveEvent)
//    EVT_LEFT_UP(CPImageCtrl::mouseReleaseEvent)
//    EVT_SIZE(Plot2DWindow::OnResize)
    EVT_PAINT ( Plot2DWindow::OnPaint )
END_EVENT_TABLE()

Plot2DWindow::Plot2DWindow(wxWindow * parent, int id,
                            const wxPoint& pos, const wxSize& size,
                            long style)
 : wxWindow(parent, id, pos, size, style)
{
    x_min=0;
    y_min=0;
    x_max=1;
    y_max=1;

    m_XSpacing=7;
    m_YSpacing=3;
    m_axisDivLen= 4;

    // this should be based on the font size.
    // but I'm too lazy to code everything dynamically
    m_borderLeft=60;
    m_borderRight = 15;
    m_borderTop = 8;
    m_borderBottom=20;

    m_axisEqual=false;
    m_autosize = false;

    m_curvePen = *wxRED_PEN;
    m_axisPen = *wxBLACK_PEN;
    m_gridPen = wxPen(*wxBLACK, 1, wxDOT);
}

//---------------------------------------------------------------------------
Plot2DWindow::~Plot2DWindow()
{
//    delete FABrush;
//    delete FAPen;
//    delete FGBrush;
//    delete FGPen;
}


void Plot2DWindow::OnPaint(wxPaintEvent & e)
{
    wxPaintDC dc(this);
    Paint(dc);
}

//---------------------------------------------------------------------------
void Plot2DWindow::Paint(wxDC & dc)
{
    int width;
    int height;
    GetClientSize( &width, &height);

    double x_range = x_max - x_min;
    double y_range = y_max - y_min;

    // need to get correct scaling of the axis..
    FDiff2D scale;
    scale.x = (width - (m_borderLeft + m_borderRight))/x_range;
    scale.y = (height-(m_borderTop + m_borderBottom))/y_range;
    if (m_axisEqual) {
        if (scale.x > scale.y) {
            scale.x = scale.y;
        } else {
            scale.y = scale.x;
        }
    }

    PaintAxis(dc, scale);

    if (m_points.size() < 2) return;
    wxPoint * screenPoints = new wxPoint[m_points.size()];
    for (unsigned i=0; i < m_points.size(); i++) {
        screenPoints[i] = ToScreen(m_points[i], scale);
    }

    dc.SetPen(m_curvePen);
    dc.DrawLines(m_points.size(), screenPoints);
    delete[] screenPoints;
}

//---------------------------------------------------------------------------
void Plot2DWindow::SetPoints(std::vector<FDiff2D> & n_points)
{
    m_points = n_points;
    Invalidate();
}
//---------------------------------------------------------------------------

wxPoint Plot2DWindow::ToScreen(const FDiff2D & p, const FDiff2D & scale) const
{
    wxPoint mp;
    mp.x = roundi(scale.x*(p.x - x_min) + m_borderLeft);
    mp.y = roundi(scale.y*(y_max - p.y) + m_borderTop);
    return mp;
}

void Plot2DWindow::SetAxis(double xstart, double xend,
                           double ystart, double yend)
{
    x_min = xstart;
    x_max = xend;
    y_min = ystart;
    y_max = yend;

// DGSW FIXME - Unreferenced
//	double x_range = x_max - x_min;
//	double y_range = y_max - y_min;

    // expand axis a little bit.
//    x_min -= x_range * 0.05;
//    x_max += x_range * 0.05;
//    y_min -= y_range * 0.05;
//    y_max += y_range * 0.05;
    Invalidate();
}

//---------------------------------------------------------------------------
void Plot2DWindow::AutoSizeAxis()
{
    x_min = y_min = DBL_MAX;
    x_max = y_max = DBL_MIN;
    for(std::vector<FDiff2D>::iterator i = m_points.begin(); i != m_points.end(); i++){
        if (i->x < x_min) x_min = i->x;
        if (i->x > x_max) x_max = i->x;
        if (i->y < y_min) y_min = i->y;
        if (i->y > y_max) y_max = i->y;
    }
    double x_range = x_max - x_min;
    double y_range = y_max - y_min;
    if(y_range == 0){
        y_range=x_range;
        y_max +=y_range/2;
        y_min -=y_range/2;
    }
    // extend range to find sensible axis for functions near 0
//    x_min -= x_range * 0.05;
//    x_max += x_range * 0.05;
//    y_min -= y_range * 0.05;
//    y_max += y_range * 0.05;
    Invalidate();
}

//---------------------------------------------------------------------------
void Plot2DWindow::PaintAxis(wxDC & dc, const FDiff2D & scale)
{
    double x_range = x_max - x_min;
    double y_range = y_max - y_min;

    double x_axis_y;
    // select placement of x axis..
//    if (y_min * y_max < 0){
//        x_axis_y=0;
//    }else{
        x_axis_y=y_min;
//    }
    int client_width;
    int client_height;
    GetClientSize( &client_width, &client_height);

    // draw big x axis

    wxPoint p1 = ToScreen(FDiff2D(x_min,x_axis_y), scale);
    wxPoint p2 = ToScreen(FDiff2D(x_max,x_axis_y), scale);
    dc.SetPen(m_axisPen);
    dc.DrawLine(p1.x, p1.y,  p2.x, p2.y);

    // try to find maximum text width..
// DGSW FIXME - Unreferenced
//	    int charWidth = dc.GetCharWidth();
    int charHeight = dc.GetCharHeight();

    wxCoord text_w,text_h;
    wxString number;
    // this is a hack to work around the rounding problems of %f in wxString.
    number.Printf( _T("%g"), x_min);
    dc.GetTextExtent(number, &text_w, &text_h);

    double tw = (double)text_w * m_XSpacing;
    double s = x_range/((double)(client_width-(m_borderLeft+m_borderRight))/tw);

    double e =  floor(log10(s));
    s = AxisRound(s / pow(10.0,e)) * pow(10.0,e);
    int i = utils::roundi(x_min / s);
    // used to paint nice numbers
    while(i*s < x_max + 1e-10 && s > 1e-5){
        // draw x axis divisions
        wxPoint divPoint = ToScreen(FDiff2D(i*s, x_axis_y), scale);
        dc.SetPen(m_axisPen);
        dc.DrawLine(divPoint.x, divPoint.y, divPoint.x, divPoint.y + m_axisDivLen);
        dc.SetPen(m_gridPen);
        dc.DrawLine(divPoint.x, divPoint.y, divPoint.x, 1);

        wxString label;
        label.Printf( _T("%g"), i*s);
        dc.GetTextExtent(label, &text_w, &text_h);
        divPoint = divPoint + wxPoint(-text_w/2, m_axisDivLen +2);
        dc.DrawText( label, divPoint.x, divPoint.y);

        i++;
    }

    double y_axis_x;
    if (x_min * x_max < 0){
        y_axis_x=0;
    }else{
        y_axis_x=x_min;
    }
    // draw big y axis
    p1 = ToScreen(FDiff2D(y_axis_x,y_min), scale);
    p2 = ToScreen(FDiff2D(y_axis_x,y_max), scale);
    dc.SetPen(m_axisPen);
    dc.DrawLine(p1.x, p1.y,  p2.x, p2.y);

    s = y_range/((double)client_height/charHeight) * m_YSpacing;
    e =  floor(log10(s));
    s = AxisRound(s / pow(10.0,e)) * pow(10.0,e);
    i = utils::roundi(y_min / s);
    while(i*s < y_max + 1e-10 && s > 1e-5){
        // draw y axis divisions
        wxPoint divPoint = ToScreen(FDiff2D(y_axis_x,i*s), scale);
        dc.SetPen(m_axisPen);
        dc.DrawLine(divPoint.x, divPoint.y, divPoint.x -m_axisDivLen, divPoint.y);
        dc.SetPen(m_gridPen);
        // draw grid.
        dc.DrawLine(divPoint.x, divPoint.y, client_width, divPoint.y);

        wxString label;
        label.Printf( _T("%g"), i*s );
        dc.GetTextExtent(label, &text_w, &text_h);
        divPoint = divPoint + wxPoint(-m_axisDivLen -text_w - 3, -text_h/2);
        dc.DrawText( label, divPoint.x, divPoint.y);

        i++;
    }
}

void Plot2DWindow::SetAxisDivLen(int x)
{
    m_axisDivLen=x;
    Invalidate();
}

void Plot2DWindow::SetBorder(int x, int y)
{
    m_borderLeft=x;
    m_borderTop=y;
    Invalidate();
}

double Plot2DWindow::AxisRound(double d) const
{
    if (d < 1.5) return 1;
    if (d < 3) return 2;
    return 5;
}

//redraw the window
void Plot2DWindow::Invalidate()
{
    Refresh(); 
}

#if 0

void Plot2DWindow::adjustRange()
{
    // same scale for x & y axis?
    if (m_axisEqual){
        int width, height;
        GetSize(width, height);
        double screenratio = (double)width/(double)height;
        double paperratio = x_range/y_range;
        if (paperratio > screenratio){
            // extend y_range
            y_max = y_min = y_min + y_range/2;
            y_range = x_range/screenratio;
            y_max +=y_range/2;
            y_min -=y_range/2;
}else{
            // extend x_range
            x_max = x_min = x_min + x_range/2;
            x_range = y_range*screenratio;
            x_max +=x_range/2;
            x_min -=x_range/2;
}
}
}

#endif
