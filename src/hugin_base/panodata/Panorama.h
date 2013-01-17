// -*- c-basic-offset: 4 -*-
/** @file panodata/Panorama.h
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

#ifndef _PANODATA_PANORAMA_H
#define _PANODATA_PANORAMA_H

// if this file is preprocessed for SWIG, we want to ignore
// all the header inclusions that follow:

#ifndef _HSI_IGNORE_SECTION

#include <hugin_shared.h>
#include <list>
#include <appbase/DocumentData.h>
#include <panodata/PanoramaData.h>


#endif // _HSI_IGNORE_SECTION

namespace HuginBase {
    
    
    
    
/** Memento class for a Panorama object
*
*  Holds the internal state of a Panorama.
*  Used when other objects need to get/set the state without
*  knowing anything about the internals.
*
*/
class IMPEX PanoramaMemento : public PanoramaDataMemento
{
        
        friend class Panorama;
        
    public:
        PanoramaMemento()
            : PanoramaDataMemento(), needsOptimization(false), optSwitch(0), optPhotoSwitch(0)
        {};
        
        /// copy ctor.
        PanoramaMemento(const PanoramaMemento & o);
        
        /// assignment operator
        PanoramaMemento& operator=(const PanoramaMemento & o);
        
        virtual ~PanoramaMemento();
        
        
    protected:
        /** enum for supported PTScript syntax bastards */
        //  enum PTFileFormat { PTFILE_HUGIN, PTFILE_PTGUI, PTFILE_PTA };
        
        /** load a PTScript file
        *
        *  initializes the PanoramaMemento from a PTScript file
        */
        bool loadPTScript(std::istream & i, int & ptoVersion, const std::string & prefix = "");
    
        
    private:
        enum PTParseState {
            P_NONE,
            P_OUTPUT,
            P_MODIFIER,
            P_IMAGE,
            P_OPTIMIZE,
            P_CP
        };
        
        /** The images inside the panorama.
         * 
         * The image variables are stored inside. We use pointers to the real
         * objects so that the memory addresses of them remain constant when we
         * remove and swap the order of images. We should create and free images
         * when necessary.
         */
        std::vector<SrcPanoImage *> images;
        
        CPVector ctrlPoints;
        
        PanoramaOptions options;
        
        OptimizeVector optvec;
        /** stores the optimizer switch, use OR of HuginBase::OptimizerSwitches */
        int optSwitch;
        /** stores the photometric optimizer switch, use OR of HuginBase::OptimizerSwitches */
        int optPhotoSwitch;

        // indicates that changes have been made to
        // control points or lens parameters after the
        // last optimisation
        bool needsOptimization;
        
        void deleteAllImages();
};


    
/** Model for a panorama.
 *
 *  This class contains the properties of a panorama
 *  That is:
 *       - images
 *       - variables that can be optimized including links between them
 *       - control points
 *       - properites of the output panorama.
 *
 *  view and controller classes can get information about these
 *  with the getXXX Functions.
 *
 *  Images and Control points are numbered, and const references are
 *  handed out.  this means that all interaction will be based on
 *  image/control point numbers. The references are not stable,
 *  they might disappear when other functions of this class are
 *  called, so its best to get a new reference whenever you need the object.
 *
 *  This also means that the whole object is not threadsafe and concurrent
 *  access has to be synchronized from the outside.
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
class IMPEX Panorama : public ManagedPanoramaData, public AppBase::DocumentData
{
    
    public:

        /** ctor.
         */
        Panorama();

        /** dtor.
         */
        ~Panorama();
        
        
    // ====================== PanoramaData =========================================
        
        
        /** get a subset of the panorama
        *
        *  This returns a panorama that contains only the images specified by \imgs
        *  Useful for operations on a subset of the panorama
        */
        Panorama getSubset(const UIntSet & imgs) const;
        
        /** duplicate the panorama
            *
            *  returns a copy of the pano state, except for the listeners.
            */
        Panorama duplicate() const;
         
         ///
        PanoramaData* getNewSubset(const UIntSet& imgs) const
         {
             return new Panorama(this->getSubset(imgs));
         }
         
         ///
        PanoramaData* getNewCopy() const
         {
             return new Panorama(this->duplicate());
         }
        
        
    // -- Data Access --
        
    // = images =    
        
        /// number of images.
        std::size_t getNrOfImages() const
        {
            return state.images.size();
        };
        
        /// get a panorama image, counting starts with 0
        const SrcPanoImage & getImage(std::size_t nr) const
        {
            assert(nr < state.images.size());
            return *state.images[nr];
        };

        /// set a panorama image, counting starts with 0
        void setImage(std::size_t nr, const SrcPanoImage & img)
        {
            setSrcImage(nr, img);
        };
        
        /// the the number for a specific image
    //    unsigned int getImageNr(const PanoImage * image) const;
        
        /** add an Image to the panorama
         */
        unsigned int addImage(const SrcPanoImage &img);

        /** merges the panorama with the given pano */
        void mergePanorama(const Panorama &newPano);
        
        /** creates an image, from filename, and a Lens, if needed */
//        int addImageAndLens(const std::string & filename);
        
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
        
        /** swap images.
            *
            *  swaps the images, image @p img1 becomes @p img2 and the other way round
            */
        void swapImages(unsigned int img1, unsigned int img2);

        /** moves images.
            *
            * moves the image from pos1 to pos2 
            */
        void moveImage(size_t img1, size_t img2);
        
        /** get a description of a source image
         * 
         * Notice the SrcPanoImage is a copy. This removes all references to the
         * other images, which means you should use getImage instead if you
         * would like to find out about the variable links.
         * 
         * @warning Variable links cannot be accessed this way.
         */
        SrcPanoImage getSrcImage(unsigned imgNr) const;
        
        /** set input image parameters
         * 
         * This sets the values of the image variables, but does not change the
         * links.
         * @warning Variable links cannot be set this way.
         */
        void setSrcImage(unsigned int nr, const SrcPanoImage & img);
        
        /** set a new image filename
            *
            *  It is assumed that it is of the same size
            *  as the old image.
            *
            */
        void setImageFilename(unsigned int img, const std::string & fname);
        
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
        
        
    // = CPs =    
        
        /// number of control points
         std::size_t getNrOfCtrlPoints() const
        {
            return state.ctrlPoints.size();
        };
        
        /// get a control point, counting starts with 0
        const ControlPoint & getCtrlPoint(std::size_t nr) const
        {
            assert(nr < state.ctrlPoints.size());
            return state.ctrlPoints[nr];
        };
        
        /// get all control point of this Panorama
        const CPVector & getCtrlPoints() const
            { return state.ctrlPoints; };
        
        /** return all control points for a given image. */
        std::vector<unsigned int> getCtrlPointsForImage(unsigned int imgNr) const;
        
        /** return a vector of std::pairs with global ctrl point nr and ControlPoint 
         *  In the class ControlPoint the image with imgNr is always image1 */
        CPointVector getCtrlPointsVectorForImage(unsigned int imgNr) const;

        /** set all control points (Ippei: Is this supposed to be 'add' method?) */
        void setCtrlPoints(const CPVector & points);
        
        /** add a new control point.*/
        unsigned int addCtrlPoint(const ControlPoint & point);
        
        /** remove a control point.
            */
        void removeCtrlPoint(unsigned int pNr);

        /** removes duplicates control points
            */
        void removeDuplicateCtrlPoints();
        
        /** change a control Point.
            */
        void changeControlPoint(unsigned int pNr, const ControlPoint & point);
        
        /// get the number of a control point
        //    unsigned int getCtrlPointNr(const ControlPoint * point) const;
        
        /** get the next unused line number for t3, ... control point creation */
        int getNextCPTypeLineNumber() const;
        
        /** assign new mode line numbers, if required */
        void updateLineCtrlPoints();
        
        
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
        
        
    // = Variables =    
        
        /// get variables of this panorama
        VariableMapVector getVariables() const;

        /** Get the variables of an image
         * 
         * Should not be used for most GUI stuff, use the getImage(imgNr).get*
         * methods instead for each variable you want.
         * 
         * @todo change things using this to use getImage(imgNr).get*() instead.
         */
        const VariableMap getImageVariables(unsigned int imgNr) const;
        
        /** Set the variables.
            *
            *  Usually used when the optimizer results should be applied.
            *
            */
        virtual void updateVariables(const VariableMapVector & vars);
        
        /** update variables for some specific images */
        virtual void updateVariables(const UIntSet & imgs, const VariableMapVector & var);
        
        /** Set variables for a single picture.
            *
            */
        virtual void updateVariables(unsigned int imgNr, const VariableMap & var);
        
        /** update a single variable
            *
            *  It knows lenses etc and updates other images when the
            *  variable is linked
            */
        virtual void updateVariable(unsigned int imgNr, const Variable &var);
        
        /** updates the focal length by changing hfov */
        virtual void UpdateFocalLength(UIntSet imgs, double newFocalLength);
        /** updates the crop factor, try to keep focal length constant */
        virtual void UpdateCropFactor(UIntSet imgs, double newCropFactor);
        /* Link image variable functions. Used to group image variables which
         * should share the same value. The initial value is the one kept by
         * the image with number sourceImgNr.
         */
#define image_variable( name, type, default_value ) \
        virtual void linkImageVariable##name(unsigned int sourceImgNr, unsigned int destImgNr);
#include "image_variables.h"
#undef image_variable

        /* Unlink image variable functions. Makes a image variable independant
         * of the other images.
         */
#define image_variable( name, type, default_value ) \
        virtual void unlinkImageVariable##name(unsigned int imgNr);
#include "image_variables.h"
#undef image_variable
        
        /** update the global white balace of the panorama by multiplying
         * the red and blue factor of each image with given factors 
        */
        virtual void updateWhiteBalance(double redFactor, double blueFactor);

    // = Optimise Vector =    
        /** return the optimize settings stored inside panorama */
        const OptimizeVector & getOptimizeVector() const
            { return state.optvec; };

        /** set optimize setting */
        void setOptimizeVector(const OptimizeVector & optvec);
        
        /** returns optimizer master switch */
        const int getOptimizerSwitch() const
            { return state.optSwitch;};
        /** set optimizer master switch */
        void setOptimizerSwitch(const int newSwitch);

        /** return the photometric optimizer master switch */
        const int getPhotometricOptimizerSwitch() const
            { return state.optPhotoSwitch; };
        /** sets the photometric optimizer master switch */
        void setPhotometricOptimizerSwitch(const int newSwitch);

        /** @note Is this the most appropriate way to remember which variables
         * need optimisation? Can we have optimisation information in
         * ImageVariables instead now?
         */

        
    // = Panorama options =    

        /** returns the options for this panorama */
        const PanoramaOptions & getOptions() const
            { return state.options; };

        /** set new output settings
         *  This is not used directly for optimizing/stiching, but it can
         *  be feed into runOptimizer() and runStitcher().
         */
        void setOptions(const PanoramaOptions & opt);
        

        
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
        void parseOptimizerScript(std::istream & i,
                                  const UIntSet & imgs,
                                  VariableMapVector & imgVars,
                                  CPVector & ctrlPoints) const;
            
        /// create an optimizer script
        void printPanoramaScript(std::ostream & o,
                                 const OptimizeVector & optvars,
                                 const PanoramaOptions & options,
                                 const UIntSet & imgs,
                                 bool forPTOptimizer,
                                 const std::string & stripPrefix="") const;
        
        /// create the stitcher script
        void printStitcherScript(std::ostream & o,
                                 const PanoramaOptions & target,
                                 const UIntSet & imgs) const;

        
        
    //=========== ManagedPanoramaData ==============================================



    // -- Observing --
        
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
        
        /** notify observers about changes in this class
            *
            *  This needs to be called explicitly by somebody after
            *  changes have been made.
            *  Allows to compress multiple changes into one notification.
            *
            *  @param keepDirty  do not set dirty flag. useful for changing
            *                    the dirty flag itself
            */
        void changeFinished(bool keepDirty);
        
        /** notify observers about changes in this class
            *
            *  This needs to be called explicitly by somebody after
            *  changes have been made.
            *  Allows to compress multiple changes into one notification.
            *
            */
        void changeFinished()
            { changeFinished(false); }
        
        /** mark image for change notification.
         *
         *  Does not send the notification, this is left
         *  to changedFinished()
         */
        void imageChanged(unsigned int imgNr);

        /** set complete mask list for image with number */
        void updateMasksForImage(unsigned int imgNr, MaskPolygonVector newMasks);
        /** updates all active masks
         *
         * this is necessary after variables of *one* image has changed, 
         * because positive masks have to be updated 
         */
        void updateMasks(bool convertPosMaskToNeg=false);
        /** transfers given mask from image imgNr to all targetImgs
         */
        void transferMask(MaskPolygon mask,unsigned int imgNr, const UIntSet targetImgs);
        /** updates the optimize vector according to master switches */
        void updateOptimizeVector();
        /** returns set of reference image and images linked with reference images */
        std::set<size_t> getRefImages();
        /** checks if yaw/pitch/roll of reference image can be check, 
          * it depends on number and type of control points */
        void checkRefOptStatus(bool& linkRefImgsYaw, bool& linkRefImgsPitch, bool& linkRefImgsRoll);

        // -- Memento interface --
        
        /// get the internal state
        virtual PanoramaDataMemento* getNewMemento() const;
        
        /// set the internal state
        virtual bool setMementoToCopyOf(const PanoramaDataMemento* const memento);
        
        /// get the internal state
        PanoramaMemento getMemento() const
            { return state; }
        
        /// set the internal state
        void setMemento(const PanoramaMemento& memento);
        
        
    // -- Optimization Status --
        
        /** true if control points or lens variables
        *  have been changed after the last optimisation
        */
        bool needsOptimization()
            { return state.needsOptimization; };

        ///
        void markAsOptimized(bool optimized=true)
            { state.needsOptimization = !optimized; };
        


    //=========== Document Data ====================================================

    public:
        /** Reads data. You have to check with refered images after data is
         *  loaded as the file path is likely to be relative, and the image
         *  property might have been changed since the project is saved.
         */
        ReadWriteError readData(std::istream& dataInput, std::string documentType = "");
        
        ///
        ReadWriteError writeData(std::ostream& dataOutput, std::string documentType = "");

        /** true if there are unsaved changes */
        bool isDirty() const
        {
            if (dirty != AppBase::DocumentData::isDirty())
                DEBUG_WARN("modification status mismatch.");
                
            return dirty;
        }
        
        /** clear dirty flag. call after save */
        virtual void clearDirty()
        {
            AppBase::DocumentData::clearDirty();
            dirty = false;
        }
        
    protected:
        void setDirty(const bool& dirty = true)
        { 
            AppBase::DocumentData::setDirty(dirty);
            
            this->dirty = dirty;
        }
        
        
    // == additional methods for documents ==
        
    public:
        /// sets the path prefix of the images reffered with relative paths
        void setFilePrefix(std::string prefix)
            { imgFilePrefix = prefix; }
        
    protected:
        std::string getFilePrefix() const
            { return imgFilePrefix; }
        
    //=========== Internal methods =================================================
        
    public:
        /** clear the internal state. (public use deprecated) */
        void reset();
        
    protected:
        /// adjust the links of the linked variables, must be called
        /// when a lens has been changed.
        void adjustVarLinks();


        /// image addition notification
    //    void notifyImageAdded(unsigned int imgNr);

        /// image removal notification
    //    void notifyImageRemoved(unsigned int imgNr);

        /// image change notification
    //    void notifyImageChanged(unsigned int imgNr);

        

    private:

        // data
//        enum ProcessType { NO_PROCESS, OPTIMIZER, STITCHER };

        // to run stitcher & optimizer
//        ProcessType currentProcess;
//        std::string optimizerExe;
//        std::string stitcherExe;
//        std::string PTScriptFile;
        
        //
        /** center the crop for given image and all linked images */
        void centerCrop(unsigned int imgNr);
        /** return the centered crop for given image */
        vigra::Rect2D centerCropImage(unsigned int imgNr);
        /** update the crop mode in dependence of crop rect and lens projection */
        void updateCropMode(unsigned int imgNr);

        std::string imgFilePrefix;

        /// this indicates that there are unsaved changes
        bool dirty;

        PanoramaMemento state;
        std::list<PanoramaObserver *> observers;
        /// the images that have been changed since the last changeFinished()
        UIntSet changedImages;

        bool m_forceImagesUpdate;

        std::set<std::string> m_ptoptimizerVarNames;
};




} // namespace




#endif // _PANORAMA_H
