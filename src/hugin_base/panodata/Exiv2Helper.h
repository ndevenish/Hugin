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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
        // read a single value/single array from exifData
        bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, long & value);
        bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, float & value);
        bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::string & value);
        bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::vector<float> & values);

        bool getExiv2Value(Exiv2::ExifData& exifData, uint16_t tagID, std::string groupName, std::string & value);
        bool getExiv2Value(Exiv2::ExifData& exifData, uint16_t tagID, std::string groupName, double & value);

        // read the RedBalance and BlueBalance data from makernotes of different manufactures
        bool readRedBlueBalance(Exiv2::ExifData &exifData, double & redBalance, double & blueBalance);
    };
};
#endif