// -*- c-basic-offset: 4 -*-

/** @file panoinc.h
 *
 *  @common include file for the hugin project
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
 *
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

#ifndef _PANO_INC_H
#define _PANO_INC_H

// =====
// ===== Standard Includes
// =====
#ifdef __unix__
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <vector>
#include <set>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <math.h>

#ifdef _MSC_VER
#pragma warning (disable : 4267 4355)
#endif

// =====
// ===== VIGRA library
// =====
#include <vigra/stdimage.hxx>
#include <vigra/rgbvalue.hxx>
#include <vigra/basicimage.hxx>
#include <vigra/accessor.hxx>
#include <vigra/iteratortraits.hxx>
#include <vigra/numerictraits.hxx>
#include <vigra/imageiterator.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/copyimage.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/convolution.hxx>
#include <vigra/resizeimage.hxx>

// =====
// ===== Hugin specific with no dependencies to WX
// =====
#include <hugin_utils/utils.h>
#include <hugin_utils/platform.h>
#include <hugin_utils/stl_utils.h>
#include <hugin_math/hugin_math.h>
#include <hugin_math/Vector3.h>
#include <hugin_math/Matrix3.h>

#include <panodata/ControlPoint.h>
#include <panodata/Lens.h>
#include <panodata/Panorama.h>
#include <panodata/PanoramaOptions.h>
#include <panodata/PanoramaVariable.h>
#include <panodata/SrcPanoImage.h>

#endif // _PANO_INC_H

