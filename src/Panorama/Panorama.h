// -*- c-basic-offset: 4 -*-
/** @file Panorama2.h
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

#ifndef _PANORAMA_H
#define _PANORAMA_H

#include <cassert>
#include <vector>
#include <sstream>

//#include <qxmlelement.h>
//#include <Magick++.h>

#include "../utils.h"
#include "PanoImage.h"

class Process;

namespace PT {

class Panorama;

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
    const std::string & getName()
        { return name; }

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

    enum ProjectionFormat { RECTILINEAR = 0,
                            PANORAMIC = 1,
                            CIRCULAR_FISHEYE = 2,
                            FULL_FRAME_FISHEYE = 3,
                            EQUIRECTANGULAR = 4};

    Lens()
        : exifFocalLength(0.0),
          exifFocalLengthConversionFactor(0.0),
          exifHFOV(0.0),
          focalLength(0),
          focalLengthConversionFactor(1),
          HFOV(0),
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
    ProjectionFormat projectionFormat;
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
/*
    ControlPoint()
        : image1Nr(0), image1Nr(0),
          x1(0),y1(0),
          x2(0),y2(0),
          error(0), mode(X_Y)
        { };
*/
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
          width(0), height(0),
          outfile("panorama.jpg"),outputFormat("JPEG"),
          quality(80),progressive(false),
          colorCorrection(NONE), colorReferenceImage(0),
          gamma(1.0), interpolator(POLY_3)
        {};

    virtual ~PanoramaOptions() {};

    enum ProjectionFormat { RECTILINEAR = 0,
                            CYLINDRICAL = 1,
                            EQUIRECTANGULAR = 2 };

    enum ColorCorrection { NONE = 0,
                           BRIGHTNESS_COLOR,
                           BRIGHTNESS,
                           COLOR };

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
typedef std::vector<PanoImage*> ImagePtrVector;
typedef std::vector<ImageVariables> VariablesVector;
typedef std::vector<OptimizerSettings> OptimizeVector;


/** this handler class will receive change events from the Panorama.
 *
 *  Maybe a fine grained event interface is better, but it can be
 *  added later.
 */

class PanoramaObserver
{
public:
    virtual ~PanoramaObserver()
        { };
    virtual void panoramaChanged() = 0;
};


/// helper functions for parsing a script line
bool getPTParam(std::string & output, const std::string & line, const std::string & parameter);

template <class T>
bool getParam(T & v, const std::string & line, const std::string & parameter);

bool readVar(Variable & var, const std::string & line);


/** memento class, contains the panorama state.
 *
 *  useful for undo purposes.
 */
class PanoramaMemento
{
public:
    PanoramaMemento()
        { };

private:
    friend class PT::Panorama;
    // state members for the state

};

/** Model for a panorama.
 *
 *  This class contains the properties of a panorama
 *  That is:
 *       - pictures
 *       - variables that can be optimized, they will not be stored
 *         inside the pictures, because some of them are related to other variables.
 *         for example linked lens parameters.
 *       - control points
 *       - properites of the output (?).
 *
 *  view and controller classes can get information about these
 *  with the getFunctions.
 *
 *  Images, Lens, and Control points are numbered, and const references are
 *  handed out.  this means that all interaction will be based on
 *  image/lens/control point numbers. The references are not stable,
 *  they might disappear when other functions of this class are
 *  called, so its best to get a new reference whenever you need the object.
 *
 *  This also means that the whole object is not threadsafe and concurrent
 *  access has to be synchronized from the outside.
 *
 *  Changes should be made through command objects, not with direct calls.
 *
 *  @todo should the changer must call the report() functions?
 *
 *  @todo should we add constraints for the simple / advanced functionality
 *        to the model? I have to think a bit more about that issue. maybe the
 *        contraints can be factored out into another class that corrects
 *        then when updating. or we could have different models..
 *        SimplePanorama and AdvancedPanorama.
 *
 *  also, it is useful to use the memento pattern for the internal
 *  state, so that redo/undo for complex interactions can be
 *  implemented without too much pain.
 *
 */

class Panorama
{
public:

    /** ctor.
     */
    Panorama(PanoramaObserver * o = 0);

    /** dtor.
     */
    virtual ~Panorama();

    /// serialize to an xml document
//    QDomElement toXML(QDomDocument & doc);

    /// read from an xml document
//    void setFromXML(const QDomNode & elem);

    /** set a handler that is notified when the panorama changes.
     *
     *  only one handler is possible, this will overwrite the old handler
     */
    void setObserver(PanoramaObserver * o)
        { observer = o; }

    // query interface, used by the view to get information about
    // the panorama.

    /// number of images.
    std::size_t getNrOfImages() const
        { return images.size(); };
    /// get a panorama image, counting starts with 0
    const PanoImage & getImage(std::size_t nr) const
        {
            assert(nr < images.size());
            return *(images[nr]);
        };
    /// the the number for a specific image
//    unsigned int getImageNr(const PanoImage * image) const;


    /// number of control points
    std::size_t getNrOfCtrlPoints() const
        { return ctrlPoints.size(); };
    /// get a control point, counting starts with 0
    const ControlPoint & getCtrlPoint(std::size_t nr) const
        {
            assert(nr < ctrlPoints.size());
            return ctrlPoints[nr];
        };
    /** return all control points for a given image. */
    std::vector<unsigned int> getCtrlPointsForImage(unsigned int imgNr) const;
    /// get the number of a control point
//    unsigned int getCtrlPointNr(const ControlPoint * point) const;

    /// get all control point of this Panorama
    const CPVector & getCtrlPoints() const
        { return ctrlPoints; }

    /// get variables of this panorama
    const VariablesVector & getVariables() const;

    /// get variables of an image
    const ImageVariables & getVariable(unsigned int imgNr) const;


    /** get a lens
     */
    const Lens & getLens(unsigned int lensNr) const;

    /** get number of lenses */
    unsigned int getNrOfLenses() const
        { return lenses.size(); }


    const PanoramaOptions & getOptions() const
        { return options; }


    // iterator like interface for the images and control points
//    ImageVector::const_iterator

    /** optimize panorama variables.
     *
     *  this will start the optimization process.
     *
     *  after PTOptimizer exited/or was killed, use
     *  readOptimizerOutput() to get the optimized variables.
     *
     *  @param optvars what variables should be optimized
     *  @param proc Process that will run PTOptimizer
     *  @param options specify wich output format is used.
     *                 influences H or V style control points.
     *  @return true if PTOptimizer could be started
     */
    bool runOptimizer(Process & proc, const OptimizeVector & optvars, const PanoramaOptions & options) const;

    /** uses default options */
    bool runOptimizer(Process & proc, const OptimizeVector & optvars) const;

    /** this will stitch the Panorama.
     *
     *  @param proc Process that will run PTOptimizer
     *  @param target description of output image
     *  @return true if PTStitcher could be started
     */
    bool runStitcher(Process & proc, const PanoramaOptions & target) const;

    /** parse optimzier output
     *
     *  @param vars will be set the the optimzied variables
     *  @param ctrlPoints will contain the controlpoints, with distance
     *         information
     *
     *  @return false on error (could not read optimizer output, parse error)
     */
    void readOptimizerOutput(VariablesVector & vars, CPVector & ctrlPoints) const;

    // ============================================================
    //
    // function that modifies the internal state.
    // should not be used directly, but through command objects.
    // ============================================================


    /** Set the variables.
     *  Usually used when the optimizer results should be applied.
     */
    void updateVariables(const VariablesVector & vars);

    /** Set variables for a single picture.
     *
     */
    void updateVariables(unsigned int imgNr, const ImageVariables & var);

    /** update control points distances.
     *
     *  updates control distances and position in final panorama
     *  usually used to set the changes from the optimization.
     */
    void updateCtrlPoints(const CPVector & controlPoints);

    /** add an Image to the panorama
     *  @return image number
     */
    unsigned int addImage(const std::string & filename);

    /** remove an Image.
     *
     *  also deletes/updates all associated control points
     *
     */
    void removeImage(unsigned int nr);

    /** change image properties.
     *
     *  @todo what should be changed? Lens settings etc. they are part
     *        of OptimizeVariables, but might need extra fields.
     */
    void changeImage();

    /** set a lens for this image. */
    void setLens(unsigned int imgNr, unsigned int lensNr);

    //===================================

    /** add a new control point.
     */
    unsigned int addCtrlPoint(const ControlPoint & point);

    /** remove a control point.
     */
    void removeCtrlPoint(unsigned int pNr);

    /** change a control Point.
     */
    void changeControlPoint(unsigned int pNr, const ControlPoint & point);

    //=============================

    /** add a new lens.
     */
    unsigned int addLens(const Lens & lens);

    /** update a lens
     */
    void updateLens(unsigned int lensNr, const Lens & lens);

    /** set new output settings
     *  This is not used directly for optimizing/stiching, but it can
     *  be feed into runOptimizer() and runStitcher().
     */
    void setOptions(const PanoramaOptions & opt);


    /// get the internal state
    PanoramaMemento getMemento() const;

    /// set the internal state
    void setMemento(PanoramaMemento & state);

    /// read after optimization, fills in control point errors.
    void parseOptimizerScript(std::istream & i, VariablesVector & imgVars, CPVector & ctrlPoints) const;

    /// create an optimizer script
    void printOptimizerScript(std::ostream & o,
                              const OptimizeVector & optvars,
                              const PanoramaOptions & options) const;

    /// create the stitcher script
    void printStitcherScript(std::ostream & o, const PanoramaOptions & target) const;

    // subject interface
    /// call after a change has happend
    void changeFinished();


protected:

    /// remove all data, creates a fresh, empty panorama
    void reset();

    /// adjust the links of the linked variables, must be called
    /// when a lens has been changed.
    void adjustVarLinks();

    void updateLens(unsigned int imgNr);


private:

    // data
    enum ProcessType { NO_PROCESS, OPTIMIZER, STITCHER };

    // to run stitcher & optimizer
    ProcessType currentProcess;
    std::string optimizerExe;
    std::string stitcherExe;
    std::string PTScriptFile;

    bool dirty;

    ImagePtrVector images;
    VariablesVector variables;

    CPVector ctrlPoints;

    // FIXME support lenses
    std::vector<Lens> lenses;

    PanoramaObserver * observer;

    PanoramaOptions options;
};

} // namespace

#endif // _PANORAMA2_H
