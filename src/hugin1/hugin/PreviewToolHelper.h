// -*- c-basic-offset: 4 -*-

/** @file PreviewToolHelper.h
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

/* A preview tool helper manages information that any tool may want to use,
 * and guides the interaction between tools. These features are available:
 * - Notification of user events:
 *      - When the mouse moves
 *      - When the set of images under the mouse pointer changes
 *      - When the mouse button is pressed / released.
 *      - When a key is pressed / released
 * - Notifications when drawing things on the preview, allowing:
 *      - Drawing under / above the set of images.
 *      - Drawing under / above each individual image.
 *      - Replacing or stopping the drawing of each individual image.
 * - Notification of when:
 *      - The tool is activated.
 * - The tool can request the mouse position.
 * - The tool can request a list of images under the mouse.
 * - The tool can access the panorama, and make changes to it.
 * - The tool can access the ViewState. This allows it to respond to, and cause,
 *     interactive changes.
 * - The tool can deactivate itself, for example, when asked to give up events.
 * - Possible extensions:
 *      - What images have been selected
 *      - Provide a menu over the preview
 */

#ifndef _PREVIEWTOOLHELPER_H
#define _PREVIEWTOOLHELPER_H


#if __APPLE__
#include "panoinc_WX.h"
#include "panoinc.h"
#endif
#include "hugin_utils/utils.h"
#include "base_wx/platform.h"
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/string.h>

#include <set>
#include <vector>

#include <hugin_math/hugin_math.h>
#include "ViewState.h"
#include "PT/Panorama.h"

class PreviewTool;
class GLPreviewFrame;

class PreviewToolHelper
{
public:
    enum Event
    {
        MOUSE_MOVE, MOUSE_PRESS, KEY_PRESS,
        DRAW_UNDER_IMAGES, DRAW_OVER_IMAGES,
        IMAGES_UNDER_MOUSE_CHANGE, REALLY_DRAW_OVER_IMAGES
    };
    PreviewToolHelper(PT::Panorama *pano,
                      ViewState *view_state,
                      GLPreviewFrame * frame);
    ~PreviewToolHelper();
    // working with tools
    // Activate a tool, the tool will then request notifications it needs.
    // Then return a list of tools that had to be dactivated to comply.
    std::set<PreviewTool*> ActivateTool(PreviewTool *tool);
    // deactivate a tool: remove all it's notifications.
    void DeactivateTool(PreviewTool *tool);
    
    // Events
    // the x and y coordinates are in pixels from the top left of the panorama.
    void MouseMoved(int x, int y, wxMouseEvent & e);
    // pressed is true if the button is pushed down, false if let go
    void MouseButtonEvent(wxMouseEvent &e);
    // keycode is the wxWidgets keycode.
    void KeypressEvent(int keycode, int modifiers, bool pressed);
    void BeforeDrawImages();
    void AfterDrawImages();
    // Return true if we want it drawn, return false and draw the image as the
    // tools specify otherwise.
    bool BeforeDrawImageNumber(unsigned int image);
    void AfterDrawImageNumber(unsigned int image);
    void MouseLeave();
    
    // Get information
    std::set<unsigned int> GetImageNumbersUnderMouse();
    hugin_utils::FDiff2D GetMousePosition();
    ViewState *GetViewStatePtr();
    PT::Panorama *GetPanoramaPtr();
    
    // Setting up notifications
    void NotifyMe(Event event, PreviewTool *tool);
    void NotifyMeBeforeDrawing(unsigned int image_nr, PreviewTool *tool);
    void NotifyMeAfterDrawing(unsigned int image_nr, PreviewTool *tool);
    void DoNotNotifyMe(Event event, PreviewTool *tool);
    void DoNotNotifyMeBeforeDrawing(unsigned int image_nr, PreviewTool *tool);
    void DoNotNotifyMeAfterDrawing(unsigned int image_nr, PreviewTool *tool);
    
    // status message to be something relevant for a tool.
    void SetStatusMessage(wxString message);
private:
    std::set<PreviewTool *> tools_deactivated;
    PT::Panorama *pano;
    ViewState *view_state;
    GLPreviewFrame *frame;
    
    double mouse_x, mouse_y;
    
    // What tools are notified of what events.
    std::set<PreviewTool *> mouse_move_notified_tools;
    PreviewTool * mouse_button_notified_tool;
    PreviewTool * keypress_notified_tool;
    std::set<PreviewTool *> draw_under_notified_tools;
    std::set<PreviewTool *> draw_over_notified_tools;
    std::set<PreviewTool *> really_draw_over_notified_tools;
    std::set<PreviewTool *> images_under_mouse_notified_tools;
    // these are vectors: the index is the image that a single tool uses.
    std::vector<PreviewTool *> image_draw_begin_tools;
    std::vector<PreviewTool *> image_draw_end_tools;
    // stop notifying the given tool of an event.
    void RemoveTool(PreviewTool *tool, PreviewTool **single);
    void RemoveTool(PreviewTool *tool, std::set<PreviewTool *> *set);
    void RemoveTool(PreviewTool *tool, std::vector<PreviewTool *> *vector);
    void RemoveTool(PreviewTool *tool, std::vector<PreviewTool *> *vector,
                    unsigned int index);
    // set the tool up for notification, deactivating any tools in the way.
    void AddTool(PreviewTool *tool, PreviewTool **single);
    void AddTool(PreviewTool *tool, std::set<PreviewTool *> *set);
    void AddTool(PreviewTool *tool, std::vector<PreviewTool *> *vector,
                 unsigned int index);
                 
    // is the set of images under the mouse up to date?
    bool images_under_mouse_current, mouse_over_pano;
    // which images are under the mouse?
    std::set<unsigned int> images_under_mouse;
    void UpdateImagesUnderMouse();
    void InvalidateImagesUnderMouse();
};

#endif
