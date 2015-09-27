// -*- c-basic-offset: 4 -*-

/** @file ViewState.h
 *
 *  @author James Legg
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/* A ViewState or VisualizationState holds information about what is visible, the state of the
 * various bits of the panorama as it is shown in a preview, and the scaling of
 * the preview. The state of the panorama  options and images are stored so
 * that we can continuously change properties interactively without bothering
 * the real panorama object and therefore the undo / redo history. The classes
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
 *
 * The information is divided into two classes:
 *      -   The first class is the ViewState class which holds information that is 
 *          relevant for all Visualizations like the preview, panosphere or plane.
 *          This is mainly dominated by the manipulation of the Textures related stuff
 *          which are the same for all visualizations.
 *          The ViewState also encapsulates all the VisualizationState in itself so that
 *          it can properly inform them when any change occurs
 *
 *      -   The second class is the VisualizationState class which is subclassed for each
 *          type of visualization. The VisualizationState class holds information specific
 *          to each visualization. This includes the state of the meshes, state of the viewport, etc.
 *
 */

#ifndef __VIEWSTATE_H
#define __VIEWSTATE_H

#include "base_wx/wxutils.h"
#include <panodata/PanoramaData.h>
#include <panodata/Panorama.h>
#include "OutputProjectionInfo.h"
#include <vigra/diff2d.hxx>
#include "TextureManager.h"
#include "MeshManager.h"

#include <panodata/PanoramaOptions.h>

class GLViewer;
class VisualizationState;


class ViewState : public HuginBase::PanoramaObserver
{
public:
    // constructor: we need to know what panorama we deal with.
    ViewState(HuginBase::Panorama *pano,  bool supportMultiTexture);
    ~ViewState();
    // when the real panorama changes, we want to update ourself to reflect it.
    // we will force a redraw if anything worthwhile changes.
    void panoramaChanged(HuginBase::Panorama &pano);
    void panoramaImagesChanged(HuginBase::Panorama&, const HuginBase::UIntSet&);

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
    
    TextureManager * GetTextureManager() {return m_tex_manager;}
    
    bool GetSupportMultiTexture() const { return m_multiTexture; };
    // These functions are used to identify what needs to be redone on the next
    // redraw.
    // return true if we should update a texture's photometric correction
    bool RequireRecalculatePhotometric();
    // return true if we should check the generated mip levels of the textures
    bool RequireRecalculateImageSizes();
    // return true if we should update a mask
    bool RequireRecalculateMasks(unsigned int image_nr);
    // return true if images have been removed
    bool ImagesRemoved();
    
    // this is called when a draw has been performed, so we can assume the
    // drawing state (textures, meshes) are now all up to date.
    void FinishedDraw();
    
    // update the meshes and textures as necessary before drawing.
    void DoUpdates();

    void Redraw();

    std::map<VisualizationState*, bool> vis_states;

protected:

    HuginBase::Panorama *m_pano;
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
    std::map<unsigned int, bool> active;
    std::map<unsigned int, fbool> dirty_mask;
    bool dirty_image_sizes, images_removed;
    // reset all the dirty flags.
    void Clean();
    
    // this stores all the textures we need.
    TextureManager *m_tex_manager;
    bool m_multiTexture;
};

class VisualizationState
{
public:

    template <class M>
    VisualizationState(HuginBase::Panorama* pano, ViewState* view_state, GLViewer * viewer, void (*RefreshFunction)(void*), void *arg, M* classArg)
    {
        m_pano = pano;
        m_view_state = view_state;
        RefreshFunc = RefreshFunction;
        refreshArg = arg;
        dirty_draw = true;
        dirty_viewport = true;
        unsigned int number_of_images = m_pano->getNrOfImages();
        for (unsigned int img = 0; img < number_of_images; img++)
        {
            dirty_mesh[img].val = true;
        }
        genscale = 0.0;
        m_mesh_manager = new M(m_pano, this);
        m_view_state->vis_states[this] = true;
        m_viewer = viewer;
    }

    virtual ~VisualizationState();

    virtual HuginBase::PanoramaOptions *GetOptions();
    virtual OutputProjectionInfo *GetProjectionInfo();
    virtual HuginBase::SrcPanoImage *GetSrcImage(unsigned int image_nr);

    virtual void SetOptions(const HuginBase::PanoramaOptions *new_opts) {}
    virtual void SetSrcImage(unsigned int image_nr, HuginBase::SrcPanoImage * new_img) {}

    // return true if we need to recalculate the mesh
    bool RequireRecalculateMesh (unsigned int image_nr);
    // return true if we need to redraw at all
    bool RequireDraw();
    // return true if we should check the renderers' viewport
    bool RequireRecalculateViewport();

    // the scale is the number of screen pixels per panorama pixel.
    float GetScale();
    void SetScale(float scale);

    // stuff used directly for drawing the preview, made accessible for tools.
    unsigned int GetMeshDisplayList(unsigned int image_nr);
    MeshManager * GetMeshManager() {return m_mesh_manager;}

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

    ViewState* getViewState() {return m_view_state;}

    // redraw the preview, but only if something has changed.
    void Redraw();

    // update the meshes and textures as necessary before drawing.
    void DoUpdates();

    void SetDirtyMesh(int image_nr) {dirty_mesh[image_nr].val = true;}
    void ForceRequireRedraw();
    void SetDirtyViewport() {dirty_viewport = true;}

    GLViewer * GetViewer() {return m_viewer;}

protected:

    HuginBase::Panorama *m_pano;

    class fbool // a bool that initialises to false.
    {
    public:
        fbool()
        {
            val = false;
        }
        bool val;
    };
    // redoing specific only for a certain visualization
    std::map<unsigned int, fbool> dirty_mesh;
    bool dirty_draw, dirty_viewport;

    float scale, genscale, gl_scale;
    vigra::Rect2D visible_area;
    void (*RefreshFunc)(void *);
    void *refreshArg;

    // this stores all the meshes we need.
    MeshManager *m_mesh_manager;
    ViewState *m_view_state;

    GLViewer * m_viewer;
private:
    // don't copy this class
    VisualizationState(const VisualizationState& other);
    VisualizationState& operator=(const VisualizationState& other);
};

class OverviewVisualizationState : public VisualizationState
{
public:
    template <class M>
    OverviewVisualizationState(HuginBase::Panorama* pano, ViewState* view_state, GLViewer * viewer, void(*RefreshFunction)(void*), void *arg, M* classArg)
        : VisualizationState(pano, view_state, viewer, RefreshFunction, arg, (M*) classArg) {}

};

class PanosphereOverviewVisualizationState : public OverviewVisualizationState
{
public:

    PanosphereOverviewVisualizationState(HuginBase::Panorama* pano, ViewState* view_state, GLViewer * viewer, void(*RefreshFunction)(void*), void *arg);
    ~PanosphereOverviewVisualizationState();

    HuginBase::PanoramaOptions *GetOptions();
    OutputProjectionInfo *GetProjectionInfo();

//    HuginBase::SrcPanoImage *GetSrcImage(unsigned int image_nr);
//    void SetSrcImage(unsigned int image_nr, HuginBase::SrcPanoImage * new_img );

    void SetOptions(const HuginBase::PanoramaOptions * new_opts);

    //camera properties
    double getAngY() {return angy;}
    double getAngX() {return angx;}
    double getR() {return R;}
    double getFOV() {return fov;}

    double getSphereRadius() {return sphere_radius;}

    void setAngX(double angx_in);
    void setAngY(double angy_in);
    void setR(double r) {R = r;}


protected:

    double angy;
    double angx;
    double R;
    double fov;

    double sphere_radius;

//    std::map<unsigned int, HuginBase::SrcPanoImage> img_states;
    HuginBase::PanoramaOptions opts;
    OutputProjectionInfo *projection_info;


};

class PlaneOverviewVisualizationState : public OverviewVisualizationState
{
public:

    PlaneOverviewVisualizationState(HuginBase::Panorama* pano, ViewState* view_state, GLViewer * viewer, void(*RefreshFunction)(void*), void *arg);
    ~PlaneOverviewVisualizationState();

    HuginBase::PanoramaOptions *GetOptions();
    OutputProjectionInfo *GetProjectionInfo();

    void SetOptions(const HuginBase::PanoramaOptions * new_opts);

    //camera properties
    double getR() {return R;}
    double getFOV() {return fov;}
    double getX() {return X;}
    double getY() {return Y;}

    void setR(double r) {R = r;}
    void setX(double x) {X = x;}
    void setY(double y) {Y = y;}
    
protected:

    double X;
    double Y;
    double R;
    double fov;

    HuginBase::PanoramaOptions opts;
    OutputProjectionInfo *projection_info;

};

#endif

