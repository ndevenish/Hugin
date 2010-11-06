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
#if !defined Hugin_shared || !defined _WINDOWS
#define GLEW_STATIC
#endif
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

PreviewDifferenceTool::PreviewDifferenceTool(ToolHelper *helper)
    : Tool(helper)
{
    // rather boring constructor
}

/** call this function only after OpenGL context was created */
bool PreviewDifferenceTool::CheckOpenGLCanDifference()
{
    if(GLEW_ARB_imaging)
        return true;
    if((glBlendEquation!=NULL) && (GLEW_EXT_blend_subtract))
        return true;
    return false;
}

void PreviewDifferenceTool::Activate()
{
    over_image = false;
    // Activate difference tool only if OpenGL extension GL_ARB_imaging is available
    // cause the extension contains required functions like glBlendEquation
    // In general this check should be superfluous, due to the fact that the preview frame
    // must check the OpenGL capabilities and never call this method if differencing is
    // not supported, but check twice is saver.
    if (CheckOpenGLCanDifference())
    {
        helper->NotifyMe(PreviewToolHelper::IMAGES_UNDER_MOUSE_CHANGE, this);
    }
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
            helper->NotifyMe(PreviewToolHelper::DRAW_UNDER_IMAGES, this);
        }
        // The lowest image number is drawn first, since the set is sorted.
        // Get notifications so we can draw it:
        image_number = *(image_set.begin());
        helper->NotifyMeBeforeDrawing(image_number, this);
        // Redraw the panorama with this image negated from the others.
        helper->GetVisualizationStatePtr()->ForceRequireRedraw();
        helper->GetVisualizationStatePtr()->Redraw();
        over_image = true;
    } else {
        if (over_image)
        {
            // We were showing a difference, now there are no images under the
            // mouse pointer:
            // Drop notifications, we don't need to do anything extra.
            over_image = false;
            helper->DoNotNotifyMe(PreviewToolHelper::DRAW_OVER_IMAGES, this);
            helper->DoNotNotifyMe(PreviewToolHelper::DRAW_UNDER_IMAGES, this);
            helper->DoNotNotifyMeBeforeDrawing(image_number, this);
            // redraw the panorama without the negated image.
            helper->GetVisualizationStatePtr()->ForceRequireRedraw();
            helper->GetVisualizationStatePtr()->Redraw();
        }
    }
}


void PreviewDifferenceTool::AfterDrawImagesEvent()
{
    // Get the display list used to generate the image
    unsigned int display_list;
        display_list = helper->GetVisualizationStatePtr()->GetMeshDisplayList(image_number);    
    TextureManager *tex_m = helper->GetViewStatePtr()->GetTextureManager();
    tex_m->BindTexture(image_number);
    // we will use a subtractive blend
    glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    // multiply by the alpha value, so transparent areas appear black.
    glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
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
    tex_m->DisableTexture();
    glBlendFunc(GL_DST_COLOR, GL_ONE);
    for (unsigned short int count = 0; count < DIFFERENCE_DOUBLE; count++)
    {
        glCallList(display_list);
    }
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

//draw the images below all other images so that the difference is not computed agains something drawn in the background
void PreviewDifferenceTool::BeforeDrawImagesEvent()
{
    unsigned int display_list = helper->GetVisualizationStatePtr()->GetMeshDisplayList(image_number);    
    helper->GetViewStatePtr()->GetTextureManager()->DrawImage(image_number, display_list);
}


bool PreviewDifferenceTool::BeforeDrawImageEvent(unsigned int image)
{
    return false;
}

