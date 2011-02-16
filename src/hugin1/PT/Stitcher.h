// -*- c-basic-offset: 4 -*-
/** @file hugin1/PT/Stitcher.h
 *
 *  Contains various routines used for stitching panoramas.
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

#ifndef _Hgn1_STITCHER_H
#define _Hgn1_STITCHER_H

#include <nona/Stitcher.h>

#include "PT/RemappedPanoImage.h"

namespace PT {

    using HuginBase::Nona::estimateBlendingOrder;
    using HuginBase::Nona::Stitcher;
    using HuginBase::Nona::MultiImageRemapper;
    using HuginBase::Nona::TiffMultiLayerRemapper;
    using HuginBase::Nona::AlphaVector;
    using HuginBase::Nona::CalcMaskUnion;
    using HuginBase::Nona::WeightedStitcher;
    using HuginBase::Nona::ReduceToDifferenceFunctor;
    using HuginBase::Nona::ReduceStitcher;
    using HuginBase::Nona::SimpleStitcher;
    using HuginBase::Nona::StackingBlender;
    using HuginBase::Nona::SeamBlender;
    using HuginBase::Nona::DifferenceBlender;
    using HuginBase::Nona::stitchPanoIntern;
    using HuginBase::Nona::stitchPanorama;
    using HuginBase::Nona::stitchPanoGray_8_16;
    using HuginBase::Nona::stitchPanoGray_32_float;
    using HuginBase::Nona::stitchPanoRGB_8_16;
    using HuginBase::Nona::stitchPanoRGB_32_float;

}

#endif // _STITCHER_H
