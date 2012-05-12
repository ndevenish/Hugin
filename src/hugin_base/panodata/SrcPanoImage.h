// -*- c-basic-offset: 4 -*-

/** @file SrcPanoImage.h
 *
 *  @brief 
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *          James Legg
 *
 * !! from PanoImage.h 1970
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _PANODATA_SRCPANOIMAGE_H
#define _PANODATA_SRCPANOIMAGE_H

// if this file is preprocessed for SWIG, we want to ignore
// all the header inclusions that follow:

#ifndef _HSI_IGNORE_SECTION

#include <hugin_shared.h>
#include <hugin_config.h>
#include <iostream>
#include <vector>
#include <vigra/diff2d.hxx>

#include <hugin_utils/utils.h>
#include <hugin_math/hugin_math.h>
#include "PanoramaVariable.h"
#include "ImageVariable.h"
#include "Mask.h"

#ifdef HUGIN_USE_EXIV2
#include <exiv2/exif.hpp>
#endif

#endif // _HSI_IGNORE_SECTION

namespace HuginBase {

class Panorama;

/** Base class containing all the variables, but missing some of the other
 * important functions and with some daft accessors.
 *
 * Used for lazy metaprogramming, we include image_variables.h several times
 * with different defintions of image_variable to get all the repetitive bits
 * out the way. This should reduce typos and cut and paste errors.
 */
class IMPEX BaseSrcPanoImage
{
public:
    ///
    enum Projection {
        RECTILINEAR = 0,
        PANORAMIC = 1,
        CIRCULAR_FISHEYE = 2,
        FULL_FRAME_FISHEYE = 3,
        EQUIRECTANGULAR = 4,
        FISHEYE_ORTHOGRAPHIC = 8,
        FISHEYE_STEREOGRAPHIC = 10,
        FISHEYE_EQUISOLID = 21,
        FISHEYE_THOBY = 20
    };
    
    ///
    enum CropMode {
        NO_CROP=0,
        CROP_RECTANGLE=1,
        CROP_CIRCLE=2
    };

    /// vignetting correction mode (bitflags, no real enum)
    enum VignettingCorrMode { 
        VIGCORR_NONE = 0,      ///< no vignetting correction
        VIGCORR_RADIAL = 1,    ///< radial vignetting correction
        VIGCORR_FLATFIELD = 2, ///< flatfield correction
        VIGCORR_DIV = 4        ///< correct by division.
    };

    ///
    enum ResponseType {
        RESPONSE_EMOR=0,                 ///< empirical model of response
        RESPONSE_LINEAR,                 ///< linear response
        RESPONSE_GAMMA,                  ///< a simple gamma response curve
        RESPONSE_FILE,                   ///< load response curve from file (not implemented yet)
        RESPONSE_ICC                     ///< use ICC for transformation into linear data (not implemented yet)
    };

    /// Check that the variables match.
    bool operator==(const BaseSrcPanoImage & other) const;    
    
    
public:
    ///
    BaseSrcPanoImage()
    {
        setDefaults();
    }
    
    virtual ~BaseSrcPanoImage() {};
    
    // property accessors
public:
    // get[variable name] functions. Return the value stored in the ImageVariable.
#define image_variable( name, type, default_value ) \
    type get##name() const { return m_##name.getData(); }
#include "image_variables.h"
#undef image_variable

    // get[variable name]IV functions. Return a const reference to the ImageVariable.
#define image_variable( name, type, default_value ) \
    const ImageVariable<type > & get##name##IV() const { return m_##name; }
#include "image_variables.h"
#undef image_variable

    // set[variable name] functions
#define image_variable( name, type, default_value ) \
    void set##name(type data) { m_##name.setData(data); }
#include "image_variables.h"
#undef image_variable

    /* The link[variable name] functions
     * Pass a pointer to another SrcPanoImg and the respective image variable
     * will be shared between the images. Afterwards, changing the variable with
     * set[variable name] on either image also sets the other image.
     */
#define image_variable( name, type, default_value ) \
    void link##name (BaseSrcPanoImage * target) \
    { m_##name.linkWith(&(target->m_##name)); }
#include "image_variables.h"
#undef image_variable

    /* The unlink[variable name] functions
     * Unlinking a variable makes it unique to this image. Then changing it will
     * not affect the other images.
     */
#define image_variable( name, type, default_value ) \
    void unlink##name () \
    { m_##name.removeLinks(); }
#include "image_variables.h"
#undef image_variable

    /* The [variable name]isLinked functions
     * Returns true if the variable has links, or false if it is independant.
     */
#define image_variable( name, type, default_value ) \
    bool name##isLinked () const \
    { return m_##name.isLinked(); }
#include "image_variables.h"
#undef image_variable

    /* The [variable name]isLinkedWith functions
     * Returns true if the variable is linked with the equivalent variable in
     * the specified image, false otherwise.
     */
#define image_variable( name, type, default_value ) \
    bool name##isLinkedWith (const BaseSrcPanoImage & image) const \
    { return m_##name.isLinkedWith(&(image.m_##name)); }
#include "image_variables.h"
#undef image_variable

protected:
    ///
    void setDefaults();

    // the image variables m_[variable name]
#define image_variable( name, type, default_value ) \
    ImageVariable<type > m_##name;
#include "image_variables.h"
#undef image_variable
};


/** All variables of a source image.
 *
 *  In the long term, this simplified class will replace
 *  PanoImage and Image options and the variables array.
 *  All image variables are stored in this class, regardless of what the
 *  variable is attached to (lens, sensor, position).
 */
class IMPEX SrcPanoImage : public BaseSrcPanoImage
{
public:
    ///
    SrcPanoImage()
    {
        setDefaults();
        successfullEXIFread=false;
    }
    
    virtual ~SrcPanoImage() {};
public:
    /** initialize a SrcPanoImage from a file. Will read image
     *  size and EXIF data to initialize as many fields as possible
     *  (most importatly HFOV and exposure value)
     */
    SrcPanoImage(const std::string &filename)
    {
        setDefaults();
        m_Filename = filename;
        double crop = 0;
        double fl = 0;
        successfullEXIFread=readEXIF(fl, crop, true, true);
    };
    /** return true, if EXIF infomation was read sucessful */
    const bool hasEXIFread() const {return successfullEXIFread;};
    
    
public:
    /** "resize" image,
     *  adjusts all distortion coefficients for usage with a source image
     *  of size @p size
     */
    void resize(const vigra::Size2D & size);

    /** check if a coordinate is inside the source image
     */
    bool isInside(vigra::Point2D p, bool ignoreMasks=false) const;

    ///
    bool horizontalWarpNeeded();

    // Accessors
    // These are either:
    // #- extra ones that are derived from image varibles, or
    // #- replacements where we need extra processing.
public:
    bool getCorrectTCA() const;
    
    /** Set the crop mode.
     * 
     * This sets the cropping region to the entire image when set to NO_CROP,
     * unlike the lazy metaprogrammed equivalent in BaseSrcPanoImage.
     */
    void setCropMode(CropMode val);

    /** returns true, if projection requires cicular crop */
    bool isCircularCrop() const;

    /** Set the image size in pixels
     * 
     * If we aren't cropping the image, set the size to the entire image 
     */
    void setSize(vigra::Size2D val);
    
    hugin_utils::FDiff2D getRadialDistortionCenter() const;
    
    hugin_utils::FDiff2D getRadialVigCorrCenter() const;
    
    // these are linked to ExposureValue, which is done with the above.
    // exposure value  is log2 of inverse exposure factor.
    double getExposure() const;
    void setExposure(const double & val);
    
    
    /** Get the width of the image in pixels.
     * 
     * Should not be used, use getSize().width() instead.
     * This is here for compatiblity with PnaoImage, but should be removed.
     * 
     * @todo replace all calls to getWidth() with getSize().width().
     */
    int getWidth() const
    { return getSize().width(); }
    
    /** Get the height of the image in pixels.
     * 
     * Should not be used, use getSize().height() instead.
     * This is here for compatiblity with PnaoImage, but should be removed.
     * 
     * @todo replace all calls to getHeight() with getSize().height().
     */
    int getHeight() const
    { return getSize().height(); }
        
    double getVar(const std::string & name) const;
    
    void setVar(const std::string & name, double val);
    
    /** Return all the image variables in a variable map
     * 
     * Returns a map of all the variables for this image. It is adivisable to
     * use the individual getX functions where apropriate instead.
     * 
     * @todo remove this infavour of the individual get*() functions. This
     * creates a map of all the variables, regardless of which ones are actually
     * needed, every time it is called.
     */
    VariableMap getVariableMap() const;
    
    /** try to convert Exif date time string to struct tm 
     *  @return 0, if conversion was sucessfull */
    const int getExifDateTime(struct tm* datetime) const;

    /** unlinking vignetting parameters should unlink the vignetting correction mode
     */
    void unlinkRadialVigCorrCoeff ()
    {
        m_RadialVigCorrCoeff.removeLinks();
        m_VigCorrMode.removeLinks();
    }
    
    /** unlinking vignetting parameters should unlink the vignetting correction mode
     */
    void unlinkRadialVigCorrCenterShift ()
    {
        m_RadialVigCorrCenterShift.removeLinks();
        m_VigCorrMode.removeLinks();
    }
    
    /** unlinking the EMOR parameters should unlink the correction mode.
     */
    void unlinkEMoRParams ()
    {
        m_EMoRParams.removeLinks();
        m_ResponseType.removeLinks();
    }
    
    /** linking vignetting parameters should link the vignetting correction mode
     */
    void linkRadialVigCorrCoeff (SrcPanoImage * target)
    {
        m_RadialVigCorrCoeff.linkWith(&(target->m_RadialVigCorrCoeff));
        m_VigCorrMode.linkWith(&(target->m_VigCorrMode));
    }

    /** linking vignetting parameters should link the vignetting correction mode
     */
    void linkRadialVigCorrCenterShift (SrcPanoImage * target)
    {
        m_RadialVigCorrCenterShift.linkWith(&(target->m_RadialVigCorrCenterShift));
        m_VigCorrMode.linkWith(&(target->m_VigCorrMode));
    }
    
    /** linking the EMOR parameters should link the correction mode.
     */
    void linkEMoRParams (SrcPanoImage * target)
    {
        m_EMoRParams.linkWith(&(target->m_EMoRParams));
        m_ResponseType.linkWith(&(target->m_ResponseType));
    }
    
    void linkStack (SrcPanoImage * target)
    { m_Stack.linkWith(&(target->m_Stack)); }
    
    /** try to fill out information about the image, by examining the exif data
    *  focalLength and cropFactor will be updated with the ones read from the exif data
    *  If no or not enought exif data was found and valid given focalLength and cropFactor
    *  settings where provided, they will be used for computation of the HFOV.
    */
    bool readEXIF(double & focalLength, double & cropFactor, bool applyEXIF, bool applyExposureValue);
    bool readEXIF(double & focalLength, double & cropFactor, double & eV, bool applyEXIF, bool applyExposureValue);
    
    /** calculate hfov of an image given focal length, image size and crop factor */
    static double calcHFOV(SrcPanoImage::Projection proj, double fl, double crop, vigra::Size2D imageSize);
    
    /** calcualte focal length, given crop factor and hfov */
    static double calcFocalLength(SrcPanoImage::Projection proj, double hfov, double crop, vigra::Size2D imageSize);

    /** calculate crop factor, given focal length and hfov */
    static double calcCropFactor(SrcPanoImage::Projection proj, double hfov, double focalLength, vigra::Size2D imageSize);

    /** updates the focal length, changes the hfov to reflect thew newFocalLength */
    void updateFocalLength(double newFocalLength);
    /** updates the crop factor, the hfov is calculates so that focal length remains the same */
    void updateCropFactor(double focalLength, double newCropFactor);

    /** tries to read cropfactor from lensfun database
        you need to call SrcPanoImage::readEXIF before to fill some values 
        @return true, if information could be read from database */
    bool readCropfactorFromDB();
    /** tries to read projection and crop area from lensfun database
        you need to call SrcPanoImage::readEXIF before to fill some values 
        @return true, if information could be read from database */
    bool readProjectionFromDB();
    /** tries to read distortion data from lensfun database
        you need to call SrcPanoImage::readEXIF before to fill some values 
        @return true, if information could be read from database */
    bool readDistortionFromDB();
    /** tries to read vignetting data from lensfun database
        you need to call SrcPanoImage::readEXIF before to fill some values 
        @return true, if information could be read from database */
    bool readVignettingFromDB();

    /** returns true, if image has masks associated */
    bool hasMasks() const;
    /** returns true, if image has positive masks */
    bool hasPositiveMasks() const;
    /** returns true, if image has active masks */
    bool hasActiveMasks() const;
    /** add newMask to list of masks */
    void addMask(MaskPolygon newMask);
    /** add newMask to list of active masks */
    void addActiveMask(MaskPolygon newMask);
    /** clears list of active masks */
    void clearActiveMasks();
    /** changes type of mask with index to given newType */
    void changeMaskType(unsigned int index, HuginBase::MaskPolygon::MaskType newType);
    /** delete mask at index */
    void deleteMask(unsigned int index);
    /** delete all masks */
    void deleteAllMasks();
    /** writes all mask lines to stream, using given image number */
    void printMaskLines(std::ostream &o, unsigned int newImgNr) const;
    /** returns true, if point p is inside of one mask polygon */
    bool isInsideMasks(vigra::Point2D p) const;

private:

    /** convenience functions to work with Exiv2 */
    bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, long & value);
    bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, float & value);
    bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::string & value);
    bool getExiv2Value(Exiv2::ExifData& exifData, uint16_t tagID, std::string groupName, std::string & value);

    /** Check if Exiv orientation tag can be trusted */
    bool trustExivOrientation();
    bool successfullEXIFread;
};

typedef std::vector<SrcPanoImage> ImageVector;

} // namespace

#endif // PANOIMAGE_H
