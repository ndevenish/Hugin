// -*- c-basic-offset: 4 -*-
/** @file OverviewOutlinesTool.h
 *
 *  @author Darko Makreshanski
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


#ifndef __OVERVIEW_OUTLINES_TOOL_H__
#define __OVERVIEW_OUTLINES_TOOL_H__

#include "Tool.h"

class GLViewer;

class OverviewOutlinesTool : public HuginBase::PanoramaObserver
{
    public:
        OverviewOutlinesTool(ToolHelper *, GLViewer * preview);
        virtual ~OverviewOutlinesTool();


        void panoramaChanged(HuginBase::PanoramaData &pano);
        void panoramaImagesChanged(HuginBase::PanoramaData&, const HuginBase::UIntSet&) {}

        void MouseMoveEvent(double x, double y, wxMouseEvent & e);

        class Rect {
        public:
            Rect(double left, double top, double right, double bottom) {
                val[0][0] = left;   val[0][1] = top;
                val[1][0] = left;   val[1][1] = bottom;
                val[2][0] = right;  val[2][1] = bottom;
                val[3][0] = right;  val[3][1] = top;
            }
            Rect transformImgCoord(HuginBase::PTools::Transform *trans) {
                Rect res(0,0,0,0);
                for (int s = 0 ; s < 4 ; s++) {
                    double x,y;
                    trans->transformImgCoord(x,y, val[s][0], val[s][1]);
                    res.val[s][0] = x;
                    res.val[s][1] = y;
                }
                return res;
            }
            void center(double &x, double &y) {
                x = (val[0][0] + val[1][0] + val[2][0] + val[3][0]) / 4.0;
                y = (val[0][1] + val[1][1] + val[2][1] + val[3][1]) / 4.0;
            }
            double val[4][2];
        };

        virtual void drawBackground() {}

    protected:

        void draw();

        void DrawRect(double left, double top, double right, double bottom, bool outline, double linewidth = 1.0);

        ToolHelper * thelper;

        bool dirty_meshes;

        //TODO take into consideration scale of gl canvas
        //resolution of the mesh
        const static double res = 10;
        const static double mindist = 2;
    
        GLViewer * preview;

        unsigned int display_list_number_canvas;
        unsigned int display_list_number_crop;
        unsigned int display_list_number_canvas_outline;
        unsigned int display_list_number_crop_outline;

        /* data */
};

class PanosphereOverviewOutlinesTool : public OverviewOutlinesTool, public PanosphereOverviewTool
{
    public:

        PanosphereOverviewOutlinesTool(PanosphereOverviewToolHelper* helper, GLViewer * preview) : PanosphereOverviewTool(helper), OverviewOutlinesTool(helper, preview) {}

        void Activate();
        void AfterDrawImagesBackEvent();
        void AfterDrawImagesFrontEvent();


        void drawBackground();


};

class PlaneOverviewOutlinesTool : public OverviewOutlinesTool, public PlaneOverviewTool
{
    public:

        PlaneOverviewOutlinesTool(PlaneOverviewToolHelper* helper, GLViewer * preview) : PlaneOverviewTool(helper), OverviewOutlinesTool(helper, preview) {}

        void Activate();
        void AfterDrawImagesEvent();

        void drawBackground();
    
};


#endif /* __OVERVIEW_OUTLINES_TOOL_H__ */

