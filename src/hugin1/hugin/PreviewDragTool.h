// -*- c-basic-offset: 4 -*-
/** @file PreviewDragTool.h
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

/* This tool allows the user to change the yaw, pitch and roll of a connected
 * component of images by dragging them.
 * If there are multiple components under the pointer when the user begins to
 * drag, the lowest number one is chosen, since it is drawn on top.
 *
 * Modifiers   | Variables changed
 * none        | yaw and pitch
 * shift       | yaw or pitch, depending on biggest difference.
 * control     | roll
 *
 * You can also change only roll by using the secondary mouse button (if there
 * is one).
 *
 *(Reasoning: control click on mac is equivalent right click on other platforms,
 * no modifiers should be close to old behaviour (where no modifiers were used),
 * and shift was chosen arbitrarily.)
 */

#ifndef _PREVIEWDRAGTOOL_H
#define _PREVIEWDRAGTOOL_H

#include "PreviewTool.h"

#include <map>
#include <hugin_math/Matrix3.h>

class PreviewDragTool : public PreviewTool
{
public:
    PreviewDragTool(PreviewToolHelper *helper);
    void Activate();
    void MouseMoveEvent(double x, double y, wxMouseEvent & e);
    void MouseButtonEvent(wxMouseEvent &e);
    void AfterDrawImagesEvent();
    class AngleStore
    {
    public:
        double yaw, pitch, roll;
        void Set(HuginBase::SrcPanoImage *img);
        void Move(Matrix3 *matrix,
                  double &yaw_out,  double &pitch_out,  double &roll_out);
    };
private:
    std::map<unsigned int, AngleStore> image_angles;
    std::set<unsigned int> draging_images;
    bool drag_yaw, drag_pitch, drag_roll;
    double start_angle, shift_angle;
    hugin_utils::FDiff2D centre, start_coordinates, shift_coordinates;
    bool shift, control;
    Matrix3 rotation_matrix;
    void SetRotationMatrix(double yaw_shift, double pitch_shift,
                           double roll_shift,
                           double yaw_start, double pitch_start,
                           double roll_start);
};

#endif

