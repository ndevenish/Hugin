// -*- c-basic-offset: 4 -*-

/** @file PanoToolsOptimizerWrapper.cpp
 *
 *  @brief wraps around PTOptimizer
 * 
 *  !!derives from PT/PTOptimise.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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
#include "PanoToolsOptimizerWrapper.h"

//------------------------------------------------------------------------------

//#define DEBUG_WRITE_OPTIM_OUTPUT
//#define DEBUG_WRITE_OPTIM_OUTPUT_FILE "hugin_debug_optim_results.txt"

namespace HuginBase { namespace PTools {

unsigned int optimize(PanoramaData& pano,
                      const char * userScript)
{
    char * script = 0;
    unsigned int retval = 0;

    if (userScript == 0) {
        std::ostringstream scriptbuf;
        UIntSet allImg;
        fill_set(allImg,0, unsigned(pano.getNrOfImages()-1));
        pano.printPanoramaScript(scriptbuf, pano.getOptimizeVector(), pano.getOptions(), allImg, true);
        script = strdup(scriptbuf.str().c_str());
    } else {
        script = const_cast<char *>(userScript);
    }

    OptInfo		opt;
	AlignInfo	ainf;

    if (ParseScript( script, &ainf ) == 0)
	{
		if( CheckParams( &ainf ) == 0 )
		{
			ainf.fcn	= fcnPano;
			
			SetGlobalPtr( &ainf ); 
			
			opt.numVars 		= ainf.numParam;
			opt.numData 		= ainf.numPts;
			opt.SetVarsToX		= SetLMParams;
			opt.SetXToVars		= SetAlignParams;
			opt.fcn			= ainf.fcn;
			*opt.message		= 0;

			RunLMOptimizer( &opt );
			ainf.data		= opt.message;
            // get results from align info.
#ifdef DEBUG_WRITE_OPTIM_OUTPUT
            fullPath path;
            StringtoFullPath(&path, DEBUG_WRITE_OPTIM_OUTPUT_FILE );

		    ainf.data		= opt.message;
            WriteResults( script, &path, &ainf, distSquared, 0);
#endif
            pano.updateVariables( GetAlignInfoVariables(ainf) );
            pano.updateCtrlPointErrors( GetAlignInfoCtrlPoints(ainf) );
		} else {
            std::cerr << "Bad params" << std::endl;
            retval = 2;
        }
		DisposeAlignInfo( &ainf );
    } else {
        std::cerr << "Bad params" << std::endl;
        retval = 1;
    }
    if (! userScript) {
        free(script);
    }
    return retval;
}

}} //namespace
