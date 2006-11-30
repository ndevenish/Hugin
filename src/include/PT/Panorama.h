// -*- c-basic-offset: 4 -*-
/** @file Panorama.h
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

#include "common/math.h"
#include "common/utils.h"

#include <vector>
#include <set>
#include <sstream>

#include "PT/PanoImage.h"
#include "PT/PanoramaMemento.h"


namespace PT {

class Panorama;
class PanoCommand;

typedef std::set<unsigned int> UIntSet;
typedef std::vector<unsigned int> UIntVector;

/// helper functions for parsing a script line
bool getPTParam(std::string & output, const std::string & line, const std::string & parameter);

#if 0
template <class T>
bool getParam(T & value, const std::string & line, const std::string & name)
{
    std::string s;
    if (!getPTParam(s, line, name)) {
        return false;
    }
    std::istringstream is(s);
    is >> value;
    return true;
}
#endif

template <class T>
bool getIntParam(T & value, const std::string & line, const std::string & name)
{
    std::string s;
    if (!getPTParam(s, line, name)) {
        return false;
    }
    std::istringstream is(s);
    is >> value;
    return true;
}

bool readVar(Variable & var, int & link, const std::string & line);

bool getPTStringParam(std::string & output, const std::string & line,
	              const std::string & parameter);

bool getPTStringParamColon(std::string & output, const std::string & line, const std::string & parameter);

bool getDoubleParam(double & d, const std::string & line, const std::string & name);

bool getPTDoubleParam(double & value, int & link,
                      const std::string & line, const std::string & var);
		

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
    /** Notification about a Panorama change.
     *
     *  This function will always be called, even when the
     *  change could be handled by panoramaImageAdded() or
     *  other notify functions.
     *
     *  This allows lazy observers to just listen to
     *  panoramaChanged().
     *
     */
    virtual void panoramaChanged(Panorama &pano)
        { DEBUG_DEBUG("Default panoramaChanged called"); };

    /** notifies about changes to images
     *
     *  Images might have been added/removed. to find out
     *  how many images are still there, use Panorama::getNrOfImages.
     *
     *  @param pano the panorama object that changed
     *  @param changed set of changed images
     *
     */
    virtual void panoramaImagesChanged(Panorama &pano, const UIntSet & changed)
        { DEBUG_DEBUG("DEFAULT handler method"); };

    /** notification about a new image.
     *
     *  It is called whenever an image has been added.
     */
//    virtual void panoramaImageAdded(Panorama &pano, unsigned int imgNr)
//        { DEBUG_WARN("DEFAULT handler method"); };

    /** notifiy about the removal of an image.
     *
     *  always called when an image is removed.
     *  Beware: the image might already destroyed when this is called.
     */
//    virtual void panoramaImageRemoved(Panorama &pano, unsigned int imgNr)
//        { DEBUG_WARN("DEFAULT handler method"); };

    /** notify about an image change.
     *
     *  This is called whenever the image (for example the filename)
     *  or something the image depends on (for example: Lens, Control
     *  Points) has changed.
     */
//    virtual void panoramaImageChanged(Panorama &pano, unsigned int imgNr)
//        { DEBUG_TRACE(""); };
};


/** Model for a panorama.
 *
 *  This class contains the properties of a panorama
 *  That is:
 *       - images
 *       - variables that can be optimized, they will not be stored
 *         inside the pictures, because some of them are related to other variables.
 *         for example linked lens parameters.
 *       - control points
 *       - properites of the output panorama.
 *
 *  view and controller classes can get information about these
 *  with the getXXX Functions.
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
 *  The Lens handling is a quite complicated thing. I divide the variables
 *  into two groups:
 *     - image variables (yaw, pitch, roll). They specify the placement
 *       in the final panorama
 *     - lens variables. They are connected to the process of image
 *       creation and are used to correct various defects that
 *       occur during image creation. They might be the same
 *       for each picture taken with the same equipment and settings or
 *       change even if the settings were the same, because of sloppy
 *       mechanic or other random infuences.
 *
 *
 *  Changes should be made through command objects, not with direct calls.
 *
 *  @todo should the changer call the report() functions?
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
 */

class Panorama
{
public:

    /** ctor.
     */
    Panorama();

    /** dtor.
     */
    virtual ~Panorama();

    /** add a panorama observer.
     *
     *  It will recieve all change messages.
     *  An observer can only be added once. if its added twice,
     *  the second addObserver() will have no effect.
     */
    void addObserver(PanoramaObserver *o);

    /** remove a panorama observer.
     *
     *  Observers must be removed before they are destroyed,
     *  else Panorama will try to notify them after they have been
     *  destroyed
     *
     *  @return true if observer was known, false otherwise.
     */
    bool removeObserver(PanoramaObserver *observer);

    /** remove all panorama observers.
     *
     *  @warning this is a hack. it must not be used on normal Panorama's.
     */
    void clearObservers();

    // query interface, used by the view to get information about
    // the panorama.

    /// number of images.
    std::size_t getNrOfImages() const
        { return state.images.size(); };
    /// get a panorama image, counting starts with 0
    const PanoImage & getImage(std::size_t nr) const
        {
            assert(nr < state.images.size());
            return state.images[nr];
        };

    /// get a complete description of a source image
    SrcPanoImage getSrcImage(unsigned imgNr) const;

    /// set a panorama image, counting starts with 0
    void setImage(std::size_t nr, PanoImage img)
        {
            assert(nr < state.images.size());
            state.images[nr] = img;
        };
    /// the the number for a specific image
//    unsigned int getImageNr(const PanoImage * image) const;


    /// number of control points
    std::size_t getNrOfCtrlPoints() const
        { return state.ctrlPoints.size(); };
    /// get a control point, counting starts with 0
    const ControlPoint & getCtrlPoint(std::size_t nr) const
        {
            assert(nr < state.ctrlPoints.size());
            return state.ctrlPoints[nr];
        };
    /** return all control points for a given image. */
    std::vector<unsigned int> getCtrlPointsForImage(unsigned int imgNr) const;
    /// get the number of a control point
//    unsigned int getCtrlPointNr(const ControlPoint * point) const;

    /// get all control point of this Panorama
    const CPVector & getCtrlPoints() const
        { return state.ctrlPoints; }

    /// get variables of this panorama
    const VariableMapVector & getVariables() const;

    /// get variables of an image
    const VariableMap & getImageVariables(unsigned int imgNr) const;


    /** return the optimize settings stored inside panorama */
    const OptimizeVector & getOptimizeVector() const
        { return state.optvec; };

    /** set optimize setting */
    void setOptimizeVector(const OptimizeVector & optvec);

    /** get a lens
     */
    const Lens & getLens(unsigned int lensNr) const;

    /** get number of lenses */
    unsigned int getNrOfLenses() const
        { return unsigned(state.lenses.size()); }

    /** returns the options for this panorama */
    const PanoramaOptions & getOptions() const
        { return state.options; }

    /** calculates the horizontal and vertial FOV of the complete panorama
     *
     *  @return HFOV,VFOV
     */
    FDiff2D calcFOV() const;

    /** calculate the HFOV and height so that the whole input
     *  fits in into the output panorama */
    void fitPano(double & HFOV, double & height);

    // iterator like interface for the images and control points
//    ImageVector::const_iterator


    /** parse optimzier output
     *
     *  @param set of image numbers that where used during by
     *         printPanoramaScript().
     *  @param vars will be set the the optimzied variables
     *  @param ctrlPoints will contain the controlpoints, with distance
     *         information
     *
     *  @return false on error (could not read optimizer output, parse error)
     */
    void readOptimizerOutput(const UIntSet & imgs, VariableMapVector & vars, CPVector & ctrlPoints) const;

    // ============================================================
    //
    // function that modifies the internal state.
    // should not be used directly, but through command objects.
    // ============================================================


    /** clear the internal state. */
    void reset();


    /** Set the variables.
     *
     *  Usually used when the optimizer results should be applied.
     *
     */
    void updateVariables(const VariableMapVector & vars);

    /** update variables for some specific images */
    void updateVariables(const UIntSet & imgs, const VariableMapVector & var);

    /** Set variables for a single picture.
     *
     */
    void updateVariables(unsigned int imgNr, const VariableMap & var);

    /** update a single variable
     *
     *  It knows lenses etc and updates other images when the
     *  variable is linked
     */
    void updateVariable(unsigned int imgNr, const Variable &var);

    /** center panorama horizontically */
    void centerHorizontically();

    /** rotate the complete panorama
     *
     *  Will modify the position of all images.
     */
    void rotatePanorama(double yaw, double pitch, double roll);

    /** update control points distances.
     *
     *  updates control distances and position in final panorama
     *  usually used to set the changes from the optimization.
     *  The control points must be the same as in
     */
    void updateCtrlPointErrors(const CPVector & controlPoints);

    /** update control points for a subset of images.
     *
     *  Usually, the control point subset is created using subset()
     *  The number and ordering and control points must not be changed
     *  between the call to subset() and this function.
     */
    void updateCtrlPointErrors(const UIntSet & imgs, const CPVector & cps);

    /** add an Image to the panorama
     *
     *  The Image must be initialized, the Lens must exist.
     *
     */
    unsigned int addImage(const PanoImage &img, const VariableMap &vars);

    /** creates an image, from filename, and a Lens, if needed */
    int addImageAndLens(const std::string & filename, double HFOV);

    /** add an Image to the panorama
     *  @return image number
     */
//    unsigned int addImage(const std::string & filename);

    /** remove an Image.
     *
     *  also deletes/updates all associated control points
     *  and the Lens, if it was only used by this image.
     */
    void removeImage(unsigned int nr);

    /** set input image parameters */
    void setSrcImg(unsigned int nr, const SrcPanoImage & img);

    /** set a new image filename
     *
     *  It is assumed that it is of the same size
     *  as the old image.
     *
     */
    void setImageFilename(unsigned int img, const std::string & fname);

    /** change image properties.
     */
    void setImageOptions(unsigned int i, const ImageOptions & opts);

    /** set a lens for this image.
     *
     *  copies all lens variables into the image.
     */
    void setLens(unsigned int imgNr, unsigned int lensNr);

    /** swap images.
     *
     *  swaps the images, image @p img1 becomes @p img2 and the other way round
     */
    void swapImages(unsigned int img1, unsigned int img2);

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

    /** set all control points */
    void setCtrlPoints(const CPVector & points);

    //=============================

    /** add a new lens.
     *
     */
    unsigned int addLens(const Lens & lens);


    /** Change the variable for a single lens
     *
     *  updates a lens variable, copies it into
     *  all images.
     *
     */
    void updateLensVariable(unsigned int lensNr, const LensVariable &var);


    /** update a lens
     *
     *  Changes the lens variables in all images of this lens.
     */
    void updateLens(unsigned int lensNr, const Lens & lens);

    /** remove a lens
     *
     *  it is only possible when it is not used by any image.
     */
    void removeLens(unsigned int lensNr);


    /** remove unused lenses.
     *
     *  some operations might create lenses that are not
     *  referenced by any image. This functions removes them.
     *
     */
    void removeUnusedLenses();

    //=============================

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
    void parseOptimizerScript(std::istream & i, const UIntSet & imgs,
                              VariableMapVector & imgVars,
                              CPVector & ctrlPoints) const;

    /// create an optimizer script
    void printPanoramaScript(std::ostream & o,
                             const OptimizeVector & optvars,
                             const PanoramaOptions & options,
                             const UIntSet & imgs,
                             bool forPTOptimizer,
                             const std::string & stripPrefix="");

    /// create the stitcher script
    void printStitcherScript(std::ostream & o, const PanoramaOptions & target,
		                     const UIntSet & imgs) const;

    // subject interface
    /** notify observers about changes in this class
     *
     *  This needs to be called explicitly by somebody after
     *  changes have been made.
     *  Allows to compress multiple changes into one notification.
     *
     *  @param keepDirty  do not set dirty flag. useful for changing
     *                    the dirty flag itself
     */
    void changeFinished(bool keepDirty=false);

    /** clear dirty flag. call after save */
    void clearDirty()
        {
            dirty = false;
            changeFinished(true);
        }

    /** true if there are unsaved changes */
    bool isDirty() const
        { return dirty; }

    /** mark an image as active or inactive.
        *
        *  This is only a flag, that can be turned on or off.
        *  If an image is marked active, then it should
        *  be used for optimizing and stitching.
        *
        *  However, this is not done automatically. One has
        *  to use getActiveImages() to get the numbers of the
        *  active images, and pass these to the respective
        *  functions that do the stitching or optimisation
        */
    void activateImage(unsigned int imgNr, bool active=true);

    /** get active images */
    UIntSet getActiveImages() const;

    /** get a subset of the panorama
     *
     *  This returns a panorama that contains only the images specified by \imgs
     *  Useful for operations on a subset of the panorama
     */
    Panorama getSubset(const PT::UIntSet & imgs) const;
    
    /** duplicate the panorama
     *
     *  returns a copy of the pano state, except for the listeners.
     */
    Panorama duplicate() const;

protected:


    /// adjust the links of the linked variables, must be called
    /// when a lens has been changed.
    void adjustVarLinks();

    /** copy inherited variables to image variables
     *
     *  only copies inherited variables
     */
    void updateLensToImages(unsigned int lensNr);

    /** copy lens variables to image variables.
     *  update all images that use lensNr
     */
    void copyLensToImages(unsigned int lensNr);

    /** copy the lens variables to image.
     *
     *  just update imgNr
     */
    void copyLensVariablesToImage(unsigned int imgNr);

    /// image addition notification
//    void notifyImageAdded(unsigned int imgNr);

    /// image removal notification
//    void notifyImageRemoved(unsigned int imgNr);

    /// image change notification
//    void notifyImageChanged(unsigned int imgNr);

    /** mark image for change notification.
     *
     *  Does not send the notification, this is left
     *  to changedFinished()
     */
    void imageChanged(unsigned int imgNr);


private:

    // data
    enum ProcessType { NO_PROCESS, OPTIMIZER, STITCHER };

    // to run stitcher & optimizer
    ProcessType currentProcess;
    std::string optimizerExe;
    std::string stitcherExe;
    std::string PTScriptFile;

    /// this indicates that there are unsaved changes
    bool dirty;

    PanoramaMemento state;
    std::set<PanoramaObserver *> observers;
    /// the images that have been changed since the last changeFinished()
    UIntSet changedImages;

    bool m_forceImagesUpdate;

    std::set<std::string> m_ptoptimizerVarNames;
};


/** function to calculate the scaling factor so that the distances
    in the input image and panorama image are similar at the panorama center
 */
double calcOptimalPanoScale(const SrcPanoImage & src,
                                const PanoramaOptions & dest);

// helper functions, workaround for gcc 3.3, which doesn't find
// the map_get template functions.
PT::LensVariable & map_get(PT::LensVarMap &m, const std::string & key);
const PT::LensVariable & const_map_get(const PT::LensVarMap &m, const std::string & key);
PT::Variable & map_get(PT::VariableMap &m, const std::string & key);
const PT::Variable & const_map_get(const PT::VariableMap &m, const std::string & key);


struct PTPrograms
{
    PTPrograms()
    {
        // default programs
        nona = "nona";
        PTStitcher = "PTStitcher";
        PTremap = "PTremap";
        PTroller = "PTroller";
        PTblender = "PTblender";
        enblend = "enblend";
        smartblend = "smartblend";
    }

    std::string nona;
    std::string PTStitcher;
    std::string PTremap;
    std::string PTroller;
    std::string PTblender;
    std::string enblend;
    std::string smartblend;
};


/** create a makefile and associated project file for rendering */
void createMakefile(const Panorama & pano,
                    const std::string & ptofile,
                    const std::string & outputPrefix,
                    const PT::PanoramaOptions & opts,
                    const PT::UIntSet & imgs,
                    const PTPrograms & progs,
                    const std::string & includePath,
                    std::ostream & o);


} // namespace




#endif // _PANORAMA_H
