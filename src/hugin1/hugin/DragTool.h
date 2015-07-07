// -*- c-basic-offset: 4 -*-
/** @file DragTool.h
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PREVIEWDRAGTOOL_H
#define _PREVIEWDRAGTOOL_H

#include "Tool.h"

#include <map>
#include <hugin_math/Matrix3.h>

/** Allows the user to change the yaw, pitch and roll of a connected
 * component of images by dragging them in the fast preview.
 * If there are multiple components under the pointer when the user begins to
 * drag, the lowest number one is chosen, since it is drawn on top.
 * <TABLE>
 * <tr> <td> Modifiers </td> <td> Variables changed </td> </tr>
 * <tr> <td> none      </td> <td> yaw and pitch     </td> </tr>
 * <tr> <td> shift     </td> <td> yaw or pitch, depending on biggest difference. </td> </tr>
 * <tr> <td> control   </td> <td> roll </td> </tr>
 * </TABLE>
 *
 * You can also change only roll by using the secondary mouse button (if there
 * is one).
 *
 * Reasoning: control click on mac is equivalent right click on other platforms,
 * no modifiers should be close to old behaviour (where no modifiers were used),
 * alt moves windows on some window managers, leaving shift as the only common
 * modifier suitable for constrained drag.
 */
class DragTool : public Tool
{
public:
    explicit DragTool(ToolHelper *helper);
    void Activate();
    void MouseMoveEvent(double x, double y, wxMouseEvent & e);
    void MouseButtonEvent(wxMouseEvent &e);

    virtual void ReallyAfterDrawImagesEvent() = 0;

    class ParamStore
    {
    public:
        double yaw, pitch, roll, TrX, TrY, TrZ, Tpy, Tpp;
        void Set(HuginBase::SrcPanoImage *img);
        void Move(Matrix3 *matrix,
                  double &yaw_out,  double &pitch_out,  double &roll_out, double &TrX_out, double &TrY_out, double &TrZ_out,
                  double &Tpy_out, double &Tpp_out);
    };

	//dragging mode
	enum DragMode {
		drag_mode_normal,
		drag_mode_mosaic
	};
    void setDragMode(DragMode drag_mode);
    DragMode getDragMode();
    
    void getTranslationShift(double &delta_x, double &delta_y);
    
protected:
    std::map<unsigned int, ParamStore> image_params;
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
    DragMode drag_mode;
    bool custom_drag;
};

class PreviewDragTool : public DragTool
{
public:
    explicit PreviewDragTool(PreviewToolHelper *helper) : DragTool(helper) {}

    void ReallyAfterDrawImagesEvent(); 

};

class OverviewDragTool : public DragTool
{
public:
    explicit OverviewDragTool(OverviewToolHelper *helper) : DragTool(helper) {}

    void ReallyAfterDrawImagesEvent();

};

#endif

