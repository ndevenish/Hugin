// -*- c-basic-offset: 4 -*-
/** @file ImageVariableTranslate.h
 *
 *  @brief Convenience functions for SrcPanoImage to use on the image variables.
 *
 *  @author James Legg
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PANODATA_IMAGEVARIABLETRANSLATE_H
#define _PANODATA_IMAGEVARIABLETRANSLATE_H

namespace HuginBase
{
/* We define stuff to use the PTO codes i image_variables.h
 */
/** Parent class to anything that aids conversion between PTO file variables and
 * the image variables of SrcPanoImg.
 * 
 * Although this class doesn't really do anything itself, it will be used when
 * the variable in SrcPanoImg is not written to a file.
 * 
 * We derive children of these to handle more specific cases, which are usually
 * templates to handle similar things (e.g. all vectors)
 * We then typedef to make a convertor for each variable in SrcPanoImg.
 */
class IMPEX PTOVariableConverterNoOp
{
public:
    /** Check if a given pto format variable name applies to this image
     * variable.
     * 
     * @return true if we can handle this variable name, false otherwise.
     * @param name The code used to identify the variable in the PTO file
     * format. For example "Eev".
     */
    inline static bool checkApplicability(const std::string& name)
    {
        return false;
    }
    
    /** Get a pto format variable's value, given its name and a reference to the
     * variable in SrcPanoImg.
     * 
     * It is a precondition of this function that the checkApplicability
     * function returns true when passed name.
     * 
     * @return 0 in this case, but child objects should return the value to
     * write in a pto file for this variable.
     * @param name The code used to identify the variable in the PTO file
     * format. For example "Eev".
     * @param var Some ImageVariable used by SrcPanoImg to hold this variable.
     * @tparam T the template parameter of the ImageVariable var.
     */
    template <class T>
    inline static double getValueFromVariable(
                                        const std::string& name,
                                        const ImageVariable<T> & var
                                      )
    {
        return 0;
    }
    
    /** Set a ImageVariable in SrcPanoImg given the pto format variable.
     * 
     * It is a precondition of this function that the checkApplicability
     * function returns true when passed name.
     * 
     * @param name The name used in the PTO format image variable.
     * @param var The image variable used in SrcPanoImg to change.
     * @param value The value specified in the PTO format image variable.
     * @tparam T The template parameter of ImageVariable var.
     */
    template <class T>
    inline static void setValueFromVariable(
                                        const std::string& name,
                                        ImageVariable<T> & var,
                                        const double value
                                    )
    {
    }
    
    /** Add all PTO format variables specifed by an image variable in SrcPanoImg
     * to a VariableMap.
     * 
     * @param var The image variable used in SrcPanoImg to get values from.
     * @param map The VariableMap
     * @tparam T The template parameter of ImageVariable var.
     */
    template <class T>
    inline static void addToVariableMap(
                            const ImageVariable<T> & var,
                            VariableMap & map
                                )
    {
    }
};

/** Object to group conversion functions for PTO format variables of up to three
 * characters representing a single variable in SrcPanoImg.
 * @tparam code1 1st character of identifier used in PTO file format.
 * @tparam code2 2nd character of identifier used in PTO file format (or '/0').
 * @tparam code3 3rd character of identifier used in PTO file format (or '/0').
 * @tparam T type used in the ImageVariable. Should really be double, but some
 * are small integers or enumerations.
 */
template <char code1, char code2 = '\0', char code3 = '\0', class T = double>
class IMPEX PTOVariableConverterSingle
{
public:
    inline static bool checkApplicability(const std::string& name)
    {
        static const char code[] = {code1, code2, code3, '\0'};
        return ((std::string)code) == name;
    }
    
    inline static double getValueFromVariable(const std::string& name, const ImageVariable<T> & var)
    {
        return var.getData();
    }
    
    inline static void setValueFromVariable(const std::string& name, ImageVariable<T> & var, const double value)
    {
        var.setData((T)value);
    }
    
    inline static void addToVariableMap(const ImageVariable<T> & var, VariableMap & map)
    {
        static const char code[] = {code1, code2, code3, '\0'};
        map.insert(std::make_pair(code, Variable(code, (double)var.getData())));
    }
};


/** Object to group conversion functions for PTO format variables representing
 * a std::vector variable in SrcPanoImg, using an identifying first character.
 * 
 * It can only represent PTO format variables that can be expressed as some
 * constant character followed by a index character, where the first element of
 * the vector has the index character 'a' and the others follow in order.
 * 
 * @tparam base_code string used in PTO file format upto the the character.
 * @tparam T type used in SrcImageVariable. Should really be double, but could
 * be float or something.
 * @tparam size The number of letters and the size of the vector.
 */
template <char base_code, class T = double, size_t size = 4>
class IMPEX PTOVariableConverterVectorChar
{
public:
    inline static bool checkApplicability(const std::string& name)
    {
        return name.size() == 2 && name[0] == base_code && name[1] >= 'a' && name[1] < 'a' + char(size);
    }
    
    inline static double getValueFromVariable(const std::string& name, const ImageVariable<std::vector<T> > & var)
    {
        return var.getData()[name[1]-'a'];
    }
    
    inline static void setValueFromVariable(const std::string& name, ImageVariable<std::vector<T> > & var, const double value)
    {
        std::vector<T> temp = var.getData();
        temp[name[1]-'a'] = value;
        var.setData(temp);
    }
    
    inline static void addToVariableMap(const ImageVariable<std::vector<T> > & var, VariableMap & map)
    {
        char s[3] = {base_code, 'a', '\0'};
        for (size_t index = 0; index < size; index++, s[1]++)
        {
            map.insert(std::make_pair(s, Variable(s, (double)var.getData()[index])));
        }
    }
};

/** Object to group conversion functions for PTO format variables representing
 * a std::vector variable in SrcPanoImg, using characters stating from 'a'.
 * 
 * This can only be used for one variable, and it is RadialDistortion.
 * 
 * @tparam base_code string used in PTO file format upto the the character.
 * @tparam T type used in the ImageVariable. Should really be double, but could
 * be float or something.
 * @tparam size The number of letters and the size of the vector.
 */
template <class T = double, size_t size = 3>
class IMPEX PTOVariableConverterVector
{
public:
    inline static bool checkApplicability(const std::string& name)
    {
        return name[0] >= 'a' && name[0] < 'a' + char(size);
    }
    
    inline static double getValueFromVariable(const std::string& name, const ImageVariable<std::vector<T> > & var)
    {
        return var.getData()[name[0]-'a'];
    }
    
    inline static void setValueFromVariable(const std::string& name, ImageVariable<std::vector<T> > & var, const double value)
    {
        std::vector<T> temp = var.getData();
        temp[name[0]-'a'] = value;
        var.setData(temp);
    }
    
    inline static void addToVariableMap(const ImageVariable<std::vector<T> > & var, VariableMap & map)
    {
        char s[] = "a";
        for (size_t index = 0; index < size; index++, s[0]++)
        {
            map.insert(std::make_pair(s, Variable(s, (double)var.getData()[index])));
        }
    }
};

/** Object to group conversion functions for PTO format variables representing
 * a hugin_utils::FDiff2D variable in SrcPanoImg. Each element must have a pto
 * file format code that is 1 or 2 characters in length.
 * 
 * @tparam code_x The PTO format code that matches the x memeber in the FDiff2D.
 * @tparam code_y The PTO format code that matches the y memeber in the FDiff2D.
 */
template <char code_x1, char code_y1, char code_x2 = '\0', char code_y2 = '\0'>
class IMPEX PTOVariableConverterFDiff2D
{
public:
    inline static bool checkApplicability(const std::string& name)
    {
        static const char code_x[] = {code_x1, code_x2, '\0'};
        static const char code_y[] = {code_y1, code_y2, '\0'};
        return name == (std::string) code_x || name == (std::string) code_y;
    }
    
    inline static double getValueFromVariable(const std::string& name, const ImageVariable<hugin_utils::FDiff2D> & var)
    {
        static const char code_x[] = {code_x1, code_x2, '\0'};
        return name == (std::string) code_x ? var.getData().x : var.getData().y;
    }
    
    inline static void setValueFromVariable(const std::string& name, ImageVariable<hugin_utils::FDiff2D> & var, const double value)
    {
        hugin_utils::FDiff2D temp = var.getData();
        static const char code_x[] = {code_x1, code_x2, '\0'};
        (name == (std::string)code_x ? temp.x : temp.y) = value;
        var.setData(temp);
    }
    
    inline static void addToVariableMap(const ImageVariable<hugin_utils::FDiff2D> & var, VariableMap & map)
    {
        static const char s_x[] = {code_x1, code_x2, '\0'};
        static const char s_y[] = {code_y1, code_y2, '\0'};
        map.insert(std::make_pair(s_x, Variable(s_x, var.getData().x)));
        map.insert(std::make_pair(s_y, Variable(s_y, var.getData().y)));
    }
};

// Now we can define a type to use for every PanoramaVariable.
// We should make sure that any given code is only applicable to one of these.
/// @see image_variables.h

/// @todo This could be n, but it is a string, not a double.
typedef PTOVariableConverterNoOp PTOVariableConverterForFilename;
typedef PTOVariableConverterNoOp PTOVariableConverterForSize;
//typedef PTOVariableConverterSingle<'f','\0', '\0', SrcPanoImage::Projection> PTOVariableConverterForProjection;
typedef PTOVariableConverterNoOp PTOVariableConverterForProjection;
typedef PTOVariableConverterSingle<'v'> PTOVariableConverterForHFOV;
typedef PTOVariableConverterNoOp PTOVariableConverterForCropFactor;

typedef PTOVariableConverterNoOp PTOVariableConverterForResponseType;
typedef PTOVariableConverterVectorChar<'R', float, 5> PTOVariableConverterForEMoRParams;
typedef PTOVariableConverterSingle<'E', 'e', 'v'> PTOVariableConverterForExposureValue;
typedef PTOVariableConverterNoOp PTOVariableConverterForGamma ;
typedef PTOVariableConverterSingle<'E', 'r'>  PTOVariableConverterForWhiteBalanceRed;
typedef PTOVariableConverterSingle<'E', 'b'>  PTOVariableConverterForWhiteBalanceBlue;

typedef PTOVariableConverterSingle<'r'> PTOVariableConverterForRoll;
typedef PTOVariableConverterSingle<'p'> PTOVariableConverterForPitch;
typedef PTOVariableConverterSingle<'y'> PTOVariableConverterForYaw;

typedef PTOVariableConverterSingle<'T','r','X'> PTOVariableConverterForX;
typedef PTOVariableConverterSingle<'T','r','Y'> PTOVariableConverterForY;
typedef PTOVariableConverterSingle<'T','r','Z'> PTOVariableConverterForZ;
typedef PTOVariableConverterSingle<'T','p','y'> PTOVariableConverterForTranslationPlaneYaw;
typedef PTOVariableConverterSingle<'T','p','p'> PTOVariableConverterForTranslationPlanePitch;

typedef PTOVariableConverterSingle<'j'> PTOVariableConverterForStack;

typedef PTOVariableConverterVector<double, 3> PTOVariableConverterForRadialDistortion;
typedef PTOVariableConverterNoOp PTOVariableConverterForRadialDistortionRed;
typedef PTOVariableConverterNoOp PTOVariableConverterForRadialDistortionBlue;
typedef PTOVariableConverterFDiff2D<'d', 'e'> PTOVariableConverterForRadialDistortionCenterShift;
typedef PTOVariableConverterFDiff2D<'g', 't'> PTOVariableConverterForShear;

typedef PTOVariableConverterNoOp PTOVariableConverterForCropMode;
/// @todo This could be S, but it is 4 integers in the form a,b,c,d -hence not a double.
typedef PTOVariableConverterNoOp PTOVariableConverterForCropRect;
typedef PTOVariableConverterNoOp PTOVariableConverterForAutoCenterCrop;

/// @todo This could be Vf, but it is a string, not a double.
typedef PTOVariableConverterNoOp PTOVariableConverterForFlatfieldFilename;
//typedef PTOVariableConverterSingle<'V', 'm', '\0', int> PTOVariableConverterForVigCorrMode;
typedef PTOVariableConverterNoOp PTOVariableConverterForVigCorrMode;
typedef PTOVariableConverterVectorChar<'V', double, 4> PTOVariableConverterForRadialVigCorrCoeff;
typedef PTOVariableConverterFDiff2D<'V','V', 'x','y'> PTOVariableConverterForRadialVigCorrCenterShift;

typedef PTOVariableConverterNoOp PTOVariableConverterForExifModel;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifMake;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifLens;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifCropFactor;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifFocalLength;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifOrientation;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifAperture;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifISO;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifDistance;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifFocalLength35;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifExposureTime;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifDate;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifExposureMode;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifRedBalance;
typedef PTOVariableConverterNoOp PTOVariableConverterForExifBlueBalance;
typedef PTOVariableConverterNoOp PTOVariableConverterForFileMetadata;

#if 0
//panotools variables, currently not used
typedef PTOVariableConverterNoOp PTOVariableConverterForFeatherWidth;
typedef PTOVariableConverterNoOp PTOVariableConverterForMorph;
#endif
typedef PTOVariableConverterNoOp PTOVariableConverterForMasks;
typedef PTOVariableConverterNoOp PTOVariableConverterForActiveMasks;
typedef PTOVariableConverterNoOp PTOVariableConverterForActive;
typedef PTOVariableConverterNoOp PTOVariableConverterForLensNr;
}

#endif
