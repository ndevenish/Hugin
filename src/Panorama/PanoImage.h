// -*- c-basic-offset: 4 -*-
//
// Pablo d'Angelo <pablo@mathematik.uni-ulm.de>
// Last change: Time-stamp: <05-May-2003 00:06:14 pablo@island.wh-wurm.uni-ulm.de>
//
//

#ifndef PANOIMAGE_H
#define PANOIMAGE_H

#include <iostream>
#include <Magick++.h>

/*
#include <qstring.h>
#include <qdom.h>
#include <qpixmap.h>
*/

namespace PT {

class Panorama;

/** optimization & stitching options. */
class ImageOptions {

public:
    enum ImageSource { DIGITAL_CAMERA, SCANNER };
    ImageOptions()
        : featherWidth(10),
          ignoreFrameWidth(0),
          morph(false),
          lensNr(0),
          source(DIGITAL_CAMERA)
        { };

    // isn't the c++ compiler supposed to create a default operator== ?
    bool operator==(const ImageOptions & o) const
        {
            return (featherWidth == o.featherWidth &&
                    ignoreFrameWidth == o.ignoreFrameWidth &&
                    morph == o.morph &&
                    lensNr == o.lensNr &&
                    source == o.source
                );
        }
//        QDomElement toXML(QDomDocument & doc);
//        void setFromXML(const QDomNode & node);


    // PT state
    /// u10           specify width of feather for stitching. default:10
    unsigned int featherWidth;
    /// m20           ignore a frame 20 pixels wide. default: 0
    unsigned int ignoreFrameWidth;
    /// Morph-to-fit using control points.
    bool morph;

    // the lens of this image
    unsigned int lensNr;
    /// image source, used to determine the linking of variables
    ImageSource source;
};


    /** This class holds an source image.
     *
     *  It contains information about its settings for the panorama,
     *  and holds the Image data (as an ImageMagick image)
     *
     *  An image should not depend on the panorama.
     */
    class PanoImage
    {
    public:
        PanoImage(const std::string & filename);
        // create from xml node
//        PanoImage(QDomNode & node);

        virtual ~PanoImage();

        virtual const char * isA() const { return "PanoImage"; };

//        QDomElement toXML(QDomDocument & doc);
//        void setFromXML(QDomNode & node);

        std::string getFilename() const
            { return filename; }

        const ImageOptions & getOptions() const
            { return options; }

        void setOptions(const ImageOptions & opt)
            { options = opt; }

        unsigned int getHeight() const
            { return height; }
        unsigned int getWidth() const
            { return width; }

        void setLens(unsigned int l)
            { options.lensNr = l; }
        unsigned int getLens() const
            { return options.lensNr; }

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
    };

} // namespace

#endif // PANOIMAGE_H
