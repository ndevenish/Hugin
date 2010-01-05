// -*- c-basic-offset: 4 -*-
/** @file MeshManager.cpp
 *
 *  @author James Legg
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

MeshManager::MeshManager(PT::Panorama *pano, ViewState *view_state)
    :   m_pano(pano),
        view_state(view_state),
        layout_mode_on(false)
{
}

MeshManager::~MeshManager()
{
    meshes.clear();
}

void MeshManager::CheckUpdate()
{
    unsigned int old_size = meshes.size();    
    // Resize to fit if images were removed
    while (meshes.size() > m_pano->getNrOfImages())
    {
        meshes.pop_back();
    }
    // check each existing image individualy.
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        if (view_state->RequireRecalculateMesh(i))
        {
            meshes[i].Update();
        }
    }
    // add any new images.
    for (unsigned int i = old_size; i < m_pano->getNrOfImages(); i++)
    {
        DEBUG_INFO("Making new mesh remapper for image " << i << ".");
        meshes.push_back(MeshInfo(m_pano, i, view_state, layout_mode_on));
    }
}

void MeshManager::RenderMesh(unsigned int image_number) const
{
    meshes[image_number].CallList();
}

unsigned int MeshManager::GetDisplayList(unsigned int image_number) const
{
    return meshes[image_number].display_list_number;
}

void MeshManager::SetLayoutMode(bool state)
{
    if (layout_mode_on == state) return;
    layout_mode_on = state;
    /* All meshes must be recalculated, since the layout mode uses meshes that
     * do not resemble properly remapped images.
     */
    meshes.clear();
}

void MeshManager::SetLayoutScale(double scale)
{
    for(unsigned int i=0;i<meshes.size();i++)
        meshes[i].SetScaleFactor(scale);
};

MeshManager::MeshInfo::MeshInfo(PT::Panorama * m_pano_in,
                                unsigned int image_number_in,
                                ViewState * view_state_in,
                                bool layout_mode_on_in)
    :   display_list_number(glGenLists(1)), // Find a free display list.
        image_number(image_number_in),
        m_pano(m_pano_in),
        m_view_state(view_state_in),
        remap(layout_mode_on_in ? (MeshRemapper *) new LayoutRemapper(m_pano, image_number, m_view_state)
                                : (MeshRemapper *) new ChoosyRemapper(m_pano, image_number, m_view_state)),
        layout_mode_on(layout_mode_on_in),
        scale_factor(3.0)
{
    Update();
}

MeshManager::MeshInfo::MeshInfo(const MeshInfo & source)
    // copy remap object and display list, instead of references.
    :   display_list_number(glGenLists(1)),
    image_number(source.image_number),
    m_pano(source.m_pano),
    m_view_state(source.m_view_state),
    remap(source.layout_mode_on ? (MeshRemapper *) new LayoutRemapper(source.m_pano, source.image_number, source.m_view_state)
                                : (MeshRemapper *) new ChoosyRemapper(source.m_pano, source.image_number, source.m_view_state)),
    layout_mode_on(source.layout_mode_on),
    scale_factor(3.0)
{
    Update();
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
        double scale = m_view_state->GetVisibleArea().width() /
                       sqrt((double) m_pano->getNrOfImages()) / scale_factor;
        MeshRemapper & remapper_ref = *remap;
        LayoutRemapper &r = dynamic_cast<LayoutRemapper &>(remapper_ref);
        r.setScale(scale);
    }
    CompileList();
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
    DEBUG_INFO("Preparing to compile a display list for " << image_number
              << ".");
    DEBUG_ASSERT(remap);
    unsigned int number_of_faces = 0;
    glNewList(display_list_number, GL_COMPILE);
        remap->UpdateAndResetIndex();
        DEBUG_INFO("Specifying faces in display list.");
        #ifndef WIREFRAME
        glBegin(GL_QUADS);
        #endif
            // get each face's coordinates from the remapper
            MeshRemapper::Coords coords;
            while (remap->GetNextFaceCoordinates(&coords))
            {
                number_of_faces++;
                // go in an anticlockwise direction
                #ifdef WIREFRAME
                glBegin(GL_LINE_LOOP);
                #endif
                glTexCoord2dv(coords.tex_c[0][0]);
                glVertex2dv(coords.vertex_c[0][0]);
                glTexCoord2dv(coords.tex_c[0][1]);
                glVertex2dv(coords.vertex_c[0][1]);
                glTexCoord2dv(coords.tex_c[1][1]);
                glVertex2dv(coords.vertex_c[1][1]);
                glTexCoord2dv(coords.tex_c[1][0]);
                glVertex2dv(coords.vertex_c[1][0]);
                #ifdef WIREFRAME
                glEnd();
                #endif
            }
        #ifndef WIREFRAME
        glEnd();
        #endif
    glEndList();
    DEBUG_INFO("Prepared a display list for " << image_number << ", using "
              << number_of_faces << " face(s).");
}

