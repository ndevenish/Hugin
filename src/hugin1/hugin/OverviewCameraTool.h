// -*- c-basic-offset: 4 -*-
/** @file OverviewCameraTool.h
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#ifndef __OVERVIEW_CAMERA_TOOL_H__
#define __OVERVIEW_CAMERA_TOOL_H__

#include "Tool.h"
#include "ToolHelper.h"


/**
 * tool for the manipulation of the opengl 'camera' properties
 * It handles rotation of the camera position around the panosphere and
 * zooming in/out (i.e. moving the camera closer or further away from the panosphere)
 * TODO add manipulation of the FOV of the camera, i.e. the perspective
 */
class PanosphereOverviewCameraTool : public PanosphereOverviewTool
{
    public:
        explicit PanosphereOverviewCameraTool(PanosphereOverviewToolHelper* helper) : PanosphereOverviewTool (helper) {}
        virtual ~PanosphereOverviewCameraTool() {}

        void Activate();

        void MouseMoveEvent(double x, double y, wxMouseEvent & e);
        void MouseButtonEvent(wxMouseEvent &e);
        void MouseWheelEvent(wxMouseEvent &);
        
        void ChangeZoomLevel(bool zoomIn, double scale = 1.1);
        void KeypressEvent(int keycode, int modifiers, bool pressed);

    private:

        //lower limit for the distance of the camera with respect to the sphere radius
        static const double limit_low;
        //upper limit for the distance of the camera with respect to the sphere radius
        static const double limit_high;

        bool down;
        //starting position of the mouse
        double start_x, start_y;
        //starting position of the camera
        double start_angx, start_angy;
};

/**
 * tool for manipulation of the opengl 'camera' properties
 * It handles the position of the camera in 3 dimensions.
 * dragging with middle click or ctrl + left click for adjusting the XY position
 * and mouse wheel for adjusting Z position (zooming in/out)
 */
class PlaneOverviewCameraTool : public PlaneOverviewTool
{
    public:
        explicit PlaneOverviewCameraTool(PlaneOverviewToolHelper * helper) : PlaneOverviewTool (helper) {}
        virtual ~PlaneOverviewCameraTool() {}

        void Activate();

        void MouseMoveEvent(double x, double y, wxMouseEvent & e);
        void MouseButtonEvent(wxMouseEvent &e);
        void MouseWheelEvent(wxMouseEvent &);
        
        void ChangeZoomLevel(bool zoomIn, double scale = 1.1);
        void KeypressEvent(int keycode, int modifiers, bool pressed);

    private:

        int counter;
        bool down;
        //starting position of the mouse
        double start_x, start_y;
        //starting position of the camera
        double start_pos_x, start_pos_y;
};

#endif /* __OVERVIEW_DRAG_TOOL_H__ */

