// -*- c-basic-offset: 4 -*-
/** @file Exiv2Helper.h
 *
 *  @brief helper functions to work with Exif data via the exiv2 library
 * 
 *
 *  @author Pablo d'Angelo, T. Modes
 *
 */

/*
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

#ifndef _EXIV2HELPER_H
#define _EXIV2HELPER_H

#include <string>
#include <vector>
#include <exiv2/exif.hpp>

// these functions are intentionally not exposed to public
// they are supposed only be used by SrcPanoImage.cpp for internal use

namespace HuginBase
{
    namespace Exiv2Helper
    {
        // helper function to return a single value
        const double getExiv2ValueDouble(Exiv2::ExifData& exifData, Exiv2::ExifData::const_iterator it);
        const double getExiv2ValueDouble(Exiv2::ExifData& exifData, std::string keyName);
        const std::string getExiv2ValueString(Exiv2::ExifData& exifData, Exiv2::ExifData::const_iterator it);
        const std::string getExiv2ValueString(Exiv2::ExifData& exifData, std::string keyName);
        const long getExiv2ValueLong(Exiv2::ExifData& exifData, Exiv2::ExifData::const_iterator it);
        const long getExiv2ValueLong(Exiv2::ExifData& exifData, std::string keyName);
        
        // read the RedBalance and BlueBalance data from makernotes of different manufactures
        bool readRedBlueBalance(Exiv2::ExifData &exifData, double & redBalance, double & blueBalance);
        // read the crop factor from the EXIF data, code for handling with different variants for saving crop factor
        const double getCropFactor(Exiv2::ExifData &exifData, long width, long height);
        // read the lens name for EXIF or makernotes data
        const std::string getLensName(Exiv2::ExifData &exifData);
    };
};
#endif