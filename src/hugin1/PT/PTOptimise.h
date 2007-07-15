// -*- c-basic-offset: 4 -*-
/** @file PTOptimise.h
 *
 *  functions to call the optimizer of panotools.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PTOptimise.h 1951 2007-04-15 20:54:49Z dangelo $
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

#ifndef _Hgn1_PTOPTIMISE_H
#define _Hgn1_PTOPTIMISE_H

#include <panotools/PanoToolsOptimizerWrapper.h>
#include <algorithms/optimizer/PTOptimizer.h>

#include "PT/Panorama.h"

namespace PTools
{

    using HuginBase::PTools::optimize;

    void smartOptimize(PT::Panorama & pano)
    {
        HuginBase::SmartOptimise(pano).run();
    }

    void autoOptimise(PT::Panorama & pano)
    {
        HuginBase::AutoOptimise(pano).run();
    }

}




#endif // _PTOPTIMISE_H
