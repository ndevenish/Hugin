// -*- c-basic-offset: 4 -*-
//
// Pablo d'Angelo <pablo@mathematik.uni-ulm.de>
// Last change: Time-stamp: <04-Apr-2003 23:07:55 pablo@island.wh-wurm.uni-ulm.de>
//
//

#ifndef PANOIMAGE_H
#define PANOIMAGE_H

#include <iostream>
#include <qstring.h>
#include <qdom.h>
#include <qpixmap.h>

namespace PT {

    class Panorama;

    struct LensSettings {

        enum ProjectionFormat { RECTILINEAR = 0,
                                PANORAMIC = 1,
                                CIRCULAR_FISHEYE = 2,
                                FULL_FRAME_FISHEYE = 3,
                                EQUIRECTANGULAR = 4};

        LensSettings()
            : exifFocalLength(0.0),
              exifFocalLengthConversionFactor(0.0),
              exifHFOV(0.0),
              focalLength(35),
              focalLengthConversionFactor(1),
              HFOV(50),
              projectionFormat(RECTILINEAR),
              a(0),b(0),c(0),
              d(0),e(0)
            {}

        QDomElement toXML(QDomDocument & doc);
        void setFromXML(const QDomNode & node);

        void update(const LensSettings & l)
            {
                focalLength = l.focalLength;
                focalLengthConversionFactor = l.focalLengthConversionFactor;
                HFOV = l.HFOV;
                projectionFormat = l.projectionFormat;
                a = l.a;
                b = l.b;
                c = l.c;
                d = l.d;
                e = l.e;
            }

        double exifFocalLength;
        // factor for conversion of focal length to
        // 35 mm film equivalent.
        double exifFocalLengthConversionFactor;
        double exifHFOV;

        double focalLength;
        double focalLengthConversionFactor;
        double HFOV;
        ProjectionFormat projectionFormat;
        // lens correction parameters
        double a,b,c;
        // horizontal & vertical offset
        // XXXX maybe these are not really lens settings.
        // XXXX can be different between scanned images as well.
        double d,e;
    };

    /** position of image inside the panorama.
     */
    struct ImagePosition {
        ImagePosition()
            : yaw(0), pitch(0), roll(0)
            { }
        QDomElement toXML(QDomDocument & doc);
        void setFromXML(const QDomNode & node);

        double yaw;
        double pitch;
        double roll;
    };

    /** optimization & stitching options. */
    struct ImageOptions {
        ImageOptions()
            : featherWidth(10),
              ignoreFrameWidth(0),
              morph(false),
              optimizeYaw(true),
              optimizeRoll(true),
              optimizePitch(true),
              optimizeFOV(false),
              optimizeA(false),
              optimizeB(true),
              optimizeC(false),
              optimizeD(false),
              optimizeE(false)
            { };

        QDomElement toXML(QDomDocument & doc);
        void setFromXML(const QDomNode & node);

        //seam options
        /// u10           specify width of feather for stitching. default:10
        unsigned int featherWidth;

        /// m20           ignore a frame 20 pixels wide. default: 0
        unsigned int ignoreFrameWidth;

        /// Morph-to-fit using control points.
        bool morph;

        // optimize options
        bool optimizeYaw;
        bool optimizeRoll;
        bool optimizePitch;
        bool optimizeFOV;
        bool optimizeA;
        bool optimizeB;
        bool optimizeC;
        bool optimizeD;
        bool optimizeE;
    };

    class PanoImage
    {
    public:
        PanoImage(Panorama & parent, const QString & filename);
        // create from xml node
        PanoImage(Panorama & parent, QDomNode & node);

        /// common init for all constructors
        void init();
        virtual ~PanoImage();

        virtual const char * isA() const { return "PanoImage"; };

        QDomElement toXML(QDomDocument & doc);
        void setFromXML(QDomNode & node);

        void printImageLine(std::ostream &o);
        void printStitchImageLine(std::ostream &o);
        void printOptimizeLine(std::ostream &o);

        unsigned int getNr();

        QString getFilename() const
            { return filename; }

        const QPixmap & getPixmap();

        const LensSettings & getLens() const
            { return lens; }
        void updateLens(const LensSettings & l)
            {
                lens.update(l);
                changed();
            }

        const ImagePosition & getPosition() const
            { return position; };

        void setPosition(const ImagePosition & pos)
            {
                position = pos;
                changed();
            }

        const ImageOptions & getOptions() const
            { return options; };

        void setOptions(const ImageOptions & opts)
            {
                options = opts;
                changed();
            }

        unsigned int getHeight() const
            { return height; }
        unsigned int getWidth() const
            { return width; }

    private:

        void changed();
        /// read image info (size, exif header)
        bool readImageInformation();
        // read information from file
        bool readJPEGInfo();
        bool readTIFFInfo();
        bool readPNGInfo();

        bool isLandscape;

        PT::Panorama & pano;
        
        // image properties needed by Panorama tools.
        QString filename;
        int height,width;

        ImagePosition position;
        LensSettings lens;
        ImageOptions options;
        
        QPixmap pixmap;
    };

} // namespace

#endif // PANOIMAGE_H
