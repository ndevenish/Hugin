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

#ifndef MY_PANO_INC_H
#define MY_PANO_INC_H

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
#include "vigra/stdimage.hxx"
#include "vigra/rgbvalue.hxx"
#include "vigra/basicimage.hxx"
#include "vigra/accessor.hxx"
#include "vigra/iteratortraits.hxx"
#include "vigra/numerictraits.hxx"
#include "vigra/imageiterator.hxx"
#include "vigra/transformimage.hxx"
#include "vigra/copyimage.hxx"
#include "vigra/functorexpression.hxx"
#include "vigra/convolution.hxx"
#include "vigra/resizeimage.hxx"

// =====
// ===== Hugin specific with no dependencies to WX
// =====
#include "common/math.h"
#include "common/utils.h"
#include "common/stl_utils.h"
#include "common/Vector3.h"
#include "common/Matrix3.h"

#include "PT/Panorama.h"
#include "PT/PanoImage.h"
#include "PT/PanoramaMemento.h"
#include "PT/PanoCommand.h"
//#include "PT/Transforms.h"
#include "PT/Interpolators.h"
#include "PT/PanoToolsInterface.h"
#include "PT/SpaceTransform.h"
#include "PT/ImageTransforms.h"

#endif

