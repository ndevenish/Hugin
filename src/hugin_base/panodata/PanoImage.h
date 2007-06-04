// -*- c-basic-offset: 4 -*-

/** @file PanoImage.h
 *
 *  @brief implementation of HFOVDialog Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoImage.h 1970 2007-04-18 22:26:56Z dangelo $
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

#ifndef PANOIMAGE_H
#define PANOIMAGE_H

#include <iostream>
#include <vector>
#include <common/utils.h>
#include <common/math.h>
#include <vigra/diff2d.hxx>

namespace PT {

class Panorama;
    

/** optimization & stitching options. */
struct ImageOptions {

    ImageOptions()
    : featherWidth(10),
      ignoreFrameWidth(0),
      morph(false),
      docrop(false),
      autoCenterCrop(true),
      m_vigCorrMode(VIGCORR_RADIAL|VIGCORR_DIV),
      responseType(0),
      active(true)
     { };

    // PT state
    /// u10           specify width of feather for stitching. default:10
    unsigned int featherWidth;
    /// m20           ignore a frame 20 pixels wide. default: 0
    unsigned int ignoreFrameWidth;

    /// Morph-to-fit using control points.
    bool morph;

    // crop parameters
    bool docrop;
    bool autoCenterCrop;
    vigra::Rect2D cropRect;

    /// vignetting correction mode (bitflags, no real enum)
    enum VignettingCorrMode { 
        VIGCORR_NONE = 0,      ///< no vignetting correction
        VIGCORR_RADIAL = 1,    ///< radial vignetting correction
        VIGCORR_FLATFIELD = 2, ///< flatfield correction
        VIGCORR_DIV = 4        ///< correct by division.
    };
    int m_vigCorrMode;
    // coefficients for vignetting correction (even degrees: 0,2,4,6, ...)
    std::string m_flatfield;
    // the response type (Rt)
    int responseType;

    // is image active (displayed in preview and used for optimisation)
    bool active;
};



/** This class holds an source image.
 *
 *  It contains information about its settings for the panorama.
 *
 *  An image should not depend on the panorama.
 */
class PanoImage
{
public:
    PanoImage()
    {
        width=0;
        height=0;
        lensNr=0;
    };

    PanoImage(const std::string &filename,  int width,int height,
              int lens);

//        PanoImage(const std::string & filename);
    // create from xml node
//        PanoImage(QDomNode & node);

    virtual ~PanoImage();

    virtual const char * isA() const { return "PanoImage"; };

    std::string getFilename() const
        { return filename; }

    void setFilename(std::string fn)
        { filename = fn; }

    const ImageOptions & getOptions() const
        { return options; }

    void setOptions(const ImageOptions & opt)
        { options = opt; }

    unsigned int getHeight() const
        { return height; }
    unsigned int getWidth() const
        { return width; }

    void setLensNr(unsigned int l)
        { lensNr = l; }
    unsigned int getLensNr() const
        { return lensNr; }

    void setSize(const vigra::Size2D & sz)
        { width =sz.x; height = sz.y; }

    void setFeatherWidth(unsigned int w)
    { options.featherWidth = w; };

private:
    /// common init for all constructors
    void init();
    /// read image info (size, exif header)
    bool readImageInformation();

    // image properties needed by Panorama tools.

    std::string filename;
    int height,width;

    bool imageRead;
    ImageOptions options;
    // the lens of this image
    unsigned int lensNr;
};


} // namespace

#endif // PANOIMAGE_H
