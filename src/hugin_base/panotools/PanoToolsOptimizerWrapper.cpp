// -*- c-basic-offset: 4 -*-

/** @file PanoToolsOptimizerWrapper.cpp
 *
 *  @brief wraps around PTOptimizer
 * 
 *  !!derives from PT/PTOptimise.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PTOptimise.cpp 1945 2007-04-15 20:44:07Z dangelo $
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

// libpano includes ------------------------------------------------------------
extern "C" {
#ifdef __INTEL__
#define __INTELMEMO__
#undef __INTEL__
#endif
    
#ifdef HasPANO13
#include <pano13/panorama.h>
#else
#include <pano12/panorama.h>
#endif
    
#ifdef __INTELMEMO__
#define __INTEL__
#undef __INTELMEMO__
#endif
    
#ifdef HasPANO13
#include <pano13/filter.h>
#else
#include <pano12/filter.h>
#endif
}

// remove stupid #defines from the evil windows.h
#ifdef __WXMSW__
#include <wx/msw/winundef.h>
#undef DIFFERENCE
#ifdef MIN
#undef MIN
#endif
#ifdef MAX
#undef MAX
#endif
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// missing prototype in filter.h
extern "C" {
    int CheckParams( AlignInfo *g );
}

//------------------------------------------------------------------------------

#include <sstream>
#include <hugin_utils/utils.h>

#include "PanoToolsInterface.h"
#include "PanoToolsOptimizerWrapper.h"


//#define DEBUG_WRITE_OPTIM_OUTPUT
//#define DEBUG_WRITE_OPTIM_OUTPUT_FILE "hugin_debug_optim_results.txt"


namespace HuginBase { namespace PTools {

/*
void optimize_PT(const Panorama & pano,
                               const PT::UIntVector &imgs,
                               const OptimizeVector & optvec,
                               VariableMapVector & vars,
                               CPVector & cps,
                               int maxIter)
{
//    VariableMapVector res;
    // setup data structures
    aPrefs    aP;
    OptInfo   opt;

    SetAdjustDefaults(&aP);
    AlignInfoWrap aInfo;
    // copy pano information into libpano data structures
    if (aInfo.setInfo(pano, imgs, vars, cps, optvec)) {
        aInfo.setGlobal();

        opt.numVars 		= aInfo.gl.numParam;
        opt.numData 		= aInfo.gl.numPts;
        opt.SetVarsToX		= SetLMParams;
        opt.SetXToVars		= SetAlignParams;
        opt.fcn			= aInfo.gl.fcn;
        *opt.message		= 0;

        DEBUG_DEBUG("starting optimizer");
        ::RunLMOptimizer( &opt );
        std::ostringstream oss;
        oss << "optimizing images";
        for (UIntVector::const_iterator it = imgs.begin(); it != imgs.end(); ++it) {
            if (it + 1 != imgs.end()) {
                oss << *it << ",";
            } else {
                oss << *it;
            }
        }
        oss << "\n" << opt.message;
        DEBUG_DEBUG("optimizer finished:" << opt.message);

        vars = aInfo.getVariables();
        cps = aInfo.getCtrlPoints();
    }
}
*/

void optimize(PanoramaData& pano,
                      const char * userScript)
{
    char * script = 0;

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
            pano.updateVariables(getAlignInfoVariables(ainf) );
            pano.updateCtrlPointErrors( getAlignInfoCtrlPoints(ainf) );
		}
		DisposeAlignInfo( &ainf );
    }
    if (! userScript) {
        free(script);
    }
}


#if 0
void PTools::optimize(Panorama & pano,
                      utils::MultiProgressDisplay & progDisplay,
                      int maxIter)
{
//    VariableMapVector res;
    // setup data structures
    aPrefs    aP;
    OptInfo   opt;

    SetAdjustDefaults(&aP);
    AlignInfoWrap aInfo;
    // copy pano information int libpano data structures
    if (aInfo.setInfo(pano)) {
        aInfo.setGlobal();

        opt.numVars 		= aInfo.gl.numParam;
        opt.numData 		= aInfo.gl.numPts;
        opt.SetVarsToX		= SetLMParams;
        opt.SetXToVars		= SetAlignParams;
        opt.fcn			= aInfo.gl.fcn;
        *opt.message		= 0;

        DEBUG_DEBUG("starting optimizer");
        RunLMOptimizer( &opt );

#ifdef DEBUG
        fullPath path;
        StringtoFullPath(&path, "c:/debug_optimizer.txt");

		aInfo.gl.data		= opt.message;
        WriteResults( "debug_test", &path, &aInfo.gl, distSquared, 0);
#endif


        std::ostringstream oss;
        /*
        oss << "optimizing images";
        for (UIntVector::const_iterator it = imgs.begin(); it != imgs.end(); ++it) {
            if (it + 1 != imgs.end()) {
                oss << *it << ",";
            } else {
                oss << *it;
            }
        }
        oss << "\n" << opt.message;
        progDisplay.setMessage(oss.str());
        */
        DEBUG_DEBUG("optimizer finished:" << opt.message);

        pano.updateVariables(aInfo.getVariables());
        pano.updateCtrlPointErrors( aInfo.getCtrlPoints());

    }
}
#endif


}} //namespace