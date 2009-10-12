// -*- c-basic-offset: 4 -*-
/** @file MeshManager.h
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

#ifndef _MESHMANAGER_H
#define _MESHMANAGER_H

#include "PT/Panorama.h"

class MeshRemapper;
class ViewState;

/** A MeshManager handles the graphics system representation of a remapping,
 * by creating OpenGL display lists that draw a remapped image.
 * The coordinates used in the display list are calculated by a MeshRemapper
 */
class MeshManager
{
public:
    MeshManager(PT::Panorama *pano, ViewState *view_state);
    ~MeshManager();
    void CheckUpdate();
    void RenderMesh(unsigned int image_number);
    unsigned int GetDisplayList(unsigned int image_number);
    /// Remove meshes for images that have been deleted.
    void CleanMeshes();
private:
    PT::Panorama  * m_pano;
    ViewState * view_state;
    void UpdateMesh(unsigned int image_number);
    class MeshInfo
    {
    public:
        MeshInfo();
        void SetSource(PT::Panorama *m_pano, unsigned int image_number,
                       ViewState *view_state);
        ~MeshInfo();
        void CallList();
        void Update();
        unsigned int display_list_number;
    private:
        unsigned int image_number;
        PT::Panorama *m_pano;
        MeshRemapper *remap;
        void CompileList();
    };
    std::map<unsigned int, MeshInfo> meshes;
};

#endif

