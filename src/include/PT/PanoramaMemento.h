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
    void Lens::setHFOV(double d);

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

private:

    vigra::Size2D m_imageSize;
    LensProjectionFormat m_projectionFormat;
    FDiff2D m_sensorSize;
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
                            EQUIRECTANGULAR = 2 };

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
        QTVR
    };

    /** blending mechanism */
    enum BlendingMechanism {
	NO_BLEND,
	WEIGHTED_BLEND,
	SPLINE_BLEND,
	CHESSBOARD_BLEND
    };

    /** type of color correction
     */
    enum ColorCorrection { NONE = 0,
                           BRIGHTNESS_COLOR,
                           BRIGHTNESS,
                           COLOR };

    PanoramaOptions()
        : projectionFormat(EQUIRECTANGULAR),
          HFOV(360), VFOV(180),
          width(3000),
          outfile("panorama.JPG"),outputFormat(JPEG),
          quality(90),
	  tiff_saveROI(false),
          colorCorrection(NONE), colorReferenceImage(0),
          gamma(1.0), interpolator(vigra_ext::INTERP_CUBIC),
          optimizeReferenceImage(0),
          featherWidth(10),
          remapAcceleration(MAX_SPEEDUP),
          blendMode(WEIGHTED_BLEND)
        {};

    void reset()
        {
            HFOV = 360;
            VFOV = 180;
            width = 3000;
            outfile = "panorama.JPG";
            quality = 90;
	    tiff_saveROI = false;
            colorCorrection = NONE;
            colorReferenceImage = 0;
            optimizeReferenceImage = 0;
            gamma = 1.0;
            interpolator = vigra_ext::INTERP_CUBIC;
            featherWidth = 10;
            outputFormat = JPEG;
            remapAcceleration = MAX_SPEEDUP;
            blendMode = WEIGHTED_BLEND;
        }
    virtual ~PanoramaOptions() {};

//    QDomNode toXML(QDomDocument & doc) const;

//    void setFromXML(const QDomNode & elem);

    void printScriptLine(std::ostream & o) const;

    /// return string name of output file format
    static const std::string & PanoramaOptions::getFormatName(FileFormat f);

    /** returns the FileFormat corrosponding to name.
     *
     *  if name is not recognized, FileFormat::TIFF is returned
     */
    static FileFormat PanoramaOptions::getFormatFromName(const std::string & name);

    /** calculate height of the output panorama
     *
     *  height is derived from HFOV and VFOV.
     *  formula: widht * VFOV/HFOV
     *
     */
    unsigned int getHeight() const;

    unsigned int getWidth() const
        {
            return width;
        }
    // they are public, because they need to be set through
    // get/setOptions in Panorama.

    ProjectionFormat projectionFormat;
    double HFOV;
    double VFOV;
    unsigned int width;
    std::string outfile;
    FileFormat outputFormat;
    // jpeg options
    int quality;
    // TIFF options
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

private:
    static const std::string fileformatNames[];
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

    /** improved and robustified PTScript loader */
    bool loadPTScript2_old(std::istream &i, const std::string &prefix);

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

    // FIXME support lenses
    std::vector<Lens> lenses;
    PanoramaOptions options;

    OptimizeVector optvec;

};

} // namespace
#endif // _PANORAMAMEMENTO_H
