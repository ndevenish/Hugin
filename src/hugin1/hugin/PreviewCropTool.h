// -*- c-basic-offset: 4 -*-

/** @file PreviewCropTool.h
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PREVIEWCROPTOOL_H
#define _PREVIEWCROPTOOL_H

#include "Tool.h"

/** Allow the user to change the cropping region by dragging it in the fast
 *  preview.
 */
class PreviewCropTool : public PreviewTool
{
public:
    explicit PreviewCropTool(PreviewToolHelper *helper);
    void Activate();
    void ReallyAfterDrawImagesEvent();
    void MouseMoveEvent(double x, double y, wxMouseEvent & e);
    void MouseButtonEvent(wxMouseEvent &e);
private:
    bool moving_left, moving_right, moving_top, moving_bottom, mouse_down;
    double top, bottom, left, right;
    double start_drag_x, start_drag_y;
    HuginBase::PanoramaOptions start_drag_options, opts;
    // region of intrest while displayed during change by dragging.
    vigra::Rect2D new_roi;
};

#endif
