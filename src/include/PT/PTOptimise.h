// -*- c-basic-offset: 4 -*-
/** @file PTOptimise.h
 *
 *  functions to call the optimizer of panotools.
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

#ifndef _PTOPTIMISE_H
#define _PTOPTIMISE_H

#include "PT/Panorama.h"
#include "PT/PanoToolsInterface.h"

namespace PTools
{

    /** optimize the images specified in this set.
     *
     *  control points between the images should exist.
     */
    PT::VariableMapVector optimisePair(PT::Panorama & pano,
                                   const PT::OptimizeVector & optvec,
                                   unsigned int firstImg,
                                   unsigned int secondImg);

    /** autooptimise the panorama (does local optimisation first) */
    PT::VariableMapVector autoOptimise(PT::Panorama & pano);
}




#endif // _PTOPTIMISE_H
