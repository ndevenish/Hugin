// -*- c-basic-offset: 4 -*-
/** @file platform.h
 *
 *
 *  platform/compiler specific stuff.
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

#ifndef HUGIN_PLATFORM_H
#define HUGIN_PLATFORM_H

#include <math.h>

namespace utils
{

inline double round(double x)
{
    return floor(x+0.5);
}

inline float roundf(float x)
{
    return (float) floor(x+0.5f);
}

inline int roundi(double x)
{
    return (int) floor(x+0.5);
}

};



#endif // HUGIN_PLATFORM_H
