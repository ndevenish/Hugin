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
#include <map>
#include <algorithm>
#include <set>
#include <math.h>

#ifdef HasPANO13
extern "C" {

#ifdef __INTEL__
#define __INTELMEMO__
#undef __INTEL__
#endif

#include "pano13/panorama.h"

#ifdef __INTELMEMO__
#define __INTEL__
#undef __INTELMEMO__
#endif

// remove stupid #defines from the evil windows.h

#ifdef DIFFERENCE
#undef DIFFERENCE
#endif

#ifdef MIN
#undef MIN
#endif

#ifdef MAX
#undef MAX
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif
}
#endif

#include "PT/PanoImage.h"

#include "vigra_ext/Interpolators.h"

namespace PT {

/** a variable has a value and a name.
 *
 *  linking is only supported by LinkedVariable, which
 *  is only used by Lens.
 */
class Variable
{
public:
    Variable(const std::string & name, double val = 0.0)
        : name(name), value(val)
        { };
    virtual ~Variable() {};

    /// print this variable
    virtual std::ostream & print(std::ostream & o) const;

    const std::string & getName() const
        { return name; }
    void setValue(double v)
        { value = v; }
    double getValue() const
        { return value; }

protected:

    std::string name;
    double value;
};


/** A lens variable can be linked.
 *
 *  It is only used in the lens class, not directly in the images.
 */
class LensVariable : public Variable
{
public:
    LensVariable(const std::string & name, double value, bool link=false)
        : Variable(name, value), linked(link)
        { };
    virtual ~LensVariable() {}
    virtual std::ostream & printLink(std::ostream & o, unsigned int link) const;

    bool isLinked() const
        { return linked; }
    void setLinked(bool l=true)
        { linked = l; }
private:
    bool linked;
};



/** functor to print a variable. */
struct PrintVar : public std::unary_function<Variable, void>
{
    PrintVar(std::ostream & o) : os(o) { }
    void operator() (Variable x) const { x.print(os) << " "; }
    std::ostream& os;
};

typedef std::map<std::string,Variable> VariableMap;
typedef std::vector<VariableMap> VariableMapVector;
typedef std::map<std::string,LensVariable> LensVarMap;

/** fill map with all image & lens variables */
void fillVariableMap(VariableMap & vars);

/** just lens variables */
void fillLensVarMap(LensVarMap & vars);

/** print a variable map to \p o */
void printVariableMap(std::ostream & o, const VariableMap & vars);

class Lens {

public:
    /** Lens type
     */
    enum LensProjectionFormat { RECTILINEAR = 0,
                                PANORAMIC = 1,
                                CIRCULAR_FISHEYE = 2,
                                FULL_FRAME_FISHEYE = 3,
                                EQUIRECTANGULAR = 4};


    /** construct a new lens.
     *
     */
    Lens();

//    QDomElement toXML(QDomDocument & doc);
//    void setFromXML(const QDomNode & node);

    /** try to fill Lens data (HFOV) from EXIF header
     *
     *  @return true if focal length was found, lens will contain
     *          the correct data.
     */
//    bool readEXIF(const std::string & filename);

    /** get projection type */
    LensProjectionFormat getProjection() const
    {
        return m_projectionFormat;
    }

    /** set projection type */
    void setProjection(LensProjectionFormat l) {
        m_projectionFormat = l;
    }

    /** get HFOV in degrees */
    double getHFOV() const;

    /** set HFOV in degrees */
    void setHFOV(double d);

    /** get focal length of lens, it is calculated from the HFOV */
    double getFocalLength() const;

    /** set focal length, updates HFOV */
    void setFocalLength(double);

    /** get crop factor, d35mm/dreal */
    double getCropFactor() const;

    /** Set the crop factor.
     *
     *  This will recalculate the sensor size.
     *
     *  @param c is the ratio of the sensor diagonals:
     *                factor = diag35mm / real_diag
     */
    void setCropFactor(double c);

    /** get sensor dimension */
    FDiff2D getSensorSize() const
    {
        return m_sensorSize;
    }

    /** set sensor dimensions. Only square pixels are supported so far.*/
    void setSensorSize(const FDiff2D & size);

    /** return the sensor ratio (width/height)
     */
    double getAspectRatio() const
    {
        return (double)m_imageSize.x / m_imageSize.y;
    }

    /** check if the image associated with this lens is in landscape orientation.
     */
    bool isLandscape() const
    {
        return m_imageSize.x >= m_imageSize.y;
    }

    /** get the image size, in pixels */
    vigra::Size2D getImageSize() const
    {
        return m_imageSize;
    }

    /** set image size in pixels */
    void setImageSize(const vigra::Size2D & sz)
    {
        m_imageSize = sz;
    }

    /** try to read image information from file */
    bool initFromFile(const std::string & filename, double &cropFactor, double & roll);

//    double isLandscape() const {
//        return sensorRatio >=1;
//    }

    // updates everything, including the lens variables.
    void update(const Lens & l);


//    bool isLandscape;
    // these are the lens specific settings.
    // lens correction parameters
    LensVarMap variables;
    static char *variableNames[];

    bool m_hasExif;
private:

    LensProjectionFormat m_projectionFormat;
    vigra::Size2D m_imageSize;
    FDiff2D m_sensorSize;
};


/// represents a control point
class ControlPoint
{
public:
    /** minimize x,y or both. higher numbers mean multiple line
     * control points
     */
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
                 int mode = X_Y)
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
    int mode;

    static std::string modeNames[];
};

/** Panorama image options
 *
 *  this holds the settings for the final panorama
 */
class PanoramaOptions
{
public:


    /** Projection of final panorama
     */
    enum ProjectionFormat { RECTILINEAR = 0,
                            CYLINDRICAL = 1,
                            EQUIRECTANGULAR = 2,
                            FULL_FRAME_FISHEYE = 3,
                            STEREOGRAPHIC = 4,
                            MERCATOR = 5,
                            TRANSVERSE_MERCATOR = 6,
                            SINUSOIDAL = 7,
                            LAMBERT = 8,
                            LAMBERT_AZIMUTHAL = 9,
                            ALBERS_EQUAL_AREA_CONIC = 10,
                            MILLER_CYLINDRICAL = 11
    };

    /** PTStitcher acceleration */
    enum PTStitcherAcceleration {
        NO_SPEEDUP,
        MAX_SPEEDUP,
        MEDIUM_SPEEDUP  // for projects with morphing.
    };

    /** Fileformat
     */
    enum FileFormat {
        JPEG = 0,
        PNG,
        TIFF,
        TIFF_m,
        TIFF_mask,
        TIFF_multilayer,
        TIFF_multilayer_mask,
        PICT,
        PSD,
        PSD_m,
        PSD_mask,
        PAN,
        IVR,
        IVR_java,
        VRML,
        QTVR,
        HDR,
        HDR_m
    };

    /** blending mechanism */
    enum BlendingMechanism {
        NO_BLEND=0,
        PTBLENDER_BLEND=1,
        ENBLEND_BLEND=2,
        SMARTBLEND_BLEND=3
    };

    enum Remapper {
        NONA=0,
        PTMENDER
    };

    /** type of color correction
     */
    enum ColorCorrection { NONE = 0,
                           BRIGHTNESS_COLOR,
                           BRIGHTNESS,
                           COLOR };

    PanoramaOptions()
        {
            reset();
        };

    void reset()
        {
            m_projectionFormat = EQUIRECTANGULAR;
#ifdef HasPANO13
            panoProjectionFeaturesQuery(m_projectionFormat, &m_projFeatures);
#endif
            m_hfov = 360;
            m_size = vigra::Size2D(3000, 1500);
            m_roi = vigra::Rect2D(m_size);
            outfile = "panorama.JPG";
            quality = 90;
            tiff_saveROI = false;
            tiffCompression = "NONE";
            colorCorrection = NONE;
            colorReferenceImage = 0;
            optimizeReferenceImage = 0;
            gamma = 1.0;
            interpolator = vigra_ext::INTERP_CUBIC;
            featherWidth = 10;
            outputFormat = JPEG;
            remapAcceleration = MAX_SPEEDUP;
            blendMode = NO_BLEND;
            remapper = NONA;
            saveCoordImgs = false;
            huberSigma = 0;
        }
    virtual ~PanoramaOptions() {};

    void printScriptLine(std::ostream & o) const;

    /// return string name of output file format
    static const std::string & getFormatName(FileFormat f);

    /** returns the FileFormat corrosponding to name.
     *
     *  if name is not recognized, FileFormat::TIFF is returned
     */
    static FileFormat getFormatFromName(const std::string & name);

    /** set panorama width 
     *  keep the HFOV, if keepView=true
     */
    void setWidth(unsigned int w, bool keepView = true);

    /* get panorama width */
    unsigned int getWidth() const
    {
        return m_size.x;
    }

    /** set panorama height 
     *
     *  This changes the panorama vfov
     */
    void setHeight(unsigned int h);

    /** get panorama height */
    unsigned int getHeight() const
    {
        return m_size.y;
    }

    /// get size of output image
    vigra::Size2D getSize() const
    {
        return m_size;
    }

    const vigra::Rect2D & getROI() const
    { return m_roi; }
    void setROI(const vigra::Rect2D & val)
    { m_roi = val; }

    /** set the Projection format and adjust the hfov/vfov
     *  if nessecary
     */
    void setProjection(ProjectionFormat f);

    PanoramaOptions::ProjectionFormat getProjection() const
    {
        return m_projectionFormat;
    };

    /** Get the optional projection parameters */
    const std::vector<double> & getProjectionParameters() const;

    /** set the optional parameters (they need to be of the correct size) */
    void setProjectionParameters(const std::vector<double> & params);

    /** true, if FOV calcuations are supported for projection \p f */
    bool fovCalcSupported(ProjectionFormat f) const;

    /** set the horizontal field of view.
     *  also updates the image height (keep pano
     *  field of view similar.)
     */
    void setHFOV(double h, bool keepView=true);

    double getHFOV() const
    {
        return m_hfov;
    }

    void setVFOV(double v);
    double getVFOV() const;

    /** get maximum possible hfov with current projection */
    double getMaxHFOV() const;
    /** get maximum possible vfov with current projection */
    double getMaxVFOV() const;

    DestPanoImage getDestImage() const;

    const std::string & getOutputExtension();

    // they are public, because they need to be set through
    // get/setOptions in Panorama.

    std::string outfile;
    FileFormat outputFormat;
    // jpeg options
    int quality;
    // TIFF options
    std::string tiffCompression;
    bool tiff_saveROI;

    ColorCorrection colorCorrection;
    unsigned int colorReferenceImage;

    // misc options
    double gamma;
    vigra_ext::Interpolator interpolator;

    unsigned int optimizeReferenceImage;
    unsigned int featherWidth;

    PTStitcherAcceleration remapAcceleration;
    BlendingMechanism blendMode;
    Remapper remapper;

    bool saveCoordImgs;

    double huberSigma;

#ifdef HasPANO13
    pano_projection_features m_projFeatures;
#endif


private:
    static const std::string fileformatNames[];
    static const std::string fileformatExt[];
    double m_hfov;
//    unsigned int m_width;
//    unsigned int m_height;
    ProjectionFormat m_projectionFormat;

    std::vector<double> m_projectionParams;
    vigra::Size2D m_size;
    vigra::Rect2D m_roi;
};

typedef std::vector<ControlPoint> CPVector;
typedef std::vector<PanoImage> ImageVector;
typedef std::vector<std::set<std::string> > OptimizeVector;
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
     : needsOptimization(false)
        { };
    /// copy ctor.
//    PanoramaMemento(const PanoramaMemento & o);
    /// assignment operator
//    PanoramaMemento & operator=(const PanoramaMemento & o);
    virtual ~PanoramaMemento();

    /** enum for supported PTScript syntax bastards */
    enum PTFileFormat { PTFILE_HUGIN, PTFILE_PTGUI, PTFILE_PTA };


    /** load a PTScript file
     *
     *  initializes the PanoramaMemento from a PTScript file
     */
    bool loadPTScript(std::istream & i, const std::string & prefix = "");

private:

    enum PTParseState { P_NONE,
                        P_OUTPUT,
                        P_MODIFIER,
                        P_IMAGE,
                        P_OPTIMIZE,
                        P_CP
    };


    friend class PT::Panorama;
    // state members for the state

    ImageVector images;
    VariableMapVector variables;

    CPVector ctrlPoints;

    std::vector<Lens> lenses;
    PanoramaOptions options;

    OptimizeVector optvec;

    // indicates that changes have been made to
    // control points or lens parameters after the
    // last optimisation
    bool needsOptimization;
};

} // namespace
#endif // _PANORAMAMEMENTO_H
