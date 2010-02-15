// -*- c-basic-offset: 4 -*-
/** @file PreviewPanoMaskTool.cpp
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

#include "PreviewPanoMaskTool.h"
#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

PreviewPanoMaskTool::PreviewPanoMaskTool(PreviewToolHelper *helper)
    : PreviewTool(helper)
{
}

void PreviewPanoMaskTool::Activate()
{
    // we draw to the sentcil buffer the desired shape, and enable the stencil
    // test before the images are rendered. After they have all been drawn, we
    // turn off stenciling so the other tools can draw of the complete area.
    helper->NotifyMe(PreviewToolHelper::REALLY_DRAW_OVER_IMAGES, this);
}

void PreviewPanoMaskTool::BeforeDrawImagesEvent()
{
}

void PreviewPanoMaskTool::ReallyAfterDrawImagesEvent()
{
    switch (helper->GetViewStatePtr()->GetOptions()->getProjection())
    {
        case HuginBase::PanoramaOptions::SINUSOIDAL:
            helper->GetViewStatePtr()->GetTextureManager()->DisableTexture();
            glColor3f(0.0, 0.0, 0.0);
            {
                // Under a sinusodial projection, we mask off the sides.
                OutputProjectionInfo *info = helper->GetViewStatePtr()->
                                                            GetProjectionInfo();
                double x, y;
                glBegin(GL_QUAD_STRIP);
                    for (double p = -90; p < 90; p += 1.0)
                    {
                        info->AngularToImage(x, y, -180.0, p);
                        glVertex2d(x, y); glVertex2d(0.0, y);
                    }
                glEnd();
                double width = helper->GetViewStatePtr()->GetOptions()->
                                                              getSize().width();
                glBegin(GL_QUAD_STRIP);
                    for (double p = -90; p < 90; p += 1.0)
                    {
                        info->AngularToImage(x, y, -180.0, p);
                        glVertex2d(width - x, y); glVertex2d(width, y);
                    }
                glEnd();
            }
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0, 1.0, 1.0);
            break;
        case HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC:
            // Under a albers equal area conic projection, we mask a circle
            // segment with a hole in the middle. The dimensions and centre
            // are depended on the projection parameters.
            helper->GetViewStatePtr()->GetTextureManager()->DisableTexture();
            glColor3f(0.0, 0.0, 0.0);
            glStencilFunc(GL_EQUAL, 1, 1);
            glEnable(GL_TEXTURE_2D);
            glColor3f(1.0, 1.0, 1.0);
            break;
        default:
            // most projections don't need any of this.
            // Always pass the stencil test.
            glStencilFunc(GL_ALWAYS, 1, 1);
            break;
    }
}

