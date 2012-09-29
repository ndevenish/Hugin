// -*- c-basic-offset: 4 -*-
/** @file ProjectionGridTool.h
 *
 *  @author Darko Makreshanski
 *
 *  @brief implementation of ProjectionGridTool Class
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

#if !defined Hugin_shared || !defined _WINDOWS
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#ifdef __APPLE__
  #include <GLUT/glut.h>
#endif


#include "ProjectionGridTool.h"

ProjectionGridTool::ProjectionGridTool(ToolHelper* helper) : helper_g(helper)
{
    mesh_info = NULL;
    texture_created = false;
}

ProjectionGridTool::~ProjectionGridTool()
{
    if (mesh_info)
    {
        delete mesh_info;
    }
}

void PreviewProjectionGridTool::Activate()
{
    helper->NotifyMe(ToolHelper::DRAW_OVER_IMAGES, this);
    helper->NotifyMe(ToolHelper::DRAW_UNDER_IMAGES, this);
}

void PreviewProjectionGridTool::BeforeDrawImagesEvent()
{
}

void PreviewProjectionGridTool::AfterDrawImagesEvent()
{
    DEBUG_DEBUG("begin");
    if (!texture_created)
    {
        if (!createTexture())
        {
            return;
        }
    }
    if (!mesh_info)
    {
        createMesh();
    }

    mesh_info->Update();
    glColor4f(1,1,1,1);
    glEnable( GL_TEXTURE_2D );
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if(helper->GetViewStatePtr()->GetSupportMultiTexture())
    {
        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_num);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, texture_num);
    };
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    mesh_info->CallList();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND);
}

void PanosphereOverviewProjectionGridTool::Activate()
{
    ((PanosphereOverviewToolHelper*)helper)->NotifyMe(PanosphereOverviewToolHelper::DRAW_OVER_IMAGES_BACK, this);
    ((PanosphereOverviewToolHelper*)helper)->NotifyMe(PanosphereOverviewToolHelper::DRAW_UNDER_IMAGES_BACK, this);
    ((PanosphereOverviewToolHelper*)helper)->NotifyMe(PanosphereOverviewToolHelper::DRAW_OVER_IMAGES_FRONT, this);
    ((PanosphereOverviewToolHelper*)helper)->NotifyMe(PanosphereOverviewToolHelper::DRAW_UNDER_IMAGES_FRONT, this);
}

void PanosphereOverviewProjectionGridTool::BeforeDrawImagesBackEvent()
{
}

void PanosphereOverviewProjectionGridTool::BeforeDrawImagesFrontEvent()
{
}

void PanosphereOverviewProjectionGridTool::AfterDrawImagesBackEvent()
{
    DEBUG_DEBUG("begin");
    if (!texture_created)
    {
        if (!createTexture())
        {
            return;
        }
    }

    if (!mesh_info)
    {
        createMesh();
    }

    DEBUG_DEBUG("resources created");
    glColor4f(1,1,1,0.3);
    glEnable( GL_TEXTURE_2D );
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if(helper->GetViewStatePtr()->GetSupportMultiTexture())
    {
        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_num);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, texture_num);
    };

    glMatrixMode(GL_TEXTURE);
    // using just a sphere instead of the remapped mesh for better quality
    glPushMatrix();
    glRotated(180,1,0,0);
    glMatrixMode(GL_MODELVIEW);
    GLUquadric* grid = gluNewQuadric();
    gluQuadricTexture(grid, GL_TRUE);

    glPushMatrix();
    glScalef(-1,1,1);
    glRotated(-90,1,0,0);
    gluSphere(grid, 101,40,20);
    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glDisable(GL_BLEND);
    glMatrixMode(GL_MODELVIEW);
    DEBUG_DEBUG("end");
}

void PanosphereOverviewProjectionGridTool::AfterDrawImagesFrontEvent()
{
    if (!texture_created)
    {
        if (!createTexture())
        {
            return;
        }
    }

    if (!mesh_info)
    {
        createMesh();
    }

    glColor4f(1,1,1,1);
    glEnable( GL_TEXTURE_2D );
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    DEBUG_DEBUG("proj grid tex " << texture_num);
    if(helper->GetViewStatePtr()->GetSupportMultiTexture())
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_num);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, texture_num);
    };
    glMatrixMode(GL_TEXTURE);

    //using just a sphere instead of the remapped mesh for better quality
    glPushMatrix();
    glRotated(180,1,0,0);
    glMatrixMode(GL_MODELVIEW);

    GLUquadric* grid = gluNewQuadric();
    gluQuadricTexture(grid, GL_TRUE);
    glPushMatrix();
    glScalef(-1,1,1);
    glRotated(-90,1,0,0);
    gluSphere(grid, 101,40,20);
    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glDisable(GL_BLEND);
    glMatrixMode(GL_MODELVIEW);

}

void PreviewProjectionGridTool::createMesh()
{
    HuginBase::SrcPanoImage image;
    image.setSize(vigra::Size2D(3600,1780));
    image.setHFOV(360);
    image.setProjection(HuginBase::BaseSrcPanoImage::EQUIRECTANGULAR);
    mesh_info = new MeshManager::MeshInfo(helper->GetPanoramaPtr(), &image, helper->GetVisualizationStatePtr(), false);
}

void PanosphereOverviewProjectionGridTool::createMesh()
{
    DEBUG_DEBUG("Create mesh projection grid");
    HuginBase::SrcPanoImage image;
    image.setSize(vigra::Size2D(3600,1780));
    image.setHFOV(360);
    image.setProjection(HuginBase::BaseSrcPanoImage::EQUIRECTANGULAR);
    mesh_info = new MeshManager::PanosphereOverviewMeshInfo(helper->GetPanoramaPtr(), &image, helper->GetVisualizationStatePtr(), false);
    DEBUG_DEBUG("End create mesh projection grid");
}

/**
 * create the texture by iterating through each pixela and checking how much each pixel should be filled
 */
bool ProjectionGridTool::createTexture()
{
    glGenTextures(1,(GLuint*) &texture_num);

    if(helper_g->GetViewStatePtr()->GetSupportMultiTexture())
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_num);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, texture_num);
    };


    GLint texSize; 
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
    int width_p = 12;
    int height_p = 11;
    int width = std::min(1 << width_p,texSize);
    int height = std::min(1 << height_p, texSize >> 1);

    int hor_lines = 20;
    int ver_lines = 40;
    double line_width_per = 0.02;
    double dw = width / ver_lines;
    double dh = height / hor_lines;
    double line_width =  (dw < dh) ? line_width_per * dw : line_width_per * dh;

    unsigned char *image = new unsigned char[width * height * 4];
    unsigned char *pix_start = image;

    int horLineNr=0;
    for (int y = 0 ; y < height ; y++)
    {
        bool onHorLine=false;
        if((horLineNr+0.5)*dh-line_width/2.0<y)
        {
            if(y<=(horLineNr+0.5)*dh+line_width/2.0)
            {
                onHorLine=true;
            }
            else
            {
                horLineNr++;
            };
        };
        int verLineNr=0;
        for (int x = 0 ; x < width ; x++)
        {
            bool onVerLine=false;
            if((verLineNr+0.5)*dw-line_width/2.0<x)
            {
                if(x<=(verLineNr+0.5)*dw+line_width/2.0)
                {
                    onVerLine=true;
                }
                else
                {
                    verLineNr++;
                };
            };

            if(onHorLine || onVerLine)
            {
                pix_start[0]=0;
                pix_start[1]=255;
                pix_start[2]=255;
                pix_start[3]=255;
            }
            else
            {
                pix_start[0]=255;
                pix_start[1]=255;
                pix_start[2]=255;
                pix_start[3]=0;
            }
            pix_start += 4;
        }
    }

    bool has_error = false;
    GLint error;
    error = gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char *) image);
    delete [] image;
    static bool checked_anisotropic = false;
    static bool has_anisotropic;
    static float anisotropy;
    if (!checked_anisotropic)
    {
        // check if it is supported
        if (GLEW_EXT_texture_filter_anisotropic)
        {
            has_anisotropic = true;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
            DEBUG_INFO("Using anisotropic filtering at maximum value "
                      << anisotropy);
        } else {
            has_anisotropic = false;
            DEBUG_INFO("Anisotropic filtering is not available.");
        }
        checked_anisotropic = true;
    }
    if (has_anisotropic)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        anisotropy);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (error)
    {
        DEBUG_ERROR("GLU Error when building mipmap levels: "
                  << gluErrorString(error) << ".");
        has_error = true;
    }
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        DEBUG_ERROR("GL Error when building mipmap levels: "
                  << gluErrorString(error) << ".");
        has_error = true;
    }
    DEBUG_INFO("Finsihed loading texture.");
    if (has_error) {
        return false;
    } else {
        texture_created = true;
        return true;
    }
}
