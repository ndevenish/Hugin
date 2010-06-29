// -*- c-basic-offset: 4 -*-

/** @file ViewState.cpp
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

#include "ViewState.h"
#include "MeshManager.h"

ViewState::ViewState(PT::Panorama *pano,
                     void (*RefreshFunction)(void *), bool supportMultiTexture, void * arg)
{
    m_pano = pano;
    RefreshFunc = RefreshFunction;
    refreshArg = arg;
    m_multiTexture=supportMultiTexture;
    m_pano->addObserver(this);
    // we will need to update everything for this panorama.
    dirty_image_sizes = true;
    dirty_viewport = true;
    dirty_draw = true;
    images_removed = true;
    number_of_images = m_pano->getNrOfImages();
    for (unsigned int img = 0; img < number_of_images; img++)
    {
        img_states[img] = m_pano->getSrcImage(img);
        dirty_mesh[img].val = true;
        dirty_mask[img].val = false;
    }
    opts = m_pano->getOptions();
    projection_info = new OutputProjectionInfo(&opts);
    genscale = 0.0;
    // now set the texture manager up.
    m_tex_manager = new TextureManager(m_pano, this);
    // same for the mesh manager
    m_mesh_manager = new MeshManager(m_pano, this);
}

ViewState::~ViewState()
{
    m_pano->removeObserver(this);
    delete projection_info;
    delete m_tex_manager;
    delete m_mesh_manager;
}

float ViewState::GetScale()
{
    return scale;
}

void ViewState::SetScale(float scale_in)
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
            for (unsigned int img = 0; img < number_of_images; img++)
            {
                dirty_mesh[img].val = true;
            }        
        }
    }
}

void ViewState::panoramaChanged(HuginBase::PanoramaData &pano)
{
    // anything could have happened, check everything.
    HuginBase::PanoramaOptions new_opts  = m_pano->getOptions();
    SetOptions(&new_opts);
    unsigned int imgs = m_pano->getNrOfImages();
    for (unsigned int img = 0; img < imgs; img++)
    {
        HuginBase::SrcPanoImage new_image = m_pano->getSrcImage(img);
        SetSrcImage(img, &new_image);
        // has the enabled state changed in the preview?
        bool new_active = m_pano->getImage(img).getOptions().active;
        if (new_active != active[img])
        {
            dirty_draw = true;
            active[img] = new_active;
        }
    }
    // has the number of images changed?
    if (imgs < number_of_images)
    {
        // we've lost some
        dirty_image_sizes = true;
        dirty_draw = true;
        images_removed = true;
    } else if (imgs > number_of_images)
    {
        // added images. Assume it doesn't affect the rest.
        dirty_image_sizes = true;
        dirty_draw = true;
        // FIXME more might need to be done, if the new images are not the last
        // ones in order of image number.
    }
    number_of_images = imgs;
    // check if it is worth redrawing. This will update everything else as
    // necessary.
    if (RequireDraw())
    {
        RefreshFunc(refreshArg);
    }
}

void ViewState::panoramaImagesChanged(HuginBase::PanoramaData&,
                                      const HuginBase::UIntSet&)
{
    // actually this stuff is handled by panoramaChanged.
}

void ViewState::SetOptions(const HuginBase::PanoramaOptions *new_opts)
{
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
            dirty_mesh[img].val = true;
        }
        // we should also change the viewport to fit new the dimensions.
        dirty_viewport = true;
        dirty_draw = true;
    }
    if (  new_opts->outputExposureValue != opts.outputExposureValue)
    {
        // output exposure changed. All image photometrics are now different.
        dirty_draw = true;
        dirty_photometrics = true;
    }
    if (   new_opts->getROI() != opts.getROI()
       )
    {
        // this is all done every frame anyway.
        dirty_draw = true;
    }
    // store the new options
    opts = *new_opts;
    if (dirty_viewport)
    {
        // we need to update the projection info as well.
        delete projection_info;
        projection_info = 0;
        projection_info = new OutputProjectionInfo(&opts);
    }
}

void ViewState::SetSrcImage(unsigned int image_nr, HuginBase::SrcPanoImage *new_img)
{
    if (number_of_images <= image_nr)
    {
        // this must be an addition, since we didn't have this many images.
        dirty_mesh[image_nr].val = true;
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
            dirty_mesh[image_nr].val = true;
            dirty_draw = true;
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
            dirty_mask[image_nr].val=true;
            dirty_draw=true;
        };
    }
    // store the new options
    img_states[image_nr] = *new_img;
}

void ViewState::ForceRequireRedraw()
{
    // this is generally called by preview tools. We let them manage themselves.
    // often they give some user interface thing that doesn't reflect a change
    // in the panorama at all, so we let them force a redraw.
    dirty_draw = true;
}

void ViewState::Redraw()
{
    if (RequireDraw())
    {
        RefreshFunc(refreshArg);
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

bool ViewState::RequireRecalculateMesh (unsigned int image_nr)
{
    if (number_of_images > image_nr)
    {
        return dirty_mesh[image_nr].val;
    }
    // if we didn't think there were enough images, create the mesh for the
    //   first time
    return true;
}

bool ViewState::RequireRecalculateImageSizes()
{
    return dirty_image_sizes;
}

bool ViewState::RequireRecalculatePhotometric()
{
    return dirty_photometrics;
}

bool ViewState::RequireRecalculateMasks(unsigned int image_nr)
{
    if (number_of_images > image_nr)
    {
        return dirty_mask[image_nr].val;
    }
    return false;
}

bool ViewState::RequireDraw()
{
    return dirty_draw;
}

bool ViewState::RequireRecalculateViewport()
{
    return dirty_viewport;
}

bool ViewState::ImagesRemoved()
{
    return images_removed;
}

void ViewState::FinishedDraw()
{
    // update our copy of the state and clear all the dirty flags.
    number_of_images = m_pano->getNrOfImages();
    img_states.clear();
    active.clear();
    for (unsigned int img = 0; img < number_of_images; img++)
    {
        img_states[img] = m_pano->getImage(img);
        active[img] = m_pano->getImage(img).getOptions().active;
    }
    opts = m_pano->getOptions();
    
    Clean();
}

void ViewState::Clean()
{
    dirty_mesh.clear();
    dirty_image_sizes = false;
    dirty_draw = false;
    images_removed = false;
    dirty_viewport = false;
    dirty_photometrics = false;
    dirty_mask.clear();
}

void ViewState::DoUpdates()
{
    m_tex_manager->CheckUpdate();
    m_mesh_manager->CheckUpdate();
}

unsigned int ViewState::GetMeshDisplayList(unsigned int image_number)
{
    return m_mesh_manager->GetDisplayList(image_number);
}

