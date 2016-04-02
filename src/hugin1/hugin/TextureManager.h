// -*- c-basic-offset: 4 -*-

/** @file TextureManager.h
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/* a TextureManager sets up openGL textures for each of the input images in a
 * panorama. It scales images down so they should fit in graphics memory.
 */

/* Texture sizes must be powers of two. The texture sizes are the biggest size
 * that does not scale up the input image, or the biggest size supported by the
 * hardware, whichever is smaller. However, not all of the texture is defined
 * at any time, so the memory consumption is limited.
 * The textures are mipmapped, i.e. they use progressivly smaller versions.
 * The mipmaps are defined in a range from the smallest 1*1 pixel version to
 * the largest mipmap that doesn't go over the texture space budget. OpenGL
 * uses mip level 0 to be the original image, and log2(max(width, height)) + 1
 * to be the smallest 1 by 1 pixel image. We use this convention too.
 */

#ifndef _TextureManager_h
#define _TextureManager_h

#include <string>
#include <map>
#include <memory>
#include <huginapp/ImageCache.h>
#include "panodata/Panorama.h"

//class GLViewer;
class ViewState;

class TextureManager
{
public:
    TextureManager(HuginBase::Panorama *pano, ViewState *view);
    virtual ~TextureManager();
    // selct the texture for the requested image in opengl
    void DrawImage(unsigned int image_number, unsigned int display_list);
    // react to the images & fields of view changing. We can update the
    // textures here.
    void CheckUpdate();
    // change the OpenGL state for rendering the textures.
    void Begin();
    void End();
    // set to true if we are doing photmetric correction
    void SetPhotometricCorrect(bool state);
    // return true if we are doing photometric correction.
    bool GetPhotometricCorrect() {return photometric_correct;}
    // get the OpneGL texture name for a given image
    unsigned int GetTextureName(unsigned int image_number);
    // binds the texture for a given image
    void BindTexture(unsigned int image_number);
    // disables the image textures
    void DisableTexture(bool maskOnly=false);
    // update the texture when image cache has finished loading
    void LoadingImageFinished(int min, int max,
        bool texture_photometric_correct,
        const HuginBase::PanoramaOptions &dest_img,
        const HuginBase::SrcPanoImage &state);

protected:
    HuginBase::Panorama  * m_pano;
    ViewState *view_state;
    float viewer_exposure;
    // remove textures for deleted images.
    void CleanTextures();
    class TextureInfo
    {
    public:
        explicit TextureInfo(ViewState *new_view_state);
        // specify log2(width) and log2(height) for the new texture's size.
        // this is the size of mip level 0, which may or may not be defined.
        TextureInfo(ViewState *new_view_state, unsigned int width_p, unsigned int height_p);
        ~TextureInfo();
        // width and height are the size of the texture. This can be different
        // to the image size, we have to scale to powers of two. The texture
        // size is the biggest we can use for the image without scaling up
        // (unless the hardware doesn't support textures that big)
        // we generally have only lower mip levels defined though.
        unsigned int width, height;
        // log base 2 of the above, cached.
        unsigned int width_p, height_p;
        // min_lod is the most detailed mipmap level defined
        int min_lod;
        
        void DefineLevels(int min, int max,
                          bool photometric_correct,
                          const HuginBase::PanoramaOptions &dest_img,
                          const HuginBase::SrcPanoImage &state);
        void DefineMaskTexture(const HuginBase::SrcPanoImage &srcImg);
        void UpdateMask(const HuginBase::SrcPanoImage &srcImg);
        void SetMaxLevel(int level);
        void Bind();
        void BindImageTexture();
        void BindMaskTexture();
        unsigned int GetNumber() {return num;};
        // if the image has a mask, we want to use alpha blending to draw it.
        bool GetUseAlpha() {return has_mask;};
        bool GetHasActiveMasks() {return has_active_masks;};
    private:
        unsigned int num;     // the openGL texture name
        unsigned int numMask; // the openGL texture name for the mask
        bool has_mask; // this is the alpha channel
        bool has_active_masks; // has active masks
        ViewState *m_viewState;
        /// a request for an image, if it was not loaded before.
        HuginBase::ImageCache::RequestPtr m_imageRequest;
        // this binds a new texture in openGL and sets the various parameters
        // we need for it.
        void CreateTexture();
        void SetParameters();
    };
    
    // A TextureKey uniquely identifies a texture. It contains the filename
    // of the image and the photometric correction properties used to make it.
    class TextureKey
    {
    public:
        TextureKey();
        TextureKey(const HuginBase::SrcPanoImage * source, bool *photometric_correct_ptr);
            
        // TODO all of this lot should probably be made read only
        std::string filename;
        double exposure, white_balance_red, white_balance_blue;
        std::vector<float> EMoR_params;
        std::vector<double> radial_vig_corr_coeff;
        hugin_utils::FDiff2D radial_vig_corr_center_shift;
        int vig_corr_mode;
        HuginBase::SrcPanoImage::ResponseType response_type;
        std::vector<double> radial_distortion_red;
        std::vector<double> radial_distortion_blue;
        double gamma;
        std::string masks;
                
        // when stored in the textures map, this should be set to something that
        // always indicates if photometric correction comparisons should be made
        bool *photometric_correct;
        // we need to specify our own comparison function as the photometric
        // correction parameters do not need to be compared when we are not
        // using photometric correction.
        const bool operator==(const TextureKey& comp) const;
        // we need to be able to order the keys
        const bool operator<(const TextureKey& comp) const;
    private:
        void SetOptions(const HuginBase::SrcPanoImage *source);
    };
    // we map filenames to TexturesInfos, so we can keep track of
    // images' textures when the numbers change.
    typedef std::map<TextureKey, std::shared_ptr<TextureInfo> > TexturesMap;
    TexturesMap textures;
    // Our pixel budget for all textures.
    unsigned int GetMaxTotalTexels();
    // this is the maximum size a single texture is supported on the hardware.
    unsigned int GetMaxTextureSizePower(); 
    float texel_density;          // multiply by angles to get optimal size.
    bool photometric_correct;
};

#endif
   
