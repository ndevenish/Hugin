// -*- c-basic-offset: 4 -*-

/** @file SpaceTransform.cpp
 *
 *  @brief implementation of Space Transformation
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
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

#ifndef _Hgn1_SPACETRANSFORM_H
#define _Hgn1_SPACETRANSFORM_H


#include <nona/SpaceTransform.h>

#include "PT/Panorama.h"


namespace PT {

    using HuginBase::Nona::_FuncParams;
    using HuginBase::Nona::trfn;
    using HuginBase::Nona:: _fDesc;
    using HuginBase::Nona::SpaceTransform;
    using HuginBase::Nona::combinePolynom4;
    using HuginBase::Nona::traceImageOutline;
    using HuginBase::Nona::estScaleFactorForFullFrame;
    using HuginBase::Nona::estRadialScaleCrop;

}

#endif
