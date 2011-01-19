/** @file hsi.i
 *
 *  @brief interface definition for the hugin scripting interface
 *
 *  @author Kay F. Jahnke
 *
 */

/*  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  KFJ 2011-01-14
 * 
 */

%module hsi

%{
#define _HUGIN_SCRIPTING_INTERFACE

#include <hugin_shared.h>
#include <iostream>
#include <stdio.h>

#include <panodata/PanoramaData.h>
#include <panodata/Panorama.h>

#include <panotools/PanoToolsUtils.h>
#include <panotools/PanoToolsInterface.h>
#include <panotools/PanoToolsOptimizerWrapper.h>

#include <algorithm/PanoramaAlgorithm.h>
#include <algorithm/ControlPointCreatorAlgorithm.h>
#include <algorithm/StitcherAlgorithm.h>

#include <algorithms/assistant_makefile/AssistantMakefilelibExport.h>
#include <algorithms/basic/CalculateCPStatistics.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <algorithms/basic/CalculateOptimalScale.h>
#include <algorithms/basic/CalculateOverlap.h>
#include <algorithms/basic/RotatePanorama.h>
#include <algorithms/basic/StraightenPanorama.h>
#include <algorithms/basic/TranslatePanorama.h>
#include <algorithms/control_points/CleanCP.h>
#include <algorithms/nona/CalculateFOV.h>
#include <algorithms/nona/CenterHorizontally.h>
#include <algorithms/nona/ComputeImageROI.h>
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/nona/NonaFileStitcher.h>
// #include <algorithms/nona/NonaImageStitcher.h>
// #include <algorithms/optimizer/ImageGraph.h>
#include <algorithms/optimizer/PhotometricOptimizer.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include <algorithms/panorama_makefile/PanoramaMakefilelibExport.h>
#include <algorithms/point_sampler/PointSampler.h>


using namespace std;
using namespace HuginBase;
using namespace AppBase;

extern Panorama * pano_open ( const char * infile ) ;
extern void pano_close ( Panorama * pano ) ;
extern istream * make_std_ifstream ( const char * charp ) ;
extern ostream * make_std_ofstream ( const char * charp ) ;
extern void hello_python ( HuginBase::Panorama * pano ) ;
%}
// we need this to modify a few things in PanoramaVariable.h:

#define _HUGIN_SCRIPTING_INTERFACE

#include <iostream>
#include <stdio.h>

%feature("autodoc", "1") ;
%feature("notabstract") Panorama ;

%import <hugin_shared.h>
%include <appbase/DocumentData.h>
%include <panodata/PanoramaData.h>

using namespace std;
using namespace HuginBase;

// the next bunch of includes pull in what's used of the STL
// note that STL types used in template instantiations, where these
// types which are result of the instantiation are pulled in by
// means of an empty class definition, have to be present here 

%include "std_vector.i"
%include "std_pair.i"
%include "std_map.i"
%include "std_set.i"
%include "std_string.i"

%template(UIntVector) vector<unsigned int>;
%template(UIntSet) set<unsigned int>;

// first we need a few declarations to make swig generate
// proxy classes for some types in the headers which we
// don't want to fully puul in:

// we can't possibly pull in the full vigra interface,
// but just defining these two classes is enough in this context
// since the vigra types are used only to pass in and out
// x,y coordinates:

namespace vigra
{
 class Size2D
  {
   public:
    int x ;
    int y ;
   Size2D ( int ix , int iy ) : x(ix) , y(iy) {} ;
  } ;

 class Point2D
  {
   public:
    int x ;
    int y ;
   Point2D ( int ix , int iy ) : x(ix) , y(iy) {} ;
  } ;
} ;

// same for this type, which is used for a pair of double coords

namespace hugin_utils
{
 class FDiff2D
  {
   public:
    double x ;
    double y ;
   FDiff2D ( double ix , double iy ) : x(ix) , y(iy) {} ;
  } ;
} ;

// in SrcPanoImage.h, there is a facility
// to read out EXIV time stamps into struct tm
// (does this work????)

struct tm
{
  int tm_sec;			/* Seconds.	[0-60] (1 leap second) */
  int tm_min;			/* Minutes.	[0-59] */
  int tm_hour;			/* Hours.	[0-23] */
  int tm_mday;			/* Day.		[1-31] */
  int tm_mon;			/* Month.	[0-11] */
  int tm_year;			/* Year	- 1900.  */
  int tm_wday;			/* Day of week.	[0-6] */
  int tm_yday;			/* Days in year.[0-365]	*/
  int tm_isdst;			/* DST.		[-1/0/1]*/
} ;

// the next section pulls in all the header files we want wrapped.
// The header files often use specified templates, which we need
// to explicitly define here, so that swig can produce a proxy
// for them. The section is ordered by header file.
	 
%template(ImageVector) std::vector<SrcPanoImage>;

%include <panodata/ImageVariable.h>

// the next three currently won't yield. are they needed at all?
// %include <panodata/ImageVariableGroup.h>
// %include <panodata/StandardImageVariableGroups.h>
// %include <panodata/ImageVariableTranslate.h>

%include <panodata/Lens.h>

%include <panodata/SrcPanoImage.h>

%template(VariableMap)       std::map< std::string, Variable>;
%template(VariableMapVector) std::vector< std::map<std::string, Variable> > ;
%template(LensVarMap)        std::map<std::string,LensVariable> ;
%template(OptimizeVector)    std::vector<std::set<std::string> > ;

%include <panodata/PanoramaVariable.h>

%template(CPVector) std::vector<ControlPoint>;
%template(CPoint) std::pair<unsigned int, ControlPoint>;
%template(CPointVector) std::vector< std::pair<unsigned int, ControlPoint> >;

%include <panodata/ControlPoint.h>

%include <panodata/DestPanoImage.h>

%include <panodata/PanoramaOptions.h>

// the next one seems redundant with the one after it?!
%include <panodata/PanoramaData.h>
%include <panodata/Panorama.h>

// include the PT interface

%include <panotools/PanoToolsUtils.h>
%include <panotools/PanoToolsInterface.h>
%include <panotools/PanoToolsOptimizerWrapper.h>

%include <algorithm/PanoramaAlgorithm.h>
%include <algorithm/ControlPointCreatorAlgorithm.h>
%include <algorithm/StitcherAlgorithm.h>

%include <algorithms/assistant_makefile/AssistantMakefilelibExport.h>
%include <algorithms/basic/CalculateCPStatistics.h>
%include <algorithms/basic/CalculateMeanExposure.h>
%include <algorithms/basic/CalculateOptimalROI.h>
%include <algorithms/basic/CalculateOptimalScale.h>
%include <algorithms/basic/CalculateOverlap.h>
%include <algorithms/basic/RotatePanorama.h>
%include <algorithms/basic/StraightenPanorama.h>
%include <algorithms/basic/TranslatePanorama.h>
%include <algorithms/control_points/CleanCP.h>
%include <algorithms/nona/CalculateFOV.h>
%include <algorithms/nona/CenterHorizontally.h>
%include <algorithms/nona/ComputeImageROI.h>
%include <algorithms/nona/FitPanorama.h>
%include <algorithms/nona/NonaFileStitcher.h>
// %include <algorithms/nona/NonaImageStitcher.h>
// %include <algorithms/optimizer/ImageGraph.h>
%include <algorithms/optimizer/PhotometricOptimizer.h>
%include <algorithms/optimizer/PTOptimizer.h>
%include <algorithms/panorama_makefile/PanoramaMakefilelibExport.h>
%include <algorithms/point_sampler/PointSampler.h>


// finally we have a bunch of helper functions that reside in
// hsi.cpp:

extern HuginBase::Panorama * pano_open ( const char * infile ) ;
extern void pano_close ( HuginBase::Panorama * pano ) ;
extern std::istream * make_std_ifstream ( const char * charp ) ;
extern std::ostream * make_std_ofstream ( const char * charp ) ;
extern void hello_python ( HuginBase::Panorama * pano ) ;
