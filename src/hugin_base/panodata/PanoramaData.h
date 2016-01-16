// -*- c-basic-offset: 4 -*-
/** @file PanoramaData.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PANORAMA_H
#define _PANORAMA_H

// if this file is preprocessed for SWIG, we want to ignore
// all the header inclusions that follow:

#ifndef _HSI_IGNORE_SECTION

#include <hugin_shared.h>
#include <vector>
#include <set>
#include <iostream>

#include <hugin_math/Matrix3.h>
#include <panodata/PanoramaVariable.h>
#include <panodata/SrcPanoImage.h>
#include <panodata/ControlPoint.h>
#include <panodata/Lens.h>
#include <panodata/PanoramaOptions.h>

#endif // _HSI_IGNORE_SECTION

namespace HuginBase {

///
typedef std::set<unsigned int> UIntSet;

///
typedef std::vector<unsigned int> UIntVector;

typedef std::vector<UIntSet> UIntSetVector;

/** Model for a panorama.
 *
 *  This class contains the properties of a panorama
 *  That is:
 *       - images
 *       - variables that can be optimized.
 *       - links between image variables
 *       - control points
 *       - properites of the output panorama.
 *
 *  view and controller classes can get information about these
 *  with the getXXX Functions.
 *
 *  Images and Control points are numbered, and const references are
 *  handed out.  this means that all interaction will be based on
 *  image or control point numbers. The references are not stable,
 *  they might disappear when other functions of this class are
 *  called, so its best to get a new reference whenever you need the object.
 *
 *  This also means that the whole object is not threadsafe and concurrent
 *  access has to be synchronized from the outside.
 *
 */
class IMPEX PanoramaData
{
    
public:
    
    ///
    virtual ~PanoramaData() {};
    
    
    /* get a subset of the panorama
    *
    *  This returns a panorama that contains only the images specified by \imgs
    *  Useful for operations on a subset of the panorama
    */
//  virtual Panorama getSubset(const UIntSet& imgs) const;
    
    /* duplicate the panorama
    *
    *  returns a copy of the pano state, except for the listeners.
    */
//  virtual Panorama duplicate() const;
    
    ///
    virtual PanoramaData* getNewSubset(const UIntSet& imgs) const =0;
    
    ///
    virtual PanoramaData* getNewCopy() const =0;
    
    virtual PanoramaData* getUnlinkedSubset(UIntSetVector& imageGroups) const = 0;
    
// -- Data Access --
    
// = images =    
    
    /// number of images.
    virtual std::size_t getNrOfImages() const =0;
    
    /// get a panorama image, counting starts with 0
    virtual const SrcPanoImage& getImage(std::size_t nr) const =0;

    /// set a panorama image, counting starts with 0
    virtual void setImage(std::size_t nr, const SrcPanoImage & img) =0;
    
    /// the the number for a specific image
//    virtual unsigned int getImageNr(const PanoImage * image) const =0;
    
    /** add an Image to the panorama
    *
    *  The Image must be initialized, the Lens must exist.
    *
    */
    virtual unsigned int addImage(const SrcPanoImage& img) =0;
    
    /** creates an image, from filename, and a Lens, if needed */
//    virtual int addImageAndLens(const std::string & filename) =0;
    
    /** add an Image to the panorama
    *  @return image number
    */
//  virtual unsigned int addImage(const std::string & filename) =0;
    
    /** remove an Image.
        *
        *  also deletes/updates all associated control points.
        */
    virtual void removeImage(unsigned int nr) =0;
    
    /** swap images.
        *
        *  swaps the images, image @p img1 becomes @p img2 and the other way round
        */
    virtual void swapImages(unsigned int img1, unsigned int img2) =0;

    /** moves images.
        *
        * moves the image from pos1 to pos2 
        */
    virtual void moveImage(size_t img1, size_t img2) =0;

    /// get a complete description of a source image
    virtual SrcPanoImage getSrcImage(unsigned imgNr) const =0;
    
    /** set input image parameters
     * TODO: Propagate changes to linked images.
    */
    virtual void setSrcImage(unsigned int nr, const SrcPanoImage & img) =0;
    
    /** set a new image filename
    *
    *  It is assumed that it is of the same size
    *  as the old image.
    *
    */
    virtual void setImageFilename(unsigned int img, const std::string & fname) =0;
    
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
    virtual  void activateImage(unsigned int imgNr, bool active=true) =0;
    
    /** get active images */
    virtual  UIntSet getActiveImages() const =0;
    
        /* Link image variable functions. Used to group image variables which
         * should share the same value. The initial value is the one kept by
         * the image with number sourceImgNr.
         */
#define image_variable( name, type, default_value ) \
        virtual void linkImageVariable##name(unsigned int sourceImgNr, unsigned int destImgNr) =0;
#include "image_variables.h"
#undef image_variable

        /* Unlink image variable functions. Makes a image variable independant
         * of the other images.
         */
#define image_variable( name, type, default_value ) \
        virtual void unlinkImageVariable##name(unsigned int imgNr) =0;
#include "image_variables.h"
#undef image_variable    
    
    
// = CPs =    
    
    /// number of control points
    virtual  std::size_t getNrOfCtrlPoints() const =0;
    
    /// get a control point, counting starts with 0
    virtual const ControlPoint & getCtrlPoint(std::size_t nr) const =0;
    
    /// get all control point of this Panorama
    virtual const CPVector & getCtrlPoints() const =0;
    
    /** return all control points for a given image. */
    virtual std::vector<unsigned int> getCtrlPointsForImage(unsigned int imgNr) const =0;
    
    /** set all control points (Ippei: Is this supposed to be 'add' method?) */
    virtual void setCtrlPoints(const CPVector & points) =0;
    
    /** add a new control point.*/
    virtual unsigned int addCtrlPoint(const ControlPoint & point) =0;
    
    /** remove a control point.
    */
    virtual void removeCtrlPoint(unsigned int pNr) =0;
    
    /** removes duplicates control points
        */
    virtual void removeDuplicateCtrlPoints() =0;

    /** change a control Point.
    */
    virtual void changeControlPoint(unsigned int pNr, const ControlPoint & point) =0;
    
    /// get the number of a control point
//  virtual unsigned int getCtrlPointNr(const ControlPoint * point) const =0;
    
    /** get the next unused line number for t3, ... control point creation */
    virtual int getNextCPTypeLineNumber() const =0;
    
    /** assign new mode line numbers, if required */
    virtual void updateLineCtrlPoints() =0;
    
    
    /** update control points distances.
    *
    *  updates control distances and position in final panorama
    *  usually used to set the changes from the optimization.
    *  The control points must be the same as in
    */
    virtual void updateCtrlPointErrors(const CPVector & controlPoints) =0;
    
    /** update control points for a subset of images.
    *
    *  Usually, the control point subset is created using subset()
    *  The number and ordering and control points must not be changed
    *  between the call to subset() and this function.
    */
    virtual void updateCtrlPointErrors(const UIntSet & imgs, const CPVector & cps) =0;
    
// = Variables =    
/*   TODO most of this section needs fixing. The image variables are now stored
 *        in SrcPanoImage objects, the PanoramaData object should just duplicate
 *        the changes across all images sharing variables.
 *        We also need access to the links of the variables.
 *        At some point, this functions should be removed. Do not create new
 *        code using them. Instead, use getSrcImage.
 */
    /// get variables of this panorama
    virtual VariableMapVector getVariables() const =0;

    /** get variables of an image
    *
    *   Depreciated: Please use SrcPanoImage objects for variable access.
    * 
    * @todo remove when not used.
    * 
    * Note: no longer returns a reference as the no variable map with the
    * correct values exists, now we use individual member variables in
    * SrcPanoImage.
    */
    virtual const VariableMap getImageVariables(unsigned int imgNr) const =0;
    
    /** Set the variables.
    *
    *  Usually used when the optimizer results should be applied.
    *
    */
    virtual void updateVariables(const VariableMapVector & vars) =0;
    
    /** update variables for some specific images */
    virtual void updateVariables(const UIntSet & imgs, const VariableMapVector & var) =0;
    
    /** Set variables for a single picture.
    *
    */
    virtual void updateVariables(unsigned int imgNr, const VariableMap & var) =0;
    
    /** update a single variable
    *
    *  It updates other images when the variable is linked
    */
    virtual void updateVariable(unsigned int imgNr, const Variable &var) =0;

    /** update the global white balace of the panorama by multiplying
     * the red and blue factor of each image with given factors 
     */
    virtual void updateWhiteBalance(double redFactor, double blueFactor) =0;
    
// = Optimise Vector =    

    /** return the optimize settings stored inside panorama */
    virtual const OptimizeVector & getOptimizeVector() const =0;

    /** set optimize setting */
    virtual void setOptimizeVector(const OptimizeVector & optvec) =0;


    /** returns optimizer master switch */
    virtual const int getOptimizerSwitch() const =0;
    /** set optimizer master switch */
    virtual void setOptimizerSwitch(const int newSwitch) =0;

    /** return the photometric optimizer master switch */
    virtual const int getPhotometricOptimizerSwitch() const =0;
    /** sets the photometric optimizer master switch */
    virtual void setPhotometricOptimizerSwitch(const int newSwitch) =0;

// = Panorama options =    

    /** returns the options for this panorama */
    virtual const PanoramaOptions & getOptions() const =0;

    /** set new output settings
     *  This is not used directly for optimizing/stiching, but it can
     *  be feed into runOptimizer() and runStitcher().
     */
    virtual void setOptions(const PanoramaOptions & opt) =0;
    

    
// -- script interface --
        
   /** read after optimization, fills in control point errors.
    *
    *  @param set of image numbers that where used during by
    *         printPanoramaScript().
    *  @param vars will be set the the optimzied variables
    *  @param ctrlPoints will contain the controlpoints, with distance
    *         information
    *
    *  @return false on error (could not read optimizer output, parse error)
    */
    virtual void parseOptimizerScript(std::istream & i,
                              const UIntSet & imgs,
                              VariableMapVector & imgVars,
                              CPVector & ctrlPoints) const =0;
    
    /// create an optimizer script
    virtual void printPanoramaScript(std::ostream & o,
                             const OptimizeVector & optvars,
                             const PanoramaOptions & options,
                             const UIntSet & imgs,
                             bool forPTOptimizer,
                             const std::string & stripPrefix="") const =0;
    
    /// create the stitcher script
    virtual void printStitcherScript(std::ostream & o,
                             const PanoramaOptions & target,
                             const UIntSet & imgs) const =0;
    
    
// -- maintainance --
    
public:
    /// tells the data container to perform some maintainance if neccesary
    virtual void changeFinished() =0;
    
    /// mark image change for maintainance 
    virtual void imageChanged(unsigned int imgNr) =0;
    /** set complete mask list for image with number */
    virtual void updateMasksForImage(unsigned int imgNr, MaskPolygonVector newMasks)=0;
    /** updates all active masks
      *
      * this is necessary after variables of *one* image has changed, 
      * because positive masks have to be updated 
      */
    virtual void updateMasks(bool convertPosMaskToNeg=false)=0;
    /** transfers given mask from image imgNr to all targetImgs
        */
    virtual void transferMask(MaskPolygon mask,unsigned int imgNr, const UIntSet& targetImgs)=0;
    /** updates the optimize vector according to master switches */
    virtual void updateOptimizeVector()=0;
    /** returns set of reference image and images linked with reference images */
    virtual std::set<size_t> getRefImages()=0;
    /** checks if yaw/pitch/roll of reference image can be check, 
        * it depends on number and type of control points */
    virtual void checkRefOptStatus(bool& linkRefImgsYaw, bool& linkRefImgsPitch, bool& linkRefImgsRoll)=0;

};



/** this handler class will receive change events from the Panorama.
*
*  Maybe a fine grained event interface is better, but it can be
*  added later.
*/
class PanoramaObserver
{
    public:
        
        ///
        virtual ~PanoramaObserver() {};
        
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
        virtual void panoramaChanged(Panorama &pano) =0;
        
        /** notifies about changes to images
        *
        *  Images might have been added/removed. to find out
        *  how many images are still there, use Panorama::getNrOfImages.
        *
        *  @param pano the panorama object that changed
        *  @param changed set of changed images
        *
        */
        virtual void panoramaImagesChanged(Panorama& pano,
                                           const UIntSet& changed) =0;
        
        
};


/** Memento class for a PanoramaData object
*
*  Preserves the internal state of a PanoramaData.
*  Used when other objects need to get/set the state without
*  knowing anything about the internals.
*
*/
class IMPEX PanoramaDataMemento
{
    protected: 
        /// force pure abstract behaviour
        PanoramaDataMemento() {};
    
    public:
        ///
        virtual ~PanoramaDataMemento() {};
};


///
class IMPEX ManagedPanoramaData : public PanoramaData
{
    public:
        
        ///
        virtual ~ManagedPanoramaData() {};
        
        
        // -- Observing --
        
    public:
        /** add a panorama observer.
        *
        *  It will recieve all change messages.
        *  An observer can only be added once. if its added twice,
        *  the second addObserver() will have no effect.
        */
        virtual void addObserver(PanoramaObserver *o) =0;
        
        /** remove a panorama observer.
        *
        *  Observers must be removed before they are destroyed,
        *  else Panorama will try to notify them after they have been
        *  destroyed
        *
        *  @return true if observer was known, false otherwise.
        */
        virtual bool removeObserver(PanoramaObserver *observer) =0;
        
        /** remove all panorama observers.
        *
        *  @warning this is a hack. it must not be used on normal Panorama's.
        */
        virtual void clearObservers() =0;
        
        /** notify observers about changes in this class
        *
        *  This needs to be called explicitly by somebody after
        *  changes have been made.
        *  Allows to compress multiple changes into one notification.
        */
        virtual void changeFinished() =0;
        
        /** clear dirty flag. call after load, save or new project */
        virtual void clearDirty() =0;

        /** mark image for change notification.
        *
        *  Does not send the notification, this is left
        *  to changedFinished()
        */
        virtual void imageChanged(unsigned int imgNr) =0;
        
        
        // -- Memento interface --
        
    public:
        /// get the internal state
        virtual PanoramaDataMemento* getNewMemento() const =0;
        
        /// set the internal state
        virtual bool setMementoToCopyOf(const PanoramaDataMemento* const memento) =0;
        
        
        // -- Optimization Status --
        
    public:
        /** true if control points or lens variables
        *  have been changed after the last optimisation
        */
        virtual bool needsOptimization() =0;
        
        ///
        virtual void markAsOptimized(bool optimized=true) =0;
        
};


} // namespace




#endif // _PANORAMA_H
