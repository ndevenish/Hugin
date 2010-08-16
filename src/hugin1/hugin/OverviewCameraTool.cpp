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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



#include "OverviewCameraTool.h"
#include "GLViewer.h"

void PanosphereOverviewCameraTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(ToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(ToolHelper::MOUSE_WHEEL, this);
    down = false;
}

void PanosphereOverviewCameraTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    if (down) {
        hugin_utils::FDiff2D pos = helper->GetMouseScreenPosition();
        PanosphereOverviewVisualizationState*  state = (PanosphereOverviewVisualizationState*) helper->GetVisualizationStatePtr();
        double b_angx = state->getAngX();
        double b_angy = state->getAngY();
        //FIXME: include a scale factor for the panosphere
        double scale = (state->getR() - state->getSphereRadius()) / 40000.0;
        state->setAngX((pos.x - start_x) * scale + start_angx);
        double ey = (pos.y - start_y) * scale + start_angy;
        if (ey >= M_PI / 2.0) {ey = M_PI / 2.0 - 0.0001;}
        if (ey <= -M_PI / 2.0) {ey = -M_PI / 2.0 + 0.0001;}
        state->setAngY(ey);
        state->Redraw();
    }
}

void PanosphereOverviewCameraTool::MouseButtonEvent(wxMouseEvent &e)
{
//    DEBUG_DEBUG("mouse ov drag button");
    if ((e.ButtonDown() && (!helper->IsMouseOverPano() || e.ControlDown() || e.AltDown())) || e.MiddleDown()) {
        down = true;
        hugin_utils::FDiff2D pos = helper->GetMouseScreenPosition();
        start_x = pos.x;
        start_y = pos.y;
        PanosphereOverviewVisualizationState*  state = (PanosphereOverviewVisualizationState*) helper->GetVisualizationStatePtr();
        start_angx = state->getAngX();
        start_angy = state->getAngY();
    }
    if (e.ButtonUp()) {
        if (down) {
            down = false;
        }
    }
}

void PanosphereOverviewCameraTool::MouseWheelEvent(wxMouseEvent &e)
{
    double scale = 1.1;
    PanosphereOverviewVisualizationState*  state = (PanosphereOverviewVisualizationState*) helper->GetVisualizationStatePtr();
    double radius = state->getSphereRadius();
    if (e.GetWheelRotation() < 0) {
        if (state->getR() < limit_high * radius) {
            state->setR((state->getR() - radius) * scale + radius);
            state->SetDirtyViewport();
            state->ForceRequireRedraw();
            state->Redraw();
        }
    }
    if (e.GetWheelRotation() > 0) {
        if (state->getR() > limit_low * radius) {
            state->setR((state->getR() - radius) / scale + radius);
            state->SetDirtyViewport();
            state->ForceRequireRedraw();
            state->Redraw();
        }
    }
}


void PlaneOverviewCameraTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(ToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(ToolHelper::MOUSE_WHEEL, this);
    down = false;
}

void PlaneOverviewCameraTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    if (down) {
        PlaneOverviewToolHelper * thelper = (PlaneOverviewToolHelper*) helper;
        PlaneOverviewVisualizationState*  state = (PlaneOverviewVisualizationState*) helper->GetVisualizationStatePtr();
//
        //same code as in tool helper to get position on the z-plane but with initial position
        double d = state->getR();

        int tcanv_w, tcanv_h;
        state->GetViewer()->GetClientSize(&tcanv_w,&tcanv_h);

        double canv_w, canv_h;
        canv_w = tcanv_w;
        canv_h = tcanv_h;
        
        double fov = state->getFOV();

        double fovy, fovx;
        if (canv_w > canv_h) {
            fovy = DEG_TO_RAD(fov);
            fovx = 2 * atan( tan(fovy / 2.0) * canv_w / canv_h);
        } else {
            fovx = DEG_TO_RAD(fov);
            fovy = 2 * atan( tan(fovx / 2.0) * canv_h / canv_w);
        }

        double vis_w, vis_h;
        vis_w = 2.0 * tan ( fovx / 2.0 ) * d;
        vis_h = 2.0 * tan ( fovy / 2.0 ) * d;

        //position of the mouse on the z=0 plane
        double prim_x, prim_y;
        prim_x = (double) x / canv_w * vis_w - vis_w / 2.0 + start_pos_x;
        prim_y = ((double) y / canv_h * vis_h - vis_h / 2.0 - start_pos_y);

//        DEBUG_DEBUG("mouse ov tool 1 " << state->getX() << " " << state->getY());
        state->setX((-prim_x + start_x) + start_pos_x);
        state->setY((prim_y - start_y) + start_pos_y);
//        DEBUG_DEBUG("mouse ov tool 2 " << state->getX() << " " << state->getY());
        state->ForceRequireRedraw();
        state->Redraw();
    }
}

void PlaneOverviewCameraTool::MouseButtonEvent(wxMouseEvent &e)
{
    PlaneOverviewToolHelper * thelper = (PlaneOverviewToolHelper*) helper;
    PlaneOverviewVisualizationState*  state = (PlaneOverviewVisualizationState*) helper->GetVisualizationStatePtr();
//    DEBUG_DEBUG("mouse ov drag button");
    if (((e.ControlDown() || e.AltDown()) && e.LeftDown()) || e.MiddleDown()) {
        down = true;
        start_x = thelper->getPlaneX();
        start_y = thelper->getPlaneY();
        start_pos_x = state->getX();
        start_pos_y = state->getY();
    }
    if (e.LeftUp() || e.MiddleUp()) {
        if (down) {
            down = false;
        }
    }
}


void PlaneOverviewCameraTool::MouseWheelEvent(wxMouseEvent &e)
{
    double scale = 1.1;
    PlaneOverviewVisualizationState*  state = (PlaneOverviewVisualizationState*) helper->GetVisualizationStatePtr();
    if (e.GetWheelRotation() < 0) {
        state->setR(state->getR() * scale);
    }
    if (e.GetWheelRotation() > 0) {
        state->setR(state->getR() / scale);
    }
    state->SetDirtyViewport();
    state->ForceRequireRedraw();
    state->Redraw();
}

