// -*- c-basic-offset: 4 -*-
//
// Pablo d'Angelo <pablo.dangelo@web.de>
// Last change: Time-stamp: <26-Oct-2003 18:37:34 pablo@island.wh-wurm.uni-ulm.de>
//
//

#ifndef PANOIMAGE_H
#define PANOIMAGE_H

#include <iostream>
#include <vector>
#include <vigra/diff2d.hxx>

namespace PT {

class Panorama;

/** optimization & stitching options. */
class ImageOptions {

public:
    ImageOptions()
        : featherWidth(10),
          ignoreFrameWidth(0),
          morph(false),
		  docrop(false),
          autoCenterCrop(true),
          m_vigCorrMode(VIGCORR_NONE),
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
        
        void setFeatherWidth(unsigned int w)
            { options.featherWidth = w; }

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
