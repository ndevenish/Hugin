// -*- c-basic-offset: 4 -*-

/** @file PreviewTool.h
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

/* PreviewTool is an abstract base class for the interactive tools that work
 * with the OpenGL accelerated preview. They can respond to the users actions
 * over the preview, or changes in the panorama, and can draw extras above,
 * below, or instead of the panorama images.
 */
#ifndef _PREVIEWTOOL_H
#define _PREVIEWTOOL_H

#include "hugin_utils/utils.h"
#include <wx/event.h>

#include "PreviewToolHelper.h"

class PreviewTool
{
public:
    // when inhereting, please use this:
    PreviewTool(PreviewToolHelper *helper);
    // Lots of stub functions here. Your tools should override a few of them.
    
    /* When the user switches on the tool, Activate is called. We should
     * register events we want to respond to with the helper when activating,
     * these are lost when the tool is deactivated, but we are not notified of
     * this. Activate will be called again if the tool is reactivated, we should
     * clean up anything we need to from the previous use here. */
    virtual void Activate() = 0;
    
    // These are called when events happen, providing we requested notification.
    // The coordinates are in the space of the panorama output at full size.
    virtual void MouseMoveEvent(double x, double y, wxMouseEvent & e) {}
    virtual void MouseButtonEvent(wxMouseEvent &e) {}
    virtual void ImagesUnderMouseChangedEvent() {}
    virtual void KeypressEvent(int keycode, int modifiers, bool pressed) {}
    // In the next few events we are free to draw stuff in OpenGL.
    virtual void BeforeDrawImagesEvent() {}
    virtual void AfterDrawImagesEvent() {}
    // if the tool uses this event to draw the image itself, return false.
    // otherwise, return true and the image will be drawn normally.
    // Use this, for example, to alter the in-order drawing 
    virtual bool BeforeDrawImageEvent(unsigned int image) {return true;}
    // called just after the image was drawn normally.
    virtual void AfterDrawImageEvent(unsigned int image) {}
protected:
    PreviewToolHelper *helper;
};

#endif

