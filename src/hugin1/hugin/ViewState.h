// -*- c-basic-offset: 4 -*-

/** @file ViewState.h
 *
 *  @author James Legg
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

/* A ViewState holds information about what is visible, the state of the
 * various bits of the panorama as it is shown in a preview, and the scaling of
 * the preview. The state of the panorama  options and images are stored so
 * that we can continuously change properties interactively without bothering
 * the real panorama object and therefore the undo / redo history. The ViewState
 * also manages what needs to be recalculated to update the view.
 *
 * When a change occurs, we examine any differences with our stored state that
 * we care about. It doesn't matter if the change is caused by an interactive
 * tool or an update to the panorama object.
 * If we find a change that makes our preview out of sync, we record what needs
 * to be done to bring it up to date. Note some changes do not create any
 * difference to the preview, so we don't even want to redraw sometimes.
 * After the changes have been made, we can state what needs redoing when asked.
 * The texture manager and mesh manager will ask what they need to do.
 * After that, any changes we suggested should have been made, and FinishDraw()
 * is called. At this point we declare everything is clean and up to date again.
 *
 * We don't intitiate the calculations ourself when doing things interactively
 * so we can group several changes together, e.g. when dragging a group of
 * images together SetSrcImage is called for each image but we only want to
 * redraw after they have all been moved. However, when the panorama changes
 * we want to initiate an update the preview ourself, so all the main GUI
 * interactions work.
 */

#ifndef __VIEWSTATE_H
#define __VIEWSTATE_H

#include "hugin_utils/utils.h"
#include <panodata/PanoramaData.h>
#include <panodata/Panorama.h>
#include "PT/Panorama.h"
#include "OutputProjectionInfo.h"
#include <vigra/diff2d.hxx>
#include "TextureManager.h"
#include "MeshManager.h"

class ViewState : public HuginBase::PanoramaObserver
{
public:
    // constructor: we need to know what panorama we deal with.
    ViewState(PT::Panorama *pano, void (*RefreshFunction)(void *), void *arg);
    ~ViewState();
    // the scale is the number of screen pixels per panorama pixel.
    float GetScale();
    void SetScale(float scale);
    
    // when the real panorama changes, we want to update ourself to reflect it.
    // we will force a redraw if anything worthwhile changes.
    void panoramaChanged(HuginBase::PanoramaData &pano);
    void panoramaImagesChanged(HuginBase::PanoramaData&, const HuginBase::UIntSet&);

    // For interactive control, the real panorama does not change. Instead one
    // of the following functions will be called:
    void SetOptions(const HuginBase::PanoramaOptions *new_opts);
    void SetSrcImage(unsigned int image_nr, HuginBase::SrcPanoImage *new_img);
    void SetLens(unsigned int lens_nr, HuginBase::Lens *new_lens);
    // someone else decides we need to redraw next time around.
    void ForceRequireRedraw();
    // then we compare with the stored state and set dirty flags as neceassry.
    
    HuginBase::PanoramaOptions *GetOptions();
    OutputProjectionInfo *GetProjectionInfo();
    HuginBase::SrcPanoImage *GetSrcImage(unsigned int image_nr);
    
    // stuff used directly for drawing the preview, made accessible for tools.
    unsigned int GetMeshDisplayList(unsigned int image_nr);
    MeshManager * GetMeshManager() {return m_mesh_manager;}
    TextureManager * GetTextureManager() {return m_tex_manager;}
    
    // These functions are used to identify what needs to be redone on the next
    // redraw.
    // return true if we need to recalculate the mesh
    bool RequireRecalculateMesh (unsigned int image_nr);
    // return true if we should check the generated mip levels of the textures
    bool RequireRecalculateImageSizes();
    // return true if we should update a texture's photometric correction
    bool RequireRecalculatePhotometric();
    // return true if we need to redraw at all
    bool RequireDraw();
    // return true if we should check the renderers' viewport
    bool RequireRecalculateViewport();
    // return true if images have been removed
    bool ImagesRemoved();
    
    // this is called when a draw has been performed, so we can assume the
    // drawing state (textures, meshes) are now all up to date.
    void FinishedDraw();
    
    // The visible area is the part of the panarama visible in the view. The
    // coordinates are in panorama pixels.
    void SetVisibleArea(vigra::Rect2D area)
    {
        /* TODO with zooming, update meshes that were generated with this area
         * in mind. Zooming changes the scale, which updates the meshes.
         * Panning on the other hand needs to recalculate meshes as they can
         * ignore the stuff off-screen
         */
        visible_area = area;
    }
    vigra::Rect2D GetVisibleArea()
    {
        return visible_area;
    }
    
    // redraw the preview, but only if something has changed.
    void Redraw();
    // update the meshes and textures as necessary before drawing.
    void DoUpdates();
protected:
    PT::Panorama *m_pano;
    float scale, genscale;
    vigra::Rect2D visible_area;
    void (*RefreshFunc)(void *);
    void *refreshArg;
    std::map<unsigned int, HuginBase::SrcPanoImage> img_states;
    HuginBase::PanoramaOptions opts;
    OutputProjectionInfo *projection_info;
    // std::map<unsigned int, HuginBase::Lens> lens_states;
    unsigned int number_of_images;
    class fbool // a bool that initialises to false.
    {
    public:
        fbool()
        {
            val = false;
        }
        bool val;
    };
    // what needs redoing?
    bool dirty_photometrics;
    std::map<unsigned int, fbool> dirty_mesh;
    std::map<unsigned int, bool> active;
    bool dirty_image_sizes, dirty_draw, images_removed, dirty_viewport;
    
    // reset all the dirty flags.
    void Clean();
    
    // this stores all the textures we need.
    TextureManager *m_tex_manager;
    // this stores all the meshes we need.
    MeshManager *m_mesh_manager;
};

#endif

