// -*- c-basic-offset: 4 -*-
/** @file PanoToolsInterface.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#ifndef _PANOTOOLSINTERFACE_H
#define _PANOTOOLSINTERFACE_H

#include <PT/Panorama.h>
#include <PT/PanoramaMemento.h>

extern "C" {
#include <pano12/panorama.h>
#include <pano12/filter.h>
}
class wxImage;

/** mainly consists of wrapper around the pano tools library,
 *  to assist in ressource management and to provide a
 *  nicer interface.
 *
 *  It can be used to feed data from our model directly into the
 *  panotools library
 */
namespace PTools {


template <class T, int nr>
void fillVector(T vec[3], T &val, int len)
{
    for (int i=0; i<len; i++) vec[i] = val;
}


/** set an output image, with properties from @p opts,
 *  that points to the bitmap data of @p imgData
 */
void setDestImage(Image & image, wxImage & imgData, const PT::PanoramaOptions & opts);

/** fills @p image with a complete input image, including distortion
 *  correction parameters if @p correctDistortions is set.
 */
void setFullImage(Image & image, wxImage & imgData, const PT::VariableMap & vars,
                  const PT::Lens::LensProjectionFormat format,
                  bool correctDistortions);

// internal function, used by setFullImage() to set the distortion parameters
void initCPrefs(cPrefs & p, const PT::VariableMap &vars);

/** create an empty aPrefs structure, suitable for transforming
 *  a input picture into an output picture.
 *
 *  the input/output pictures must be specified with: setAdjustSrcImg()
 *  and setAdjustDestImg()
 */
void createAdjustPrefs(aPrefs  & p, TrformStr & transf);
    
/** set a new input image for inserting into the panorama.
 */
void setAdjustSrcImg(TrformStr & trf, aPrefs & ap,
                     wxImage & src, const PT::VariableMap & vars,
                     const PT::Lens::LensProjectionFormat format,
                     bool correctDistortions);
    
/** set a new output image for the panorama */
void setAdjustDestImg(TrformStr & trf, aPrefs & ap,
                      wxImage & dest,
                      const PT::PanoramaOptions & opts);
                             

void freeTrform(TrformStr & trf);


/** prepare a Trform struct for the adjust operation, image -> pano
 *  see use createAdjustPrefs(), setAdjustSrcImg() and setAdjustDestImg()
 *  to specify the images and transformation options
 */
void createAdjustTrform(TrformStr & trf);

/** free the resources associated with a TrformStr.
 *   createAdjustTrform() must have been used to create @p trf
 */
void freeTrform(TrformStr & trf);
    

/** Stitch a Panorama into an output image
 *
 *  stitchs a set of images (@p imgNrs) of Panorama @p pano into the
 *  output image @p dest
 *
 *  it uses a nearest neighbour transform and doesn't do any color
 *  or distortion correction
 *
 *  @bug only the last remapped image is in the buffer,
 *       since panotools overwrites the previous images..
 *       Have to fix this somehow.
 */
bool stitchImage(wxImage & dest, const PT::Panorama & pano, 
                 PT::UIntSet imgNrs, const PT::PanoramaOptions & opts);


/** remaps a single image into its final projection */
bool mapImage(wxImage & dest, const PT::Panorama & pano,
              unsigned imgNr, const PT::PanoramaOptions & opts);


#if 0
/** wrapper for the image struct */
class PTools::PImage
{
public:

    /** constructor for output images */
    PImage(wxImage & imgData);

    /** constructor for input images */
    PImage(wxImage & imgData,
           const PT::VariableMap & srcVars,
           PT::Lens::LensProjectionFormat format,
           bool correctDistortions = false);

    // init cPrefs struct without correction
    void initCPrefs(cPrefs & p)
        {
            p.magic = 20;
            p.radial = 0;
            p.vertical = 0;
            p.horizontal = 0;
            p.shear = 0;
            p.resize = 0;
            p.luminance = 0;
            p.cutFrame = 0;
            p.fourier = 0;
        }

    // init cPrefs struct with correction values
    void initCPrefs(cPrefs & p, const PT::VariableMap &vars);

    ::Image image;
};



/** Object wrapper around TrformStr.
 *
 *  for automatic resource management
 *
 */
class Transform
{
public:
    Transform(const PT::Panorama & pano, unsigned int srcImgNr,
              wxImage & sImg);

    ~Transform()
        { delete srcImg; }

    /** remap the source image into a panorama */
    bool remap(wxImage & dest, const PT::PanoramaOptions & opts);

private:
    void init(wxImage & srcImg, const PT::VariableMap & vars,
         PT::Lens::LensProjectionFormat srcProj);

    TrformStr  trf;

private:
    PImage * srcImg;
};

#endif

} // namespace

#endif // _PANOTOOLSINTERFACE_H
