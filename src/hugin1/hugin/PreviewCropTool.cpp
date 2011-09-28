// -*- c-basic-offset: 4 -*-

/** @file PreviewCropTool.cpp
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

#include "panoinc_WX.h"
#include "panoinc.h"
#include "PreviewCropTool.h"
#include <config.h>
#include "base_wx/platform.h"
#include "hugin/config_defaults.h"
#include "CommandHistory.h"
#include "wxPanoCommand.h"

#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

PreviewCropTool::PreviewCropTool(PreviewToolHelper *helper)
    : PreviewTool(helper)
{
    
}

void PreviewCropTool::Activate()
{
    helper->NotifyMe(PreviewToolHelper::MOUSE_MOVE, this);
    helper->NotifyMe(PreviewToolHelper::MOUSE_PRESS, this);
    helper->NotifyMe(PreviewToolHelper::REALLY_DRAW_OVER_IMAGES, this);
    mouse_down = false;
    moving_left = false;
    moving_right = false;
    moving_top = false;
    moving_bottom = false;
    helper->SetStatusMessage(_("Drag the inside of the cropping rectangle to adjust the crop."));
}

void PreviewCropTool::ReallyAfterDrawImagesEvent()
{
    // draw lines for the border to crop:
    // We use 1/4 of the smallest dimension as the size of an internal margin
    // inside the cropping region.
    // dragging a point in that margin moves the edges for the side we are on
    // (so the corners move both edges)
    // dragging a point in the middle moves the whole frame.
    
    // find the cropped region
    HuginBase::PanoramaOptions *opts = helper->GetVisualizationStatePtr()->getViewState()->GetOptions();
    vigra::Rect2D roi = opts->getROI();
    double width = (double) roi.width(),
          height = (double) roi.height(),
          margin = (width > height ? height : width) / 4.0;
    top = (double) roi.top() + margin;
    bottom = (double) roi.bottom() - margin;
    left = (double) roi.left() + margin;
    right = (double) roi.right() - margin;
   
    // now draw boxes to indicate what dragging would do.
    if (!mouse_down)
    {
        glEnable(GL_BLEND);
        helper->GetVisualizationStatePtr()->getViewState()->GetTextureManager()->DisableTexture();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0, 1.0, 1.0, 0.38197);
        glBegin(GL_QUADS);
            if (moving_left)
            {
                glVertex2d(roi.left(), top); glVertex2d(left, top);
                glVertex2d(left, bottom); glVertex2d(roi.left(), bottom);
            }
            if (moving_right)
            {
                glVertex2d(right, top); glVertex2d(roi.right(), top);
                glVertex2d(roi.right(), bottom); glVertex2d(right, bottom);
            }
            if (moving_top)
            {
                glVertex2d(right, top); glVertex2d(right, roi.top());
                glVertex2d(left, roi.top()); glVertex2d(left, top);
            }
            if (moving_bottom)
            {
                glVertex2d(right, bottom); glVertex2d(right, roi.bottom());
                glVertex2d(left, roi.bottom()); glVertex2d(left, bottom);
            }
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    } else {
        // while dragging, reset the displayed ROI to ours incase something else
        // tries to redraw the preview with the panorama's real ROI.
        opts->setROI(new_roi);
        helper->GetViewStatePtr()->SetOptions(opts);
    }
}

void PreviewCropTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{

//    std::cout << "outlines tool " << x << " " << y << std::endl;
//    double xp, yp;
//    HuginBase::PTools::Transform transform;
//    HuginBase::SrcPanoImage image;
//    image.setSize(vigra::Size2D(360,180));
//    image.setHFOV(360);
//    image.setProjection(HuginBase::BaseSrcPanoImage::EQUIRECTANGULAR);
//    if (helper->GetPanoramaPtr()->getNrOfImages() > 0) {
////        transform.createTransform(*helper->GetViewStatePtr()->GetSrcImage(0), *(helper->GetVisualizationStatePtr()->GetOptions()));
//        transform.createTransform(image, *(helper->GetVisualizationStatePtr()->GetOptions()));
//        transform.transformImgCoord(xp,yp,x,y);
//    std::cout << "outlines tool " << xp << " " << yp << std::endl;
//    }

    if (mouse_down)
    {
        vigra::Rect2D roi = start_drag_options.getROI();
        if (moving_left)
        {
            if (roi.left()<roi.right())
            {
                // apply the movement only if it does not bring about a negative crop
                unsigned int left_d = (int) (start_drag_x - x);
                roi.setUpperLeft(vigra::Point2D(roi.left() - left_d, roi.top()));
            }
        }
        if (moving_top)
        {
            if (roi.top()<roi.bottom())
            {
                // apply the movement only if it does not bring about a negative crop
                unsigned int top_d = (int) (start_drag_y - y);
                roi.setUpperLeft(vigra::Point2D(roi.left(), roi.top() - top_d));
            }
        }
        if (moving_right)
        {
            if (roi.left()<roi.right())
            {
                // apply the movement only if it does not bring about a negative crop
                unsigned int right_d = (int) (start_drag_x - x);
                roi.setLowerRight(vigra::Point2D(roi.right() - right_d, 
                                                 roi.bottom()));
            }
        }
        if (moving_bottom)
        {
            // apply the movement only if it does not bring about a negative crop
            if (roi.top()<roi.bottom())
            {
                unsigned int bottom_d = (int) (start_drag_y - y);
                roi.setLowerRight(vigra::Point2D(roi.right(),
                                                 roi.bottom() - bottom_d));
            }
        }
        // apply the movement only if it does not bring about a negative crop
		if((roi.top()<roi.bottom())&&(roi.left()<roi.right()))
        {
            opts.setROI(roi);
            new_roi = roi;
            helper->GetVisualizationStatePtr()->getViewState()->SetOptions(&opts);
            helper->GetVisualizationStatePtr()->Redraw();
        }
    } else {
        start_drag_x = x;
        start_drag_y = y;
        // check if the pointer moved to a region where dragging does something
        // different. If so, update the display to reflect it.
        // Note that since we normally only allow redraws to rerender the scene
        // if the view of the panorama has changed, we have to force a redraw.
        // (our moving boxes aren't known by the view_state!)
        int changes = 0;
        if ((x < left) != moving_left)
        {
            moving_left = !moving_left;
            changes++;
        }
        if ((x > right) != moving_right)
        {
            moving_right = !moving_right;
            changes++;
        }
        if ((y < top) != moving_top)
        {
            moving_top = !moving_top;
            changes++;
        }
        if ((y > bottom) != moving_bottom)
        {
            moving_bottom = !moving_bottom;
            changes++;
        }
        if (!(moving_left || moving_right || moving_top || moving_bottom))
        {
            // in the middle moves the whole region
            moving_left = true; moving_right = true;
            moving_top = true; moving_bottom = true;
            changes++;
        }
        // the middle section is an exception to the other 4 rules:
        if (changes == 5) changes = 0;
        // Draw if the boxes we show are different to what is already drawn.
        if (changes)
        {
            // since we didn't change the panorama, view_state doesn't think we
            // should redraw. Persuade it otherwise:
            helper->GetVisualizationStatePtr()->ForceRequireRedraw();
            helper->GetVisualizationStatePtr()->Redraw(); // now redraw.
        }
    }
}     


void PreviewCropTool::MouseButtonEvent(wxMouseEvent &e)
{
    if (e.GetButton() == wxMOUSE_BTN_LEFT)
    {
        if (e.ButtonDown())
        {
            start_drag_options = *helper->GetVisualizationStatePtr()->getViewState()->GetOptions();
            opts = start_drag_options;
            new_roi = opts.getROI();
            mouse_down = true;
            moving_left = false;
            moving_right = false;
            moving_top = false;
            moving_bottom = false;
            if (start_drag_x < left) moving_left = true;
            if (start_drag_x > right) moving_right = true;
            if (start_drag_y < top) moving_top = true;
            if (start_drag_y > bottom) moving_bottom = true;
            // in the middle, move everything.
            if (!(moving_left || moving_right || moving_top || moving_bottom))
            {
                moving_left = true;
                moving_right = true;
                moving_top = true;
                moving_bottom = true;
            }
        } else {
            if (mouse_down)
            {
                mouse_down = false;
                moving_left = false;
                moving_right = false;
                moving_top = false;
                moving_bottom = false;
                // set the new cropping region permanently.
                GlobalCmdHist::getInstance().addCommand(
                    new PT::SetPanoOptionsCmd(*(helper->GetPanoramaPtr()),
                                              opts));
            }
        }
    }
}

