// -*- c-basic-offset: 4 -*-
/** @file PanoramaMemento.h
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

#ifndef _PANORAMAMEMENTO_H
#define _PANORAMAMEMENTO_H


#include <string>
#include <vector>
#include <math.h>

#include "PT/PanoImage.h"

namespace PT {

/** a variable has a value, name and can be linked to another variable.
 *
 *  (I believe linking is only possible for HFOV,a,b,c,d and
 *  e. linking the position might not be a good idea ;)
 */
class Variable
{
public:
    Variable(const std::string & name)
        : name(name), value(0), linkImage(0), linked(false)
        { };

    bool operator==(const Variable & o) const
        {
            if (linked) {
                return (name == o.name &&
                        value == o.value &&
                        linkImage == o.linkImage &&
                        linked == o.linked);
            } else {
                return (name == o.name &&
                        value == o.value &&
                        linked == o.linked);

            }
        }

    /// print this variable
    std::ostream & print(std::ostream & o, bool printLinks = true) const;

    void link(unsigned int imgNr)
        { linkImage = imgNr; linked = true; }
    bool isLinked()
        { return linked; }
    unsigned int getLink()
        { return linkImage; }
    void unlink()
        { linked = false; };
    const std::string & getName() const
        { return name; }
    void setValue(double v)
        { value = v; }
    double getValue() const
        { return value; }

private:

    std::string name;
    double value;
    unsigned int linkImage;
    bool linked;
};

/// variables of an image
class ImageVariables
{
public:
    ImageVariables()
        : roll("r"), pitch("p"), yaw("y"), HFOV("v"),
          a("a"), b("b"), c("c"),d("d"),e("e")
        { };
    bool operator==(const ImageVariables & o) const
        { return ( roll == o.roll &&
                   pitch == o.pitch &&
                   yaw == o.yaw &&
                   HFOV == o.HFOV &&
                   a == o.a &&
                   b == o.b &&
                   c == o.c &&
                   d == o.d &&
                   e == o.e);
        }

    void updateValues(const ImageVariables & vars);

    std::ostream & print(std::ostream & o, bool printLinks = true) const;

    Variable roll, pitch, yaw, HFOV, a, b, c, d, e;
};

/// specifies which variables should be optimized
class OptimizerSettings
{
public:
    OptimizerSettings()
        : roll(true), pitch(true), yaw(true),
          HFOV(false), a(false), b(false), c(false), d(false), e(false)
        { };
    bool roll, pitch, yaw, HFOV, a, b, c, d, e;
    std::ostream & printOptimizeLine(std::ostream & o, unsigned int nr) const;
};


class Lens {

public:
    /** Lens type
     */
    enum LensProjectionFormat { RECTILINEAR = 0,
                                PANORAMIC = 1,
                                CIRCULAR_FISHEYE = 2,
                                FULL_FRAME_FISHEYE = 3,
                                EQUIRECTANGULAR_LENS = 4};


    Lens()
        : exifFocalLength(0.0),
          exifFocalLengthConversionFactor(0.0),
          exifHFOV(90.0),
          focalLength(0),
          focalLengthConversionFactor(1),
          HFOV(90),
          projectionFormat(RECTILINEAR),
          a(0),b(0),c(0),
          d(0),e(0)
        {}

//    QDomElement toXML(QDomDocument & doc);
//    void setFromXML(const QDomNode & node);

    /** try to fill Lens data (HFOV) from EXIF header
     *
     *  @return true if focal length was found, lens will contain
     *          the correct data.
     */
    bool readEXIF(const std::string & filename);

    void update(const Lens & l)
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
    LensProjectionFormat projectionFormat;
    // lens correction parameters
    double a,b,c;
    // horizontal & vertical offset
    // FIXME maybe these are not really lens settings.
    // FIXME can be different between scanned images as well.
    double d,e;
};


/// represents a control point
class ControlPoint
{
public:
    /// minimize x,y or both
    enum OptimizeMode {
        X_Y = 0,  ///< evaluate x,y
        X,        ///< evaluate x, points are on a vertical line
        Y         ///< evaluate y, points are on a horizontal line
    };
    ControlPoint()
        : image1Nr(0), image2Nr(0),
          x1(0),y1(0),
          x2(0),y2(0),
          error(0), mode(X_Y)
        { };

//    ControlPoint(Panorama & pano, const QDomNode & node);
    ControlPoint(unsigned int img1, double sX, double sY,
                 unsigned int img2, double dX, double dY,
                 OptimizeMode mode = X_Y)
        : image1Nr(img1), image2Nr(img2),
          x1(sX),y1(sY),
          x2(dX),y2(dY),
          error(0), mode(mode)
        { };

    bool operator==(const ControlPoint & o) const
        {
            return (image1Nr == o.image1Nr &&
                    image2Nr == o.image2Nr &&
                    x1 == o.x1 && y1 == o.y1 &&
                    x2 == o.x2 && y2 == o.y2 &&
                    mode == o.mode &&
                    error == o.error);
        }

    const std::string & getModeName(OptimizeMode mode) const;

//    QDomNode toXML(QDomDocument & doc) const;

//    void setFromXML(const QDomNode & elem, Panorama & pano);

    /// swap (image1Nr,x1,y1) with (image2Nr,x2,y2)
    void mirror();

    unsigned int image1Nr;
    unsigned int image2Nr;
    double x1,y1;
    double x2,y2;
    double error;
    OptimizeMode mode;

    static std::string modeNames[];
};

/** Projection of final panorama
 */
    enum ProjectionFormat { RECTILINEAR = 0,
                            CYLINDRICAL = 1,
                            EQUIRECTANGULAR = 2 };

/** type of color correction
 */
    enum ColorCorrection { NONE = 0,
                           BRIGHTNESS_COLOR,
                           BRIGHTNESS,
                           COLOR };

/** soften the stairs if they occure
 */
    enum Interpolator {
        POLY_3 = 0,
        SPLINE_16,
        SPLINE_36,
        SINC_256,
        SPLINE_64,
        BILINEAR,
        NEAREST_NEIGHBOUR,
        SINC_1024
    };

/** Fileformat
 */
enum FileFormat {
    JPEG = 0,
    PNG,
    TIFF,
    TIFF_mask,
    TIFF_nomask,
    PICT,
    PSD,
    PSD_mask,
    PSD_nomask,
    PAN,
    IVR,
    IVR_java,
    VRML,
    QTVR
};

/** Panorama image options
 *
 *  this holds the settings for the final panorama
 */
class PanoramaOptions
{
public:

    PanoramaOptions()
        : projectionFormat(EQUIRECTANGULAR),
          HFOV(360),
          width(300), height(600),
          outfile("panorama.JPG"),outputFormat("JPEG"),
          quality(80),progressive(false),
          colorCorrection(NONE), colorReferenceImage(0),
          gamma(1.0), interpolator(POLY_3)
        {};

    virtual ~PanoramaOptions() {};

//    QDomNode toXML(QDomDocument & doc) const;

//    void setFromXML(const QDomNode & elem);

    void printScriptLine(std::ostream & o) const;

    // they are public, because they need to be set through
    // get/setOptions in Panorama.

    ProjectionFormat projectionFormat;
    double HFOV;
    unsigned int width;
    unsigned int height;
    std::string outfile;
    std::string outputFormat;
    // jpeg options
    int quality;
    bool progressive;
    ColorCorrection colorCorrection;
    unsigned int colorReferenceImage;

    // misc options
    double gamma;
    Interpolator interpolator;

};

typedef std::vector<ControlPoint> CPVector;
typedef std::vector<PanoImage> ImageVector;
typedef std::vector<ImageVariables> VariablesVector;
typedef std::vector<OptimizerSettings> OptimizeVector;
typedef std::vector<Lens> LensVector;



class Panorama;
/** Memento class for a Panorama object
 *
 *  Holds the internal state of a Panorama.
 *  Used when other objects need to get/set the state without
 *  knowing anything about the internals.
 *
 *  It is also used for saving/loading (the state can be serialized
 *  to an xml file).
 *
 *  @todo xml support
 */
class PanoramaMemento
{
public:
    PanoramaMemento()
        { };
    /// copy ctor.
//    PanoramaMemento(const PanoramaMemento & o);
    /// assignment operator
//    PanoramaMemento & operator=(const PanoramaMemento & o);
    virtual ~PanoramaMemento();
private:
    friend class PT::Panorama;
    // state members for the state

    ImageVector images;
    VariablesVector variables;

    CPVector ctrlPoints;

    // FIXME support lenses
    std::vector<Lens> lenses;
    PanoramaOptions options;

};

} // namespace
#endif // _PANORAMAMEMENTO_H
