// -*- c-basic-offset: 4 -*-
/** @file PanoramaData.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Panorama.h 1947 2007-04-15 20:46:00Z dangelo $
 *
 * !! from Panorama.h 1947 
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


class Matrix3;



namespace PT {

class PanoramaData;

typedef std::set<unsigned int> UIntSet;
typedef std::vector<unsigned int> UIntVector;


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
    virtual void panoramaChanged(PanoramaData &pano)
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
    virtual void panoramaImagesChanged(PanoramaData &pano, const UIntSet & changed)
        { DEBUG_DEBUG("DEFAULT handler method"); };

    /** notification about a new image.
     *
     *  It is called whenever an image has been added.
     */
//    virtual void panoramaImageAdded(PanoramaData &pano, unsigned int imgNr)
//        { DEBUG_WARN("DEFAULT handler method"); };

    /** notifiy about the removal of an image.
     *
     *  always called when an image is removed.
     *  Beware: the image might already destroyed when this is called.
     */
//    virtual void panoramaImageRemoved(PanoramaData &pano, unsigned int imgNr)
//        { DEBUG_WARN("DEFAULT handler method"); };

    /** notify about an image change.
     *
     *  This is called whenever the image (for example the filename)
     *  or something the image depends on (for example: Lens, Control
     *  Points) has changed.
     */
//    virtual void panoramaImageChanged(PanoramaData &pano, unsigned int imgNr)
//        { DEBUG_TRACE(""); };
    
};


/** Memento class for a PanoramaData object
*
*  Preserves the internal state of a PanoramaData.
*  Used when other objects need to get/set the state without
*  knowing anything about the internals.
*
*/
class PanoramaDataMemento
{
public:
    
    virtual ~PanoramaMemento();
    
    //    virtual PanoramaMemento & operator=(const PanoramaMemento & o);
    
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

class PanoramaData
{
    
public:

    virtual ~PanoramaData();
    
    /** get a subset of the panorama
        *
        *  This returns a panorama that contains only the images specified by \imgs
        *  Useful for operations on a subset of the panorama
        */
    virtual Panorama getSubset(const PT::UIntSet & imgs) const;
    
    /** duplicate the panorama
        *
        *  returns a copy of the pano state, except for the listeners.
        */
    virtual Panorama duplicate() const;
    
    /** clear the internal state. */
    virtual void reset();
    
    
// -- Observing --

    /** add a panorama observer.
        *
        *  It will recieve all change messages.
        *  An observer can only be added once. if its added twice,
        *  the second addObserver() will have no effect.
        */
    virtual void addObserver(PanoramaObserver *o);
    
    /** remove a panorama observer.
        *
        *  Observers must be removed before they are destroyed,
        *  else Panorama will try to notify them after they have been
        *  destroyed
        *
        *  @return true if observer was known, false otherwise.
        */
    virtual bool removeObserver(PanoramaObserver *observer);
    
    /** remove all panorama observers.
        *
        *  @warning this is a hack. it must not be used on normal Panorama's.
        */
    virtual void clearObservers();
    
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
    
    
// -- Optimization Status --
    
    /** true if control points or lens variables
    *  have been changed after the last optimisation
    */
    virtual bool needsOptimization();
    
    virtual void markAsOptimized(bool optimized=true);
    
    
// -- Data Access [TODO: clean up] --
    
    /// number of images.
    virtual  std::size_t getNrOfImages() const;
    
    /// get a panorama image, counting starts with 0
    virtual const PanoImage & getImage(std::size_t nr) const;

    /// get a complete description of a source image
    virtual SrcPanoImage getSrcImage(unsigned imgNr) const;

    /// set a panorama image, counting starts with 0
    virtual void setImage(std::size_t nr, PanoImage img);
    
    /// the the number for a specific image
//    virtual unsigned int getImageNr(const PanoImage * image) const;

    /// number of control points
    virtual  std::size_t getNrOfCtrlPoints() const;
    
    /// get a control point, counting starts with 0
    virtual const ControlPoint & getCtrlPoint(std::size_t nr) const;
    
    /** return all control points for a given image. */
    virtual std::vector<unsigned int> getCtrlPointsForImage(unsigned int imgNr) const;
    
    /// get the number of a control point
//    virtual unsigned int getCtrlPointNr(const ControlPoint * point) const;

    /// get all control point of this Panorama
    virtual const CPVector & getCtrlPoints() const;

    /** get the next unused line number for t3, ... control point creation */
    virtual int getNextCPTypeLineNumber() const;
    
    /// get variables of this panorama
    virtual const VariableMapVector & getVariables() const;

    /// get variables of an image
    virtual const VariableMap & getImageVariables(unsigned int imgNr) const;

    /** return the optimize settings stored inside panorama */
    virtual const OptimizeVector & getOptimizeVector() const;

    /** set optimize setting */
    virtual void setOptimizeVector(const OptimizeVector & optvec);

    /** get a lens */
    virtual const Lens & getLens(unsigned int lensNr) const;

    /** get number of lenses */
    virtual unsigned int getNrOfLenses() const;

    /** returns the options for this panorama */
    virtual const PanoramaOptions & getOptions() const;

    // iterator like interface for the images and control points
//    ImageVector::const_iterator
        


    /** add an Image to the panorama
     *
     *  The Image must be initialized, the Lens must exist.
     *
     */
    virtual unsigned int addImage(const PanoImage &img, const VariableMap &vars);

    /** creates an image, from filename, and a Lens, if needed */
    virtual int addImageAndLens(const std::string & filename, double HFOV);

    /** add an Image to the panorama
     *  @return image number
     */
//    virtual unsigned int addImage(const std::string & filename);

    /** remove an Image.
     *
     *  also deletes/updates all associated control points
     *  and the Lens, if it was only used by this image.
     */
    virtual void removeImage(unsigned int nr);

    /** set input image parameters */
    virtual void setSrcImage(unsigned int nr, const SrcPanoImage & img);

    /** set a new image filename
     *
     *  It is assumed that it is of the same size
     *  as the old image.
     *
     */
    virtual void setImageFilename(unsigned int img, const std::string & fname);

    /** change image properties.
     */
    virtual void setImageOptions(unsigned int i, const ImageOptions & opts);

    /** set a lens for this image.
     *
     *  copies all lens variables into the image.
     */
    virtual void setLens(unsigned int imgNr, unsigned int lensNr);

    /** swap images.
     *
     *  swaps the images, image @p img1 becomes @p img2 and the other way round
     */
    virtual void swapImages(unsigned int img1, unsigned int img2);

    //===================================

    /** add a new control point.
     */
    virtual unsigned int addCtrlPoint(const ControlPoint & point);

    /** remove a control point.
     */
    virtual void removeCtrlPoint(unsigned int pNr);

    /** change a control Point.
     */
    virtual void changeControlPoint(unsigned int pNr, const ControlPoint & point);

    /** set all control points */
    virtual void setCtrlPoints(const CPVector & points);

    /** assign new mode line numbers, if required */
    virtual void updateLineCtrlPoints();

    //=============================

    /** add a new lens.
     *
     */
    virtual unsigned int addLens(const Lens & lens);

    /** Change the variable for a single lens
     *
     *  updates a lens variable, copies it into
     *  all images.
     *
     */
    virtual void updateLensVariable(unsigned int lensNr, const LensVariable &var);

    /** update a lens
     *
     *  Changes the lens variables in all images of this lens.
     */
    virtual void updateLens(unsigned int lensNr, const Lens & lens);

    /** remove a lens
     *
     *  it is only possible when it is not used by any image.
     */
    virtual void removeLens(unsigned int lensNr);

    /** remove unused lenses.
     *
     *  some operations might create lenses that are not
     *  referenced by any image. This functions removes them.
     *
     */
    virtual void removeUnusedLenses();

    //=============================

    /** set new output settings
     *  This is not used directly for optimizing/stiching, but it can
     *  be feed into runOptimizer() and runStitcher().
     */
    virtual void setOptions(const PanoramaOptions & opt);
    
    //=============================
    
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
    
    
// -- Memento interface --
    
    /// get the internal state
    virtual PanoramaMemento getMemento() const;

    /// set the internal state
    virtual void setMemento(PanoramaMemento & state);

    
// -- script interface --
        
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
    virtual void readOptimizerOutput(const UIntSet & imgs, VariableMapVector & vars, CPVector & ctrlPoints) const;
    
    
    /// read after optimization, fills in control point errors.
    virtual void parseOptimizerScript(std::istream & i, const UIntSet & imgs,
                                      VariableMapVector & imgVars,
                                      CPVector & ctrlPoints) const;
    
    /// create an optimizer script
    virtual void printPanoramaScript(std::ostream & o,
                                     const OptimizeVector & optvars,
                                     const PanoramaOptions & options,
                                     const UIntSet & imgs,
                                     bool forPTOptimizer,
                                     const std::string & stripPrefix="") const;
    
    /// create the stitcher script
    virtual void printStitcherScript(std::ostream & o, const PanoramaOptions & target,
                                     const UIntSet & imgs) const;
    
    
// -- document interface [TODO: to be moved out] --

    /** clear dirty flag. call after save */
    void clearDirty()
    {
        dirty = false;
        changeFinished(true);
    }
    
    /** true if there are unsaved changes */
    bool isDirty() const
    { return dirty; }
    

};



} // namespace




#endif // _PANORAMA_H
