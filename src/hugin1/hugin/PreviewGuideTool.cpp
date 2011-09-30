// -*- c-basic-offset: 4 -*-

/** @file PreviewGuideTool.cpp
 *
 *  @author T. Modes
 *
 *  @brief implementation of ToolHelper for drawing guide lines over pano
 *
 */

/*  This program is free software; you can redistribute it and/or
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

#include "PreviewGuideTool.h"
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

PreviewGuideTool::PreviewGuideTool(PreviewToolHelper *helper) : PreviewTool(helper)
{
}

void PreviewGuideTool::Activate()
{
    helper->NotifyMe(PreviewToolHelper::REALLY_DRAW_OVER_IMAGES, this);
}

void DrawThirds(const vigra::Rect2D roi)
{
    double width3 = (double) roi.width()/3.0;
    double height3 = (double) roi.height()/3.0;
    glBegin(GL_LINES);
    glVertex2f((double)roi.left()+width3,roi.top());
    glVertex2f((double)roi.left()+width3,roi.bottom());
    glVertex2f((double)roi.left()+2.0*width3,roi.top());
    glVertex2f((double)roi.left()+2.0*width3,roi.bottom());
    glVertex2f(roi.left(), (double)roi.top()+height3);
    glVertex2f(roi.right(),(double)roi.top()+height3);
    glVertex2f(roi.left(), (double)roi.top()+2.0*height3);
    glVertex2f(roi.right(),(double)roi.top()+2.0*height3);
    glEnd();
};

void DrawGoldenRatio(const vigra::Rect2D roi)
{
    double width = (double) roi.width();
    double height = (double) roi.height();
    glBegin(GL_LINES);
    glVertex2f((double)roi.left()+width*0.382,roi.top());
    glVertex2f((double)roi.left()+width*0.382,roi.bottom());
    glVertex2f((double)roi.left()+width*0.618,roi.top());
    glVertex2f((double)roi.left()+width*0.618,roi.bottom());
    glVertex2f(roi.left(), (double)roi.top()+height*0.382);
    glVertex2f(roi.right(),(double)roi.top()+height*0.382);
    glVertex2f(roi.left(), (double)roi.top()+height*0.618);
    glVertex2f(roi.right(),(double)roi.top()+height*0.618);
    glEnd();
};

void DrawDiagonal(const vigra::Rect2D roi)
{
    glBegin(GL_LINES);
    glVertex2f(roi.left(), roi.top());
    glVertex2f(roi.right(), roi.bottom());
    glVertex2f(roi.left(), roi.bottom());
    glVertex2f(roi.right(), roi.top());
    glVertex2f(roi.left(), roi.top()+roi.height()/2.0);
    glVertex2f(roi.right(), roi.top()+roi.height()/2.0);
    glVertex2f(roi.left()+roi.width()/2.0, roi.top());
    glVertex2f(roi.left()+roi.width()/2.0, roi.bottom());
    glEnd();
};

void DrawTriangle(const vigra::Rect2D roi, const bool up)
{
    double w=roi.width();
    double h=roi.height();
    double x=w/(1+pow(w/h,2));
    double y=h/(1+pow(w/h,2));
    glBegin(GL_LINES);
    if(up)
    {
        glVertex2f(roi.left(), roi.bottom());
        glVertex2f(roi.right(), roi.top());
        glVertex2f(roi.right()-x, roi.top()+y);
        glVertex2f(roi.right(), roi.bottom());
        glVertex2f(roi.left()+x, roi.bottom()-y);
        glVertex2f(roi.left(), roi.top());
    }
    else
    {
        glVertex2f(roi.left(), roi.top());
        glVertex2f(roi.right(), roi.bottom());
        glVertex2f(roi.right()-x, roi.bottom()-y);
        glVertex2f(roi.right(), roi.top());
        glVertex2f(roi.left()+x, roi.top()+y);
        glVertex2f(roi.left(), roi.bottom());
    }
    glEnd();
};

void DrawDiagonalMethod(const vigra::Rect2D roi)
{
    double w=roi.width();
    double h=roi.height();
    glBegin(GL_LINES);
    glVertex2f(roi.left(), roi.top());
    if(w>h)
    {
        glVertex2f(roi.left()+h, roi.bottom());
    }
    else
    {
        glVertex2f(roi.right(), roi.top()+w);
    };
    glVertex2f(roi.left(), roi.bottom());
    if(w>h)
    {
        glVertex2f(roi.left()+h, roi.top());
    }
    else
    {
        glVertex2f(roi.right(), roi.bottom()-w);
    };
    glVertex2f(roi.right(), roi.top());
    if(w>h)
    {
        glVertex2f(roi.right()-h, roi.bottom());
    }
    else
    {
        glVertex2f(roi.left(), roi.top()+w);
    };
    glVertex2f(roi.right(), roi.bottom());
    if(w>h)
    {
        glVertex2f(roi.right()-h, roi.top());
    }
    else
    {
        glVertex2f(roi.left(), roi.bottom()-w);
    };
    glEnd();
};

void PreviewGuideTool::ReallyAfterDrawImagesEvent()
{
    if(m_guide==NONE)
    {
        return;
    };
    HuginBase::PanoramaOptions *opts = helper->GetViewStatePtr()->GetOptions();
    vigra::Rect2D roi = opts->getROI();
    glDisable(GL_TEXTURE_2D);
    glColor3f(1,1,0);
    switch(m_guide)
    {
        case THIRDS:
            DrawThirds(roi);
            break;
        case GOLDENRATIO:
            DrawGoldenRatio(roi);
            break;
        case DIAGONAL:
            DrawDiagonal(roi);
            break;
        case TRIANGLE_DOWN:
            DrawTriangle(roi,false);
            break;
        case TRIANGLE_UP:
            DrawTriangle(roi,true);
            break;
        case DIAGONAL_METHOD:
            DrawDiagonalMethod(roi);
            break;
    };
    glEnable(GL_TEXTURE_2D);
}

void PreviewGuideTool::SetGuideStyle(const Guides newGuideStyle)
{
    m_guide=newGuideStyle;
    helper->GetViewStatePtr()->Redraw();
};

const PreviewGuideTool::Guides PreviewGuideTool::GetGuideStyle() const
{
    return m_guide;
};

