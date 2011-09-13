// -*- c-basic-offset: 4 -*-

/** @file ToolHelper.cpp
 *
 *  @author James Legg
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



#include "ToolHelper.h"
#include "Tool.h"
#include "GLPreviewFrame.h"
#include "GLViewer.h"
#include "MeshManager.h"

ToolHelper::ToolHelper(PT::Panorama *pano_in,
                                     VisualizationState *visualization_state_in,
                                     GLPreviewFrame *frame_in)
{
    pano = pano_in;
    visualization_state = visualization_state_in;
    frame = frame_in;
    images_under_mouse_current = false;
    mouse_over_pano = false;
    mouse_screen_x = 0;
    mouse_screen_y = 0;
    mouse_pano_x = 0;
    mouse_pano_y = 0;
}

ToolHelper::~ToolHelper()
{
}

std::set<Tool *> ToolHelper::ActivateTool(Tool *tool)
{
    tools_deactivated.clear();
    tool->Activate();
    return tools_deactivated;
}

void ToolHelper::DeactivateTool(Tool *tool)
{
    tools_deactivated.insert(tool);
    // To deactivate it we need to give up all of its notifications.
    RemoveTool(tool, &mouse_button_notified_tools);
    RemoveTool(tool, &keypress_notified_tools);
    RemoveTool(tool, &mouse_move_notified_tools);
    RemoveTool(tool, &draw_under_notified_tools);
    RemoveTool(tool, &draw_over_notified_tools);
    RemoveTool(tool, &image_draw_begin_tools);
    RemoveTool(tool, &image_draw_end_tools);
    RemoveTool(tool, &images_under_mouse_notified_tools);
    RemoveTool(tool, &really_draw_over_notified_tools);
}

void ToolHelper::MouseMoved(int x, int y, wxMouseEvent & e)
{
    mouse_screen_x = x;
    mouse_screen_y = y;
    // now tell tools that want notification.
    std::set<Tool *>::iterator iterator;
    for (iterator = mouse_move_notified_tools.begin();
         iterator != mouse_move_notified_tools.end(); iterator++)
    {
        (*iterator)->MouseMoveEvent(mouse_screen_x, mouse_screen_y, e);
    }
    // If the mouse has moved, then we don't know what is underneath it anoymore
    InvalidateImagesUnderMouse();
}

void ToolHelper::InvalidateImagesUnderMouse()
{
    images_under_mouse_current = false;
    // if there are tools that want to know when the images under the mouse
    // pointer change, we better detect the images and notfiy them is applicable
    if (!images_under_mouse_notified_tools.empty())
    {
        // there are tools that want to know... so find out:
        std::set<unsigned int> old_images_under_mouse = images_under_mouse;
        UpdateImagesUnderMouse();
        if (old_images_under_mouse != images_under_mouse)
        {
            // The list has changed. Notifiy all tools that requested it.
            std::set<Tool *>::iterator iterator;
            for (iterator = images_under_mouse_notified_tools.begin();
                 iterator != images_under_mouse_notified_tools.end();
                 iterator++)
            {
                (*iterator)->ImagesUnderMouseChangedEvent();
            }
        }
    }
}

void ToolHelper::MouseButtonEvent(wxMouseEvent &e)
{
//    // if there is a tool monitoring mouse button presses, notify it
//    if (mouse_button_notified_tool)
//    {
//        mouse_button_notified_tool->MouseButtonEvent(e);
//    }
    std::set<Tool *>::iterator iterator;
    for (iterator = mouse_button_notified_tools.begin();
         iterator != mouse_button_notified_tools.end(); iterator++)
    {
        (*iterator)->MouseButtonEvent(e);
    }

}

void ToolHelper::MouseWheelEvent(wxMouseEvent &e)
{
    std::set<Tool *>::iterator iterator;
    for (iterator = mouse_wheel_notified_tools.begin();
         iterator != mouse_wheel_notified_tools.end(); iterator++)
    {
        (*iterator)->MouseWheelEvent(e);
    }

}


void ToolHelper::KeypressEvent(int keycode, int modifiers, bool pressed)
{
//    if (keypress_notified_tool)
//    {
//        keypress_notified_tool->KeypressEvent(keycode, modifiers, pressed);
//    }
    std::set<Tool *>::iterator iterator;
    for (iterator = keypress_notified_tools.begin();
         iterator != keypress_notified_tools.end(); iterator++)
    {
        (*iterator)->KeypressEvent(keycode, modifiers, pressed);
    }

}

void ToolHelper::BeforeDrawImages()
{
    // let all tools that want to draw under the images do so.
    std::set<Tool *>::iterator iterator;
    for (iterator = draw_under_notified_tools.begin();
         iterator != draw_under_notified_tools.end(); iterator++)
    {
        (*iterator)->BeforeDrawImagesEvent();
    }
    // Since we are drawing a new frame, lets assume something has changed,
    // however we want to keep with no images under the mouse if the mouse is
    // not on the panorama.
    if (mouse_over_pano)
    {
        InvalidateImagesUnderMouse();
    }
}

void ToolHelper::AfterDrawImages()
{
//    std::cerr << "begin" << std::endl;
    // let all tools that want to draw on top of the images do so.
    std::set<Tool *>::iterator iterator;
    for (iterator = draw_over_notified_tools.begin();
         iterator != draw_over_notified_tools.end(); iterator++)
    {
//        std::cerr << "tool after draw" << std::endl;
        (*iterator)->AfterDrawImagesEvent();
    }
//    std::cerr << "after draw images" << std::endl;
    // The overlays are done separetly to avoid errors with blending order.
    for (iterator = really_draw_over_notified_tools.begin();
         iterator != really_draw_over_notified_tools.end(); iterator++)
    {
        (*iterator)->ReallyAfterDrawImagesEvent();
    }
//    std::cerr << "really after draw images" << std::endl;
}

bool ToolHelper::BeforeDrawImageNumber(unsigned int image)
{
    if (image_draw_begin_tools.size() > image)
    {
        if (image_draw_begin_tools[image].size() > 0)
        {
            bool result = true;
            std::set<Tool *>::iterator it;
            for(it = image_draw_begin_tools[image].begin() ; it != image_draw_begin_tools[image].end() ; it++) {
                result &= (*it)->BeforeDrawImageEvent(image);
            }
            return result;
//            return image_draw_begin_tools[image]->BeforeDrawImageEvent(image);
        }
    }
    return true;
}

void ToolHelper::AfterDrawImageNumber(unsigned int image)
{
    if (image_draw_end_tools.size() > image)
    {
        if (image_draw_end_tools[image].size() > 0)
        {
            std::set<Tool *>::iterator it;
            for(it = image_draw_end_tools[image].begin() ; it != image_draw_end_tools[image].end() ; it++) {
                (*it)->BeforeDrawImageEvent(image);
            }
//            image_draw_end_tools[image]->AfterDrawImageEvent(image);
        }
    }
}

void ToolHelper::MouseLeave()
{
    // if the mouse leaves the preview, there are no images under the mouse
    // pointer anymore.
    mouse_over_pano = false;
    images_under_mouse.clear();
    images_under_mouse_current = true;
    if (!images_under_mouse_notified_tools.empty())
    {
        // notify tools that the set has changed.
        std::set<Tool *>::iterator iterator;
        for (iterator = images_under_mouse_notified_tools.begin();
             iterator != images_under_mouse_notified_tools.end();
             iterator++)
        {
            (*iterator)->ImagesUnderMouseChangedEvent();
        }        
    }
}


std::set<unsigned int> ToolHelper::GetImageNumbersUnderMouse()
{
    if (!images_under_mouse_current)
    {
        UpdateImagesUnderMouse();
    }
    return images_under_mouse;
}

hugin_utils::FDiff2D ToolHelper::GetMouseScreenPosition()
{
    return hugin_utils::FDiff2D(mouse_screen_x, mouse_screen_y);
}

hugin_utils::FDiff2D ToolHelper::GetMousePanoPosition()
{
    return hugin_utils::FDiff2D(mouse_pano_x, mouse_pano_y);
}

VisualizationState *ToolHelper::GetVisualizationStatePtr()
{
    return visualization_state;
}

ViewState *ToolHelper::GetViewStatePtr()
{
    return visualization_state->getViewState();
}

PT::Panorama *ToolHelper::GetPanoramaPtr()
{
    return pano;
}

void ToolHelper::NotifyMe(Event event, Tool *tool)
{
    switch (event)
    {
        case MOUSE_MOVE:
            AddTool(tool, &mouse_move_notified_tools);
            break;
        case MOUSE_PRESS:
            AddTool(tool, &mouse_button_notified_tools);
            break;
        case MOUSE_WHEEL:
            AddTool(tool, &mouse_wheel_notified_tools);
            break;
        case KEY_PRESS:
            AddTool(tool, &keypress_notified_tools);
            break;
        case DRAW_UNDER_IMAGES:
            AddTool(tool, &draw_under_notified_tools);
            break;
        case DRAW_OVER_IMAGES:
            AddTool(tool, &draw_over_notified_tools);
            break;
        case IMAGES_UNDER_MOUSE_CHANGE:
            AddTool(tool, &images_under_mouse_notified_tools);
            break;
        case REALLY_DRAW_OVER_IMAGES:
            AddTool(tool, &really_draw_over_notified_tools);
            break;
    }   
}

void ToolHelper::NotifyMeBeforeDrawing(unsigned int image_nr,
                                              Tool *tool)
{
    AddTool(tool, &image_draw_begin_tools, image_nr);
}

void ToolHelper::NotifyMeAfterDrawing(unsigned int image_nr,
                                             Tool *tool)
{
    AddTool(tool, &image_draw_end_tools, image_nr);
}

void ToolHelper::DoNotNotifyMe(Event event, Tool *tool)
{
    // TODO we should probably check that the tools have the notification they
    // are trying to give up, as misbehaving tools could break another tool.
    switch (event)
    {
        case MOUSE_MOVE:
            RemoveTool(tool, &mouse_move_notified_tools);
            break;
        case MOUSE_PRESS:
            RemoveTool(tool, &mouse_button_notified_tools);
            break;
        case MOUSE_WHEEL:
            RemoveTool(tool, &mouse_wheel_notified_tools);
            break;
        case KEY_PRESS:
            RemoveTool(tool, &keypress_notified_tools);
            break;
        case DRAW_UNDER_IMAGES:
            RemoveTool(tool, &draw_under_notified_tools);
            break;
        case DRAW_OVER_IMAGES:
            RemoveTool(tool, &draw_over_notified_tools);
            break;
        case IMAGES_UNDER_MOUSE_CHANGE:
            RemoveTool(tool, &images_under_mouse_notified_tools);
            break;
    }   
}

void ToolHelper::DoNotNotifyMeBeforeDrawing(unsigned int image_nr,
                                              Tool *tool)
{
    RemoveTool(tool, &image_draw_begin_tools, image_nr);
}

void ToolHelper::DoNotNotifyMeAfterDrawing(unsigned int image_nr,
                                             Tool *tool)
{
    RemoveTool(tool, &image_draw_end_tools, image_nr);
}

void ToolHelper::SetStatusMessage(wxString message)
{
    // get the GLPreviewFrame to set its status bar's text.
    frame->SetStatusMessage(message);
}


// These functions remove tools from various things that keep pointers to them
// so that they can be notified. They are called when we don't want to notify
// them anymore.

void ToolHelper::RemoveTool(Tool *tool, Tool **single)
{
    if (*single == tool)
    {
        *single = 0;
    }
}

void ToolHelper::RemoveTool(Tool *tool,
                                   std::set<Tool *> *set)
{
    std::set<Tool *>::iterator iterator = set->find(tool);
    if (iterator != set->end())
    {
        set->erase(iterator);
    }
}

void ToolHelper::RemoveTool(Tool *tool,
                                  std::vector<std::set<Tool *> > *vector)
{
    // check every item for presence.
    for (unsigned int image = 0; image < vector->size(); image++)
    {
        (*vector)[image].erase(tool);
//        if ((*vector)[image] == tool)
//        {
//            (*vector)[image] = 0;
//        }
    }
}

void ToolHelper::RemoveTool(Tool *tool,
                                  std::vector<std::set<Tool *> > *vector,
                                  unsigned int index)
{
    if ((*vector).size() > index)
    {
        (*vector)[index].erase(tool);
//        if ((*vector)[index] == tool)
//        {
//            (*vector)[index] = 0;
//        }
    }
}

void ToolHelper::AddTool(Tool *tool, Tool **single)
{
    if (*single != 0 || *single == tool)
    {
        DeactivateTool(*single);
    }
    *single = tool;
}

void ToolHelper::AddTool(Tool *tool, std::set<Tool *> *set)
{
    set->insert(tool);
}

void ToolHelper::AddTool(Tool *tool,
                               std::vector<std::set<Tool *> > *vector,
                               unsigned int index)
{
    if (vector->size() < index + 1)
    {
        // increase the size of the vector to handle enough elements for index
        // to exist
        vector->resize(index + 1, std::set<Tool*>());
    }
//    else if ((*vector)[index] && (*vector)[index] != tool)
//    {
//        // if a different tool already was doing this, deactivate it.
//        DeactivateTool((*vector)[index]);
//    };
//    (*vector)[index] = tool;
    (*vector)[index].insert(tool);
}


void PreviewToolHelper::MouseMoved(int x, int y, wxMouseEvent &e)
{
    mouse_over_pano = true;
    mouse_screen_x = x;
    mouse_screen_y = y;
    // work out where the pointer is in the panorama.
    vigra::Rect2D visible = visualization_state->GetVisibleArea();
    mouse_pano_x = (double) x / visualization_state->GetScale() + (double) visible.left();
    mouse_pano_y = (double) y / visualization_state->GetScale() + (double) visible.top();
    // now tell tools that want notification.
    std::set<Tool *>::iterator iterator;
    for (iterator = mouse_move_notified_tools.begin();
         iterator != mouse_move_notified_tools.end(); iterator++)
    {
        (*iterator)->MouseMoveEvent(mouse_pano_x, mouse_pano_y, e);
    }
    // If the mouse has moved, then we don't know what is underneath it anoymore
    InvalidateImagesUnderMouse();
}

void PreviewToolHelper::UpdateImagesUnderMouse()
{
    // We want to find out which images cover the point underneath the mouse
    // pointer.
    images_under_mouse.clear();
    if (IsMouseOverPano()) {
        unsigned int num_images = pano->getNrOfImages();
        std::set<unsigned int> displayedImages = pano->getActiveImages();
        for (unsigned int image_index = 0; image_index < num_images; image_index++)
        {
            // don't try any images that are turned off
            if (displayedImages.count(image_index))
            {
                // work out if the image covers the point under the mouse.
                HuginBase::PTools::Transform transform;
                transform.createTransform(*visualization_state->GetSrcImage(image_index),
                                          *visualization_state->GetOptions());
                double image_x, image_y;
                transform.transformImgCoord(image_x, image_y, mouse_pano_x, mouse_pano_y);
                if (visualization_state->getViewState()->GetSrcImage(image_index)->isInside(vigra::Point2D(
                                                      int(image_x), int (image_y))))
                {
                    // this image is under the mouse, add it to the set.
                    images_under_mouse.insert(image_index);
                }
            }
        }
    }
    images_under_mouse_current = true;
}

void PanosphereOverviewToolHelper::UpdateImagesUnderMouse()
{
    images_under_mouse.clear();
    if (IsMouseOverPano()) {
        unsigned int num_images = pano->getNrOfImages();
        std::set<unsigned int> displayedImages = pano->getActiveImages();
        for (unsigned int image_index = 0; image_index < num_images; image_index++)
        {
            // don't try any images that are turned off
            if (displayedImages.count(image_index))
            {
                // work out if the image covers the point under the mouse.
                HuginBase::PTools::Transform transform;
                transform.createTransform(*visualization_state->GetSrcImage(image_index),
                                          *visualization_state->GetOptions());
                double image_x, image_y;
                transform.transformImgCoord(image_x, image_y, mouse_pano_x, mouse_pano_y);
                if (visualization_state->getViewState()->GetSrcImage(image_index)->isInside(vigra::Point2D(
                                                      int(image_x), int (image_y))))
                {
                    // this image is under the mouse, add it to the set.
                    images_under_mouse.insert(image_index);
                }
            }
        }
    }
    images_under_mouse_current = true;
}

void PanosphereOverviewToolHelper::MouseMoved(int x, int y, wxMouseEvent & e)
{
    PanosphereOverviewVisualizationState * panostate = (PanosphereOverviewVisualizationState*) visualization_state;

    double d = panostate->getR();
    double r = panostate->getSphereRadius();

    int tcanv_w, tcanv_h;
    panostate->GetViewer()->GetClientSize(&tcanv_w,&tcanv_h);

    double canv_w, canv_h;
    canv_w = tcanv_w;
    canv_h = tcanv_h;
    
    double fov = panostate->getFOV();

    double fovy, fovx;
    if (canv_w > canv_h) {
        fovy = DEG_TO_RAD(fov);
        fovx = 2 * atan( tan(fovy / 2.0) * canv_w / canv_h);
    } else {
        fovx = DEG_TO_RAD(fov);
        fovy = 2 * atan( tan(fovx / 2.0) * canv_h / canv_w);
    }

    double ax = tan(fovx / 2.0) * d * (x / (canv_w / 2.0) - 1);
    double ay = tan(fovy / 2.0) * d * (y / (canv_h / 2.0) - 1);

    double a_limit = r * d / sqrt(d*d - r*r);

    if (ax*ax + ay*ay < a_limit*a_limit) {

        mouse_over_pano = true;

        double ta,tb,tc;
        ta = (ax*ax + ay*ay) / (d*d) + 1;
        tb = - 2 * (ax*ax + ay*ay) / d;
        tc = ax*ax + ay*ay - r*r;

        double pz = ( -tb + sqrt(tb*tb - 4*ta*tc) ) / ( 2 * ta );
        double px = ax * (d - pz) / d;
        double py = ay * (d - pz) / d;

        double pl = sqrt(px*px + py*py + pz*pz);

        double pang_yaw = -atan(px / pz);
        double pang_pitch = asin(py / pl);

        double ang_yaw = panostate->getAngX();
        double ang_pitch = panostate->getAngY();

        double x,y,z;
        x = sin(ang_yaw)*cos(ang_pitch);
        z = cos(ang_yaw)*cos(ang_pitch);
        y = sin(ang_pitch);

        Vector3 vec(x,y,z);

        Vector3 top(0,1,0);

        Vector3 ptop = vec.Cross(top).Cross(vec).GetNormalized();

        Vector3 tres;
        tres.x = ptop.x*(ptop.x * vec.x + ptop.y*vec.y + ptop.z*vec.z) + cos(pang_yaw) * (ptop.x * (-ptop.y * vec.y - ptop.z * vec.z) + vec.x*(ptop.y*ptop.y + ptop.z*ptop.z)) + sin(pang_yaw) * (-ptop.z*vec.y + ptop.y*vec.z);
        tres.y = ptop.y*(ptop.x * vec.x + ptop.y*vec.y + ptop.z*vec.z) + cos(pang_yaw) * (ptop.y * (-ptop.x * vec.x - ptop.z * vec.z) + vec.y*(ptop.x*ptop.x + ptop.z*ptop.z)) + sin(pang_yaw) * (-ptop.z*vec.x + ptop.x*vec.z);
        tres.z = ptop.z*(ptop.x * vec.x + ptop.y*vec.y + ptop.z*vec.z) + cos(pang_yaw) * (ptop.z * (-ptop.x * vec.x - ptop.y * vec.y) + vec.z*(ptop.x*ptop.x + ptop.y*ptop.y)) + sin(pang_yaw) * (-ptop.y*vec.x + ptop.x*vec.y);

        Vector3 pside = ptop.Cross(tres).GetNormalized();
        Vector3 res;

        res.x = pside.x*(pside.x * tres.x + pside.y*tres.y + pside.z*tres.z) + cos(pang_pitch) * (pside.x * (-pside.y * tres.y - pside.z * tres.z) + tres.x*(pside.y*pside.y + pside.z*pside.z)) + sin(pang_pitch) * (-pside.z*tres.y + pside.y*tres.z);
        res.y = pside.y*(pside.x * tres.x + pside.y*tres.y + pside.z*tres.z) + cos(pang_pitch) * (pside.y * (-pside.x * tres.x - pside.z * tres.z) + tres.y*(pside.x*pside.x + pside.z*pside.z)) + sin(-pang_pitch) * (-pside.z*tres.x + pside.x*tres.z);
        res.z = pside.z*(pside.x * tres.x + pside.y*tres.y + pside.z*tres.z) + cos(pang_pitch) * (pside.z * (-pside.x * tres.x - pside.y * tres.y) + tres.z*(pside.x*pside.x + pside.y*pside.y)) + sin(pang_pitch) * (-pside.y*tres.x + pside.x*tres.y);

        double yaw, pitch;
        pitch = asin(res.y);
        yaw = atan2(res.x , res.z);

        yaw += M_PI / 2.0;
        if (yaw < 0) yaw += 2 * M_PI;
        if (yaw > 2 * M_PI) yaw -= 2 * M_PI;

        pitch += M_PI / 2.0;
        pitch = M_PI - pitch;

        mouse_pano_x = yaw / (2 * M_PI) * panostate->GetOptions()->getWidth();
        mouse_pano_y = pitch / (M_PI) * panostate->GetOptions()->getHeight();

////        cerr << "mouse " << RAD_TO_DEG(yaw) << " " << RAD_TO_DEG(pitch) << " " << " ; " << RAD_TO_DEG(pang_yaw) << " " << RAD_TO_DEG(pang_pitch) << " ; " << RAD_TO_DEG(ang_yaw) << " " << RAD_TO_DEG(ang_pitch) << " ; " << px << " " << py << " " << pz << " ; " << ax << " " << ay);

//        cerr << "mouse " << mouse_pano_x << " " << mouse_pano_y << " ; " << canv_w << " " << canv_h);

//        double t_mouse_pano_x = yaw M

    } else {

        mouse_over_pano = false;
    }
    
    ToolHelper::MouseMoved(x,y,e);
}

PanosphereOverviewToolHelper::PanosphereOverviewToolHelper(PT::Panorama *pano,
                  VisualizationState *visualization_state,
                  GLPreviewFrame * frame) : OverviewToolHelper(pano, visualization_state, frame) {}

PanosphereOverviewToolHelper::~PanosphereOverviewToolHelper() {}

void PanosphereOverviewToolHelper::NotifyMe(PanosphereOverviewEvent event, PanosphereOverviewTool * tool) {
    switch (event) {
        case DRAW_OVER_IMAGES_BACK:
            AddTool(tool, &draw_over_notified_tools_back);
        break;
        case DRAW_OVER_IMAGES_FRONT:
            AddTool(tool, &draw_over_notified_tools_front);
        break;
        case DRAW_UNDER_IMAGES_BACK:
            AddTool(tool, &draw_under_notified_tools_back);
        break;
        case DRAW_UNDER_IMAGES_FRONT:
            AddTool(tool, &draw_under_notified_tools_front);
        break;
    }
}

void PanosphereOverviewToolHelper::DoNotNotifyMe(PanosphereOverviewEvent event, PanosphereOverviewTool * tool) {
    switch (event) {
        case DRAW_OVER_IMAGES_BACK:
            RemoveTool(tool, &draw_over_notified_tools_back);
        break;
        case DRAW_OVER_IMAGES_FRONT:
            RemoveTool(tool, &draw_over_notified_tools_front);
        break;
        case DRAW_UNDER_IMAGES_BACK:
            RemoveTool(tool, &draw_under_notified_tools_back);
        break;
        case DRAW_UNDER_IMAGES_FRONT:
            RemoveTool(tool, &draw_under_notified_tools_front);
        break;
    }
}

void PanosphereOverviewToolHelper::BeforeDrawImagesBack()
{
    std::set<Tool *>::iterator iterator;
    for (iterator = draw_under_notified_tools_back.begin();
         iterator != draw_under_notified_tools_back.end(); iterator++)
    {
        ((PanosphereOverviewTool*)(*iterator))->BeforeDrawImagesBackEvent();
    }
}

void PanosphereOverviewToolHelper::BeforeDrawImagesFront()
{
    std::set<Tool *>::iterator iterator;
    for (iterator = draw_under_notified_tools_front.begin();
         iterator != draw_under_notified_tools_front.end(); iterator++)
    {
        ((PanosphereOverviewTool*)(*iterator))->BeforeDrawImagesFrontEvent();
    }
}

void PanosphereOverviewToolHelper::AfterDrawImagesBack()
{
    std::set<Tool *>::iterator iterator;
    for (iterator = draw_over_notified_tools_back.begin();
         iterator != draw_over_notified_tools_back.end(); iterator++)
    {
        ((PanosphereOverviewTool*)(*iterator))->AfterDrawImagesBackEvent();
    }
}

void PanosphereOverviewToolHelper::AfterDrawImagesFront()
{
    std::set<Tool *>::iterator iterator;
    for (iterator = draw_over_notified_tools_front.begin();
         iterator != draw_over_notified_tools_front.end(); iterator++)
    {
        ((PanosphereOverviewTool*)(*iterator))->AfterDrawImagesFrontEvent();
    }
}

void PanosphereOverviewToolHelper::DeactivateTool(Tool *tool)
{
    ToolHelper::DeactivateTool(tool);

    RemoveTool(tool, &draw_under_notified_tools_back);
    RemoveTool(tool, &draw_under_notified_tools_front);
    RemoveTool(tool, &draw_over_notified_tools_back);
    RemoveTool(tool, &draw_over_notified_tools_front);
}


PlaneOverviewToolHelper::PlaneOverviewToolHelper(PT::Panorama *pano,
                  VisualizationState *visualization_state,
                  GLPreviewFrame * frame) : OverviewToolHelper(pano, visualization_state, frame) {}

PlaneOverviewToolHelper::~PlaneOverviewToolHelper() {}

void PlaneOverviewToolHelper::MouseMoved(int x, int y, wxMouseEvent & e)
{

    PlaneOverviewVisualizationState * panostate = (PlaneOverviewVisualizationState*) visualization_state;

    double d = panostate->getR();

    int tcanv_w, tcanv_h;
    panostate->GetViewer()->GetClientSize(&tcanv_w,&tcanv_h);

    double canv_w, canv_h;
    canv_w = tcanv_w;
    canv_h = tcanv_h;
    
    double fov = panostate->getFOV();

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
    prim_x = (double) x / canv_w * vis_w - vis_w / 2.0 + panostate->getX();
    prim_y = ((double) y / canv_h * vis_h - vis_h / 2.0 - panostate->getY());

//    std::cout << "mouse ov" << plane_x << " " << plane_y << std::endl;
    plane_x = prim_x;
    plane_y = prim_y;

    double width, height;
    HuginBase::PanoramaOptions * opts = panostate->GetOptions();
    width = opts->getWidth();
    height = opts->getHeight();

    mouse_pano_x = prim_x / MeshManager::PlaneOverviewMeshInfo::scale * width + width / 2.0;
    mouse_pano_y = prim_y / MeshManager::PlaneOverviewMeshInfo::scale * width + height / 2.0;

//    std::cout << "plane mouse " << mouse_pano_x << " " << mouse_pano_y << " ; " << prim_x << " " << prim_y << " ; " << width << " " << height << std::endl;
//    cerr << "plane mouse " << mouse_pano_x << " " << mouse_pano_y);

    mouse_over_pano = true;
    
    ToolHelper::MouseMoved(x,y,e);
}

void PlaneOverviewToolHelper::UpdateImagesUnderMouse()
{
    images_under_mouse.clear();
    if (IsMouseOverPano()) {
        unsigned int num_images = pano->getNrOfImages();
        std::set<unsigned int> displayedImages = pano->getActiveImages();
        for (unsigned int image_index = 0; image_index < num_images; image_index++)
        {
            // don't try any images that are turned off
            if (displayedImages.count(image_index))
            {
                // work out if the image covers the point under the mouse.
                HuginBase::PTools::Transform transform;
                transform.createTransform(*visualization_state->GetSrcImage(image_index),
                                          *visualization_state->GetOptions());
                double image_x, image_y;
                transform.transformImgCoord(image_x, image_y, mouse_pano_x, mouse_pano_y);
                if (visualization_state->getViewState()->GetSrcImage(image_index)->isInside(vigra::Point2D(
                                                      int(image_x), int (image_y))))
                {
                    // this image is under the mouse, add it to the set.
                    images_under_mouse.insert(image_index);
                }
            }
        }
    }
    images_under_mouse_current = true;

}


