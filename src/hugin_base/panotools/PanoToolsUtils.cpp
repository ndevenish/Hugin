// -*- c-basic-offset: 4 -*-

/** @file panotools/PanoToolsUtils.cpp
 *
 *  @brief Utility calls into PanoTools using CPP interface 
 * 
 *  @author Gerry Patterson <thedeepvoice@gmail.com>
 *
 *  $Id: PanoToolsUtils.cpp 2619 2008-01-11 17:03:09Z dangelo $
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

#include <hugin_config.h>

#include <sstream>
#include <hugin_utils/utils.h>

// libpano includes ------------------------------------------------------------

#include <stdlib.h>

#ifdef _WIN32
// include windows.h with sensible defines, otherwise
// panotools might include with its stupid, commonly
// named macros all over the place.
#define _STLP_VERBOSE_AUTO_LINK
//#define _USE_MATH_DEFINES
#define NOMINMAX
#define VC_EXTRALEAN
#include <windows.h>
#undef DIFFERENCE
#endif

#include "PanoToolsInterface.h"
#include "PanoToolsUtils.h"

namespace HuginBase { namespace PTools {

void calcCtrlPointErrors (PanoramaData& pano) 
{
    if(pano.getNrOfImages()>0 && pano.getNrOfCtrlPoints()>0)
    {
        char * p=setlocale(LC_ALL,NULL);
        char * oldlocale=strdup(p);
        setlocale(LC_ALL,"C");
        UIntSet allImg;
        std::ostringstream scriptbuf;
        fill_set(allImg,0, unsigned(pano.getNrOfImages()-1));
        //create temporary non-empty optimize vector
        OptimizeVector optVec;
        std::set<std::string> opt;
        opt.insert("y");
        for(unsigned int i=0;i<pano.getNrOfImages();i++)
        {
            optVec.push_back(opt);
        };
        pano.printPanoramaScript(scriptbuf, optVec, 
                pano.getOptions(), allImg, true);

        char * script = 0;
        script = strdup(scriptbuf.str().c_str());
        AlignInfo ainf;
        if (ParseScript( script, &ainf ) == 0)
        {
            if( CheckParams( &ainf ) == 0 )
            {
                ainf.fcn = fcnPano;
                SetGlobalPtr( &ainf ); 
                pano.updateCtrlPointErrors( GetAlignInfoCtrlPoints(ainf) );
            }
        }
        setlocale(LC_ALL,oldlocale);
        free(oldlocale);
    };
}


} // PTools namespace
} // HuginBase namespace
