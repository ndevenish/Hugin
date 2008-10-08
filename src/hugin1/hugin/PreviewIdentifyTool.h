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

/* The PreviewIdentifyTool connects the image numbers with the image on the
 * preview. There are two ways it does this:
 * 1. When the user moves the mouse pointer over a image button, the image is
 *    highlighted in the preview.
 * 2. When the user moves the mouse pointer over the preview, the image under
 *    the pointer are highlighted with matching colours over the buttons.
 * The "highlighted" images are drawn on top of the other images, with a
 * coloured border.
 */

#ifndef _PREVIEWIDENTIFYTOOL_H
#define _PREVIEWIDENTIFYTOOL_H

#include "PreviewTool.h"
#include <set>

class GLPreviewFrame;

class PreviewIdentifyTool : public PreviewTool
{
public:
    PreviewIdentifyTool(PreviewToolHelper *helper, GLPreviewFrame *owner);
    ~PreviewIdentifyTool();
    void Activate();
    void ImagesUnderMouseChangedEvent();
    void AfterDrawImagesEvent();
    bool BeforeDrawImageEvent(unsigned int image);
    // these are called when the user moves the mouse over the image buttons.
    void ShowImageNumber(unsigned int image); // mouse on
    void StopShowingImages(); // mouse off
    void MouseButtonEvent(wxMouseEvent & e);
private:
    // generate a colour given how many colours we need and an index.
    void HighlightColour(unsigned int index, unsigned int count,
                        unsigned char &red, unsigned char &green,
                        unsigned char &blue);
    // the OpenGL texture name for the two borders:
    unsigned int circle_border_tex, rectangle_border_tex;
    // the set of image numbers of the images we are displaying.
    std::set<unsigned int> image_set;
    GLPreviewFrame *preview_frame;
    // the image the use last placed their mouse over the toggle button for:
    unsigned int mouse_over_image;
    bool mouse_is_over_button;
};

#endif

