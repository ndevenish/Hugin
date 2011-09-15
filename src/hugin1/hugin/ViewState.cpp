// -*- c-basic-offset: 4 -*-

/** @file ViewState.cpp
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifdef __WXMAC__
#include "panoinc_WX.h"
#include "panoinc.h"
#endif

#include "ViewState.h"
#include "MeshManager.h"



ViewState::ViewState(PT::Panorama *pano, bool supportMultiTexture)
{


    m_pano = pano;
    m_multiTexture=supportMultiTexture;
    m_pano->addObserver(this);
    // we will need to update everything for this panorama.
    dirty_image_sizes = true;
    images_removed = true;
    number_of_images = m_pano->getNrOfImages();
    for (unsigned int img = 0; img < number_of_images; img++)
    {
        img_states[img] = m_pano->getSrcImage(img);
        dirty_mask[img].val = false;
    }
    opts = m_pano->getOptions();
    projection_info = new OutputProjectionInfo(&opts);
    // now set the texture manager up.
    m_tex_manager = new TextureManager(m_pano, this);
}

ViewState::~ViewState()
{
    m_pano->removeObserver(this);
    delete projection_info;
    delete m_tex_manager;
}


void ViewState::panoramaChanged(HuginBase::PanoramaData &pano)
{

    bool require_draw=false;

    // anything could have happened, check everything.
    HuginBase::PanoramaOptions new_opts  = m_pano->getOptions();
    SetOptions(&new_opts);
    unsigned int imgs = m_pano->getNrOfImages();
    for (unsigned int img = 0; img < imgs; img++)
    {
        HuginBase::SrcPanoImage new_image = m_pano->getSrcImage(img);
        SetSrcImage(img, &new_image);
        // has the enabled state changed in the preview?
        bool new_active = m_pano->getImage(img).getActive();
        if (new_active != active[img])
        {
            require_draw = true;
            active[img] = new_active;
        }
    }
    // has the number of images changed?
    if (imgs < number_of_images)
    {
        // we've lost some
        dirty_image_sizes = true;
        require_draw = true;
        images_removed = true;
    } else if (imgs > number_of_images)
    {
        // added images. Assume it doesn't affect the rest.
        dirty_image_sizes = true;
        require_draw = true;
        // FIXME more might need to be done, if the new images are not the last
        // ones in order of image number.
    }
    number_of_images = imgs;

    if (require_draw) {
        //refresh function is called in the respective VisualizationState callback
        for (std::map<VisualizationState*,bool>::iterator it = vis_states.begin() ; it != vis_states.end() ; it++) {
            DEBUG_DEBUG("PanoChanged - iterator before");
            if (it->second) {
                DEBUG_DEBUG("PanoChanged - iterator after");
                it->first->Redraw();
            }
        }
    }

}

void ViewState::panoramaImagesChanged(HuginBase::PanoramaData&,
                                      const HuginBase::UIntSet&)
{
    // actually this stuff is handled by panoramaChanged.
}

void ViewState::SetOptions(const HuginBase::PanoramaOptions *new_opts)
{

    bool dirty_projection = false;

    for (std::map<VisualizationState*,bool>::iterator it = vis_states.begin() ; it != vis_states.end() ; it++) {

        if (!(it->second)) continue;
        
        // compare the options
        if (   new_opts->getSize() != opts.getSize()
            || new_opts->getProjection() != opts.getProjection()
            || new_opts->getProjectionParameters() != opts.getProjectionParameters()
            || new_opts->getHFOV() != opts.getHFOV()
            || new_opts->getVFOV() != opts.getVFOV()
           )
        {
            // output projection changed. All images' meshes need recalculating.
            unsigned int imgs = m_pano->getNrOfImages();
            for (unsigned int img = 0; img < imgs; img++)
            {
                it->first->SetDirtyMesh(img);
            }
            // we should also change the viewport to fit new the dimensions.
            dirty_projection = true;
            it->first->SetDirtyViewport();
            it->first->ForceRequireRedraw();
        }
        if (  new_opts->outputExposureValue != opts.outputExposureValue)
        {
            // output exposure changed. All image photometrics are now different.
            it->first->SetDirtyViewport();
            dirty_photometrics = true;
        }
        if (   new_opts->getROI() != opts.getROI()
           )
        {
            // this is all done every frame anyway.
            it->first->ForceRequireRedraw();
        }

        it->first->SetOptions(new_opts);

    }
    // store the new options
    opts = *new_opts;
    if (dirty_projection)
    {
        // we need to update the projection info as well.
        delete projection_info;
        projection_info = 0;
        projection_info = new OutputProjectionInfo(&opts);
    }
}

void ViewState::SetSrcImage(unsigned int image_nr, HuginBase::SrcPanoImage *new_img)
{
    bool dirty_mesh = false;
    bool dirty_draw = false;

    if (number_of_images <= image_nr)
    {
        // this must be an addition, since we didn't have this many images.
        dirty_mesh = true;
        dirty_image_sizes = true;
        dirty_draw = true;
    } else {
        // compare the options
        HuginBase::SrcPanoImage *img = &img_states[image_nr];
        // if the filename has changed, something has probably been deleted
        if (new_img->getFilename() != img->getFilename())
        {
            images_removed = true;
            // since we use image numbers to identify meshes and images,
            // we can't really tell what happened.
        }
        // has the projection changed?
        if (   new_img->getRoll() != img->getRoll()
            || new_img->getPitch() != img->getPitch()
            || new_img->getYaw() != img->getYaw()
            || new_img->getX() != img->getX()
            || new_img->getY() != img->getY()
            || new_img->getZ() != img->getZ()
            || new_img->getHFOV() != img->getHFOV()
            || new_img->getProjection() != img->getProjection()
            || new_img->getShear() != img->getShear()
            || new_img->getRadialDistortionCenterShift()
                                       == img->getRadialDistortionCenterShift()
            || new_img->getRadialDistortion() != img->getRadialDistortion()
            || new_img->getCropRect() != img->getCropRect()
           )
        {
            dirty_mesh = true;
            dirty_draw = true;
//            dirty_mesh[image_nr].val = true;
            // the field of view affects the image size calculations.
            if (new_img->getHFOV() != img->getHFOV())
            {
                dirty_image_sizes = true;
            }
        }
        // photometric adjustments
        if (   new_img->getVigCorrMode() != img->getVigCorrMode()
            || new_img->getRadialVigCorrCoeff() != img->getRadialVigCorrCoeff()
            || new_img->getRadialVigCorrCenterShift() !=
                                              img->getRadialVigCorrCenterShift()
            || new_img->getExposureValue() != img->getExposureValue()
            || new_img->getGamma() != img->getGamma()
            || new_img->getWhiteBalanceRed() != img->getWhiteBalanceRed()
            || new_img->getWhiteBalanceBlue() != img->getWhiteBalanceBlue()
            || new_img->getResponseType() != img->getResponseType()
           )
        {
            // real time photometric correction just needs a redraw.
            /* full photometric correction will be redone automatically by the
               TextureManager next redraw.                                    */
            /* FIXME only white balance and exposure are actually used for
               real-time photometric correction. */
            dirty_draw = true;
        }
        // mask stuff
        if(new_img->getActiveMasks() != img->getActiveMasks())
        {
            dirty_mask[image_nr].val = true;
            dirty_draw=true;
        };
    }
    // store the new options
    img_states[image_nr] = *new_img;


    for (std::map<VisualizationState*,bool>::iterator it = vis_states.begin() ; it != vis_states.end() ; it++) {
        if (!(it->second)) continue;
        if (dirty_draw) it->first->ForceRequireRedraw();
        if (dirty_mesh) it->first->SetDirtyMesh(image_nr);
        it->first->SetSrcImage(image_nr, new_img);
    }
    
}

void ViewState::ForceRequireRedraw()
{
    // this is generally called by preview tools. We let them manage themselves.
    // often they give some user interface thing that doesn't reflect a change
    // in the panorama at all, so we let them force a redraw.
    for (std::map<VisualizationState*,bool>::iterator it = vis_states.begin() ; it != vis_states.end() ; it++) {
        if (!(it->second)) continue;
        it->first->ForceRequireRedraw();
    }
}


HuginBase::PanoramaOptions *ViewState::GetOptions()
{
    return &opts;
}

OutputProjectionInfo *ViewState::GetProjectionInfo()
{
    return projection_info;
}

HuginBase::SrcPanoImage *ViewState::GetSrcImage(unsigned int image_nr)
{
    return &img_states[image_nr];
}


bool ViewState::RequireRecalculateImageSizes()
{
    return dirty_image_sizes;
}

bool ViewState::RequireRecalculatePhotometric()
{
    return dirty_photometrics;
}

bool ViewState::ImagesRemoved()
{
    return images_removed;
}

bool ViewState::RequireRecalculateMasks(unsigned int image_nr)
{
    if (number_of_images > image_nr)
    {
        return dirty_mask[image_nr].val;
    }
    return false;
}


void ViewState::FinishedDraw()
{
    // update our copy of the state and clear all the dirty flags.
    number_of_images = m_pano->getNrOfImages();
    img_states.clear();
    active.clear();
    for (unsigned int img = 0; img < number_of_images; img++)
    {
        img_states[img] = m_pano->getSrcImage(img);
        active[img] = m_pano->getImage(img).getActive();
    }
    opts = m_pano->getOptions();
    
    Clean();
}

void ViewState::Clean()
{
    dirty_image_sizes = false;
    images_removed = false;
    dirty_photometrics = false;
    dirty_mask.clear();
}

void ViewState::DoUpdates()
{
    DEBUG_DEBUG("VIEW STATE DO UPDATES");
    m_tex_manager->CheckUpdate();
    DEBUG_DEBUG("VIEW STATE END DO UPDATES");
}

void ViewState::Redraw()
{

    for (std::map<VisualizationState*,bool>::iterator it = vis_states.begin() ; it != vis_states.end() ; it++) {
        
        if (!(it->second)) continue;
        
        it->first->Redraw();
        
    }


}








VisualizationState::~VisualizationState()
{
    m_view_state->vis_states[this] = false;
    delete m_mesh_manager;
}


float VisualizationState::GetScale()
{
    return scale;
}

void VisualizationState::SetScale(float scale_in)
{
    scale = scale_in;
    // When resizing the window this can make the level detail of existing
    // meshes be too high or low, but we don't want to do to much calculation
    // so limit the forced recalculation of meshes to significant changes.
    if (genscale == 0.0)
    {
        // should only happen the first time it is used. In which case we will
        // regenerate the meshes anyways.
        genscale = scale;
    } else {
        double difference = scale > genscale ?
               scale / genscale : genscale / scale;
        if (difference > 1.25)
        {
            genscale = scale;
            unsigned int number_of_images = m_pano->getNrOfImages();
            for (unsigned int img = 0; img < number_of_images; img++)
            {
                dirty_mesh[img].val = true;
            }        
        }
    }
}


void VisualizationState::Redraw()
{
    DEBUG_DEBUG("REDRAW OUT");
    if (RequireDraw())
    {
        DEBUG_DEBUG("REDRAW IN");
        RefreshFunc(refreshArg);
    }
}

bool VisualizationState::RequireRecalculateViewport()
{
    return dirty_viewport;
}

bool VisualizationState::RequireRecalculateMesh (unsigned int image_nr)
{

    unsigned int number_of_images = m_pano->getNrOfImages();
    if (number_of_images > image_nr)
    {
        return dirty_mesh[image_nr].val;
    }
    // if we didn't think there were enough images, create the mesh for the
    //   first time
    return true;
}


bool VisualizationState::RequireDraw()
{
    return (dirty_draw);
}

void VisualizationState::ForceRequireRedraw()
{
    dirty_draw = true;
}

void VisualizationState::FinishedDraw()
{
    DEBUG_DEBUG("VIS State Finished draw");
    dirty_mesh.clear();
    dirty_viewport = false;
    dirty_draw = false;
    m_view_state->FinishedDraw();
}

void VisualizationState::DoUpdates()
{
    DEBUG_DEBUG("BEGIN UPDATES");
    m_view_state->DoUpdates();
    DEBUG_DEBUG("END UPDATES");
    m_mesh_manager->CheckUpdate();
    DEBUG_DEBUG("END UPDATES");
}

unsigned int VisualizationState::GetMeshDisplayList(unsigned int image_number)
{
    return m_mesh_manager->GetDisplayList(image_number);
}


HuginBase::PanoramaOptions * VisualizationState::GetOptions()
{
    return m_view_state->GetOptions();
}

 OutputProjectionInfo * VisualizationState::GetProjectionInfo()
{
    return m_view_state->GetProjectionInfo();
}

HuginBase::SrcPanoImage * VisualizationState::GetSrcImage(unsigned int image_nr)
{
    return m_view_state->GetSrcImage(image_nr);
}



PanosphereOverviewVisualizationState::PanosphereOverviewVisualizationState(PT::Panorama* pano, ViewState* view_state, GLViewer * viewer, void (*RefreshFunction)(void*), void *arg)
        : OverviewVisualizationState(pano, view_state, viewer, RefreshFunction, arg, (PanosphereOverviewMeshManager*) NULL) 
{
    scale = 1;

    angx = M_PI / 2.0;
    angy = 0;
    fov = 40;
    R = 500;
    sphere_radius = 100;

    opts = (*(m_view_state->GetOptions()));
    opts.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
    opts.setHFOV(360.0);
    opts.setVFOV(180.0);
    projection_info = new OutputProjectionInfo(&opts);
}

PanosphereOverviewVisualizationState::~PanosphereOverviewVisualizationState()
{
    delete projection_info;
}

HuginBase::PanoramaOptions * PanosphereOverviewVisualizationState::GetOptions()
{
    return &opts;
}

OutputProjectionInfo *PanosphereOverviewVisualizationState::GetProjectionInfo()
{
    return projection_info;
}

void PanosphereOverviewVisualizationState::SetOptions(const HuginBase::PanoramaOptions * new_opts)
{
    opts = *new_opts;
    opts.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
    opts.setHFOV(360.0);
    opts.setVFOV(180.0);
    delete projection_info;
    projection_info = new OutputProjectionInfo(&opts);
}

void PanosphereOverviewVisualizationState::setAngX(double angx_in)
{
    angx = angx_in;
    dirty_draw = true;
}

void PanosphereOverviewVisualizationState::setAngY(double angy_in)
{
    angy = angy_in;
    dirty_draw = true;
}


PlaneOverviewVisualizationState::PlaneOverviewVisualizationState(PT::Panorama* pano, ViewState* view_state, GLViewer * viewer, void (*RefreshFunction)(void*), void *arg)
        : OverviewVisualizationState(pano, view_state, viewer, RefreshFunction, arg, (PlaneOverviewMeshManager*) NULL) 
{

    scale = 1;

    fov = 60;
    R = 500;
    X = 0;
    Y = 0;

    opts = (*(m_view_state->GetOptions()));
    opts.setProjection(HuginBase::PanoramaOptions::RECTILINEAR);
    //TODO: hfov and vfov need to be divided into values for the output and values for the visualization
    opts.setHFOV(90.0);
    opts.setVFOV(90.0);
    projection_info = new OutputProjectionInfo(&opts);
}

PlaneOverviewVisualizationState::~PlaneOverviewVisualizationState()
{
    delete projection_info;
}

HuginBase::PanoramaOptions * PlaneOverviewVisualizationState::GetOptions()
{
    return &opts;
}

OutputProjectionInfo *PlaneOverviewVisualizationState::GetProjectionInfo()
{
    return projection_info;
}

void PlaneOverviewVisualizationState::SetOptions(const HuginBase::PanoramaOptions * new_opts)
{
    opts = *new_opts;
    opts.setProjection(HuginBase::PanoramaOptions::RECTILINEAR);
    opts.setHFOV(90.0);
    opts.setVFOV(90.0);
    delete projection_info;
    projection_info = new OutputProjectionInfo(&opts);
}


