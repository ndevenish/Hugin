// -*- c-basic-offset: 4 -*-
/** @file Plot2D.h
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

#ifndef _PLOT2D_H
#define _PLOT2D_H
//---------------------------------------------------------------------------

#include <vector>
//#include <stack>

/** a basic plot widget.
 *
 *  Can plot only one curve. the axis limits can be set or calculated automatically.
 * 
 *  The contrib/plot stuff shipped with wxWidgets has a very strange behaviour and
 *  design.
 *  I can't think of a situtation where using it can be used...
 *
 *  This based on code I recycled from some university assignment I wrote
 *  several years ago.
 *
 */
class Plot2DWindow : public wxWindow
{
protected:
        //    TPen *FAPen;               // Datenelement fr den Stift
//    TBrush *FABrush;           // Datenelement fr den Pinsel
//    TPen *FGPen;               // Datenelement fr den Stift
//    TBrush *FGBrush;           // Datenelement fr den Pinsel
    std::vector <FDiff2D> m_points;
    // axis properties
    double x_min,y_min, x_max, y_max;
    double m_XSpacing;
    double m_YSpacing;
    int m_axisDivLen;
    bool m_axisEqual;
    bool m_autosize;
    int m_borderLeft;
    int m_borderRight;
    int m_borderTop;
    int m_borderBottom;

    wxPen m_curvePen;
    wxPen m_axisPen;
    wxPen m_gridPen;

//    void calcBoundingBox();
    wxPoint ToScreen(const FDiff2D & p, const FDiff2D & scale) const;
//    void adjustRange();


    void OnPaint(wxPaintEvent &event);
    void Paint(wxDC & dc);
    void PaintAxis(wxDC & dc, const FDiff2D & scale);
    double AxisRound(double d) const ;

    void Invalidate();

public:
    Plot2DWindow(wxWindow * parent, int id=-1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);
    
    ~Plot2DWindow();
    
    /** set the curve points manually */
    void SetPoints(std::vector<FDiff2D> &);

    /** plot a function */
    template <class Functor>
    void Plot(Functor & f, double startx, double endx, int nPoints=100)
    {
        m_points.resize(nPoints);
        double deltax = (endx-startx)/nPoints;
        for (int i=0; i < nPoints; i++) {
            m_points[i] = FDiff2D(startx, f(startx));
            startx +=deltax;
        }
        if (m_autosize) {
            AutoSizeAxis();
        } else {
            Invalidate();
        }
    }

    /** set axis limits manually */
    void SetAxis(double xstart, double xend,
                 double ystart, double yend);

    /** calculate the axis limits based on the current curve
     */
    void AutoSizeAxis();

    /** set the spacing factor between the x and y labels */
    void SetXSpacing(double x)
    {
        m_XSpacing = x;
        Invalidate();
    }

    void SetYSpacing(double y)
    {
        m_YSpacing = y;
        Invalidate();
    }

    /** size of tick marks on the axis */
    void SetAxisDivLen(int x);

    /** width/height of border around the plot space */
    void SetBorder(int x, int y);

    /** equal scaling factor on x and y axis */
    void AxisEqual(bool b){m_axisEqual=b;Invalidate();};

    /** controls if the axis limits should be updated after each plot command */
    void SetAutosize(bool a) {m_autosize = a;};
private:
    DECLARE_EVENT_TABLE()

};
//---------------------------------------------------------------------------
#endif
