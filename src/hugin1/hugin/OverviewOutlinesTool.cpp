// -*- c-basic-offset: 4 -*-
/** @file OverviewOutlinesTool.cpp
 *
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

#include "OverviewOutlinesTool.h"
#include "GLViewer.h"

#include "GreatCircles.h"
#include <cfloat>

const double OverviewOutlinesTool::res=10;
const double OverviewOutlinesTool::mindist=2;

OverviewOutlinesTool::OverviewOutlinesTool(ToolHelper * helper, GLViewer * viewer) : preview(viewer), thelper(helper),
    display_list_number_canvas(glGenLists(1)),
    display_list_number_crop(glGenLists(1)),
    display_list_number_canvas_outline(glGenLists(1)),
    display_list_number_crop_outline(glGenLists(1)),
    dirty_meshes(true)
{
    thelper->GetPanoramaPtr()->addObserver(this);
}

OverviewOutlinesTool::~OverviewOutlinesTool()
{
}

void OverviewOutlinesTool::panoramaChanged(HuginBase::PanoramaData &pano)
{

    dirty_meshes = true;
    thelper->GetVisualizationStatePtr()->ForceRequireRedraw();
    
}


struct Rec {
    Rec(double left, double top, double right, double bottom) : left(left), top(top), right(right), bottom(bottom) {}
    Rec() {}
    double left;
    double top;
    double right;
    double bottom;
};

//#define WIREFRAME

void PanosphereOverviewOutlinesTool::Activate()
{
    ((PanosphereOverviewToolHelper*)helper)->NotifyMe(PanosphereOverviewToolHelper::DRAW_OVER_IMAGES_FRONT, this);
    ((PanosphereOverviewToolHelper*)helper)->NotifyMe(PanosphereOverviewToolHelper::DRAW_OVER_IMAGES_BACK, this);
//    helper->NotifyMe(ToolHelper::DRAW_OVER_IMAGES, this);
}


void PlaneOverviewOutlinesTool::Activate()
{
    (helper)->NotifyMe(ToolHelper::DRAW_OVER_IMAGES, this);
}

void PlaneOverviewOutlinesTool::AfterDrawImagesEvent()
{
    draw();
}

void PanosphereOverviewOutlinesTool::AfterDrawImagesBackEvent()
{
    draw();
}

void PanosphereOverviewOutlinesTool::AfterDrawImagesFrontEvent()
{
    draw();
}

void PanosphereOverviewOutlinesTool::drawBackground()
{

    double radius = ((PanosphereOverviewVisualizationState*) helper->GetVisualizationStatePtr())->getSphereRadius();
    GLUquadric* grid = gluNewQuadric();
    gluSphere(grid, radius+1,40,20);

}

void PlaneOverviewOutlinesTool::drawBackground()
{

    glBegin(GL_QUADS);

    double end = 1000000;

    glVertex3f(-end, end, 0);
    glVertex3f( end, end, 0);
    glVertex3f( end,-end, 0);
    glVertex3f(-end,-end, 0);

    glEnd();

}

void OverviewOutlinesTool::draw()
{

    if (!(preview->m_visualization_state)) {
        return;
    }


    if (dirty_meshes) {

//    std::cout << "outlines after draw images\n";
//        vigra::Rect2D trect = preview->m_visualization_state->GetVisibleArea();
//        double hscale, wscale;
//        hscale = (float) thelper->GetVisualizationStatePtr()->GetOptions()->getHeight() / (float) preview->m_visualization_state->GetOptions()->getHeight();
//        wscale =  (float) thelper->GetVisualizationStatePtr()->GetOptions()->getWidth() / (float) preview->m_visualization_state->GetOptions()->getWidth();
//        std::cerr << "outlines " << hscale << " " << wscale << std::endl;
//        std::cerr << "outlines " << trect.left() << " " << trect.top() << " " << trect.right() << " " << trect.bottom() << std::endl;
//        vigra::Rect2D rect(trect.left() * wscale, trect.top() * hscale, trect.right() * wscale, trect.bottom() * hscale);
//        std::cerr << "outlines " << rect.left() << " " << rect.top() << " " << rect.right() << " " << rect.bottom() << std::endl;

        vigra::Rect2D rect = preview->m_visualization_state->GetVisibleArea();

        glNewList(display_list_number_canvas,GL_COMPILE);
        DrawRect(rect.left(), rect.top(), rect.right(), rect.bottom(),false);
        glEndList();
        glNewList(display_list_number_canvas_outline,GL_COMPILE);
        DrawRect(rect.left(), rect.top(), rect.right(), rect.bottom(),true, 4.0);
        glEndList();

        vigra::Rect2D roi = thelper->GetViewStatePtr()->GetOptions()->getROI();
        glNewList(display_list_number_crop,GL_COMPILE);
        DrawRect(roi.left(), roi.top(), roi.right(), roi.bottom(),false);
        glEndList();
        glNewList(display_list_number_crop_outline,GL_COMPILE);
        DrawRect(roi.left(), roi.top(), roi.right(), roi.bottom(),true, 2.0);
        glEndList();

        dirty_meshes = false;
        
//    std::cout << "outlines adi " << rect.left() << " " << rect.top() << " " << rect.right() << " " << rect.bottom() << std::endl;
//    std::cout << "outlines adi " << roi.left() << " " << roi.top() << " " << roi.right() << " " << roi.bottom() << std::endl;
    }

    if (thelper->GetVisualizationStatePtr()->RequireRecalculateViewport()) {

        vigra::Rect2D rect = preview->m_visualization_state->GetVisibleArea();
        glNewList(display_list_number_canvas_outline,GL_COMPILE);
        DrawRect(rect.left(), rect.top(), rect.right(), rect.bottom(),true, 4.0);
        glEndList();

        vigra::Rect2D roi = thelper->GetViewStatePtr()->GetOptions()->getROI();
        glNewList(display_list_number_crop_outline,GL_COMPILE);
        DrawRect(roi.left(), roi.top(), roi.right(), roi.bottom(),true, 2.0);
        glEndList();

    }


    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glColor4f(0,0,0,0.50);        
    glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
    drawBackground();


    glColor4f(1.0,1.0,1.0,0.20);
    glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);
    glCallList(display_list_number_canvas);

//    std::cout << "outlines " << roi.left() << " " << roi.top() << " " << roi.right() << " " << roi.bottom() << std::endl;

    glColor4f(1.0,1.0,1.0,0.66);
    glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);
    glCallList(display_list_number_crop);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.8,0.8,0.8,0.5);
    glCallList(display_list_number_canvas_outline);
    glColor4f(0.8,0.8,0.8,0.6);
    glCallList(display_list_number_crop_outline);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor4f(1,1,1,1);

}

void OverviewOutlinesTool::DrawRect(double left, double top, double right, double bottom, bool outline, double line_width)
{

    vigra::Size2D size = thelper->GetVisualizationStatePtr()->GetOptions()->getSize();
    double mlength = (size->y > size->x) ? size->y : size->x;

    double fov = thelper->GetVisualizationStatePtr()->GetOptions()->getHFOV();
    
    //since res was initially estimated for size of 360x100;
    //TODO make res scale dependent
    double tres = res * mlength / fov;
    double tmindist = mindist * mlength / fov;

//    std::cerr << "outlines " << mlength << std::endl;
//    std::cerr << "outlines " << right << std::endl;

    double safety = 1;
    left += safety;
    top += safety;
    right -= safety;
    bottom -= safety;

    HuginBase::PTools::Transform transform;
    HuginBase::SrcPanoImage image;
//    image.setSize(vigra::Size2D(360,180));
    image.setSize(size);
    image.setHFOV(fov);
    switch(thelper->GetVisualizationStatePtr()->GetOptions()->getProjection()) {
        case HuginBase::PanoramaOptions::EQUIRECTANGULAR:
            image.setProjection(HuginBase::BaseSrcPanoImage::EQUIRECTANGULAR);
            break;
        case HuginBase::PanoramaOptions::RECTILINEAR:
            image.setProjection(HuginBase::BaseSrcPanoImage::RECTILINEAR);
            break;
    }
    
    transform.createTransform(image, *(preview->m_visualization_state->GetOptions()));

    HuginBase::PanoramaOptions::ProjectionFormat proj = thelper->GetViewStatePtr()->GetOptions()->getProjection();

    switch(proj) {

        case HuginBase::PanoramaOptions::SINUSOIDAL:
        case HuginBase::PanoramaOptions::LAMBERT:
        case HuginBase::PanoramaOptions::LAMBERT_AZIMUTHAL:
        case HuginBase::PanoramaOptions::FULL_FRAME_FISHEYE:
        case HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC:
        case HuginBase::PanoramaOptions::ORTHOGRAPHIC:
        case HuginBase::PanoramaOptions::EQUISOLID:
        case HuginBase::PanoramaOptions::THOBY_PROJECTION:

            {

            //in this case just the divide the base rectangle by a certain amount of steps

            float steps = 40;

            double wstep = (float)(right - left) / steps;
            double hstep = (float)(bottom - top) / steps;
            
            for(int w = 0 ; w < steps ; w++) {
            for(int h = 0 ; h < steps ; h++) {

                //if outline is needed just consider edge cases
                if (outline) {
                    if (!(w == 0 || h == 0 || w == steps - 1 || steps - 1)) {
                        continue;
                    }
                }

                Rect rec(left + w * wstep, top + h * hstep, left + (w+1) * wstep, top + (h+1)* hstep);
                Rect tr = rec.transformImgCoord(&transform);

                double edge1 = (tr.val[0][0]-tr.val[1][0])*(tr.val[0][0]-tr.val[1][0]) + (tr.val[0][1]-tr.val[1][1])*(tr.val[0][1]-tr.val[1][1]);
                double edge2 = (tr.val[1][0]-tr.val[2][0])*(tr.val[1][0]-tr.val[2][0]) + (tr.val[1][1]-tr.val[2][1])*(tr.val[1][1]-tr.val[2][1]);
                double edge3 = (tr.val[2][0]-tr.val[3][0])*(tr.val[2][0]-tr.val[3][0]) + (tr.val[2][1]-tr.val[3][1])*(tr.val[2][1]-tr.val[3][1]);
                double edge4 = (tr.val[3][0]-tr.val[0][0])*(tr.val[3][0]-tr.val[0][0]) + (tr.val[3][1]-tr.val[0][1])*(tr.val[3][1]-tr.val[0][1]);

                
               //this section should be added to avoid certain problems with these projections 
               //(just disregard edges with lengths larger than some value with respect to the radius of the sphere)
               //TODO this was commented when support for mosaic plane was added, make this work again
//                double maxlimit = (radius/2.0)*(radius/2.0);
//                if (
//                        proj == HuginBase::PanoramaOptions::SINUSOIDAL ||
//                        proj == HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC
//                    )
//                if (edge1 > maxlimit || edge2 > maxlimit || edge3 > maxlimit || edge4 > maxlimit) {
//                    continue;
//                }

                if (outline) {

//                    glBegin(GL_LINES);
                    bool edges[4] = {false,false,false,false};
                    if (w == 0) {
                        edges[0] = true;
                    }
                    if (w == steps - 1) {
                        edges[2] = true;
                    }
                    if (h == 0) {
                        edges[3] = true;
                    }
                    if (h == steps - 1) {
                        edges[1] = true;
                    }
                    for (int i = 0 ; i < 4 ; i++) {
                        if (edges[i]) {


                            //draw a line with the help of the GreatCircles class so that a mesh is drawn instead of just a line
                            int plus = i+1;
                            if (plus == 4) plus = 0;
                            hugin_utils::FDiff2D cd1(tr.val[i][0], tr.val[i][1]);
                            hugin_utils::FDiff2D cd2(tr.val[plus][0], tr.val[plus][1]);
                            GreatCircleArc::LineSegment line;
                            line.vertices[0] = cd1;
                            line.vertices[1] = cd2;
                            line.doGL(line_width, thelper->GetVisualizationStatePtr());


//                            for (int j = 0 ; j < 2 ; j++) {
//                                int plus = i+j;
//                                if (plus == 4) plus = 0;
//                                double x,y,z;
//                                double tx,ty;
//                                tx = tr.val[plus][0];
//                                ty = tr.val[plus][1];
////                                ty = ty - 90;
////                                tx = tx - 180;
////                                ty *= -1;
////                                double x,y,z;
////                                MeshManager::MeshInfo::Coord3D coord = MeshManager::PanospheOverviewMeshInfo::Convert(x,y,z,tx,ty,radius);
//                                hugin_utils::FDiff2D cd(tx,ty);
//                                MeshManager::MeshInfo::Coord3D coord = thelper->GetVisualizationStatePtr()->GetMeshManager()->GetCoord3D(cd);
//                                glVertex3f(coord.x,coord.y,coord.z);
//                            }

                        }
                    }
//                    glEnd();
                       
                } else {

                    //in this case draw the mesh

                    #ifdef WIREFRAME
                    glBegin(GL_LINE_LOOP);
                    #else
                    glBegin(GL_POLYGON);
                    #endif
                    for (int s = 0 ; s < 4 ; s++) {
                        double tx,ty;
                        tx = tr.val[s][0];
                        ty = tr.val[s][1];
//                        ty = ty - 90;
//                        tx = tx - 180;
//                        ty *= -1;
                        hugin_utils::FDiff2D cd(tx,ty);
                        MeshManager::MeshInfo::Coord3D coord = thelper->GetVisualizationStatePtr()->GetMeshManager()->GetCoord3D(cd);
                        glVertex3f(coord.x,coord.y,coord.z);
                    }
                    glEnd();

                }

            }
            }

            }

        break;

        case HuginBase::PanoramaOptions::RECTILINEAR:
        case HuginBase::PanoramaOptions::EQUIRECTANGULAR:
        case HuginBase::PanoramaOptions::CYLINDRICAL:
        case HuginBase::PanoramaOptions::STEREOGRAPHIC:
        case HuginBase::PanoramaOptions::MERCATOR:
        case HuginBase::PanoramaOptions::TRANSVERSE_MERCATOR:
        case HuginBase::PanoramaOptions::MILLER_CYLINDRICAL:
        case HuginBase::PanoramaOptions::PANINI:
        case HuginBase::PanoramaOptions::EQUI_PANINI:
        case HuginBase::PanoramaOptions::BIPLANE:
        case HuginBase::PanoramaOptions::TRIPLANE:
        case HuginBase::PanoramaOptions::GENERAL_PANINI:
        case HuginBase::PanoramaOptions::ARCHITECTURAL:

            //stack to keep the rectangles to be divided
            std::vector<Rec> stack;

            stack.push_back(Rec(left,top,right,bottom));
            //if outline needs to be obtained, after each rectangle another rectangle is pushed back 
            if (outline) {
                stack.push_back(Rec(true, true, true, true));
            }
            
            while(stack.size() > 0) {

                Rec edge;
                if (outline) {
                    edge = stack[stack.size() - 1];
                    stack.pop_back();
                }

                Rec top_rec = stack[stack.size() - 1];
                stack.pop_back();

                if (outline) {
                    if (!(edge.left || edge.top || edge.right || edge.bottom )) {
                        continue;
                    }
                }

                Rect rect(top_rec.left, top_rec.top, top_rec.right, top_rec.bottom);

                Rect tr = rect.transformImgCoord(&transform);

                double edge1 = (tr.val[0][0]-tr.val[1][0])*(tr.val[0][0]-tr.val[1][0]) + (tr.val[0][1]-tr.val[1][1])*(tr.val[0][1]-tr.val[1][1]);
                double edge2 = (tr.val[1][0]-tr.val[2][0])*(tr.val[1][0]-tr.val[2][0]) + (tr.val[1][1]-tr.val[2][1])*(tr.val[1][1]-tr.val[2][1]);
                double edge3 = (tr.val[2][0]-tr.val[3][0])*(tr.val[2][0]-tr.val[3][0]) + (tr.val[2][1]-tr.val[3][1])*(tr.val[2][1]-tr.val[3][1]);
                double edge4 = (tr.val[3][0]-tr.val[0][0])*(tr.val[3][0]-tr.val[0][0]) + (tr.val[3][1]-tr.val[0][1])*(tr.val[3][1]-tr.val[0][1]);

//                std::cout << "outlines  " << top_rec.left << " " << top_rec.top << " " << top_rec.right << " " << top_rec.bottom << std::endl;
//                std::cout << "outlines  " << edge1 << " " << edge2 << " " << edge3 << " " << edge4 << std::endl;

//                std::cout << "outlines  " << tr.val[0][0] << " " << tr.val[0][1] << std::endl;
//                std::cout << "outlines  " << tr.val[1][0] << " " << tr.val[1][1] << std::endl;
//                std::cout << "outlines  " << tr.val[2][0] << " " << tr.val[2][1] << std::endl;
//                std::cout << "outlines  " << tr.val[3][0] << " " << tr.val[3][1] << std::endl;

                double ressq = tres * tres;

                bool divide_ver = false;
                bool divide_hor = false;

                bool invalid = false;
                int countx = 0, county = 0;


                //decide whether to divide the current rectangle
                if (
                        (edge1 > ressq || edge3 > ressq) 
                            && 
                        abs(top_rec.top - top_rec.bottom) > tmindist
                    ) {

                        divide_ver = true;

                } else if (
                            (edge2 > ressq || edge4 > ressq) 
                                && 
                            abs(top_rec.left - top_rec.right) > tmindist
                        ) {

                        divide_hor = true;

                }

                if (divide_ver) {

                    stack.push_back(Rec(top_rec.left,top_rec.top,top_rec.right,(top_rec.top+top_rec.bottom)/2.0));
                    if (outline) {
                        stack.push_back(Rec(edge.left, edge.top, edge.right, false));
                    }
                    stack.push_back(Rec(top_rec.left,(top_rec.top+top_rec.bottom)/2.0,top_rec.right,top_rec.bottom));
                    if (outline) {
                        stack.push_back(Rec(edge.left, false, edge.right, edge.bottom));
                    }
                }

                if (divide_hor) {

                    stack.push_back(Rec(top_rec.left,top_rec.top,(top_rec.left+top_rec.right)/2.0,top_rec.bottom));
                    if (outline) {
                        stack.push_back(Rec(edge.left, edge.top, false, edge.bottom));
                    }
                    stack.push_back(Rec((top_rec.left+top_rec.right)/2.0,top_rec.top,top_rec.right,top_rec.bottom));
                    if (outline) {
                        stack.push_back(Rec(false, edge.top, edge.right, edge.bottom));
                    }
                
                } 

                if (!(divide_ver || divide_hor)) {

                    //draw it
                    
                    if (outline) {
//                        glBegin(GL_LINES);
                        bool edges[4];
                        edges[0] = edge.left!=0;
                        edges[1] = edge.bottom!=0;
                        edges[2] = edge.right!=0;
                        edges[3] = edge.top!=0;
                        for (int i = 0 ; i < 4 ; i++) {
                            if (edges[i]) {
//                            std::cout << "outlines line!!" << i << "\n";
//                            std::cout << "outlines  " << top_rec.left << " " << top_rec.top << " " << top_rec.right << " " << top_rec.bottom << std::endl;

                            int plus = i+1;
                            if (plus == 4) plus = 0;
                            hugin_utils::FDiff2D cd1(tr.val[i][0], tr.val[i][1]);
                            hugin_utils::FDiff2D cd2(tr.val[plus][0], tr.val[plus][1]);
                            GreatCircleArc::LineSegment line;
                            line.vertices[0] = cd1;
                            line.vertices[1] = cd2;
                            line.doGL(line_width, thelper->GetVisualizationStatePtr());
                            
//                            MeshManager::MeshInfo::Coord3D coord1 = thelper->GetVisualizationStatePtr()->GetMeshManager()->GetCoord3D(cd1);
//                            MeshManager::MeshInfo::Coord3D coord2 = thelper->GetVisualizationStatePtr()->GetMeshManager()->GetCoord3D(cd2);
//                            glVertex3f(coord1.x,coord1.y,coord1.z);
//                            glVertex3f(coord2.x,coord2.y,coord2.z);

            
//                            for (int j = 0 ; j < 2 ; j++) {
//                                int plus = i+j;
//                                if (plus == 4) plus = 0;
//                                double x,y,z;
//                                double tx,ty;
//                                tx = tr.val[plus][0];
//                                ty = tr.val[plus][1];
////                                ty = ty - 90;
////                                tx = tx - 180;
////                                ty *= -1;

//                                
//                                hugin_utils::FDiff2D cd(tx,ty);
//                                MeshManager::MeshInfo::Coord3D coord = thelper->GetVisualizationStatePtr()->GetMeshManager()->GetCoord3D(cd);
//                                glVertex3f(coord.x,coord.y,coord.z);
//                            }
                            }
                        }
//                        glEnd();
                    } else {
                        #ifdef WIREFRAME
                        glBegin(GL_LINE_LOOP);
                        #else
                        glBegin(GL_POLYGON);
                        #endif
                        for (int s = 0 ; s < 4 ; s++) {
                            double tx,ty;
                            tx = tr.val[s][0];
                            ty = tr.val[s][1];
//                            ty = ty - 90;
//                            tx = tx - 180;
//                            ty *= -1;
                            hugin_utils::FDiff2D cd(tx,ty);
                            MeshManager::MeshInfo::Coord3D coord = thelper->GetVisualizationStatePtr()->GetMeshManager()->GetCoord3D(cd);
                            glVertex3f(coord.x,coord.y,coord.z);
                        }
                        glEnd();
                    }
                }

            }

        break;

    }

}



