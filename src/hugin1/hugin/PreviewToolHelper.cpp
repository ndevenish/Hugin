// -*- c-basic-offset: 4 -*-

/** @file PreviewToolHelper.cpp
 *
 *  @author James Legg
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


#include "PreviewToolHelper.h"
#include "PreviewTool.h"
#include "GLPreviewFrame.h"

PreviewToolHelper::PreviewToolHelper(PT::Panorama *pano_in,
                                     ViewState *view_state_in,
                                     GLPreviewFrame *frame_in)
{
    pano = pano_in;
    view_state = view_state_in;
    frame = frame_in;
    mouse_button_notified_tool = 0;
    keypress_notified_tool = 0;
    images_under_mouse_current = false;
    mouse_over_pano = true;
}

PreviewToolHelper::~PreviewToolHelper()
{
}

std::set<PreviewTool *> PreviewToolHelper::ActivateTool(PreviewTool *tool)
{
    tools_deactivated.clear();
    tool->Activate();
    return tools_deactivated;
}

void PreviewToolHelper::DeactivateTool(PreviewTool *tool)
{
    tools_deactivated.insert(tool);
    // To deactivate it we need to give up all of its notifications.
    RemoveTool(tool, &mouse_button_notified_tool);
    RemoveTool(tool, &keypress_notified_tool);
    RemoveTool(tool, &mouse_move_notified_tools);
    RemoveTool(tool, &draw_under_notified_tools);
    RemoveTool(tool, &draw_over_notified_tools);
    RemoveTool(tool, &image_draw_begin_tools);
    RemoveTool(tool, &image_draw_end_tools);
    RemoveTool(tool, &images_under_mouse_notified_tools);
    RemoveTool(tool, &really_draw_over_notified_tools);
}

void PreviewToolHelper::MouseMoved(int x, int y, wxMouseEvent & e)
{
    mouse_over_pano = true;
    // work out where the pointer is in the panorama.
    vigra::Rect2D visible = view_state->GetVisibleArea();
    mouse_x = (double) x / view_state->GetScale() + (double) visible.left();
    mouse_y = (double) y / view_state->GetScale() + (double) visible.top();
    // now tell tools that want notification.
    std::set<PreviewTool *>::iterator iterator;
    for (iterator = mouse_move_notified_tools.begin();
         iterator != mouse_move_notified_tools.end(); iterator++)
    {
        (*iterator)->MouseMoveEvent(mouse_x, mouse_y, e);
    }
    // If the mouse has moved, then we don't know what is underneath it anoymore
    InvalidateImagesUnderMouse();
}

void PreviewToolHelper::InvalidateImagesUnderMouse()
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
            std::set<PreviewTool *>::iterator iterator;
            for (iterator = images_under_mouse_notified_tools.begin();
                 iterator != images_under_mouse_notified_tools.end();
                 iterator++)
            {
                (*iterator)->ImagesUnderMouseChangedEvent();
            }
        }
    }
}

void PreviewToolHelper::MouseButtonEvent(wxMouseEvent &e)
{
    // if there is a tool monitoring mouse button presses, notify it
    if (mouse_button_notified_tool)
    {
        mouse_button_notified_tool->MouseButtonEvent(e);
    }
}

void PreviewToolHelper::KeypressEvent(int keycode, int modifiers, bool pressed)
{
    if (keypress_notified_tool)
    {
        keypress_notified_tool->KeypressEvent(keycode, modifiers, pressed);
    }
}

void PreviewToolHelper::BeforeDrawImages()
{
    // let all tools that want to draw under the images do so.
    std::set<PreviewTool *>::iterator iterator;
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

void PreviewToolHelper::AfterDrawImages()
{
    // let all tools that want to draw on top of the images do so.
    std::set<PreviewTool *>::iterator iterator;
    for (iterator = draw_over_notified_tools.begin();
         iterator != draw_over_notified_tools.end(); iterator++)
    {
        (*iterator)->AfterDrawImagesEvent();
    }
    // The overlays are done separetly to avoid errors with blending order.
    for (iterator = really_draw_over_notified_tools.begin();
         iterator != really_draw_over_notified_tools.end(); iterator++)
    {
        (*iterator)->ReallyAfterDrawImagesEvent();
    }
}

bool PreviewToolHelper::BeforeDrawImageNumber(unsigned int image)
{
    if (image_draw_begin_tools.size() > image)
    {
        if (image_draw_begin_tools[image])
        {
            return image_draw_begin_tools[image]->BeforeDrawImageEvent(image);
        }
    }
    return true;
}

void PreviewToolHelper::AfterDrawImageNumber(unsigned int image)
{
    if (image_draw_end_tools.size() > image)
    {
        if (image_draw_end_tools[image])
        {
            image_draw_end_tools[image]->AfterDrawImageEvent(image);
        }
    }
}

void PreviewToolHelper::MouseLeave()
{
    // if the mouse leaves the preview, there are no images under the mouse
    // pointer anymore.
    mouse_over_pano = false;
    images_under_mouse.clear();
    images_under_mouse_current = true;
    if (!images_under_mouse_notified_tools.empty())
    {
        // notify tools that the set has changed.
        std::set<PreviewTool *>::iterator iterator;
        for (iterator = images_under_mouse_notified_tools.begin();
             iterator != images_under_mouse_notified_tools.end();
             iterator++)
        {
            (*iterator)->ImagesUnderMouseChangedEvent();
        }        
    }
}


std::set<unsigned int> PreviewToolHelper::GetImageNumbersUnderMouse()
{
    if (!images_under_mouse_current)
    {
        UpdateImagesUnderMouse();
    }
    return images_under_mouse;
}

hugin_utils::FDiff2D PreviewToolHelper::GetMousePosition()
{
    return hugin_utils::FDiff2D(mouse_x, mouse_y);
}

ViewState *PreviewToolHelper::GetViewStatePtr()
{
    return view_state;
}

PT::Panorama *PreviewToolHelper::GetPanoramaPtr()
{
    return pano;
}

void PreviewToolHelper::NotifyMe(Event event, PreviewTool *tool)
{
    switch (event)
    {
        case MOUSE_MOVE:
            AddTool(tool, &mouse_move_notified_tools);
            break;
        case MOUSE_PRESS:
            AddTool(tool, &mouse_button_notified_tool);
            break;
        case KEY_PRESS:
            AddTool(tool, &keypress_notified_tool);
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

void PreviewToolHelper::NotifyMeBeforeDrawing(unsigned int image_nr,
                                              PreviewTool *tool)
{
    AddTool(tool, &image_draw_begin_tools, image_nr);
}

void PreviewToolHelper::NotifyMeAfterDrawing(unsigned int image_nr,
                                             PreviewTool *tool)
{
    AddTool(tool, &image_draw_end_tools, image_nr);
}

void PreviewToolHelper::DoNotNotifyMe(Event event, PreviewTool *tool)
{
    // TODO we should probably check that the tools have the notification they
    // are trying to give up, as misbehaving tools could break another tool.
    switch (event)
    {
        case MOUSE_MOVE:
            RemoveTool(tool, &mouse_move_notified_tools);
            break;
        case MOUSE_PRESS:
            RemoveTool(tool, &mouse_button_notified_tool);
            break;
        case KEY_PRESS:
            RemoveTool(tool, &keypress_notified_tool);
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

void PreviewToolHelper::DoNotNotifyMeBeforeDrawing(unsigned int image_nr,
                                              PreviewTool *tool)
{
    RemoveTool(tool, &image_draw_begin_tools, image_nr);
}

void PreviewToolHelper::DoNotNotifyMeAfterDrawing(unsigned int image_nr,
                                             PreviewTool *tool)
{
    RemoveTool(tool, &image_draw_end_tools, image_nr);
}

void PreviewToolHelper::SetStatusMessage(wxString message)
{
    // get the GLPreviewFrame to set its status bar's text.
    frame->SetStatusMessage(message);
}


// These functions remove tools from various things that keep pointers to them
// so that they can be notified. They are called when we don't want to notify
// them anymore.

void PreviewToolHelper::RemoveTool(PreviewTool *tool, PreviewTool **single)
{
    if (*single == tool)
    {
        *single = 0;
    }
}

void PreviewToolHelper::RemoveTool(PreviewTool *tool,
                                   std::set<PreviewTool *> *set)
{
    std::set<PreviewTool *>::iterator iterator = set->find(tool);
    if (iterator != set->end())
    {
        set->erase(iterator);
    }
}

void PreviewToolHelper::RemoveTool(PreviewTool *tool,
                                  std::vector<PreviewTool *> *vector)
{
    // check every item for presence.
    for (unsigned int image = 0; image < vector->size(); image++)
    {
        if ((*vector)[image] == tool)
        {
            (*vector)[image] = 0;
        }
    }
}

void PreviewToolHelper::RemoveTool(PreviewTool *tool,
                                  std::vector<PreviewTool *> *vector,
                                  unsigned int index)
{
    if ((*vector).size() > index)
    {
        if ((*vector)[index] == tool)
        {
            (*vector)[index] = 0;
        }
    }
}

void PreviewToolHelper::AddTool(PreviewTool *tool, PreviewTool **single)
{
    if (*single != 0 || *single == tool)
    {
        DeactivateTool(*single);
    }
    *single = tool;
}

void PreviewToolHelper::AddTool(PreviewTool *tool, std::set<PreviewTool *> *set)
{
    set->insert(tool);
}

void PreviewToolHelper::AddTool(PreviewTool *tool,
                               std::vector<PreviewTool *> *vector,
                               unsigned int index)
{
    if (vector->size() < index + 1)
    {
        // increase the size of the vector to handle enough elements for index
        // to exist
        vector->resize(index + 1, 0);
    }
    else if ((*vector)[index] && (*vector)[index] != tool)
    {
        // if a different tool already was doing this, deactivate it.
        DeactivateTool((*vector)[index]);
    };
    (*vector)[index] = tool;
}


void PreviewToolHelper::UpdateImagesUnderMouse()
{
    // We want to find out which images cover the point underneath the mouse
    // pointer.
    images_under_mouse.clear();
    unsigned int num_images = pano->getNrOfImages();
    std::set<unsigned int> displayedImages = pano->getActiveImages();
    for (unsigned int image_index = 0; image_index < num_images; image_index++)
    {
        // don't try any images that are turned off
        if (displayedImages.count(image_index))
        {
            // work out if the image covers the point under the mouse.
            HuginBase::PTools::Transform transform;
            transform.createTransform(*view_state->GetSrcImage(image_index),
                                      *view_state->GetOptions());
            double image_x, image_y;
            transform.transformImgCoord(image_x, image_y, mouse_x, mouse_y);
            if (view_state->GetSrcImage(image_index)->isInside(vigra::Point2D(
                                                  int(image_x), int (image_y))))
            {
                // this image is under the mouse, add it to the set.
                images_under_mouse.insert(image_index);
            }
        }
    }
    images_under_mouse_current = true;
}

