// -*- c-basic-offset: 4 -*-

/** @file Tool.h
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

#ifndef _PREVIEWTOOL_H
#define _PREVIEWTOOL_H

#include "hugin_utils/utils.h"
#include <wx/event.h>

#include "ToolHelper.h"

/* PreviewTool is an abstract base class for the interactive tools that work
 * with the OpenGL accelerated preview. They can respond to the users actions
 * over the preview, or changes in the panorama, and can draw extras above,
 * below, or instead of the panorama images.
 *
 * The *Event functions are only called when the Tool requested a notification.
 * Tools may do this through an instance of the PreviewToolHelper class which
 * is passed by pointer on construction. When a tool is decativated, all the
 * notifications it monitors are removed.
 */
class Tool
{
public:
    /** Construct keeping a pointer to a PreviewToolHelper.
     * Child classes should use this to ensure helper is set.
     */
    Tool(ToolHelper *helper);
    
    virtual ~Tool();
    // Lots of stub functions here. Your tools should override a few of them.
    
    /** Switch on a tool.
     * Inherited classes also register events they want to respond to with the
     * PreviewToolHelper. As events are lost when the tool is deactivated
     * without notification to the PreviewTool, things from the last
     * activation can be cleaned up here too.
     */
    virtual void Activate() = 0;
    
    /** Notify when the mouse pointer has moved over the panorama preview.
     * The coordinates are in the space of the panorama output at full size,
     * with no output cropping.
     * @param x The horizontal position of the mouse pointer in panorama pixels.
     * @param y The vertical position of the mouse pointer in panorama pixels.
     * @param e The event created by wxWidgets.
     */
    virtual void MouseMoveEvent(double x, double y, wxMouseEvent & e) {}
    /** Notify of a mouse button press on the panorama preview.
     * @param e The event created by wxWidgets.
     */
    virtual void MouseButtonEvent(wxMouseEvent &e) {}
    /** Notify of a mouse wheel event on the panorama preview.
     * @param e The event created by wxWidgets.
     */
    virtual void MouseWheelEvent(wxMouseEvent & e) {}
    /** Notify when the images directly underneath the mouse pointer have
     * changed. It is monitored by the PreviewToolHelper.
     */
    virtual void ImagesUnderMouseChangedEvent() {}
    /** Notify of a Keypress event.
     * Currently unused as the preview cannot get keyboard input focus.
     */
    virtual void KeypressEvent(int keycode, int modifiers, bool pressed) {}
    /// Draw using OpenGL anything the tool requires underneath the images.
    virtual void BeforeDrawImagesEvent() {}
    /// Draw (using OpenGL) images above the others.
    virtual void AfterDrawImagesEvent() {}
    /// Draw (using OpenGL) the overlays, e.g. crop highlights, guides.
    virtual void ReallyAfterDrawImagesEvent() {}
    /** Draw what the tool requires just before a given image is drawn.
     * This can be used to modify how the images are drawn, prevent drawing of
     * the image, or change the order of the image drawing (with another event
     * to draw the image when it is needed).
     * @return false if the tool does not want the image drawn normally,
     * or true when the image should be drawn normally.
     */
    virtual bool BeforeDrawImageEvent(unsigned int image) {return true;}
    /// Notification called just after the image was drawn normally.
    virtual void AfterDrawImageEvent(unsigned int image) {}
protected:
    /** The PreviewToolHelper that uses the same preview window and panorama as
     * the tool should.
     */
    ToolHelper *helper;
};


class PreviewTool : public Tool
{
public:
    PreviewTool(PreviewToolHelper* helper);
    virtual ~PreviewTool();

};

class OverviewTool : public Tool
{
public:

    OverviewTool(OverviewToolHelper* helper);
    virtual ~OverviewTool();

};

class PanosphereOverviewTool : public OverviewTool
{
public:

    PanosphereOverviewTool(PanosphereOverviewToolHelper* helper);
    virtual ~PanosphereOverviewTool();

    /// Draw using opengl anything before drawing the front face culled images
    virtual void BeforeDrawImagesBackEvent() {}
    /// Draw using opengl anything after drawing the front face culled images
    virtual void AfterDrawImagesBackEvent() {}

    /// Draw using opengl anything before drawing the back face culled images
    virtual void BeforeDrawImagesFrontEvent() {}
    /// Draw using opengl anything after drawing the back face culled images
    virtual void AfterDrawImagesFrontEvent() {}
    
};

class PlaneOverviewTool : public OverviewTool
{
public:

    PlaneOverviewTool(PlaneOverviewToolHelper* helper);
    virtual ~PlaneOverviewTool();

};


#endif

