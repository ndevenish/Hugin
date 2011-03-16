// -*- c-basic-offset: 4 -*-
/** @file PreviewIdentifyTool.cpp
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

#ifndef _PREVIEWIDENTIFYTOOL_H
#define _PREVIEWIDENTIFYTOOL_H

#include "Tool.h"
#include <set>

class GLPreviewFrame;

/** Visually connect the image numbers with the image on the preview.
 * There are two ways it does this:
 * -# When the user moves the mouse pointer over a image button, the image is
 *    highlighted in the preview.
 * -# When the user moves the mouse pointer over the preview, the image under
 *    the pointer are highlighted with matching colours over the buttons.
 * 
 * The highlighted images are drawn on top of the other images, with a
 * coloured border.
 * If the mouse is over exactly two images, a click opens the control point
 * editor with those two images shown.
 */
class PreviewIdentifyTool : public Tool
{
public:
    PreviewIdentifyTool(ToolHelper *helper, GLPreviewFrame *owner);
    ~PreviewIdentifyTool();
    void Activate();
    void ImagesUnderMouseChangedEvent();
    void AfterDrawImagesEvent();
    bool BeforeDrawImageEvent(unsigned int image);
    /** Notification for when moving the mouse on an image button.
     * @param image the image number of the image the mouse is on.
     */
    void ShowImageNumber(unsigned int image);
    /// Notification for when moving the mouse off an image button.
    void StopShowingImages();
    /// Show control point editor if mouse is over two images.
    void MouseButtonEvent(wxMouseEvent & e);

    void MouseMoveEvent(double x, double y, wxMouseEvent & e);
    
    void KeypressEvent(int keycode, int modifiers, int pressed);
    
    void setConstantOn(bool constant_on_in);
    
    void UpdateWithNewImageSet(std::set<unsigned int> new_image_set);
    void ForceRedraw();
private:
    /// Generate a colour given how many colours we need and an index.
    void HighlightColour(unsigned int index, unsigned int count,
                        unsigned char &red, unsigned char &green,
                        unsigned char &blue);
    static bool texture_created;
    /// OpenGL texture name for the circular border texture.
    static unsigned int circle_border_tex;
    /// OpenGL texture name for the rectangular border texture.
    static unsigned int rectangle_border_tex;
    /// Set of image numbers of the images we are displaying highlighted.
    std::set<unsigned int> image_set;
    GLPreviewFrame *preview_frame;
    /// The image the user last placed their mouse over the button for
    unsigned int mouse_over_image;
    bool mouse_is_over_button;

    void StopUpdating();
    void ContinueUpdating();

    bool stopUpdating;
    //user has clicked and is holding left button while over panorama
    bool holdLeft;
    
    bool holdControl;

    bool constantOn;
    
};

#endif

