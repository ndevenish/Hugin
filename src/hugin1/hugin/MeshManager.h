// -*- c-basic-offset: 4 -*-
/** @file MeshManager.h
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

#ifndef _MESHMANAGER_H
#define _MESHMANAGER_H

#include "PT/Panorama.h"

#include "MeshRemapper.h"

class MeshRemapper;
class VisualizationState;

/** A MeshManager handles the graphics system representation of a remapping,
 * by creating OpenGL display lists that draw a remapped image.
 * The coordinates used in the display list are calculated by a MeshRemapper
 */
class MeshManager
{
public:
    MeshManager(PT::Panorama *pano, VisualizationState *visualization_state);
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
    void SetLayoutScale(double scale);

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
        MeshInfo(PT::Panorama * m_pano, HuginBase::SrcPanoImage * image,
                 VisualizationState * visualization_state, bool layout_mode_on);
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
        void SetScaleFactor(double scale);
        void SetSrcImage(HuginBase::SrcPanoImage * image) {this->image = *image;}

        /**
         * a class to handle a 3D point
         */
        class Coord3D
        {
            public:
                Coord3D() {}
                Coord3D(hugin_utils::FDiff2D & coord) {x = coord.x; y = coord.y; z = 0;}
                double x,y,z;
        };

        /**
         * a class to keep data of a single rectangle with texture coordinates
         */
        class MeshCoords3D {
        public:
            MeshCoords3D() {}
            MeshCoords3D(const MeshRemapper::Coords & coords);
            double tex_coords[2][2][2];
            double vertex_coords[2][2][3];
        };

        
    protected:

        virtual void BeforeCompile() {}
        virtual void Transform() {}
        virtual void AfterCompile() {}
    
        HuginBase::SrcPanoImage image;
        PT::Panorama *m_pano;
        double scale_factor;
        VisualizationState *m_visualization_state;
        /// The ramapper we should use
        MeshRemapper * remap;
        /// Use the remapper to create the display list.
        void CompileList();
        bool layout_mode_on;
    };

    /**
     * subclass of MeshInfo for the preview
     * It actually does nothing in addition to the base class
     */
    class PreviewMeshInfo : public MeshInfo
    {
    public:
        PreviewMeshInfo(PT::Panorama * m_pano, HuginBase::SrcPanoImage * image,
                 VisualizationState * visualization_state, bool layout_mode_on) : MeshInfo(m_pano, image, visualization_state, layout_mode_on) {
            Update();
        }
        PreviewMeshInfo(const PreviewMeshInfo & source) : MeshInfo((MeshInfo)source) {
            Update();
        }

        static MeshCoords3D GetMeshCoords3D(MeshRemapper::Coords &coords, VisualizationState * state) {return MeshCoords3D(coords);}
        static Coord3D GetCoord3D(hugin_utils::FDiff2D & coord, VisualizationState * state) {return Coord3D(coord);}

    };

    /**
     * a subclass for the panosphere
     * it converts coordinates obtained from an equirectangular projection to 3D coordinates on the sphere
     */
    class PanosphereOverviewMeshInfo : public MeshInfo
    {
    public:
        PanosphereOverviewMeshInfo(PT::Panorama * m_pano, HuginBase::SrcPanoImage * image,
                 VisualizationState * visualization_state, bool layout_mode_on)
            : MeshInfo(m_pano, image, visualization_state, layout_mode_on) {
                scale_factor *= scale_diff;
                Update();
            }

        PanosphereOverviewMeshInfo(const PanosphereOverviewMeshInfo & source)
            : MeshInfo((MeshInfo) source) {
                Update();
            }

        /**
         * convert from spherical to cartesian coordinates
         */
        static void Convert(double &x, double &y, double &z, double th, double ph, double r);

        static MeshCoords3D GetMeshCoords3D(MeshRemapper::Coords &coords, VisualizationState * state);
        static Coord3D GetCoord3D(hugin_utils::FDiff2D &coord, VisualizationState * state);

        /** scale factor to be used for the layout mode TODO: test this for more scenarios */
        static const double scale_diff;

    protected:

        void BeforeCompile();
        void Transform();
        void AfterCompile();

        double yaw,pitch;

    };

    /**
     * subclass for the plane overview mode.
     */
    class PlaneOverviewMeshInfo : public MeshInfo
    {
    public:
        PlaneOverviewMeshInfo(PT::Panorama * m_pano, HuginBase::SrcPanoImage * image,
                 VisualizationState * visualization_state, bool layout_mode_on)
            : MeshInfo(m_pano, image, visualization_state, layout_mode_on) {
                Update();
            }

        PlaneOverviewMeshInfo(const PlaneOverviewMeshInfo & source)
            : MeshInfo((MeshInfo) source) {
                Update();
            }

        const static double scale;
        static MeshCoords3D GetMeshCoords3D(MeshRemapper::Coords &coords, VisualizationState * state);
        static Coord3D GetCoord3D(hugin_utils::FDiff2D &coord, VisualizationState * state);

    };
    
    virtual MeshInfo::MeshCoords3D GetMeshCoords3D(MeshRemapper::Coords &coords) = 0;
    virtual MeshInfo::Coord3D GetCoord3D(hugin_utils::FDiff2D &) = 0;
    

    virtual MeshInfo * ObtainMeshInfo(HuginBase::SrcPanoImage *, bool layout_mode_on) = 0;

protected:


    PT::Panorama  * m_pano;
    VisualizationState * visualization_state;

    
    std::vector<MeshInfo*> meshes;
    bool layout_mode_on;
};


class PanosphereOverviewMeshManager : public MeshManager
{
public:
    PanosphereOverviewMeshManager(PT::Panorama *pano, VisualizationState *visualization_state) : MeshManager(pano, visualization_state) {}
    MeshInfo::MeshCoords3D GetMeshCoords3D(MeshRemapper::Coords &coords) {return MeshManager::PanosphereOverviewMeshInfo::GetMeshCoords3D(coords, visualization_state);}
    MeshInfo::Coord3D GetCoord3D(hugin_utils::FDiff2D &coord) {return MeshManager::PanosphereOverviewMeshInfo::GetCoord3D(coord,visualization_state);}

    MeshInfo * ObtainMeshInfo(HuginBase::SrcPanoImage *, bool layout_mode_on);
};

class PlaneOverviewMeshManager : public MeshManager
{
public:
    PlaneOverviewMeshManager(PT::Panorama *pano, VisualizationState *visualization_state) : MeshManager(pano, visualization_state) {}
    MeshInfo::MeshCoords3D GetMeshCoords3D(MeshRemapper::Coords &coords) {return MeshManager::PlaneOverviewMeshInfo::GetMeshCoords3D(coords, visualization_state);}
    MeshInfo::Coord3D GetCoord3D(hugin_utils::FDiff2D &coord) {return MeshManager::PlaneOverviewMeshInfo::GetCoord3D(coord,visualization_state);}

    MeshInfo * ObtainMeshInfo(HuginBase::SrcPanoImage *, bool layout_mode_on);

};

class PreviewMeshManager : public MeshManager
{
public:
    PreviewMeshManager(PT::Panorama *pano, VisualizationState *visualization_state) : MeshManager(pano, visualization_state) {}
    MeshInfo::MeshCoords3D GetMeshCoords3D(MeshRemapper::Coords &coords) {return MeshManager::PreviewMeshInfo::GetMeshCoords3D(coords, visualization_state);}
    MeshInfo::Coord3D GetCoord3D(hugin_utils::FDiff2D &coord) {return MeshManager::PreviewMeshInfo::GetCoord3D(coord,visualization_state);}

    MeshInfo * ObtainMeshInfo(HuginBase::SrcPanoImage *, bool layout_mode_on);

};


#endif

