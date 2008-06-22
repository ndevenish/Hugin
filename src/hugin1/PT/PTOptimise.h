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

#ifndef _Hgn1_PTOPTIMISE_H
#define _Hgn1_PTOPTIMISE_H

#include <panotools/PanoToolsOptimizerWrapper.h>
#include <algorithms/optimizer/PTOptimizer.h>

#include "PT/Panorama.h"
#include "PT/PanoramaMemento.h"
#include "PT/ImageGraph.h"

namespace PTools
{

    using HuginBase::PTools::optimize;

    inline void smartOptimize(PT::Panorama & pano)
    {
        HuginBase::SmartOptimise(pano).run();
    }

    inline void autoOptimise(PT::Panorama & pano)
    {
        HuginBase::AutoOptimise(pano).run();
    }

    typedef HuginBase::SmartOptimizerStub::OptMode OptMode;
    static OptMode OPT_POS=    HuginBase::SmartOptimizerStub::OPT_POS;
    static OptMode OPT_B=      HuginBase::SmartOptimizerStub::OPT_B;
    static OptMode OPT_AC=     HuginBase::SmartOptimizerStub::OPT_AC;
    static OptMode OPT_DE=     HuginBase::SmartOptimizerStub::OPT_DE;
    static OptMode OPT_HFOV=   HuginBase::SmartOptimizerStub::OPT_HFOV;
    static OptMode OPT_GT=     HuginBase::SmartOptimizerStub::OPT_GT;
    static OptMode OPT_VIG=    HuginBase::SmartOptimizerStub::OPT_VIG;
    static OptMode OPT_VIGCENTRE= HuginBase::SmartOptimizerStub::OPT_VIGCENTRE;
    static OptMode OPT_EXP=    HuginBase::SmartOptimizerStub::OPT_EXP;
    static OptMode OPT_WB=     HuginBase::SmartOptimizerStub::OPT_WB;
    static OptMode OPT_RESP=   HuginBase::SmartOptimizerStub::OPT_RESP;
    
    inline PT::OptimizeVector createOptVars(const PT::Panorama& optPano, int mode, unsigned anchorImg=0)
    {
        return HuginBase::SmartOptimizerStub::createOptVars(optPano,mode,anchorImg);
    }
}




#endif // _PTOPTIMISE_H
