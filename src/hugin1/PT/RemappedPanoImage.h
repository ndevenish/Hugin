// -*- c-basic-offset: 4 -*-
/** @file hugin1/PT/RemappedPanoImage.h
 *
 *  Contains functions to transform whole images.
 *  Can use PTools::Transform or PT::SpaceTransform for the calculations
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

#ifndef _Hgn1_REMAPPEDPANOIMAGE_H
#define _Hgn1_REMAPPEDPANOIMAGE_H

#include <nona/RemappedPanoImage.h>
#include <nona/ImageRemapper.h>

#include <vigra_ext/ResponseTransform.h>
#include "PT/PanoToolsInterface.h"

namespace PT
{

    using HuginBase::Nona::estimateImageAlpha;
    using HuginBase::Nona::estimateImageRect;
    using HuginBase::Nona::RemappedPanoImage;
    using HuginBase::Nona::remapImage;
    using HuginBase::Nona::SingleImageRemapper;
    using HuginBase::Nona::applyFlatfield;
    using HuginBase::Nona::FileRemapper;

};

#endif
