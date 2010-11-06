// -*- c-basic-offset: 4 -*-
/** @file MeshManager.cpp
 *
 *  @author James Legg
 *  @author Darko Makreshanski
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <wx/wx.h>
#include <wx/platform.h>

#if !defined Hugin_shared || !defined _WINDOWS
#define GLEW_STATIC
#endif
#include <GL/glew.h>

#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#ifdef __WXMSW__
#include <vigra/windows.h>
#endif
#include <GL/gl.h>
#endif

#include "panoinc.h"
#include "ViewState.h"
#include "MeshManager.h"
#include "ChoosyRemapper.h"
#include "LayoutRemapper.h"
#include <iostream>


// If we want to draw the outline of each face instead of shading it normally,
// uncomment this. Wireframe mode is for testing mesh quality.
// #define WIREFRAME

const double MeshManager::PanosphereOverviewMeshInfo::scale_diff=1.5;

MeshManager::MeshManager(PT::Panorama *pano, VisualizationState *visualization_state)
    :   m_pano(pano),
        visualization_state(visualization_state),
        layout_mode_on(false)
{
}

MeshManager::~MeshManager()
{
    for (std::vector<MeshInfo*>::iterator it = meshes.begin() ; it != meshes.end() ; it++) {
        delete (*it);
    }
    meshes.clear();
}

void MeshManager::CheckUpdate()
{
    unsigned int old_size = meshes.size();    
    // Resize to fit if images were removed
    while (meshes.size() > m_pano->getNrOfImages())
    {
        delete (meshes[meshes.size()-1]);
        meshes.pop_back();
    }
    // check each existing image individualy.
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        if (visualization_state->RequireRecalculateMesh(i))
        {
            DEBUG_DEBUG("Update mesh for " << i);
            meshes[i]->SetSrcImage(visualization_state->GetSrcImage(i));
            meshes[i]->Update();
        }
    }
    // add any new images.
    for (unsigned int i = old_size; i < m_pano->getNrOfImages(); i++)
    {
        DEBUG_INFO("Making new mesh remapper for image " << i << ".");
        //use the virtual method to get the right subclass for the MeshInfo
        meshes.push_back(this->ObtainMeshInfo(visualization_state->GetSrcImage(i), layout_mode_on));
    }
}

void MeshManager::RenderMesh(unsigned int image_number) const
{
    meshes[image_number]->CallList();
}

unsigned int MeshManager::GetDisplayList(unsigned int image_number) const
{
    return meshes[image_number]->display_list_number;
}

void MeshManager::SetLayoutMode(bool state)
{
    if (layout_mode_on == state) return;
    layout_mode_on = state;
    /* All meshes must be recalculated, since the layout mode uses meshes that
     * do not resemble properly remapped images.
     */
    for (std::vector<MeshInfo*>::iterator it = meshes.begin() ; it != meshes.end() ; it++) {
        delete (*it);
    }
    meshes.clear();
}

void MeshManager::SetLayoutScale(double scale)
{
    for(unsigned int i=0;i<meshes.size();i++)
        meshes[i]->SetScaleFactor(scale);
};


MeshManager::MeshInfo::MeshInfo(PT::Panorama * m_pano_in,
                                HuginBase::SrcPanoImage * image,
                                VisualizationState * visualization_state_in,
                                bool layout_mode_on_in)
    :   display_list_number(glGenLists(1)), // Find a free display list.
        image(*image),
        m_pano(m_pano_in),
        m_visualization_state(visualization_state_in),
        remap(layout_mode_on_in ? (MeshRemapper *) new LayoutRemapper(m_pano, &(this->image), m_visualization_state)
                                : (MeshRemapper *) new ChoosyRemapper(m_pano, &(this->image), m_visualization_state)),
        layout_mode_on(layout_mode_on_in),
        scale_factor(3.0)
{
}

MeshManager::MeshInfo::MeshInfo(const MeshInfo & source)
    // copy remap object and display list, instead of references.
    :   display_list_number(glGenLists(1)),
    image(source.image),
    m_pano(source.m_pano),
    m_visualization_state(source.m_visualization_state),
    remap(source.layout_mode_on ? (MeshRemapper *) new LayoutRemapper(source.m_pano, (HuginBase::SrcPanoImage*) &(source.image), source.m_visualization_state)
                                : (MeshRemapper *) new ChoosyRemapper(source.m_pano, (HuginBase::SrcPanoImage*) &(source.image), source.m_visualization_state)),
    layout_mode_on(source.layout_mode_on),
    scale_factor(3.0)
{
}



MeshManager::MeshInfo::~MeshInfo()
{
    glDeleteLists(display_list_number, 1);
    delete remap;
}

void MeshManager::MeshInfo::Update()
{
    if (layout_mode_on)
    {
        /** @todo Maybe we should find the scale once, instead of for each
         * image, and find a more asthetic way to calculate it.
         */
        double scale = m_visualization_state->GetVisibleArea().width() /
                       sqrt((double) m_pano->getNrOfImages()) / scale_factor;
        MeshRemapper & remapper_ref = *remap;
        LayoutRemapper &r = dynamic_cast<LayoutRemapper &>(remapper_ref);
        r.setScale(scale);
    }
    this->CompileList();
}

MeshManager::MeshInfo::MeshCoords3D::MeshCoords3D(const MeshRemapper::Coords & coords)
{
    for (int x = 0 ; x < 2 ; x++) {
        for (int y = 0 ; y < 2 ; y++) {
            tex_coords[x][y][0] = coords.tex_c[x][y][0];
            tex_coords[x][y][1] = coords.tex_c[x][y][1];
            vertex_coords[x][y][0] = coords.vertex_c[x][y][0];
            vertex_coords[x][y][1] = coords.vertex_c[x][y][1];
            vertex_coords[x][y][2] = 0;
        }
    }
}

void MeshManager::MeshInfo::SetScaleFactor(double scale)
{
    scale_factor=scale;
    Update();
};

void MeshManager::MeshInfo::CallList() const
{
    glCallList(display_list_number);
}

void MeshManager::MeshInfo::CompileList()
{
    // build the display list from the coordinates generated by the remapper
//    DEBUG_INFO("Preparing to compile a display list for overview for " << image_number
//              << ".");
    DEBUG_ASSERT(remap);
    bool multiTexture=m_visualization_state->getViewState()->GetSupportMultiTexture();
    unsigned int number_of_faces = 0;

    DEBUG_DEBUG("mesh update compile pano");

    this->BeforeCompile();

    glNewList(display_list_number, GL_COMPILE);

        remap->UpdateAndResetIndex();
        DEBUG_INFO("Specifying faces in display list.");

        glPushMatrix();

        this->Transform();
        
        #ifndef WIREFRAME
        glBegin(GL_QUADS);
        #endif
            // get each face's coordinates from the remapper
            MeshRemapper::Coords coords;
            bool write = true;
            while (remap->GetNextFaceCoordinates(&coords))
            {
                MeshCoords3D coords3d = m_visualization_state->GetMeshManager()->GetMeshCoords3D(coords);
//                DEBUG_DEBUG("mesh update " << coords3d.vertex_coords[0][0][0] << " " << coords3d.vertex_coords[0][0][1] << " " << coords3d.vertex_coords[0][0][2]);
                number_of_faces++;
                // go in an anticlockwise direction
                #ifdef WIREFRAME
                glBegin(GL_LINE_LOOP);
                #endif
                if(multiTexture)
                {
                    glMultiTexCoord2dv(GL_TEXTURE0,coords3d.tex_coords[0][0]);
                    glMultiTexCoord2dv(GL_TEXTURE1,coords3d.tex_coords[0][0]);
                }
                else
                    glTexCoord2dv(coords3d.tex_coords[0][0]);
                glVertex3dv(coords3d.vertex_coords[0][0]);
                if(multiTexture)
                {
                    glMultiTexCoord2dv(GL_TEXTURE0,coords3d.tex_coords[0][1]);
                    glMultiTexCoord2dv(GL_TEXTURE1,coords3d.tex_coords[0][1]);
                }
                else
                    glTexCoord2dv(coords3d.tex_coords[0][1]);
                glVertex3dv(coords3d.vertex_coords[0][1]);
                if(multiTexture)
                {
                    glMultiTexCoord2dv(GL_TEXTURE0,coords3d.tex_coords[1][1]);
                    glMultiTexCoord2dv(GL_TEXTURE1,coords3d.tex_coords[1][1]);
                }
                else
                    glTexCoord2dv(coords3d.tex_coords[1][1]);
                glVertex3dv(coords3d.vertex_coords[1][1]);
                 if(multiTexture)
                {
                    glMultiTexCoord2dv(GL_TEXTURE0,coords3d.tex_coords[1][0]);
                    glMultiTexCoord2dv(GL_TEXTURE1,coords3d.tex_coords[1][0]);
                }
                else
                   glTexCoord2dv(coords3d.tex_coords[1][0]);
                glVertex3dv(coords3d.vertex_coords[1][0]);
                #ifdef WIREFRAME
                glEnd();
                #endif
            }
        #ifndef WIREFRAME
        glEnd();
        #endif

        glPopMatrix();

    glEndList();


    this->AfterCompile();
//    DEBUG_INFO("Prepared a display list for " << image_number << ", using "
//              << number_of_faces << " face(s).");
    DEBUG_DEBUG("after compile mesh");
}

void MeshManager::PanosphereOverviewMeshInfo::Convert(double &x, double &y, double &z, double th, double ph, double r)
{
    th /= 180.0;
    th *= M_PI;
    ph /= 180.0;
    ph *= M_PI;

    x = r * sin(th) * cos(ph);
    y = r * sin(ph);
    z = r * cos(th) * cos(ph);
}

void MeshManager::PanosphereOverviewMeshInfo::BeforeCompile()
{
    yaw = image.getYaw();
    pitch = image.getPitch();

    image.setYaw(0);
    image.setPitch(0);
}

void MeshManager::PanosphereOverviewMeshInfo::Transform()
{

    glRotated(yaw, 0,-1,0);
    glRotated(pitch, -1,0,0);

}

void MeshManager::PanosphereOverviewMeshInfo::AfterCompile()
{
    image.setYaw(yaw);
    image.setPitch(pitch);
}

MeshManager::MeshInfo::Coord3D MeshManager::PanosphereOverviewMeshInfo::GetCoord3D(hugin_utils::FDiff2D & coord, VisualizationState * state)
{
    double width, height, hfov, vfov;
    HuginBase::PanoramaOptions * opts = state->GetOptions();
    width = opts->getWidth();
    height = opts->getHeight();

    hfov = 360;
    vfov = 180;

    MeshManager::MeshInfo::Coord3D res;

    double r = ((PanosphereOverviewVisualizationState*)state)->getSphereRadius();
    double th, ph;
    th = ((coord.x / width) * hfov - hfov / 2.0);
    ph = ((coord.y / height) * vfov - vfov / 2.0);

    Convert(
        res.x,
        res.y,
        res.z,
        -th,-ph,r);

    return res;
}

MeshManager::MeshInfo::MeshCoords3D MeshManager::PanosphereOverviewMeshInfo::GetMeshCoords3D(MeshRemapper::Coords &coords, VisualizationState * state)
{
    double width, height, hfov, vfov;
    HuginBase::PanoramaOptions * opts = state->GetOptions();
    width = opts->getWidth();
    height = opts->getHeight();

    hfov = 360;
    vfov = 180;

    double r = ((PanosphereOverviewVisualizationState*)state)->getSphereRadius();

    MeshCoords3D res;
    for (int x = 0 ; x < 2 ; x++) {
        for (int y = 0 ; y < 2 ; y++) {

        
            res.tex_coords[x][y][0] = coords.tex_c[x][y][0];
            res.tex_coords[x][y][1] = coords.tex_c[x][y][1];

            double th, ph;
            th = ((coords.vertex_c[x][y][0] / width) * hfov - hfov / 2.0);
            ph = ((coords.vertex_c[x][y][1] / height) * vfov - vfov / 2.0);

            Convert(
                res.vertex_coords[x][y][0],
                res.vertex_coords[x][y][1],
                res.vertex_coords[x][y][2],
                -th,-ph,r);
                
//            DEBUG_DEBUG("pano get " << res.vertex_coords[x][y][0] << " " << res.vertex_coords[x][y][1] << " " << res.vertex_coords[x][y][2]);
//            DEBUG_DEBUG("pano get " << coords.vertex_c[x][y][0] << " " << coords.vertex_c[x][y][1]);

        }
    }
    return res;
}


const double MeshManager::PlaneOverviewMeshInfo::scale = 100;

MeshManager::MeshInfo::Coord3D MeshManager::PlaneOverviewMeshInfo::GetCoord3D(hugin_utils::FDiff2D &coord, VisualizationState * state)
{
    double width, height;
    HuginBase::PanoramaOptions * opts = state->GetOptions();
    width = opts->getWidth();
    height = opts->getHeight();

    Coord3D res;
    res.x = (coord.x - width / 2.0) * scale / width;
    res.y = (coord.y - height / 2.0) * (-scale) / width;
    res.z = 0;
    return res;
}

MeshManager::MeshInfo::MeshCoords3D MeshManager::PlaneOverviewMeshInfo::GetMeshCoords3D(MeshRemapper::Coords &coords, VisualizationState * state)
{
    double width, height;
    HuginBase::PanoramaOptions * opts = state->GetOptions();
    width = opts->getWidth();
    height = opts->getHeight();

    MeshCoords3D res;
    for (int x = 0 ; x < 2 ; x++) {
        for (int y = 0 ; y < 2 ; y++) {
            res.tex_coords[x][y][0] = coords.tex_c[x][y][0];
            res.tex_coords[x][y][1] = coords.tex_c[x][y][1];

            res.vertex_coords[x][y][0] = (coords.vertex_c[x][y][0] - width / 2.0) * scale / width;
            res.vertex_coords[x][y][1] = (coords.vertex_c[x][y][1] - height / 2.0) * (-scale) / width;
            res.vertex_coords[x][y][2] = 0;
        }
    }
    return res;

}

MeshManager::MeshInfo * PanosphereOverviewMeshManager::ObtainMeshInfo(HuginBase::SrcPanoImage * src, bool layout_mode_on)
{
    return new MeshManager::PanosphereOverviewMeshInfo(m_pano, src, visualization_state, layout_mode_on);
}

MeshManager::MeshInfo * PlaneOverviewMeshManager::ObtainMeshInfo(HuginBase::SrcPanoImage * src, bool layout_mode_on)
{
    return new MeshManager::PlaneOverviewMeshInfo(m_pano, src, visualization_state, layout_mode_on);
}

MeshManager::MeshInfo * PreviewMeshManager::ObtainMeshInfo(HuginBase::SrcPanoImage * src, bool layout_mode_on)
{
    return new MeshManager::PreviewMeshInfo(m_pano, src, visualization_state, layout_mode_on);
}



