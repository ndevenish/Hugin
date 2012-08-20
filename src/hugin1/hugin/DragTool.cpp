// -*- c-basic-offset: 4 -*-
/** @file DragTool.cpp
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

#include <config.h>
#include "panoinc_WX.h"
#include "panoinc.h"

#include "DragTool.h"
#include "CommandHistory.h"
#include "PT/PanoCommand.h"
#include "PT/ImageGraph.h"

#include <math.h>
#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "GLPreviewFrame.h"

DragTool::DragTool(ToolHelper *helper)
    : Tool(helper)
{
}

void DragTool::setDragMode(PreviewDragTool::DragMode dmode)
{
	drag_mode = dmode;
}

DragTool::DragMode DragTool::getDragMode()
{
	return drag_mode;
}

//find Tr parameters shift based on polar coordinates
void DragTool::getTranslationShift(double &delta_x, double &delta_y)
{
	double startx = atan(DEG_TO_RAD(start_coordinates.x));
	double starty = atan(DEG_TO_RAD(start_coordinates.y));
	double shiftx = atan(DEG_TO_RAD(shift_coordinates.x));
	double shifty = atan(DEG_TO_RAD(shift_coordinates.y));
	delta_x = shiftx - startx;
	delta_y = shifty - starty;
}


void DragTool::Activate()
{
    drag_yaw = false; drag_pitch = false; drag_roll = false;
    shift = false; control = false;
    // register notifications
    helper->NotifyMe(PreviewToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(PreviewToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(PreviewToolHelper::REALLY_DRAW_OVER_IMAGES, this);
    // a handy message for the user:
    helper->SetStatusMessage(_("Drag to move images (optionally use shift to constrain), or roll with right-drag or Ctrl-drag."));
}

void DragTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
    hugin_utils::FDiff2D pos = helper->GetMousePanoPosition();
    x = pos.x;
    y = pos.y;
    if (drag_yaw || drag_pitch || drag_roll)
    {
        

        // how far are we moving?
        if (drag_yaw || drag_pitch)
        {

            double yaw, pitch;
            helper->GetVisualizationStatePtr()->GetProjectionInfo()->ImageToAngular(yaw,
                                               pitch, x, y);
            shift_coordinates.x = yaw;
            shift_coordinates.y = pitch;
            shift = e.m_shiftDown;
            if (shift)
            {
                if (abs(shift_coordinates.x - start_coordinates.x)
                    < abs(shift_coordinates.y - start_coordinates.y))
                {
                    shift_coordinates.x = start_coordinates.x;
                    helper->SetStatusMessage(_("Currently constrained to moving only pitch. Make a larger movement in the opposite direction to constrain to yaw."));
                    
                } else {
                    shift_coordinates.y = start_coordinates.y;
                    helper->SetStatusMessage(_("Currently constrained to moving only yaw. Make a larger movement in the opposite direction to constrain to pitch."));
                }
            }

        }
        if (drag_roll)
        {
            shift_angle = atan2(y - centre.y, x- centre.x) - start_angle;
        }

        // move the selected images on the tempory copies for display.
        // first calculate a matrix representing the transformation
        if (drag_mode == drag_mode_mosaic) {
        	//if in mosaic drag mode do not adjust yaw and pitch, just roll
			SetRotationMatrix(0,0,shift_angle, 0,0,0);
		} else {
		    SetRotationMatrix(shift_coordinates.x, shift_coordinates.y, shift_angle,
		                      start_coordinates.x,  start_coordinates.y,  0.0);
		}     

	    std::map<unsigned int, ParamStore>::iterator i;
	    for (i = image_params.begin(); i != image_params.end(); i++ )
	    {
	        HuginBase::SrcPanoImage img = *helper->GetViewStatePtr()->
	                                                      GetSrcImage(i->first);
	        double new_yaw, new_pitch, new_roll, new_TrX, new_TrY;
	
        	if (drag_mode == drag_mode_mosaic) {
        		//if in mosaic mode shift TrX and TrY parameters based on modified yaw and pitch
	        	double shift_x, shift_y;
				getTranslationShift(shift_x, shift_y);
				i->second.TrX = img.getX() + shift_x;
				i->second.TrY = img.getY() + shift_y;
			}
	        i->second.Move(&rotation_matrix, new_yaw, new_pitch, new_roll, new_TrX, new_TrY);
        	img.setX(new_TrX);
        	img.setY(new_TrY);
	    	img.setYaw(new_yaw); 
	    	img.setPitch(new_pitch); 
        	img.setRoll(new_roll);
	        helper->GetViewStatePtr()->SetSrcImage(i->first, &img);
	    }
                          
        // redraw
        helper->GetVisualizationStatePtr()->Redraw();
    }
}

void DragTool::MouseButtonEvent(wxMouseEvent &e)
{
    if (e.ButtonDown())
    {

        if (helper->IsMouseOverPano()) {
        
            control = e.m_controlDown;
            shift = e.m_shiftDown;
            
            if (control && (helper->GetPreviewFrame()->individualDragging())) {
                
                std::set<unsigned int> images_under_mouse = helper->GetImageNumbersUnderMouse();
                std::set<unsigned int>::iterator it;
                for(it = images_under_mouse.begin() ; it != images_under_mouse.end() ; it++) {
                    if (shift) {
                        helper->GetPreviewFrame()->RemoveImageFromDragGroup(*it);
                    } else {
                        helper->GetPreviewFrame()->AddImageToDragGroup(*it);
                    }
                }
                return;
            
            }
            
            switch (e.GetButton())
            {
                // primary button
                case wxMOUSE_BTN_LEFT:
                    // different things depending on modifier keys.

                    // Either no key modifiers we care about, or shift.
                    // With shift we determine an adaptive constraint based on
                    // movement in both directions.
                    drag_yaw = true; drag_pitch = true;
                    break;
                case wxMOUSE_BTN_RIGHT:
                    drag_roll = true;
                    break;
            }
            if (drag_roll)
            {
                // set centre and angle
                helper->GetViewStatePtr()->GetProjectionInfo()->AngularToImage(
                                                      centre.x, centre.y, 0.0, 0.0);
                centre.x += 0.5;
                centre.y += 0.5;
                hugin_utils::FDiff2D angular = helper->GetMousePanoPosition() - centre;
                start_angle = atan2(angular.y, angular.x);
                shift_angle = 0.0;
                // we'll always rotate around the centre of the panorama.
                start_coordinates.x = 0.0;    start_coordinates.y = 0.0;
                shift_coordinates.x = 0.0;    shift_coordinates.y = 0.0;
                helper->SetStatusMessage(_("Rotate around the centre to roll."));
            }
            if (drag_yaw || drag_pitch)
            {
                // We want to keep the point under the mouse pointer now under there
                // wherever it goes. We'll calculate the roll, pitch, and yaw
                // required to bring the centre to the point under the mouse. Then
                // we rotate the panorama's images using the yaw and pitch of the
                // movement. (Rotate start point to the centre, rotate by difference
                // in yaw and pitch gained so far while dragging, then do the
                // inverse of the rotation from start point to the centre on the
                // result.
                // set angles
                double yaw, pitch;
                hugin_utils::FDiff2D mouse_pos = helper->GetMousePanoPosition();
                helper->GetVisualizationStatePtr()->GetProjectionInfo()->ImageToAngular(yaw,
                                                   pitch, mouse_pos.x, mouse_pos.y);
                start_coordinates.x = yaw;    start_coordinates.y = pitch;
                shift_coordinates.x = yaw;    shift_coordinates.y = pitch;
                // provide a helpfull message to the user via the staus bar.
                if (shift) {
                    helper->SetStatusMessage(_("Constrained drag: make a movement and it will be snapped to the yaw or pitch."));
                } else {
                    helper->SetStatusMessage(_("Drag to move."));
                }
            }
            if (drag_roll || drag_yaw || drag_pitch)
            {
                shift_angle = 0.0;
                // record where the images are so we know what the difference is.
                // Use the component the mouse points to instead of every image.
                // Find the components

                /*
                    TODO: depending on state from the preview frame is not nice. 
                    but this cannot be avoided in this case as the drag tool must depend on a central state, 
                    which in this case is neither provided by the possibility of a central (parent) DragTool 
                    nor a parent ToolHelper. 
                    This is also the case for the identify tools, which inorder to interact with eachother
                    must rely on a central tool.
                    So the code at some point need to be refactored to provide central tool helper 
                    and central tools, similarly as the ViewState is now central 
                    compared to the VisualizationState which is specific to each visualization.
                */
                bool customDragging = helper->GetPreviewFrame()->individualDragging();
                if (customDragging) {
                    draging_images = helper->GetPreviewFrame()->GetDragGroupImages();
                    std::set<unsigned int>::iterator i, end;
                    end = draging_images.end();
                    for (i = draging_images.begin(); i != end; i++)
                    {
                        image_params[*i].Set(
                                helper->GetViewStatePtr()->GetSrcImage(*i));
                    }
                    
                } else {


                    PT::CPGraph graph;
                    PT::createCPGraph(*helper->GetPanoramaPtr(), graph);
                    PT::CPComponents components;
                    unsigned int n = PT::findCPComponents(graph, components);
                    // If there is only component, we can drag everything. Otherwise the
                    // component we want is the lowest numbered image under the mouse.
                    if (n == 1)
                    {
                        unsigned int imgs = helper->GetPanoramaPtr()->getNrOfImages();
                        draging_images.clear();
                        fill_set(draging_images, 0, imgs - 1);

                        ViewState *view_state_ptr = helper->GetViewStatePtr();
                        for (unsigned int i = 0; i < imgs; i++)
                        {
                            image_params[i].Set(view_state_ptr->GetSrcImage(i));
                        };
                    } else
                    {
                        // multiple components or none at all.
                        if (n == 0 || helper->GetImageNumbersUnderMouse().empty())
                        {
                            // we can't drag nothing.
                            drag_roll = false; drag_yaw = false; drag_pitch = false;
                            return;
                        }
                        
                        // Find the component containing the topmost image under mouse
                        unsigned int img = *helper->GetImageNumbersUnderMouse().begin();
                        for (unsigned int component_index = 0;
                             component_index < components.size(); component_index ++)
                        {
                            if (components[component_index].count(img))
                            {
                                // Found it, record which images and where they are.
                                draging_images = components[component_index];
                                std::set<unsigned int>::iterator i, end;
                                end = draging_images.end();
                                for (i = draging_images.begin(); i != end; i++)
                                {
                                    image_params[*i].Set(
                                            helper->GetViewStatePtr()->GetSrcImage(*i));
                                }
                                break;
                            }
                        }
                    }
                }
                SetRotationMatrix(shift_coordinates.x, shift_coordinates.y,
                                  shift_angle,
                                  start_coordinates.x,  start_coordinates.y,  0.0);
            }
        }
    } else {
        // check this wasn't an attempt to drag empty space.
        if (! (drag_pitch || drag_roll || drag_yaw)) return;
        
        // Finished draging images:
        drag_yaw = false; drag_pitch = false; drag_roll = false;
        // Apply the rotations permanently.
        // find where the images end up.
        std::vector<HuginBase::SrcPanoImage> src_images(draging_images.size() + 1);
        std::map<unsigned int, ParamStore>::iterator i;
        unsigned int count = 0;
        for (i = image_params.begin(); i != image_params.end(); i++)
        {
            double nyaw, npitch, nroll, nx, ny;
            i->second.Move(&rotation_matrix, nyaw, npitch, nroll, nx, ny);
            src_images[count] = helper->GetPanoramaPtr()->getSrcImage(i->first);
			src_images[count].setX(nx);
			src_images[count].setY(ny);
	        src_images[count].setYaw(nyaw);
	        src_images[count].setPitch(npitch);
	        src_images[count].setRoll(nroll);
	        count++;
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateSrcImagesCmd(*helper->GetPanoramaPtr(),
                                            draging_images,
                                            src_images)
            );
        // stop dragging
        image_params.clear();
        
        helper->SetStatusMessage(_("Drag to move images (optionally use shift to constrain), or roll with right-drag or ctrl-drag."));
    }
}



void DragTool::ParamStore::Set(HuginBase::SrcPanoImage *img)
{
    yaw = img->getYaw();
    pitch = img->getPitch();
    roll = img->getRoll();
    TrX = img->getX();
    TrY = img->getY();
}

void DragTool::ParamStore::Move(Matrix3 *matrix,
                                       double &yaw_out, double &pitch_out,
                                       double &roll_out, double &TrX_out, double &TrY_out)
{
    Matrix3 start, output_matrix;
    // convert the location of this image to a matrix.
    start.SetRotationPT(DEG_TO_RAD(yaw), DEG_TO_RAD(pitch), DEG_TO_RAD(roll));
    // move it by the matrix specified.
    output_matrix = *matrix * start;
    // get the angles from the matrix
    output_matrix.GetRotationPT(yaw_out, pitch_out, roll_out);
    yaw_out = RAD_TO_DEG(yaw_out);
    pitch_out = RAD_TO_DEG(pitch_out);
    roll_out = RAD_TO_DEG(roll_out);
    
	//for non zero Tr parameters correspondence can be kept only for rolling, so adjust TrX and TrY parameters accordingly
    Vector3 vec(0, TrY, TrX);
    Vector3 res = matrix->TransformVector(vec);
    TrX_out = res.z;
    TrY_out = res.y;
    
}

void DragTool::SetRotationMatrix(double yaw_shift, double pitch_shift,
                                        double roll_shift,
                                        double yaw_start, double pitch_start,
                                        double roll_start)
{
    Matrix3 y1_mat, r_mat, y2_mat, p1_mat, p2_mat;
    // rotates the start point to the centre
    y1_mat.SetRotationPT(-DEG_TO_RAD(yaw_start), 0.0, 0.0);
    p1_mat.SetRotationPT(0.0, DEG_TO_RAD(pitch_start), 0.0);
    // rolls the image
    r_mat.SetRotationPT(0.0, 0.0, roll_shift);
    // rotates the centre to the destination point
    p2_mat.SetRotationPT(0.0, -DEG_TO_RAD(pitch_shift), 0.0);
    y2_mat.SetRotationPT(DEG_TO_RAD(yaw_shift), 0.0, 0.0);
    
    rotation_matrix = y2_mat * p2_mat * r_mat *p1_mat * y1_mat;
}

void PreviewDragTool::ReallyAfterDrawImagesEvent()
{
    // draw guide lines down the middle.
    HuginBase::PanoramaOptions *opts = helper->GetViewStatePtr()->GetOptions();
    double width = (double) opts->getSize().width(),
           height = (double) opts->getSize().height();
    // Invert the color underneath.
    helper->GetViewStatePtr()->GetTextureManager()->DisableTexture();
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
    glEnable(GL_BLEND);
    glColor3f(1.0, 1.0, 1.0);
    glPushMatrix();
    glTranslatef(-0.5, -0.5, -0.5);    
    glBegin(GL_LINES);
        glVertex2f(width / 2.0, 0.0);
        glVertex2f(width / 2.0, height);
        glVertex2f(0.0,         height / 2.0);
        glVertex2f(width,       height / 2.0);        
    glEnd();
    // draw lines if we are dragging
    if (drag_roll)
    {
        // when rolling, a line from the centre (where we rotate around) in the
        // direction of the mouse pointer should help the user.
        double distance = width * width + height * height,
               angle = start_angle;
        glPushMatrix();
        glTranslatef(centre.x, centre.y, 0.0);
        glBegin(GL_LINES);
            // starting angle
            glVertex2d(0.0, 0.0);
            glVertex2f(distance * cos(angle), distance * sin(angle));
            // angle now used
            angle +=  shift_angle;
            glVertex2d(0.0, 0.0);
            glVertex2f(distance * cos(angle), distance * sin(angle));
        glEnd();
        glPopMatrix(); 
    }
    if (drag_pitch || drag_yaw)
    {
        // Draw a straight line in the spherical space, from the start point to
        // under the mouse. It only appears straight when using a cylinderical
        // projection or similar though, so we draw it as many line segments.
        glBegin(GL_LINE_STRIP);
            for (double t = 0.0; t <= 1.0; t+= 0.005)
            {
                double x, y, ti = 1.0 - t;
                helper->GetViewStatePtr()->GetProjectionInfo()->AngularToImage(
                             x, y,
                             t * start_coordinates.x + ti *shift_coordinates.x,
                             t * start_coordinates.y + ti *shift_coordinates.y);
                glVertex2d(x, y);
            }
        glEnd();        
    }    
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}

void OverviewDragTool::ReallyAfterDrawImagesEvent()
{

}

