// -*- c-basic-offset: 4 -*-
/** @file ProjectionGridTool.h
 *
 *  @author Darko Makreshanski
 *
 *  @brief implementation of PanosphereSphereTool Class
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

#include "PanosphereSphereTool.h"

PanosphereSphereTool::PanosphereSphereTool(PanosphereOverviewToolHelper *helper, const wxColour backgroundColour) : PanosphereOverviewTool(helper)
{
    m_background_color = backgroundColour;
}

void PanosphereSphereTool::Activate()
{
    static_cast<PanosphereOverviewToolHelper*>(helper)->NotifyMe(PanosphereOverviewToolHelper::DRAW_UNDER_IMAGES_BACK, this);
    static_cast<PanosphereOverviewToolHelper*>(helper)->NotifyMe(PanosphereOverviewToolHelper::DRAW_UNDER_IMAGES_FRONT, this);
}

void PanosphereSphereTool::BeforeDrawImagesBackEvent()
{
    helper->GetViewStatePtr()->GetTextureManager()->DisableTexture();
    glDisable(GL_TEXTURE_2D);
    glColor4f((float)m_background_color.Red() / 255, (float)m_background_color.Green() / 255, (float)m_background_color.Blue() / 255, 0.8f);
 
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLUquadric* gridb = gluNewQuadric();
    gluSphere(gridb, 101,40,20);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    
}

void PanosphereSphereTool::BeforeDrawImagesFrontEvent()
{

}

void PanosphereSphereTool::SetPreviewBackgroundColor (wxColour c)
{
    m_background_color = c;
}
