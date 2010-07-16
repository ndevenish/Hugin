
#include <GL/glew.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <GL/glut.h>

#include "GLViewer.h"

#include "OverviewOutlinesTool.h"

OverviewOutlinesTool::OverviewOutlinesTool(OverviewToolHelper * helper, GLViewer * viewer) : OverviewTool(helper), preview(viewer),
    display_list_number_canvas(glGenLists(1)),
    display_list_number_crop(glGenLists(1)),
    display_list_number_canvas_outline(glGenLists(1)),
    display_list_number_crop_outline(glGenLists(1)),
    dirty_meshes(true)
{
    helper->GetPanoramaPtr()->addObserver(this);
}

OverviewOutlinesTool::~OverviewOutlinesTool()
{

}

void OverviewOutlinesTool::panoramaChanged(HuginBase::PanoramaData &pano)
{

    dirty_meshes = true;
    helper->GetVisualizationStatePtr()->ForceRequireRedraw();
    
}

void OverviewOutlinesTool::Activate()
{
    helper->NotifyMe(ToolHelper::MOUSE_MOVE, this);
    ((ToolHelper*)helper)->NotifyMe(ToolHelper::DRAW_OVER_IMAGES, (Tool*)this);
}

void OverviewOutlinesTool::MouseMoveEvent(double x, double y, wxMouseEvent & e)
{
//    std::cout << "outlines tool " << x << " " << y << std::endl;
    double xp, yp;
    HuginBase::PTools::Transform transform;
    HuginBase::SrcPanoImage image;
    image.setSize(vigra::Size2D(360,180));
    image.setHFOV(360);
    image.setProjection(HuginBase::BaseSrcPanoImage::EQUIRECTANGULAR);
    if (helper->GetPanoramaPtr()->getNrOfImages() > 0) {
//        transform.createTransform(*helper->GetViewStatePtr()->GetSrcImage(0), *(helper->GetVisualizationStatePtr()->GetOptions()));
        transform.createTransform(image, *(helper->GetVisualizationStatePtr()->GetOptions()));
        transform.transformImgCoord(xp,yp,x,y);
//    std::cout << "outlines tool " << xp << " " << yp << std::endl;
    }
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

void OverviewOutlinesTool::AfterDrawImagesEvent()
{

    if (!preview->m_visualization_state) {
        return;
    }

    if (dirty_meshes) {

//    std::cout << "outlines after draw images\n";
        vigra::Rect2D rect = preview->m_visualization_state->GetVisibleArea();
        glNewList(display_list_number_canvas,GL_COMPILE);
        DrawRect(rect.left(), rect.top(), rect.right(), rect.bottom(),false);
        glEndList();
        glNewList(display_list_number_canvas_outline,GL_COMPILE);
        DrawRect(rect.left(), rect.top(), rect.right(), rect.bottom(),true);
        glEndList();

        vigra::Rect2D roi = helper->GetViewStatePtr()->GetOptions()->getROI();
        glNewList(display_list_number_crop,GL_COMPILE);
        DrawRect(roi.left(), roi.top(), roi.right(), roi.bottom(),false);
        glEndList();
        glNewList(display_list_number_crop_outline,GL_COMPILE);
        DrawRect(roi.left(), roi.top(), roi.right(), roi.bottom(),true);
        glEndList();

        dirty_meshes = false;
        
    std::cout << "outlines adi " << rect.left() << " " << rect.top() << " " << rect.right() << " " << rect.bottom() << std::endl;
    std::cout << "outlines adi " << roi.left() << " " << roi.top() << " " << roi.right() << " " << roi.bottom() << std::endl;
    }


    double radius = ((PanosphereOverviewVisualizationState*)helper->GetVisualizationStatePtr())->getSphereRadius();

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    

    glColor4f(0,0,0,0.25);        
    glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
    GLUquadric* grid = gluNewQuadric();
    gluSphere(grid, radius+1,40,20);

    glColor4f(1.0,1.0,1.0,1.0);
    glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);
    glCallList(display_list_number_canvas);

//    std::cout << "outlines " << roi.left() << " " << roi.top() << " " << roi.right() << " " << roi.bottom() << std::endl;

    glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);
    glCallList(display_list_number_crop);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.6,0.6,0.6,0.5);
    glCallList(display_list_number_canvas_outline);
    glColor4f(0.6,0.6,0.6,0.8);
    glCallList(display_list_number_crop_outline);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void OverviewOutlinesTool::DrawRect(double left, double top, double right, double bottom, bool outline)
{

    double safety = 1;
    left += safety;
    top += safety;
    right -= safety;
    bottom -= safety;

    HuginBase::PTools::Transform transform;
    HuginBase::SrcPanoImage image;
    image.setSize(vigra::Size2D(360,180));
    image.setHFOV(360);
    image.setProjection(HuginBase::BaseSrcPanoImage::EQUIRECTANGULAR);
    transform.createTransform(image, *(preview->m_visualization_state->GetOptions()));

    HuginBase::PanoramaOptions::ProjectionFormat proj = helper->GetViewStatePtr()->GetOptions()->getProjection();

    double radius = ((PanosphereOverviewVisualizationState*)helper->GetVisualizationStatePtr())->getSphereRadius();

    switch(proj) {

        case HuginBase::PanoramaOptions::SINUSOIDAL:
        case HuginBase::PanoramaOptions::LAMBERT:
        case HuginBase::PanoramaOptions::LAMBERT_AZIMUTHAL:
        case HuginBase::PanoramaOptions::FULL_FRAME_FISHEYE:
        case HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC:
        case HuginBase::PanoramaOptions::ORTHOGRAPHIC:
        case HuginBase::PanoramaOptions::EQUISOLID:


            {

            float steps = 40;

            double x,y,xs,ys,xd,yd;

            double wstep = (float)(right - left) / steps;
            double hstep = (float)(bottom - top) / steps;
            
            for(int w = 0 ; w < steps ; w++) {
            for(int h = 0 ; h < steps ; h++) {

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

                double maxlimit = (radius/2.0)*(radius/2.0);
                if (
                        proj == HuginBase::PanoramaOptions::SINUSOIDAL ||
                        proj == HuginBase::PanoramaOptions::ALBERS_EQUAL_AREA_CONIC
                    )
                if (edge1 > maxlimit || edge2 > maxlimit || edge3 > maxlimit || edge4 > maxlimit) {
                    continue;
                }

                if (outline) {

                    glBegin(GL_LINES);
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
                            for (int j = 0 ; j < 2 ; j++) {
                                int plus = i+j;
                                if (plus == 4) plus = 0;
                                double x,y,z;
                                double tx,ty;
                                tx = tr.val[plus][0];
                                ty = tr.val[plus][1];
                                ty = ty - 90;
                                tx = tx - 180;
                                ty *= -1;
                                MeshManager::PanosphereOverviewMeshInfo::Coords3D::Convert(x,y,z,tx,ty,radius);
                                glVertex3f(x,y,z);
                            }
                        }
                    }
                    glEnd();
                       
                } else {

                    #ifdef WIREFRAME
                    glBegin(GL_LINE_LOOP);
                    #else
                    glBegin(GL_POLYGON);
                    #endif
                    for (int s = 0 ; s < 4 ; s++) {
                        double x,y,z;
                        double tx,ty;
                        tx = tr.val[s][0];
                        ty = tr.val[s][1];
                        ty = ty - 90;
                        tx = tx - 180;
                        ty *= -1;
                        MeshManager::PanosphereOverviewMeshInfo::Coords3D::Convert(x,y,z,tx,ty,radius);
                        glVertex3f(x,y,z);
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
        
            std::vector<Rec> stack;

            stack.push_back(Rec(left,top,right,bottom));
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

                double xcs,ycs,xcd,ycd;

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

                double ressq = res * res;

                bool divide_ver = false;
                bool divide_hor = false;

                bool invalid = false;
                int countx = 0, county = 0;


                if (
                        (edge1 > ressq || edge3 > ressq) 
                            && 
                        abs(top_rec.top - top_rec.bottom) > mindist
                    ) {

                        divide_ver = true;

                } else if (
                            (edge2 > ressq || edge4 > ressq) 
                                && 
                            abs(top_rec.left - top_rec.right) > mindist
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
                    
                    if (outline) {
                        glBegin(GL_LINES);
                        bool edges[4];
                        edges[0] = edge.left;
                        edges[1] = edge.bottom;
                        edges[2] = edge.right;
                        edges[3] = edge.top;
                        for (int i = 0 ; i < 4 ; i++) {
                            if (edges[i]) {
                            std::cout << "outlines line!!" << i << "\n";
                            std::cout << "outlines  " << top_rec.left << " " << top_rec.top << " " << top_rec.right << " " << top_rec.bottom << std::endl;
                            for (int j = 0 ; j < 2 ; j++) {
                                int plus = i+j;
                                if (plus == 4) plus = 0;
                                double x,y,z;
                                double tx,ty;
                                tx = tr.val[plus][0];
                                ty = tr.val[plus][1];
                                ty = ty - 90;
                                tx = tx - 180;
                                ty *= -1;
                                MeshManager::PanosphereOverviewMeshInfo::Coords3D::Convert(x,y,z,tx,ty,radius);
                                glVertex3f(x,y,z);
                            }
                            }
                        }
                        glEnd();
                    } else {
                        #ifdef WIREFRAME
                        glBegin(GL_LINE_LOOP);
                        #else
                        glBegin(GL_POLYGON);
                        #endif
                        for (int s = 0 ; s < 4 ; s++) {
                            double x,y,z;
                            double tx,ty;
                            tx = tr.val[s][0];
                            ty = tr.val[s][1];
                            ty = ty - 90;
                            tx = tx - 180;
                            ty *= -1;
                            MeshManager::PanosphereOverviewMeshInfo::Coords3D::Convert(x,y,z,tx,ty,radius);
                            glVertex3f(x,y,z);
                        }
                        glEnd();
                    }
                }

            }

        break;

    }

}


