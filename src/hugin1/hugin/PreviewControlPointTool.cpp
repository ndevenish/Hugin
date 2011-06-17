// -*- c-basic-offset: 4 -*-

/** @file PreviewControlPointTool.cpp
 *
 *  @author James Legg
 *
 *  Copyright 2009 James Legg.
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

#include <hugin/PreviewControlPointTool.h>
#include <panodata/ControlPoint.h>


#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#ifdef __WXMSW__
#include <vigra/windows.h>
#endif
#include <GL/gl.h>
#endif

PreviewControlPointTool::PreviewControlPointTool(ToolHelper *helper)
    : Tool(helper)
{
    m_greatCircles.setVisualizationState(helper->GetVisualizationStatePtr());
}

// we want to draw over the panorama when in use, so make sure we get notice.
void PreviewControlPointTool::Activate()
{
    helper->NotifyMe(PreviewToolHelper::DRAW_OVER_IMAGES, this);
}

// The panorama has been drawn, draw the control points over the top.
void PreviewControlPointTool::AfterDrawImagesEvent()
{
    // Make Transforms for each image so we don't have to do it twice for each control point.
    MakeTransforms();

    // get all of the control points:
    const HuginBase::CPVector &control_points = helper->GetPanoramaPtr()->getCtrlPoints();
    
    // now draw each control point in turn:
    helper->GetViewStatePtr()->GetTextureManager()->DisableTexture();
    glColor3f(1.0, 0.5, 0.0);
    size_t cp_count = control_points.size();
    for (size_t cp_index = 0; cp_index < cp_count; cp_index++)
    {
        const HuginBase::ControlPoint &cp = control_points[cp_index];
        // only draw the control point if both images have been enabled.
        if (   helper->GetPanoramaPtr()->getImage(cp.image1Nr).getActive()
            && helper->GetPanoramaPtr()->getImage(cp.image2Nr).getActive())
        {
            // draw line control points blue instead of orange.
            bool line = cp.mode != HuginBase::ControlPoint::X_Y;
            if (line) 
            {
                glColor3f(0.0, 0.5, 1.0);
            }
            else
            {
                double red, green, blue;
                hugin_utils::ControlPointErrorColour(cp.error,red,green,blue);
                glColor3d(red, green, blue);
            }
            // draw a the smallest great circle arc between these two points.
            double x1, y1, x2, y2;
            transforms[cp.image1Nr].transformImgCoord(x1, y1, cp.x1, cp.y1);
            transforms[cp.image2Nr].transformImgCoord(x2, y2, cp.x2, cp.y2);
            m_greatCircles.drawLineFromSpherical(x1, y1, x2, y2);
        }
    }
    glColor3f(1.0, 1.0, 1.0);
    glEnable(GL_TEXTURE_2D);
    
    // free transforms
    delete [] transforms;
}

void PreviewControlPointTool::MakeTransforms()
{
    /// @todo [efficiency] check the ViewState to see if we can keep the last one.
    const size_t images_count = helper->GetPanoramaPtr()->getNrOfImages();
    transforms = new HuginBase::PTools::Transform [images_count];
    // make a pretend output options to get spherical coordinates.
    HuginBase::PanoramaOptions options;
    options.setWidth(360);
    options.setHeight(180);
    for (unsigned int image_number = 0; image_number < images_count; image_number++)
    {
        // we are transforming image coordinates to spherical coordinates.
        transforms[image_number].createInvTransform(
                *(helper->GetVisualizationStatePtr()->GetSrcImage(image_number)),
                options
            );
    }
}

