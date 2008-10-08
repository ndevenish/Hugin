// -*- c-basic-offset: 4 -*-
/** @file PreviewDifferenceTool.cpp
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

#include "PreviewDifferenceTool.h"
#include <config.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// This is the number of times to double the result of the difference. It should
// be between 0 and 7. Note that with  values > 0 no extra colours are used
// (infact less are due to clipping), but the difference becomes easier to see.
#define DIFFERENCE_DOUBLE 2

PreviewDifferenceTool::PreviewDifferenceTool(PreviewToolHelper *helper)
    : PreviewTool(helper)
{
    // rather boring constructor
}

void PreviewDifferenceTool::Activate()
{
    over_image = false;
    helper->NotifyMe(PreviewToolHelper::IMAGES_UNDER_MOUSE_CHANGE, this);
}

void PreviewDifferenceTool::ImagesUnderMouseChangedEvent()
{
    std::set<unsigned int> image_set = helper->GetImageNumbersUnderMouse();
    if (!image_set.empty())
    {
        // stop showing the previous one if there was one, if there wasn't, we
        // need to be able to draw over the preview.
        if (over_image)
        {
            helper->DoNotNotifyMeBeforeDrawing(image_number, this);
        } else {
            helper->NotifyMe(PreviewToolHelper::DRAW_OVER_IMAGES, this);
        }
        // The lowest image number is drawn first, since the set is sorted.
        // Get notifications so we can draw it:
        image_number = *(image_set.begin());
        helper->NotifyMeBeforeDrawing(image_number, this);
        // Redraw the panorama with this image negated from the others.
        helper->GetViewStatePtr()->ForceRequireRedraw();
        helper->GetViewStatePtr()->Redraw();
        over_image = true;
    } else {
        if (over_image)
        {
            // We were showing a difference, now there are no images under the
            // mouse pointer:
            // Drop notifications, we don't need to do anything extra.
            over_image = false;
            helper->DoNotNotifyMe(PreviewToolHelper::DRAW_OVER_IMAGES, this);
            helper->DoNotNotifyMeBeforeDrawing(image_number, this);
            // redraw the panorama without the negated image.
            helper->GetViewStatePtr()->ForceRequireRedraw();
            helper->GetViewStatePtr()->Redraw();
        }
    }
}

void PreviewDifferenceTool::AfterDrawImagesEvent()
{
    // Get the display list used to generate the image
    unsigned int display_list, texture_number;
    display_list = helper->GetViewStatePtr()->GetMeshDisplayList(image_number);    
    TextureManager *tex_m = helper->GetViewStatePtr()->GetTextureManager();
    texture_number = tex_m->GetTextureName(image_number);
    glBindTexture(GL_TEXTURE_2D, texture_number);
    // we will use a subtractive blend
    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);
    if (tex_m->GetPhotometricCorrect())
    {
        // The texture has full photometric correction, we can subtract it once
        // and be done.
        glCallList(display_list);
    } else {
        // otherwise we have to fake some of the photometric correction to get
        // good white balance and exposure.
        HuginBase::SrcPanoImage *img;
        img = helper->GetViewStatePtr()->GetSrcImage(image_number);
        float viewer_exposure = 1.0 / pow(2.0,
                    helper->GetPanoramaPtr()->getOptions().outputExposureValue);
        float es = viewer_exposure / img->getExposure();
        float scale[3] = {es / img->getWhiteBalanceRed(),
                          es,
                          es / img->getWhiteBalanceBlue()};
        // now we draw repeatedly until the image has been exposed properly.
        while (scale[0] > 0.0 && scale[1] > 0.0 && scale[2] > 0.0)
        {
            glColor3fv(scale);
            glCallList(display_list);
            scale[0] -= 1.0; scale[1] -= 1.0; scale[2] -= 1.0;
        }
        glColor3f(1.0, 1.0, 1.0);
    }
    glBlendEquation(GL_FUNC_ADD);
    // To make the difference stand out more, multiply it a few times:
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_DST_COLOR, GL_ONE);
    for (unsigned short int count = 0; count < DIFFERENCE_DOUBLE; count++)
    {
        glCallList(display_list);
    }
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

bool PreviewDifferenceTool::BeforeDrawImageEvent(unsigned int image)
{
    return false;
}

