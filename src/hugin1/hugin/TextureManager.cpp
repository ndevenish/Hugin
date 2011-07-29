// -*- c-basic-offset: 4 -*-

/** @file TextureManager.cpp
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

#include <math.h>
#include <iostream>

#include <config.h>

#ifdef __APPLE__
#include "panoinc.h"
#endif


#include "ViewState.h"
#include "TextureManager.h"
#include "huginApp.h"
#include "GLPreviewFrame.h"

#include "vigra/stdimage.hxx"
#include "vigra/resizeimage.hxx"
#include "base_wx/wxImageCache.h"
#include "photometric/ResponseTransform.h"
#include "panodata/Mask.h"

// The OpenGL Extension wrangler libray will find extensions and the latest
// supported OpenGL version on all platforms.
#if !defined Hugin_shared || !defined _WINDOWS
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <wx/platform.h>

#ifdef __WXMAC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

TextureManager::TextureManager(PT::Panorama *pano, ViewState *view_state_in)
{
    m_pano = pano;
    photometric_correct = false;
    view_state = view_state_in;
}

TextureManager::~TextureManager()
{
    // free up the textures
    textures.clear();
}

void TextureManager::DrawImage(unsigned int image_number,
                               unsigned int display_list)
{
    // bind the texture that represents the given image number.
    std::map<TextureKey, TextureInfo>::iterator it;
    HuginBase::SrcPanoImage *img_p = view_state->GetSrcImage(image_number);
    TextureKey key(img_p, &photometric_correct);
    it = textures.find(key);
    DEBUG_ASSERT(it != textures.end());
    it->second.Bind();
    glColor4f(1.0,1.0,1.0,1.0);
    if (it->second.GetUseAlpha() || it->second.GetHasActiveMasks())
    {
        // use an alpha blend if there is a alpha channel or a mask for this image.
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
    }
    if (!photometric_correct)
    {
        // When using real time photometric correction, we multiply the colour
        // components to get the white balance and exposure correct.
        HuginBase::SrcPanoImage *img = view_state->GetSrcImage(image_number);    
        // we adjust the intensity by using a darker colour
        float es = viewer_exposure / img->getExposure();
        float scale[4] = {es / img->getWhiteBalanceRed(),
                          es,
                          es / img->getWhiteBalanceBlue(),
                          1.0};
        glColor3fv(scale);
        glCallList(display_list);
        // Since the intensity was clamped to 0.0 - 1.0, we might overdraw a
        // few times to make it brighter.
        // FIXME If the image has areas masked out, these will also be
        // brightened. It might be better to do using the texture, but this
        // way we can only add the texture to the frame buffer, (we can't double
        // the intensity multiple times) and there is a cost in processing the
        // texture. It also won't work properly on partially transparent places.
        if (scale[0] > 1.0 || scale[1] > 1.0 || scale[2] >  1.0)
        {
            view_state->GetTextureManager()->DisableTexture();
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ONE);
            glColor4f(1.0, 1.0, 1.0, 1.0);
            // double the brightness for colour components until it is almost
            // right, however limit it incase it is really bright.
            bool r, g, b;
            unsigned short int count = 0;
            while ((   (r = (scale[0] > 2.0))
                   || (g = (scale[1] > 2.0))
                   || (b = (scale[2] > 2.0)))
                   && count < 9)
            {
                glColor4f(r ? 1.0 : 0.0, g ? 1.0 : 0.0, b ? 1.0 : 0.0, 1.0);
                glCallList(display_list);
                if (r) scale[0] /= 2.0;
                if (g) scale[1] /= 2.0;
                if (b) scale[2] /= 2.0;
                count++;
            }
            // now add on anything remaining.
            if (scale[0] > 1.0 || scale[1] > 1.0 || scale[2] >  1.0)
            {
                // clamped to 0.0-1.0, so it won't get darker.
                scale[0] -= 1.0; scale[1] -= 1.0; scale[2] -= 1.0;
                glColor3fv(scale);
                glCallList(display_list);
            }
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
            glColor3f(1.0, 1.0, 1.0);
        }
    } else {
        // we've already corrected all the photometrics, just draw once normally
        glCallList(display_list);
        if (it->second.GetUseAlpha() || it->second.GetHasActiveMasks())
        {
            glDisable(GL_BLEND);
        }
    }
}

unsigned int TextureManager::GetTextureName(unsigned int image_number)
{
    // bind the texture that represents the given image number.
    std::map<TextureKey, TextureInfo>::iterator it;
    HuginBase::SrcPanoImage *img_p = view_state->GetSrcImage(image_number);
    TextureKey key(img_p, &photometric_correct);
    it = textures.find(key);
    DEBUG_ASSERT(it != textures.end());
    return it->second.GetNumber();
}

void TextureManager::BindTexture(unsigned int image_number)
{
    // bind the texture that represents the given image number.
    std::map<TextureKey, TextureInfo>::iterator it;
    HuginBase::SrcPanoImage *img_p = view_state->GetSrcImage(image_number);
    TextureKey key(img_p, &photometric_correct);
    it = textures.find(key);
    DEBUG_ASSERT(it != textures.end());
    it->second.Bind();
}

void TextureManager::DisableTexture(bool maskOnly)
{
    if(view_state->GetSupportMultiTexture())
    {
        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        if(!maskOnly)
            glDisable(GL_TEXTURE_2D);
    }
    else
    {
        if(!maskOnly)
            glDisable(GL_TEXTURE_2D);
    };
};

void TextureManager::Begin()
{
    if (!photometric_correct)
    {
        // find the exposure factor to scale by.
        viewer_exposure = 1.0 / pow(2.0,
                                    m_pano->getOptions().outputExposureValue);
    };
};

void TextureManager::End()
{
}

void TextureManager::CheckUpdate()
{
    // The images or their lenses have changed.
    // Find what size we should have the textures.
    // Note that one image changing does affect the rest, if an image suddenly
    // takes up more space, the others should take up less.
    unsigned int num_images = m_pano->getNrOfImages();
    if (num_images == 0)
    {
        textures.clear();
        return;
    }
    // if we are doing photometric correction, and someone changed the output
    // exposure, all of our images are at the wrong exposure.
    if (photometric_correct && view_state->RequireRecalculatePhotometric())
    {
        textures.clear();
    }
    HuginBase::PanoramaOptions *dest_img = view_state->GetOptions();
    // Recalculuate the ideal image density if required
    // TODO tidy up once it works.
    DEBUG_INFO("Updating texture sizes.");
    // find the total of fields of view of the images, in degrees squared
    // we assume each image has the same density across all it's pixels
    double total_fov = 0.0;
    for (unsigned int image_index = 0; image_index < num_images; image_index++)
    {
        HuginBase::SrcPanoImage *src = view_state->GetSrcImage(image_index);
        double aspect = double(src->getSize().height())
                                               / double(src->getSize().width());
        total_fov += src->getHFOV() * aspect;
    };
    // now find the ideal density
    texel_density = double(GetMaxTotalTexels()) / total_fov;

    // now recalculate the best image sizes
    // The actual texture size is the biggest one possible withouth scaling the
    // image up in any direction. We only specifiy mipmap levels we can fit in
    // a given amount of texture memory, while respecting the image's FOV.
    int texels_used = 0;
    double ideal_texels_used = 0.0;
    for (unsigned int image_index = 0; image_index < num_images; image_index++)
    {    
        // find this texture
        // if it has not been created before, it will be created now.
        std::map<TextureKey, TextureInfo>::iterator it;
        HuginBase::SrcPanoImage *img_p = view_state->GetSrcImage(image_index);
        TextureKey key(img_p, &photometric_correct);
        it = textures.find(key);
        TextureInfo *texinfo;
        /* This section would allow us to reuse textures generated when we want
         * to change the size. It is not used as it causes segmentation faults
         * under Ubuntu 8.04's "ati" graphics driver.
         */
      #if 0
        if (it == textures.end())
        {
            // We haven't seen this image before.
            // Find a size for it and make its texture.
            // store the power that 2 is raised to, not the actual size
            unsigned int max_tex_width_p = int(log2(img_p->getSize().width())),
                        max_tex_height_p = int(log2(img_p->getSize().height()));
            // check this is hardware supported.
            {
              unsigned int biggest = GetMaxTextureSizePower();
              if (biggest < max_tex_width_p) max_tex_width_p = biggest;
              if (biggest < max_tex_height_p) max_tex_height_p = biggest;
            }
            std::cout << "Texture size for image " << image_index << " is "
                      << (1 << max_tex_width_p) << " by "
                      << (1 << max_tex_height_p) << "\n";
            // create a new texinfo and store the texture details.
            std::cout << "About to create new TextureInfo for "
                      << img_p->getFilename()
                      << ".\n";
            std::pair<std::map<TextureKey, TextureInfo>::iterator, bool> ins;
            ins = textures.insert(std::pair<TextureKey, TextureInfo>
                                 (TextureKey(img_p, &photometric_correct),
                // the key is used to identify the image with (or without)
                // photometric correction parameters.
                              TextureInfo(max_tex_width_p, max_tex_height_p)
                            ));
            texinfo = &((ins.first)->second);
        }
        else
        {
            texinfo = &(it->second);
        }
                
        // find the highest mipmap we want to use.
        double hfov = img_p->getHFOV(),
               aspect = double (texinfo->height) / double (texinfo->width),
               ideal_texels = texel_density * hfov * aspect,
               // we would like a mipmap with this size:
               ideal_tex_width = sqrt(ideal_texels / aspect),
               ideal_tex_height = aspect * ideal_tex_width;
        // Ideally this mipmap would bring us up to this many texels
        ideal_texels_used += ideal_texels;
        std::cout << "Ideal mip size: " << ideal_tex_width << " by "
                  << ideal_tex_height << "\n";
        // Find the smallest mipmap level that is at least this size.
        int max_mip_level = (texinfo->width_p > texinfo->height_p)
                            ? texinfo->width_p : texinfo->height_p;
        int mip_level = max_mip_level - ceil((ideal_tex_width > ideal_tex_height)
                        ? log2(ideal_tex_width) : log2(ideal_tex_height));
        // move to the next mipmap level if we are over budget.
        if ((texels_used + (1 << (texinfo->width_p + texinfo->height_p
                                  - mip_level * 2)))
            > ideal_texels_used)
        {
            // scale down
            mip_level ++;
        }
        // don't allow any mipmaps smaller than the 1 by 1 pixel one.
        if (mip_level > max_mip_level) mip_level = max_mip_level;
        // don't allow any mipmaps with a negative level of detail (scales up)
        if (mip_level < 0) mip_level = 0;
        // find the size of this level
        int mip_width_p = texinfo->width_p - mip_level,
            mip_height_p = texinfo->height_p - mip_level;
        // check if we have scaled down to a single line, and make sure we
        // limit the line's width to 1 pixel.
        if (mip_width_p < 0) mip_width_p = 0;
        if (mip_height_p < 0) mip_height_p = 0;
        
        // now count these texels as used- we are ignoring the smaller mip
        //   levels, they add 1/3 on to the size.
        texels_used += 1 << (mip_width_p + mip_height_p);
        std::cout << "biggest mipmap of image " << image_index << " is "
                  << (1 << mip_width_p) << " by " << (1 << mip_height_p)
                  << " (level " << mip_level <<").\n";
        std::cout << "Ideal texels used " << int(ideal_texels_used)
                  << ", actually used " << texels_used << ".\n\n";
        if (texinfo->min_lod != mip_level)
        {
            // maximum level required changed.
            if (texinfo->min_lod > mip_level)
            {
                // generate more levels
                texinfo->DefineLevels(mip_level,
                                      (texinfo->min_lod > max_mip_level) ?
                                      max_mip_level : texinfo->min_lod - 1,
                                      photometric_correct, dest_img,
                                      view_state->GetSrcImage(image_index));
            }
            texinfo->SetMaxLevel(mip_level);
            texinfo->min_lod = mip_level;
        }
    }
    #endif
    /* Instead of the above section, replace the whole texture when appropriate:
        */
        // Find a size for it
        double hfov = img_p->getHFOV(),
           aspect = double (img_p->getSize().height())
                                            / double (img_p->getSize().width()),
           ideal_texels = texel_density * hfov * aspect,
           // we would like a texture this size:
           ideal_tex_width = sqrt(ideal_texels / aspect),
           ideal_tex_height = aspect * ideal_tex_width;
        // shrink if bigger than the original, avoids scaling up excessively.
        if (ideal_tex_width > img_p->getSize().width())
                ideal_tex_width = img_p->getSize().width();
        if (ideal_tex_height > img_p->getSize().height())
                ideal_tex_height = img_p->getSize().height();
        // we will need to round up/down to a power of two
        // round up first, then shrink if over budget.
        // store the power that 2 is raised to, not the actual size
        unsigned int tex_width_p = int(log2(ideal_tex_width)) + 1,
                    tex_height_p = int(log2(ideal_tex_height)) + 1;
        // check this is hardware supported.
        {
          unsigned int biggest = GetMaxTextureSizePower();
          if (biggest < tex_width_p) tex_width_p = biggest;
          if (biggest < tex_height_p) tex_height_p = biggest;
        }
        
        // check if this is over budget.
        ideal_texels_used += ideal_texels; 
        // while the texture is over budget, shrink it
        while (  (texels_used + (1 << (tex_width_p + tex_height_p)))
            > ideal_texels_used)
        {
            // smaller aspect means the texture is wider.
            if ((double) (1 << tex_height_p) / (double) (1 << tex_width_p)
               < aspect)
            {
                tex_width_p--;
            } else {
                tex_height_p--;
            }
        }
        // we have a nice size
        texels_used += 1 << (tex_width_p + tex_height_p);
        if (   it == textures.end()
            || (it->second).width_p != tex_width_p
            || (it->second).height_p != tex_height_p)
        {
            // Either: 1. We haven't seen this image before
            //     or: 2. Our texture for this is image is the wrong size
            // ...therefore we make a new one the right size:
            //
            // remove duplicate key if exists
            TextureKey checkKey (img_p, &photometric_correct);
            if (textures.find(checkKey) != textures.end()) {
                // Already exists in map, remove it first before adding a new one
                textures.erase(checkKey);
            }

            std::pair<std::map<TextureKey, TextureInfo>::iterator, bool> ins;
            ins = textures.insert(std::pair<TextureKey, TextureInfo>
                                 (TextureKey(img_p, &photometric_correct),
                // the key is used to identify the image with (or without)
                // photometric correction parameters.
                              TextureInfo(view_state, tex_width_p, tex_height_p)
                            ));
           // create and upload the texture image
           texinfo = &((ins.first)->second);
           texinfo->DefineLevels(0, // minimum mip level
                                 // maximum mip level
                        tex_width_p > tex_height_p ? tex_width_p : tex_height_p,
                                photometric_correct,
                                *dest_img,
                                *view_state->GetSrcImage(image_index));
           texinfo->DefineMaskTexture(*view_state->GetSrcImage(image_index));
        }
        else
        {
            if(view_state->RequireRecalculateMasks(image_index))
            {
                //mask for this image has changed, also update only mask
                (*it).second.UpdateMask(*view_state->GetSrcImage(image_index));
            };
        }
    }
    // We should remove any images' texture when it is no longer in the panorama
    // with the ati bug work around, we might make unneassry textures whenever 
    //if (photometric_correct || view_state->ImagesRemoved())
    {
        CleanTextures();
    }
//    std::map<TextureKey, TextureInfo>::iterator it;
//    for (it = textures.begin() ; it != textures.end() ; it++) {
//        DEBUG_DEBUG("textures num " << it->second.GetNumber());
//    }
}

void TextureManager::SetPhotometricCorrect(bool state)
{
    // change the photometric correction state.
    if (state != photometric_correct)
    {
        photometric_correct = state;
        // We will need to recalculate all the images.
        /* TODO It may be possible to keep textures that have some identity
         * photometric transformation.
         * Be warned that when turning off photometric correction, two images
         * with the same filename will suddenly have the same key, which will
         * break the textures map, hence clearing now                         */
        textures.clear();
    }
}

unsigned int TextureManager::GetMaxTotalTexels()
{
    // TODO: cut off at a sensible value for available hardware, otherwise set
    // to something like 4 times the size of the screen.
    // The value is guestimated as good for 1024*512 view where each point is
    // covered by 4 images.
    return 2097152;
    // Note: since we use mipmaps, the amount of actual maximum of pixels stored
    // will be 4/3 of this value. It should use a maximum of 8MB of video memory
    // for 8 bits per channel rgb images, 12MB if we include a mask.
    // Video memory is also used for two copies of the screen and any auxilary
    // buffers, and the meshes, so we should do fine with ~24MB of video memory.
}

unsigned int TextureManager::GetMaxTextureSizePower()
{
    // get the maximum texture size supported by the hardware
    // note the value can be too small, it is for a square texture with borders.
    // we don't use borders, and the textures aren't always square.
    static unsigned int max_size_p = 0;
    if (max_size_p) return max_size_p; // don't ask openGL again.
    
    GLint max_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    if (glGetError())
    {
      DEBUG_ERROR("Cannot find maximum texture size!");
      // opengl docs say 64 pixels square is the minimum size guranteed to be supported.
      return 6;
    }
    max_size_p = int(log2(max_size));
    DEBUG_INFO("Max texture size supported is " << max_size <<
               " (2^" << max_size_p << ")");
    return max_size_p;
}

void TextureManager::CleanTextures()
{
    // clean up all the textures from removed images.
    // TODO can this be more efficient?
    unsigned int num_images = m_pano->getNrOfImages();
    bool retry = true;
    std::map<TextureKey, TextureInfo>::iterator tex;
    while (retry)
    {
      retry = false;
      for (tex = textures.begin(); tex != textures.end(); tex++)
      {
          bool found = false;
          
          // try and find an image with this key
          for (unsigned int img = 0; img < num_images; img++)
          {
              TextureKey ik(view_state->GetSrcImage(img), &photometric_correct);
              if (ik == tex->first)
              {
                  found = true;
                  break;
              }
          }
          // remove it if it was not found
          if (!found)
          {
              DEBUG_INFO("Removing old texture for " << tex->first.filename << ".");
              retry = true;
              textures.erase(tex);
              break;
          }
      }
    }
}

TextureManager::TextureInfo::TextureInfo(ViewState *new_view_state)
{
    // we shouldn't be using this. It exists only to make std::map happy.
    DEBUG_ASSERT(0);
    m_viewState=new_view_state;
    has_active_masks=false;
    CreateTexture();
}

TextureManager::TextureInfo::TextureInfo(ViewState *new_view_state, unsigned int width_p_in,
                                         unsigned int height_p_in)
{
    m_viewState=new_view_state;
    has_active_masks=false;
    width_p = width_p_in;
    height_p = height_p_in;
    width = 1 << width_p;
    height = 1 << height_p;
    CreateTexture();
}

void TextureManager::TextureInfo::CreateTexture()
{
    // Get an number for an OpenGL texture
    glGenTextures(1, (GLuint*) &num);
    DEBUG_DEBUG("textures num created " << num);
    glGenTextures(1, (GLuint*) &numMask);
    // we want to generate all levels of detail, they are all undefined.
    min_lod = 1000;
    SetParameters();
}

void TextureManager::TextureInfo::SetParameters()
{
    BindImageTexture();
    glEnable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    // we don't want the edges to repeat the other side of the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // use anistropic filtering if supported. This is good because we are
    // sretching and distorting the textures rather a lot in places and still
    // want good image quality.
    static bool checked_anisotropic = false;
    static bool has_anisotropic;
    static float anisotropy;
    if (!checked_anisotropic)
    {
        // check if it is supported
        if (GLEW_EXT_texture_filter_anisotropic)
        {
            has_anisotropic = true;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
            DEBUG_INFO("Using anisotropic filtering at maximum value "
                      << anisotropy);
        } else {
            has_anisotropic = false;
            DEBUG_INFO("Anisotropic filtering is not available.");
        }
        checked_anisotropic = true;
    }
    if (has_anisotropic)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        anisotropy);
    }
    if(m_viewState->GetSupportMultiTexture())
    {
        BindMaskTexture();
        glEnable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        // we don't want the edges to repeat the other side of the texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if(has_anisotropic)
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,anisotropy);
    };
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        DEBUG_ERROR("GL Error when setting texture parameters: "
                    << gluErrorString(error) << ".");
    }
}

TextureManager::TextureInfo::~TextureInfo()
{
    // free up the graphics system's memory for this texture
    DEBUG_DEBUG("textures num deleting " <<  num);
    glDeleteTextures(1, (GLuint*) &num);
    glDeleteTextures(1, (GLuint*) &numMask);
}

void TextureManager::TextureInfo::Bind()
{
    BindImageTexture();
    BindMaskTexture();
    if(m_viewState->GetSupportMultiTexture())
    {
        if(has_active_masks)
            glEnable(GL_TEXTURE_2D);
        else
            glDisable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
    };
}

void TextureManager::TextureInfo::BindImageTexture()
{
    if(m_viewState->GetSupportMultiTexture())
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, num);
    }
    else
        glBindTexture(GL_TEXTURE_2D, num);
};
void TextureManager::TextureInfo::BindMaskTexture()
{
    if(m_viewState->GetSupportMultiTexture())
    {
        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, numMask);
    }
};

// Note min and max refer to the mipmap levels, not the sizes of them. min has
// the biggest size.
void TextureManager::TextureInfo::DefineLevels(int min,
                                               int max,
                                               bool photometric_correct,
                                     const HuginBase::PanoramaOptions &dest_img,
                                         const HuginBase::SrcPanoImage &src_img)
{
    // This might take a while, so show a busy cursor.
    //FIXME: busy cursor creates weird problem with calling checkupdate function again and messing up the textures
//    wxBusyCursor busy_cursor;
    // activate the texture so we can change it.
    BindImageTexture();
    // find the highest allowable mip level
    int max_mip_level = (width_p > height_p) ? width_p : height_p;
    if (max > max_mip_level) max = max_mip_level;
    
    // add more detail textures. We need to get the biggest one first.
    // find the original image to scale down.
    // TODO cache full texture to disk after scaling?
    // TODO use small image if don't need bigger?
    // It is also possible to use HDR textures, but I can't see the point using
    // them as the only difference on an LDR display would be spending extra 
    // time reading the texture and converting the numbers. (float and uint16)
    // remove some cache items if we are using lots of memory:
    ImageCache::getInstance().softFlush();
    DEBUG_INFO("Loading image");
    std::string img_name = src_img.getFilename();
    ImageCache::EntryPtr entry = ImageCache::getInstance().getImageIfAvailable(img_name);
    if (!entry.get())
    {
        // Image isn't loaded yet. Request it for later.
        m_imageRequest = ImageCache::getInstance().requestAsyncImage(img_name);
        // call this function with the same parameters after the image loads
        m_imageRequest->ready.connect(0, 
            boost::bind(&TextureManager::TextureInfo::DefineLevels, this,
                        min, max, photometric_correct, dest_img, src_img));
        // After that, redraw the preview.
        m_imageRequest->ready.connect(1,
            boost::bind(&GLPreviewFrame::redrawPreview,
                        huginApp::getMainFrame()->getGLPreview()));
        
        // make a temporary placeholder image.
        GLubyte placeholder_image[64][64][4];
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                // checkboard pattern
                GLubyte c = (i/8+j/8)%2 ? 63 : 191;
                placeholder_image[i][j][0] = c;
                placeholder_image[i][j][1] = c;
                placeholder_image[i][j][2] = c;
                // alpha is low, so the placeholder is mostly transparent.
                placeholder_image[i][j][3] = 63;
            }
        }
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, 64, 64,
                               GL_RGBA, GL_UNSIGNED_BYTE,
                               placeholder_image);
        SetParameters();
        return;
    }
    // forget the request if we made one before.
    m_imageRequest = ImageCache::RequestPtr();
    DEBUG_INFO("Converting to 8 bits");
    boost::shared_ptr<vigra::BRGBImage> img = entry->get8BitImage();
    boost::shared_ptr<vigra::BImage> mask = entry->mask;
    // first make the biggest mip level.
    int wo = 1 << (width_p - min), ho = 1 << (height_p - min);
    if (wo < 1) wo = 1; if (ho < 1) ho = 1;
    // use Vigra to resize image
    DEBUG_INFO("Scaling image");
    vigra::BRGBImage out_img(wo, ho);
    // also read in the mask. OpenGL requires that the mask is in the same array
    // as the colour data, but the ImageCache doesn't work in this way.
    has_mask = mask->width()  && mask->height();
    vigra::UInt8Image *out_alpha;
    if (has_mask) out_alpha = new vigra::UInt8Image(wo, ho);
    if (wo < 2 || ho < 2)
    {
        // too small for vigra to scale
        // we still need to define some mipmap levels though, so use only (0, 0)
        for (int h = 0; h < ho; h++)
        {
            for (int w = 0; w < wo; w++)
            {
                out_img[h][w] = (*img)[0][0];
                if (has_mask) (*out_alpha)[h][w] = (*mask)[0][0];
            }
        }
    } else {
        // I think this takes to long, although it should be prettier.
        /*vigra::resizeImageLinearInterpolation(srcImageRange(*img),
                                               destImageRange(out_img));
        if (has_mask)
        {
            vigra::resizeImageLinearInterpolation(srcImageRange(*(entry->mask)),
                                          destImageRange(out_alpha));
        }*/
        
        // much faster. It shouldn't be so bad after it
        vigra::resizeImageNoInterpolation(srcImageRange(*img),
                                          destImageRange(out_img));
        if (has_mask)
        {
            vigra::resizeImageNoInterpolation(srcImageRange(*(mask)),
                                              destImageRange(*out_alpha));
        }/**/
        // now perform photometric correction
        if (photometric_correct)
        {
            DEBUG_INFO("Performing photometric correction");
            // setup photometric transform for this image type
            // this corrects for response curve, white balance, exposure and
            // radial vignetting
            HuginBase::Photometric::InvResponseTransform <unsigned char, double>
                                    invResponse(src_img);
           // Assume LDR for now.
           // if (m_destImg.outputMode == PanoramaOptions::OUTPUT_LDR) {
               // select exposure and response curve for LDR output
               std::vector<double> outLut;
               // @TODO better handling of output EMoR parameters
               // Hugin's stitcher is currently using the EMoR parameters of the first image
               // as so called output EMoR parameter, so enforce this also for the fast
               // preview window
               // vigra_ext::EMoR::createEMoRLUT(dest_img.outputEMoRParams, outLut);
               vigra_ext::EMoR::createEMoRLUT(m_viewState->GetSrcImage(0)->getEMoRParams(), outLut);
               vigra_ext::enforceMonotonicity(outLut);
               invResponse.setOutput(1.0/pow(2.0,dest_img.outputExposureValue),
                                     outLut, 255.0);
            /*} else {
               // HDR output. not sure how that would be handled by the opengl
               // preview, though. It might be possible to apply a logarithmic
               // lookup table here, and average the overlapping pixels
               // in the OpenGL renderer?
               // TODO
               invResponse.setHDROutput();
            }*/
            // now perform the corrections
            double scale_x = (double) src_img.getSize().width() / (double) wo,
                   scale_y = (double) src_img.getSize().height() / (double) ho;
            for (int x = 0; x < wo; x++)
            {
                for (int y = 0; y < ho; y++)
                {
                    double sx = (double) x * scale_x,
                           sy = (double) y * scale_y;
                    out_img[y][x] = invResponse(out_img[y][x],
                                                hugin_utils::FDiff2D(sx, sy));
                }
            }
        }
    }
    
    //  make all of the smaller ones until we are done.
    // this will use a box filter.
    // dependent on OpenGL 1.3. Might need an alternative for 1.2.
    // TODO use texture compresion?
    DEBUG_INFO("Defining mipmap levels " <<  min << " to " << max
          << " of texture " << num << ", starting with a size of "
          << wo << " by " << ho << ".");
    GLint error;
    if (has_mask)
    {
        // combine the alpha bitmap with the red green and blue one.
        unsigned char *image = new unsigned char[ho * wo * 4];
        unsigned char *pix_start = image;
        for (int h = 0; h < ho; h++)
        {
            for (int w = 0; w < wo; w++)
            {
                pix_start[0] = out_img[h][w].red();
                pix_start[1] = out_img[h][w].green();
                pix_start[2] = out_img[h][w].blue();
                pix_start[3] = (*out_alpha)[h][w];
                pix_start += 4;
            }
        }
        // We don't need to worry about levels with the ATI bug work around,
        // and Windows doesn't like it as gluBuild2DMipmapLevels is in OpenGL
        // version 1.3 and above only (Microsoft's SDK only uses 1.1)
        error = gluBuild2DMipmaps/*Levels*/(GL_TEXTURE_2D, GL_RGBA8, wo, ho,
                               GL_RGBA, GL_UNSIGNED_BYTE, /*min, min, max,*/
                               image);
        delete [] image;
        delete out_alpha;
    } else {
        // we don't need to rearange the data in memory if there is no mask.
        error = gluBuild2DMipmaps/*Levels*/(GL_TEXTURE_2D, GL_RGB8, wo, ho,
                               GL_RGB, GL_UNSIGNED_BYTE, /*min, min, max,*/
                               (unsigned char *) out_img.data());
    }
    if (error)
    {
        DEBUG_ERROR("GLU Error when building mipmap levels: "
                  << gluErrorString(error) << ".");
    }
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        DEBUG_ERROR("GL Error when bulding mipmap levels: "
                  << gluErrorString(error) << ".");
    }
    SetParameters();
    DEBUG_INFO("Finsihed loading texture.");


}

void TextureManager::TextureInfo::DefineMaskTexture(const HuginBase::SrcPanoImage &srcImg)
{
    has_active_masks=srcImg.hasActiveMasks();
    HuginBase::MaskPolygonVector masks=srcImg.getActiveMasks();
    if(has_active_masks)
    {
        unsigned int maskSize=(width>height) ? width : height;
        if(maskSize>64)
            maskSize/=2;
        BindMaskTexture();
        for(unsigned int i=0;i<masks.size();i++)
            masks[i].scale((double)maskSize/srcImg.getWidth(),(double)maskSize/srcImg.getHeight());
        vigra::UInt8Image mask(maskSize,maskSize,255);
        //we don't draw mask if the size is smaller than 4 pixel
        if(maskSize>4)
            vigra_ext::applyMask(vigra::destImageRange(mask), masks);
#ifdef __APPLE__
        // see comment to PreviewLayoutLinesTool::PreviewLayoutLinesTool
        // on MacOS a single alpha channel seems not to work, so this workaround
        unsigned char *image = new unsigned char[maskSize * maskSize * 2];
        unsigned char *pix_start = image;
        for (int h = 0; h < maskSize; h++)
        {
            for (int w = 0; w < maskSize; w++)
            {
                pix_start[0] = 255;
                pix_start[1] = mask[h][w];
                pix_start += 2;
            }
        }
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, maskSize, maskSize,
            GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, image);
        delete [] image;
#else
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA, maskSize,maskSize,GL_ALPHA, GL_UNSIGNED_BYTE,(unsigned char *) mask.data());
#endif
    };
};

void TextureManager::TextureInfo::UpdateMask(const HuginBase::SrcPanoImage &srcImg)
{
    if(m_viewState->GetSupportMultiTexture())
    {
        //delete old mask
        glDeleteTextures(1, (GLuint*) &numMask);
        //new create new mask
        glGenTextures(1, (GLuint*) &numMask);
        SetParameters();
        DefineMaskTexture(srcImg);
    };
};

void TextureManager::TextureInfo::SetMaxLevel(int level)
{
    // we want to tell openGL the highest defined mip level of our texture.
    BindImageTexture();
    // FIXME the ati graphics driver on Ubuntu is known to crash due to this
    // practice. ati users should disable direct renderering if using the
    // #if 0'ed code above.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, level);
    if(m_viewState->GetSupportMultiTexture())
    {
        // now for the mask texture
        BindMaskTexture();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, level);
    }
    // we don't set min_lod so we can 'DefineLevels' using the old value.
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        DEBUG_ERROR("Error when setting the base mipmap level: "
                  << gluErrorString(error) << ".");
    }
}

TextureManager::TextureKey::TextureKey(HuginBase::SrcPanoImage *source,
                                       bool *photometric_correct_ptr)
{
    SetOptions(source);
    photometric_correct = photometric_correct_ptr;
}

// This is only used by clean textures
const bool TextureManager::TextureKey::operator==(const TextureKey comp) const
{
    return !(*this < comp || comp < *this);
}

const bool TextureManager::TextureKey::operator<(const TextureKey comp) const
{
    // compare two keys for ordering.
    // first try the filename.
    if (filename < comp.filename) return true;
    if (filename > comp.filename) return false;
    // Are there different masks?
    if (masks < comp.masks) return true;
    if (masks > comp.masks) return false;
    // if we are not using photometric correction, the textures are equivalent.
    if (!(*photometric_correct)) return false;
    // now try the photometric properties
    if (exposure < comp.exposure) return true;
    if (exposure > comp.exposure) return false;
    if (white_balance_red < comp.white_balance_red) return true;
    if (white_balance_red > comp.white_balance_red) return false;
    if (white_balance_blue < comp.white_balance_blue) return true;
    if (white_balance_blue > comp.white_balance_blue) return false;
    if (EMoR_params < comp.EMoR_params) return true;
    if (EMoR_params > comp.EMoR_params) return false;
    if (radial_vig_corr_coeff < comp.radial_vig_corr_coeff) return true;
    if (radial_vig_corr_coeff > comp.radial_vig_corr_coeff) return false;
    if (vig_corr_mode < comp.vig_corr_mode) return true;
    if (vig_corr_mode > comp.vig_corr_mode) return false;
    if (response_type < comp.response_type) return true;
    if (response_type > comp.response_type) return false;
    if (gamma < comp.gamma) return true;
    if (gamma > comp.gamma) return false;
    if (radial_distortion_red < comp.radial_distortion_red) return true;
    if (radial_distortion_red > comp.radial_distortion_red) return false;
    if (radial_distortion_blue < comp.radial_distortion_blue) return true;
    if (radial_distortion_blue > comp.radial_distortion_blue) return false;
    // If we've reached here it should be exactly the same:
    return false;
}

void TextureManager::TextureKey::SetOptions(HuginBase::SrcPanoImage *source)
{
    filename = source->getFilename();
    // Record the masks. Images with different masks require different
    // textures since the mask is stored with them.
    std::stringstream mask_ss;
    source->printMaskLines(mask_ss, 0);
    masks = mask_ss.str();
    
    exposure = source->getExposure();
    white_balance_red = source->getWhiteBalanceRed();
    white_balance_blue = source->getWhiteBalanceBlue();
    EMoR_params = source->getEMoRParams();
    radial_vig_corr_coeff = source->getRadialVigCorrCoeff();
    vig_corr_mode = source->getVigCorrMode();
    response_type = source->getResponseType();
    gamma = source->getGamma();
    radial_distortion_red = source->getRadialDistortionRed();
    radial_distortion_blue = source->getRadialDistortionBlue();
}

