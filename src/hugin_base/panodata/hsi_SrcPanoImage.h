// -*- c-basic-offset: 4 -*-

/** @file PanoImage.h
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

#include <hugin_shared.h>
#include <hugin_config.h>
#include <iostream>
#include <vector>
#include <vigra/diff2d.hxx>

#include <hugin_utils/utils.h>
#include <hugin_math/hugin_math.h>
#include "PanoramaVariable.h"
#include "PanoImage.h"
#include "ImageVariable.h"
#include "Mask.h"

#ifdef HUGIN_USE_EXIV2
#include <exiv2/exif.hpp>
#endif

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
    
/*********** Start of precompiled section ******************/


# 1 "hsi_SrcPanoImage_accessor_macros.h"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "hsi_SrcPanoImage_accessor_macros.h"
# 13 "hsi_SrcPanoImage_accessor_macros.h"
public:



# 1 "image_variables.h" 1
# 64 "image_variables.h"
std::string getFilename() const { return m_Filename.getData(); }
vigra::Size2D getSize() const { return m_Size.getData(); }

HuginBase::BaseSrcPanoImage::Projection getProjection() const { return m_Projection.getData(); }
double getHFOV() const { return m_HFOV.getData(); }


HuginBase::BaseSrcPanoImage::ResponseType getResponseType() const { return m_ResponseType.getData(); }
std::vector<float> getEMoRParams() const { return m_EMoRParams.getData(); }
double getExposureValue() const { return m_ExposureValue.getData(); }
double getGamma() const { return m_Gamma.getData(); }
double getWhiteBalanceRed() const { return m_WhiteBalanceRed.getData(); }
double getWhiteBalanceBlue() const { return m_WhiteBalanceBlue.getData(); }


double getRoll() const { return m_Roll.getData(); }
double getPitch() const { return m_Pitch.getData(); }
double getYaw() const { return m_Yaw.getData(); }

double getX() const { return m_X.getData(); }
double getY() const { return m_Y.getData(); }
double getZ() const { return m_Z.getData(); }



double getStack() const { return m_Stack.getData(); }


std::vector<double> getRadialDistortion() const { return m_RadialDistortion.getData(); }


std::vector<double> getRadialDistortionRed() const { return m_RadialDistortionRed.getData(); }
std::vector<double> getRadialDistortionBlue() const { return m_RadialDistortionBlue.getData(); }


hugin_utils::FDiff2D getRadialDistortionCenterShift() const { return m_RadialDistortionCenterShift.getData(); }


hugin_utils::FDiff2D getShear() const { return m_Shear.getData(); }


HuginBase::BaseSrcPanoImage::CropMode getCropMode() const { return m_CropMode.getData(); }
vigra::Rect2D getCropRect() const { return m_CropRect.getData(); }
bool getAutoCenterCrop() const { return m_AutoCenterCrop.getData(); }


int getVigCorrMode() const { return m_VigCorrMode.getData(); }


std::string getFlatfieldFilename() const { return m_FlatfieldFilename.getData(); }
std::vector<double> getRadialVigCorrCoeff() const { return m_RadialVigCorrCoeff.getData(); }
hugin_utils::FDiff2D getRadialVigCorrCenterShift() const { return m_RadialVigCorrCenterShift.getData(); }
# 125 "image_variables.h"
std::string getExifModel() const { return m_ExifModel.getData(); }
std::string getExifMake() const { return m_ExifMake.getData(); }
double getExifCropFactor() const { return m_ExifCropFactor.getData(); }
double getExifFocalLength() const { return m_ExifFocalLength.getData(); }
double getExifOrientation() const { return m_ExifOrientation.getData(); }
double getExifAperture() const { return m_ExifAperture.getData(); }
double getExifISO() const { return m_ExifISO.getData(); }
double getExifDistance() const { return m_ExifDistance.getData(); }
double getExifFocalLength35() const { return m_ExifFocalLength35.getData(); }
double getExifExposureTime() const { return m_ExifExposureTime.getData(); }
std::string getExifDate() const { return m_ExifDate.getData(); }





unsigned int getFeatherWidth() const { return m_FeatherWidth.getData(); }

bool getMorph() const { return m_Morph.getData(); }



HuginBase::MaskPolygonVector getMasks() const { return m_Masks.getData(); }


HuginBase::MaskPolygonVector getActiveMasks() const { return m_ActiveMasks.getData(); }


bool getActive() const { return m_Active.getData(); }
# 18 "hsi_SrcPanoImage_accessor_macros.h" 2





# 1 "image_variables.h" 1
# 64 "image_variables.h"
const ImageVariable<std::string> & getFilenameIV() const { return m_Filename; }
const ImageVariable<vigra::Size2D> & getSizeIV() const { return m_Size; }

const ImageVariable<HuginBase::BaseSrcPanoImage::Projection> & getProjectionIV() const { return m_Projection; }
const ImageVariable<double> & getHFOVIV() const { return m_HFOV; }


const ImageVariable<HuginBase::BaseSrcPanoImage::ResponseType> & getResponseTypeIV() const { return m_ResponseType; }
const ImageVariable<std::vector<float> > & getEMoRParamsIV() const { return m_EMoRParams; }
const ImageVariable<double> & getExposureValueIV() const { return m_ExposureValue; }
const ImageVariable<double> & getGammaIV() const { return m_Gamma; }
const ImageVariable<double> & getWhiteBalanceRedIV() const { return m_WhiteBalanceRed; }
const ImageVariable<double> & getWhiteBalanceBlueIV() const { return m_WhiteBalanceBlue; }


const ImageVariable<double> & getRollIV() const { return m_Roll; }
const ImageVariable<double> & getPitchIV() const { return m_Pitch; }
const ImageVariable<double> & getYawIV() const { return m_Yaw; }

const ImageVariable<double> & getXIV() const { return m_X; }
const ImageVariable<double> & getYIV() const { return m_Y; }
const ImageVariable<double> & getZIV() const { return m_Z; }



const ImageVariable<double> & getStackIV() const { return m_Stack; }


const ImageVariable<std::vector<double> > & getRadialDistortionIV() const { return m_RadialDistortion; }


const ImageVariable<std::vector<double> > & getRadialDistortionRedIV() const { return m_RadialDistortionRed; }
const ImageVariable<std::vector<double> > & getRadialDistortionBlueIV() const { return m_RadialDistortionBlue; }


const ImageVariable<hugin_utils::FDiff2D> & getRadialDistortionCenterShiftIV() const { return m_RadialDistortionCenterShift; }


const ImageVariable<hugin_utils::FDiff2D> & getShearIV() const { return m_Shear; }


const ImageVariable<HuginBase::BaseSrcPanoImage::CropMode> & getCropModeIV() const { return m_CropMode; }
const ImageVariable<vigra::Rect2D> & getCropRectIV() const { return m_CropRect; }
const ImageVariable<bool> & getAutoCenterCropIV() const { return m_AutoCenterCrop; }


const ImageVariable<int> & getVigCorrModeIV() const { return m_VigCorrMode; }


const ImageVariable<std::string> & getFlatfieldFilenameIV() const { return m_FlatfieldFilename; }
const ImageVariable<std::vector<double> > & getRadialVigCorrCoeffIV() const { return m_RadialVigCorrCoeff; }
const ImageVariable<hugin_utils::FDiff2D> & getRadialVigCorrCenterShiftIV() const { return m_RadialVigCorrCenterShift; }
# 125 "image_variables.h"
const ImageVariable<std::string> & getExifModelIV() const { return m_ExifModel; }
const ImageVariable<std::string> & getExifMakeIV() const { return m_ExifMake; }
const ImageVariable<double> & getExifCropFactorIV() const { return m_ExifCropFactor; }
const ImageVariable<double> & getExifFocalLengthIV() const { return m_ExifFocalLength; }
const ImageVariable<double> & getExifOrientationIV() const { return m_ExifOrientation; }
const ImageVariable<double> & getExifApertureIV() const { return m_ExifAperture; }
const ImageVariable<double> & getExifISOIV() const { return m_ExifISO; }
const ImageVariable<double> & getExifDistanceIV() const { return m_ExifDistance; }
const ImageVariable<double> & getExifFocalLength35IV() const { return m_ExifFocalLength35; }
const ImageVariable<double> & getExifExposureTimeIV() const { return m_ExifExposureTime; }
const ImageVariable<std::string> & getExifDateIV() const { return m_ExifDate; }





const ImageVariable<unsigned int> & getFeatherWidthIV() const { return m_FeatherWidth; }

const ImageVariable<bool> & getMorphIV() const { return m_Morph; }



const ImageVariable<HuginBase::MaskPolygonVector> & getMasksIV() const { return m_Masks; }


const ImageVariable<HuginBase::MaskPolygonVector> & getActiveMasksIV() const { return m_ActiveMasks; }


const ImageVariable<bool> & getActiveIV() const { return m_Active; }
# 24 "hsi_SrcPanoImage_accessor_macros.h" 2





# 1 "image_variables.h" 1
# 64 "image_variables.h"
void setFilename(std::string data) { m_Filename.setData(data); }
void setSize(vigra::Size2D data) { m_Size.setData(data); }

void setProjection(HuginBase::BaseSrcPanoImage::Projection data) { m_Projection.setData(data); }
void setHFOV(double data) { m_HFOV.setData(data); }


void setResponseType(HuginBase::BaseSrcPanoImage::ResponseType data) { m_ResponseType.setData(data); }
void setEMoRParams(std::vector<float> data) { m_EMoRParams.setData(data); }
void setExposureValue(double data) { m_ExposureValue.setData(data); }
void setGamma(double data) { m_Gamma.setData(data); }
void setWhiteBalanceRed(double data) { m_WhiteBalanceRed.setData(data); }
void setWhiteBalanceBlue(double data) { m_WhiteBalanceBlue.setData(data); }


void setRoll(double data) { m_Roll.setData(data); }
void setPitch(double data) { m_Pitch.setData(data); }
void setYaw(double data) { m_Yaw.setData(data); }

void setX(double data) { m_X.setData(data); }
void setY(double data) { m_Y.setData(data); }
void setZ(double data) { m_Z.setData(data); }



void setStack(double data) { m_Stack.setData(data); }


void setRadialDistortion(std::vector<double> data) { m_RadialDistortion.setData(data); }


void setRadialDistortionRed(std::vector<double> data) { m_RadialDistortionRed.setData(data); }
void setRadialDistortionBlue(std::vector<double> data) { m_RadialDistortionBlue.setData(data); }


void setRadialDistortionCenterShift(hugin_utils::FDiff2D data) { m_RadialDistortionCenterShift.setData(data); }


void setShear(hugin_utils::FDiff2D data) { m_Shear.setData(data); }


void setCropMode(HuginBase::BaseSrcPanoImage::CropMode data) { m_CropMode.setData(data); }
void setCropRect(vigra::Rect2D data) { m_CropRect.setData(data); }
void setAutoCenterCrop(bool data) { m_AutoCenterCrop.setData(data); }


void setVigCorrMode(int data) { m_VigCorrMode.setData(data); }


void setFlatfieldFilename(std::string data) { m_FlatfieldFilename.setData(data); }
void setRadialVigCorrCoeff(std::vector<double> data) { m_RadialVigCorrCoeff.setData(data); }
void setRadialVigCorrCenterShift(hugin_utils::FDiff2D data) { m_RadialVigCorrCenterShift.setData(data); }
# 125 "image_variables.h"
void setExifModel(std::string data) { m_ExifModel.setData(data); }
void setExifMake(std::string data) { m_ExifMake.setData(data); }
void setExifCropFactor(double data) { m_ExifCropFactor.setData(data); }
void setExifFocalLength(double data) { m_ExifFocalLength.setData(data); }
void setExifOrientation(double data) { m_ExifOrientation.setData(data); }
void setExifAperture(double data) { m_ExifAperture.setData(data); }
void setExifISO(double data) { m_ExifISO.setData(data); }
void setExifDistance(double data) { m_ExifDistance.setData(data); }
void setExifFocalLength35(double data) { m_ExifFocalLength35.setData(data); }
void setExifExposureTime(double data) { m_ExifExposureTime.setData(data); }
void setExifDate(std::string data) { m_ExifDate.setData(data); }





void setFeatherWidth(unsigned int data) { m_FeatherWidth.setData(data); }

void setMorph(bool data) { m_Morph.setData(data); }



void setMasks(HuginBase::MaskPolygonVector data) { m_Masks.setData(data); }


void setActiveMasks(HuginBase::MaskPolygonVector data) { m_ActiveMasks.setData(data); }


void setActive(bool data) { m_Active.setData(data); }
# 30 "hsi_SrcPanoImage_accessor_macros.h" 2
# 40 "hsi_SrcPanoImage_accessor_macros.h"
# 1 "image_variables.h" 1
# 64 "image_variables.h"
void linkFilename (BaseSrcPanoImage * target) { m_Filename.linkWith(&(target->m_Filename)); }
void linkSize (BaseSrcPanoImage * target) { m_Size.linkWith(&(target->m_Size)); }

void linkProjection (BaseSrcPanoImage * target) { m_Projection.linkWith(&(target->m_Projection)); }
void linkHFOV (BaseSrcPanoImage * target) { m_HFOV.linkWith(&(target->m_HFOV)); }


void linkResponseType (BaseSrcPanoImage * target) { m_ResponseType.linkWith(&(target->m_ResponseType)); }
void linkEMoRParams (BaseSrcPanoImage * target) { m_EMoRParams.linkWith(&(target->m_EMoRParams)); }
void linkExposureValue (BaseSrcPanoImage * target) { m_ExposureValue.linkWith(&(target->m_ExposureValue)); }
void linkGamma (BaseSrcPanoImage * target) { m_Gamma.linkWith(&(target->m_Gamma)); }
void linkWhiteBalanceRed (BaseSrcPanoImage * target) { m_WhiteBalanceRed.linkWith(&(target->m_WhiteBalanceRed)); }
void linkWhiteBalanceBlue (BaseSrcPanoImage * target) { m_WhiteBalanceBlue.linkWith(&(target->m_WhiteBalanceBlue)); }


void linkRoll (BaseSrcPanoImage * target) { m_Roll.linkWith(&(target->m_Roll)); }
void linkPitch (BaseSrcPanoImage * target) { m_Pitch.linkWith(&(target->m_Pitch)); }
void linkYaw (BaseSrcPanoImage * target) { m_Yaw.linkWith(&(target->m_Yaw)); }

void linkX (BaseSrcPanoImage * target) { m_X.linkWith(&(target->m_X)); }
void linkY (BaseSrcPanoImage * target) { m_Y.linkWith(&(target->m_Y)); }
void linkZ (BaseSrcPanoImage * target) { m_Z.linkWith(&(target->m_Z)); }



void linkStack (BaseSrcPanoImage * target) { m_Stack.linkWith(&(target->m_Stack)); }


void linkRadialDistortion (BaseSrcPanoImage * target) { m_RadialDistortion.linkWith(&(target->m_RadialDistortion)); }


void linkRadialDistortionRed (BaseSrcPanoImage * target) { m_RadialDistortionRed.linkWith(&(target->m_RadialDistortionRed)); }
void linkRadialDistortionBlue (BaseSrcPanoImage * target) { m_RadialDistortionBlue.linkWith(&(target->m_RadialDistortionBlue)); }


void linkRadialDistortionCenterShift (BaseSrcPanoImage * target) { m_RadialDistortionCenterShift.linkWith(&(target->m_RadialDistortionCenterShift)); }


void linkShear (BaseSrcPanoImage * target) { m_Shear.linkWith(&(target->m_Shear)); }


void linkCropMode (BaseSrcPanoImage * target) { m_CropMode.linkWith(&(target->m_CropMode)); }
void linkCropRect (BaseSrcPanoImage * target) { m_CropRect.linkWith(&(target->m_CropRect)); }
void linkAutoCenterCrop (BaseSrcPanoImage * target) { m_AutoCenterCrop.linkWith(&(target->m_AutoCenterCrop)); }


void linkVigCorrMode (BaseSrcPanoImage * target) { m_VigCorrMode.linkWith(&(target->m_VigCorrMode)); }


void linkFlatfieldFilename (BaseSrcPanoImage * target) { m_FlatfieldFilename.linkWith(&(target->m_FlatfieldFilename)); }
void linkRadialVigCorrCoeff (BaseSrcPanoImage * target) { m_RadialVigCorrCoeff.linkWith(&(target->m_RadialVigCorrCoeff)); }
void linkRadialVigCorrCenterShift (BaseSrcPanoImage * target) { m_RadialVigCorrCenterShift.linkWith(&(target->m_RadialVigCorrCenterShift)); }
# 125 "image_variables.h"
void linkExifModel (BaseSrcPanoImage * target) { m_ExifModel.linkWith(&(target->m_ExifModel)); }
void linkExifMake (BaseSrcPanoImage * target) { m_ExifMake.linkWith(&(target->m_ExifMake)); }
void linkExifCropFactor (BaseSrcPanoImage * target) { m_ExifCropFactor.linkWith(&(target->m_ExifCropFactor)); }
void linkExifFocalLength (BaseSrcPanoImage * target) { m_ExifFocalLength.linkWith(&(target->m_ExifFocalLength)); }
void linkExifOrientation (BaseSrcPanoImage * target) { m_ExifOrientation.linkWith(&(target->m_ExifOrientation)); }
void linkExifAperture (BaseSrcPanoImage * target) { m_ExifAperture.linkWith(&(target->m_ExifAperture)); }
void linkExifISO (BaseSrcPanoImage * target) { m_ExifISO.linkWith(&(target->m_ExifISO)); }
void linkExifDistance (BaseSrcPanoImage * target) { m_ExifDistance.linkWith(&(target->m_ExifDistance)); }
void linkExifFocalLength35 (BaseSrcPanoImage * target) { m_ExifFocalLength35.linkWith(&(target->m_ExifFocalLength35)); }
void linkExifExposureTime (BaseSrcPanoImage * target) { m_ExifExposureTime.linkWith(&(target->m_ExifExposureTime)); }
void linkExifDate (BaseSrcPanoImage * target) { m_ExifDate.linkWith(&(target->m_ExifDate)); }





void linkFeatherWidth (BaseSrcPanoImage * target) { m_FeatherWidth.linkWith(&(target->m_FeatherWidth)); }

void linkMorph (BaseSrcPanoImage * target) { m_Morph.linkWith(&(target->m_Morph)); }



void linkMasks (BaseSrcPanoImage * target) { m_Masks.linkWith(&(target->m_Masks)); }


void linkActiveMasks (BaseSrcPanoImage * target) { m_ActiveMasks.linkWith(&(target->m_ActiveMasks)); }


void linkActive (BaseSrcPanoImage * target) { m_Active.linkWith(&(target->m_Active)); }
# 41 "hsi_SrcPanoImage_accessor_macros.h" 2
# 50 "hsi_SrcPanoImage_accessor_macros.h"
# 1 "image_variables.h" 1
# 64 "image_variables.h"
void unlinkFilename () { m_Filename.removeLinks(); }
void unlinkSize () { m_Size.removeLinks(); }

void unlinkProjection () { m_Projection.removeLinks(); }
void unlinkHFOV () { m_HFOV.removeLinks(); }


void unlinkResponseType () { m_ResponseType.removeLinks(); }
void unlinkEMoRParams () { m_EMoRParams.removeLinks(); }
void unlinkExposureValue () { m_ExposureValue.removeLinks(); }
void unlinkGamma () { m_Gamma.removeLinks(); }
void unlinkWhiteBalanceRed () { m_WhiteBalanceRed.removeLinks(); }
void unlinkWhiteBalanceBlue () { m_WhiteBalanceBlue.removeLinks(); }


void unlinkRoll () { m_Roll.removeLinks(); }
void unlinkPitch () { m_Pitch.removeLinks(); }
void unlinkYaw () { m_Yaw.removeLinks(); }

void unlinkX () { m_X.removeLinks(); }
void unlinkY () { m_Y.removeLinks(); }
void unlinkZ () { m_Z.removeLinks(); }



void unlinkStack () { m_Stack.removeLinks(); }


void unlinkRadialDistortion () { m_RadialDistortion.removeLinks(); }


void unlinkRadialDistortionRed () { m_RadialDistortionRed.removeLinks(); }
void unlinkRadialDistortionBlue () { m_RadialDistortionBlue.removeLinks(); }


void unlinkRadialDistortionCenterShift () { m_RadialDistortionCenterShift.removeLinks(); }


void unlinkShear () { m_Shear.removeLinks(); }


void unlinkCropMode () { m_CropMode.removeLinks(); }
void unlinkCropRect () { m_CropRect.removeLinks(); }
void unlinkAutoCenterCrop () { m_AutoCenterCrop.removeLinks(); }


void unlinkVigCorrMode () { m_VigCorrMode.removeLinks(); }


void unlinkFlatfieldFilename () { m_FlatfieldFilename.removeLinks(); }
void unlinkRadialVigCorrCoeff () { m_RadialVigCorrCoeff.removeLinks(); }
void unlinkRadialVigCorrCenterShift () { m_RadialVigCorrCenterShift.removeLinks(); }
# 125 "image_variables.h"
void unlinkExifModel () { m_ExifModel.removeLinks(); }
void unlinkExifMake () { m_ExifMake.removeLinks(); }
void unlinkExifCropFactor () { m_ExifCropFactor.removeLinks(); }
void unlinkExifFocalLength () { m_ExifFocalLength.removeLinks(); }
void unlinkExifOrientation () { m_ExifOrientation.removeLinks(); }
void unlinkExifAperture () { m_ExifAperture.removeLinks(); }
void unlinkExifISO () { m_ExifISO.removeLinks(); }
void unlinkExifDistance () { m_ExifDistance.removeLinks(); }
void unlinkExifFocalLength35 () { m_ExifFocalLength35.removeLinks(); }
void unlinkExifExposureTime () { m_ExifExposureTime.removeLinks(); }
void unlinkExifDate () { m_ExifDate.removeLinks(); }





void unlinkFeatherWidth () { m_FeatherWidth.removeLinks(); }

void unlinkMorph () { m_Morph.removeLinks(); }



void unlinkMasks () { m_Masks.removeLinks(); }


void unlinkActiveMasks () { m_ActiveMasks.removeLinks(); }


void unlinkActive () { m_Active.removeLinks(); }
# 51 "hsi_SrcPanoImage_accessor_macros.h" 2
# 59 "hsi_SrcPanoImage_accessor_macros.h"
# 1 "image_variables.h" 1
# 64 "image_variables.h"
bool FilenameisLinked () const { return m_Filename.isLinked(); }
bool SizeisLinked () const { return m_Size.isLinked(); }

bool ProjectionisLinked () const { return m_Projection.isLinked(); }
bool HFOVisLinked () const { return m_HFOV.isLinked(); }


bool ResponseTypeisLinked () const { return m_ResponseType.isLinked(); }
bool EMoRParamsisLinked () const { return m_EMoRParams.isLinked(); }
bool ExposureValueisLinked () const { return m_ExposureValue.isLinked(); }
bool GammaisLinked () const { return m_Gamma.isLinked(); }
bool WhiteBalanceRedisLinked () const { return m_WhiteBalanceRed.isLinked(); }
bool WhiteBalanceBlueisLinked () const { return m_WhiteBalanceBlue.isLinked(); }


bool RollisLinked () const { return m_Roll.isLinked(); }
bool PitchisLinked () const { return m_Pitch.isLinked(); }
bool YawisLinked () const { return m_Yaw.isLinked(); }

bool XisLinked () const { return m_X.isLinked(); }
bool YisLinked () const { return m_Y.isLinked(); }
bool ZisLinked () const { return m_Z.isLinked(); }



bool StackisLinked () const { return m_Stack.isLinked(); }


bool RadialDistortionisLinked () const { return m_RadialDistortion.isLinked(); }


bool RadialDistortionRedisLinked () const { return m_RadialDistortionRed.isLinked(); }
bool RadialDistortionBlueisLinked () const { return m_RadialDistortionBlue.isLinked(); }


bool RadialDistortionCenterShiftisLinked () const { return m_RadialDistortionCenterShift.isLinked(); }


bool ShearisLinked () const { return m_Shear.isLinked(); }


bool CropModeisLinked () const { return m_CropMode.isLinked(); }
bool CropRectisLinked () const { return m_CropRect.isLinked(); }
bool AutoCenterCropisLinked () const { return m_AutoCenterCrop.isLinked(); }


bool VigCorrModeisLinked () const { return m_VigCorrMode.isLinked(); }


bool FlatfieldFilenameisLinked () const { return m_FlatfieldFilename.isLinked(); }
bool RadialVigCorrCoeffisLinked () const { return m_RadialVigCorrCoeff.isLinked(); }
bool RadialVigCorrCenterShiftisLinked () const { return m_RadialVigCorrCenterShift.isLinked(); }
# 125 "image_variables.h"
bool ExifModelisLinked () const { return m_ExifModel.isLinked(); }
bool ExifMakeisLinked () const { return m_ExifMake.isLinked(); }
bool ExifCropFactorisLinked () const { return m_ExifCropFactor.isLinked(); }
bool ExifFocalLengthisLinked () const { return m_ExifFocalLength.isLinked(); }
bool ExifOrientationisLinked () const { return m_ExifOrientation.isLinked(); }
bool ExifApertureisLinked () const { return m_ExifAperture.isLinked(); }
bool ExifISOisLinked () const { return m_ExifISO.isLinked(); }
bool ExifDistanceisLinked () const { return m_ExifDistance.isLinked(); }
bool ExifFocalLength35isLinked () const { return m_ExifFocalLength35.isLinked(); }
bool ExifExposureTimeisLinked () const { return m_ExifExposureTime.isLinked(); }
bool ExifDateisLinked () const { return m_ExifDate.isLinked(); }





bool FeatherWidthisLinked () const { return m_FeatherWidth.isLinked(); }

bool MorphisLinked () const { return m_Morph.isLinked(); }



bool MasksisLinked () const { return m_Masks.isLinked(); }


bool ActiveMasksisLinked () const { return m_ActiveMasks.isLinked(); }


bool ActiveisLinked () const { return m_Active.isLinked(); }
# 60 "hsi_SrcPanoImage_accessor_macros.h" 2
# 69 "hsi_SrcPanoImage_accessor_macros.h"
# 1 "image_variables.h" 1
# 64 "image_variables.h"
bool FilenameisLinkedWith (const BaseSrcPanoImage & image) const { return m_Filename.isLinkedWith(&(image.m_Filename)); }
bool SizeisLinkedWith (const BaseSrcPanoImage & image) const { return m_Size.isLinkedWith(&(image.m_Size)); }

bool ProjectionisLinkedWith (const BaseSrcPanoImage & image) const { return m_Projection.isLinkedWith(&(image.m_Projection)); }
bool HFOVisLinkedWith (const BaseSrcPanoImage & image) const { return m_HFOV.isLinkedWith(&(image.m_HFOV)); }


bool ResponseTypeisLinkedWith (const BaseSrcPanoImage & image) const { return m_ResponseType.isLinkedWith(&(image.m_ResponseType)); }
bool EMoRParamsisLinkedWith (const BaseSrcPanoImage & image) const { return m_EMoRParams.isLinkedWith(&(image.m_EMoRParams)); }
bool ExposureValueisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExposureValue.isLinkedWith(&(image.m_ExposureValue)); }
bool GammaisLinkedWith (const BaseSrcPanoImage & image) const { return m_Gamma.isLinkedWith(&(image.m_Gamma)); }
bool WhiteBalanceRedisLinkedWith (const BaseSrcPanoImage & image) const { return m_WhiteBalanceRed.isLinkedWith(&(image.m_WhiteBalanceRed)); }
bool WhiteBalanceBlueisLinkedWith (const BaseSrcPanoImage & image) const { return m_WhiteBalanceBlue.isLinkedWith(&(image.m_WhiteBalanceBlue)); }


bool RollisLinkedWith (const BaseSrcPanoImage & image) const { return m_Roll.isLinkedWith(&(image.m_Roll)); }
bool PitchisLinkedWith (const BaseSrcPanoImage & image) const { return m_Pitch.isLinkedWith(&(image.m_Pitch)); }
bool YawisLinkedWith (const BaseSrcPanoImage & image) const { return m_Yaw.isLinkedWith(&(image.m_Yaw)); }

bool XisLinkedWith (const BaseSrcPanoImage & image) const { return m_X.isLinkedWith(&(image.m_X)); }
bool YisLinkedWith (const BaseSrcPanoImage & image) const { return m_Y.isLinkedWith(&(image.m_Y)); }
bool ZisLinkedWith (const BaseSrcPanoImage & image) const { return m_Z.isLinkedWith(&(image.m_Z)); }



bool StackisLinkedWith (const BaseSrcPanoImage & image) const { return m_Stack.isLinkedWith(&(image.m_Stack)); }


bool RadialDistortionisLinkedWith (const BaseSrcPanoImage & image) const { return m_RadialDistortion.isLinkedWith(&(image.m_RadialDistortion)); }


bool RadialDistortionRedisLinkedWith (const BaseSrcPanoImage & image) const { return m_RadialDistortionRed.isLinkedWith(&(image.m_RadialDistortionRed)); }
bool RadialDistortionBlueisLinkedWith (const BaseSrcPanoImage & image) const { return m_RadialDistortionBlue.isLinkedWith(&(image.m_RadialDistortionBlue)); }


bool RadialDistortionCenterShiftisLinkedWith (const BaseSrcPanoImage & image) const { return m_RadialDistortionCenterShift.isLinkedWith(&(image.m_RadialDistortionCenterShift)); }


bool ShearisLinkedWith (const BaseSrcPanoImage & image) const { return m_Shear.isLinkedWith(&(image.m_Shear)); }


bool CropModeisLinkedWith (const BaseSrcPanoImage & image) const { return m_CropMode.isLinkedWith(&(image.m_CropMode)); }
bool CropRectisLinkedWith (const BaseSrcPanoImage & image) const { return m_CropRect.isLinkedWith(&(image.m_CropRect)); }
bool AutoCenterCropisLinkedWith (const BaseSrcPanoImage & image) const { return m_AutoCenterCrop.isLinkedWith(&(image.m_AutoCenterCrop)); }


bool VigCorrModeisLinkedWith (const BaseSrcPanoImage & image) const { return m_VigCorrMode.isLinkedWith(&(image.m_VigCorrMode)); }


bool FlatfieldFilenameisLinkedWith (const BaseSrcPanoImage & image) const { return m_FlatfieldFilename.isLinkedWith(&(image.m_FlatfieldFilename)); }
bool RadialVigCorrCoeffisLinkedWith (const BaseSrcPanoImage & image) const { return m_RadialVigCorrCoeff.isLinkedWith(&(image.m_RadialVigCorrCoeff)); }
bool RadialVigCorrCenterShiftisLinkedWith (const BaseSrcPanoImage & image) const { return m_RadialVigCorrCenterShift.isLinkedWith(&(image.m_RadialVigCorrCenterShift)); }
# 125 "image_variables.h"
bool ExifModelisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifModel.isLinkedWith(&(image.m_ExifModel)); }
bool ExifMakeisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifMake.isLinkedWith(&(image.m_ExifMake)); }
bool ExifCropFactorisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifCropFactor.isLinkedWith(&(image.m_ExifCropFactor)); }
bool ExifFocalLengthisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifFocalLength.isLinkedWith(&(image.m_ExifFocalLength)); }
bool ExifOrientationisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifOrientation.isLinkedWith(&(image.m_ExifOrientation)); }
bool ExifApertureisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifAperture.isLinkedWith(&(image.m_ExifAperture)); }
bool ExifISOisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifISO.isLinkedWith(&(image.m_ExifISO)); }
bool ExifDistanceisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifDistance.isLinkedWith(&(image.m_ExifDistance)); }
bool ExifFocalLength35isLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifFocalLength35.isLinkedWith(&(image.m_ExifFocalLength35)); }
bool ExifExposureTimeisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifExposureTime.isLinkedWith(&(image.m_ExifExposureTime)); }
bool ExifDateisLinkedWith (const BaseSrcPanoImage & image) const { return m_ExifDate.isLinkedWith(&(image.m_ExifDate)); }





bool FeatherWidthisLinkedWith (const BaseSrcPanoImage & image) const { return m_FeatherWidth.isLinkedWith(&(image.m_FeatherWidth)); }

bool MorphisLinkedWith (const BaseSrcPanoImage & image) const { return m_Morph.isLinkedWith(&(image.m_Morph)); }



bool MasksisLinkedWith (const BaseSrcPanoImage & image) const { return m_Masks.isLinkedWith(&(image.m_Masks)); }


bool ActiveMasksisLinkedWith (const BaseSrcPanoImage & image) const { return m_ActiveMasks.isLinkedWith(&(image.m_ActiveMasks)); }


bool ActiveisLinkedWith (const BaseSrcPanoImage & image) const { return m_Active.isLinkedWith(&(image.m_Active)); }
# 70 "hsi_SrcPanoImage_accessor_macros.h" 2


protected:

    void setDefaults();




# 1 "image_variables.h" 1
# 64 "image_variables.h"
ImageVariable<std::string> m_Filename;
ImageVariable<vigra::Size2D> m_Size;

ImageVariable<HuginBase::BaseSrcPanoImage::Projection> m_Projection;
ImageVariable<double> m_HFOV;


ImageVariable<HuginBase::BaseSrcPanoImage::ResponseType> m_ResponseType;
ImageVariable<std::vector<float> > m_EMoRParams;
ImageVariable<double> m_ExposureValue;
ImageVariable<double> m_Gamma;
ImageVariable<double> m_WhiteBalanceRed;
ImageVariable<double> m_WhiteBalanceBlue;


ImageVariable<double> m_Roll;
ImageVariable<double> m_Pitch;
ImageVariable<double> m_Yaw;

ImageVariable<double> m_X;
ImageVariable<double> m_Y;
ImageVariable<double> m_Z;



ImageVariable<double> m_Stack;


ImageVariable<std::vector<double> > m_RadialDistortion;


ImageVariable<std::vector<double> > m_RadialDistortionRed;
ImageVariable<std::vector<double> > m_RadialDistortionBlue;


ImageVariable<hugin_utils::FDiff2D> m_RadialDistortionCenterShift;


ImageVariable<hugin_utils::FDiff2D> m_Shear;


ImageVariable<HuginBase::BaseSrcPanoImage::CropMode> m_CropMode;
ImageVariable<vigra::Rect2D> m_CropRect;
ImageVariable<bool> m_AutoCenterCrop;


ImageVariable<int> m_VigCorrMode;


ImageVariable<std::string> m_FlatfieldFilename;
ImageVariable<std::vector<double> > m_RadialVigCorrCoeff;
ImageVariable<hugin_utils::FDiff2D> m_RadialVigCorrCenterShift;
# 125 "image_variables.h"
ImageVariable<std::string> m_ExifModel;
ImageVariable<std::string> m_ExifMake;
ImageVariable<double> m_ExifCropFactor;
ImageVariable<double> m_ExifFocalLength;
ImageVariable<double> m_ExifOrientation;
ImageVariable<double> m_ExifAperture;
ImageVariable<double> m_ExifISO;
ImageVariable<double> m_ExifDistance;
ImageVariable<double> m_ExifFocalLength35;
ImageVariable<double> m_ExifExposureTime;
ImageVariable<std::string> m_ExifDate;





ImageVariable<unsigned int> m_FeatherWidth;

ImageVariable<bool> m_Morph;



ImageVariable<HuginBase::MaskPolygonVector> m_Masks;


ImageVariable<HuginBase::MaskPolygonVector> m_ActiveMasks;


ImageVariable<bool> m_Active;
# 80 "hsi_SrcPanoImage_accessor_macros.h" 2


/*********** End of precompiled section ******************/
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
    
    
    /** get the optimisation and stitching options, like PanoImage.
     * 
     * Do not use: eventually we want to make everything using these to ask for
     * each wanted variable directly using the get* functions instead.
     */
    ImageOptions getOptions() const;
    
    /** set the optimisation and stitching options
     * 
     * Do not use: switch stuff over to the set* functions instead.
     */
    void setOptions(const ImageOptions & opt);
    
    
    /** try to convert Exif date time string to struct tm 
     *  @return 0, if conversion was sucessfull */
    const int getExifDateTime(struct tm* datetime) const
    { return Exiv2::exifTime(m_ExifDate.getData().c_str(),datetime); }

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
    /** writes all mask lines to stream, using given image number */
    void printMaskLines(std::ostream &o, unsigned int newImgNr) const;
    /** returns true, if point p is inside of one mask polygon */
    bool isInsideMasks(vigra::Point2D p) const;

private:

    /** convenience functions to work with Exiv2 */
    bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, long & value);
    bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, float & value);
    bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::string & value);

    /** Check if Exiv orientation tag can be trusted */
    bool trustExivOrientation();
    bool successfullEXIFread;
};

typedef std::vector<SrcPanoImage> ImageVector;

} // namespace

#endif // PANOIMAGE_H
