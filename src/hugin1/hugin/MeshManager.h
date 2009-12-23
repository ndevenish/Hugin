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
    /// Remove meshes for images that have been deleted.
    void CleanMeshes();
    void RenderMesh(unsigned int image_number) const;
    unsigned int GetDisplayList(unsigned int image_number) const;
    
    /** Turn layout mode on or off.
     * 
     * When in layout mode, the images appear with their centre in the remapped
     * position, but the rest of the image is drawn undistorted around that.
     * 
     * @param state true to turn on layout mode, false to turn it off.
     */
    void SetLayoutMode(bool state);
private:
    PT::Panorama  * m_pano;
    ViewState * view_state;
    /** Handles the remapper and a display list for a specific image.
     */
    class MeshInfo
    {
    public:
        /** Constructor: Creates the mesh for a given image of a panorama.
         * @param m_pano The panorama that has the image we would like to remap
         * @param image_number The number of the image in that panorama
         * @param view_state The ViewState object for the particular view this
         * mesh will be used in.
         * @param layout_mode_on True if we should generate a mesh for layout
         * mode, false for a normally remapped mesh.
         */
        MeshInfo(PT::Panorama * m_pano, unsigned int image_number,
                 ViewState * view_state, bool layout_mode_on);
        /** copy constructor: makes a MeshInfo representing the same object but
         * using a differrent display list, allowing the first one to be freed.
         */
        MeshInfo(const MeshInfo & source);
        ~MeshInfo();
        /// Draw the mesh
        void CallList() const;
        /// Recreate the mesh when the image or panorama it represents changes.
        void Update();
        unsigned int display_list_number;
    private:
        unsigned int image_number;
        PT::Panorama *m_pano;
        ViewState *m_view_state;
        /// The ramapper we should use
        MeshRemapper * remap;
        /// Use the remapper to create the display list.
        void CompileList();
        bool layout_mode_on;
    };
    std::vector<MeshInfo> meshes;
    bool layout_mode_on;
};

#endif

