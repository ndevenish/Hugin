// -*- c-basic-offset: 4 -*-
/** @file math.h
 *
 *  @brief misc math function & classes used by other parts
 *         of the program
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: math.h 1952 2007-04-15 20:57:55Z dangelo $
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

#ifndef _Hgn1_MY_MATH_H
#define _Hgn1_MY_MATH_H

#include <hugin_math/hugin_math.h>


namespace utils
{

using hugin_utils::round;
using hugin_utils::roundf;
using hugin_utils::ceili;
using hugin_utils::floori;
using hugin_utils::roundi;

using hugin_utils::TDiff2D;

using hugin_utils::simpleClipPoint;
using hugin_utils::sqr;
using hugin_utils::norm;
using hugin_utils::euclid_dist;
using hugin_utils::sqr_dist;

using hugin_utils::calcCircleROIFromPoints;

} // namespace


#endif // MY_MATH_H
