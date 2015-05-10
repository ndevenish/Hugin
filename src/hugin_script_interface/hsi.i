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
 *  along with this software. If not, see <http://www.gnu.org/licenses/>.
 *
 *  KFJ 2011-01-14
 * 
 */

%module hsi

%{
#define SWIG_FILE_WITH_INIT

#include <hugin_shared.h>
#include <appbase/DocumentData.h>
// #include <algorithm/ControlPointCreatorAlgorithm.h>
#include <algorithms/basic/CalculateCPStatistics.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <algorithms/basic/CalculateOptimalScale.h>
#include <algorithms/basic/CalculateOverlap.h>
#include <algorithms/basic/RotatePanorama.h>
#include <algorithms/basic/LayerStacks.h>
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
#include <algorithms/point_sampler/PointSampler.h>
#include <algorithms/PanoramaAlgorithm.h>
#include <algorithms/StitcherAlgorithm.h>
#include <panodata/ImageVariableGroup.h>
#include <panodata/ImageVariableTranslate.h>
#include <panodata/PanoramaData.h>
#include <panodata/Panorama.h>
#include <panodata/SrcPanoImage.h>
#include <panodata/StandardImageVariableGroups.h>
#include <panotools/PanoToolsInterface.h>
#include <panotools/PanoToolsOptimizerWrapper.h>
#include <panotools/PanoToolsUtils.h>
#include <fstream>

using namespace std;
using namespace HuginBase;
%}

#include <ios>
#include <iostream>
using namespace std;

// make slightly nicer docstrings with information about argument
// types

%feature("autodoc", "1") ;

// I suppose somewhere there must be a pure virtual function in the
// base classes of Panorama which isn't overloaded. But I can't find
// it. So I tell SWIG to allow making of Panorama objects even though
// it thinks that Panorama really is abstract:
    
%feature("notabstract") Panorama ;

// the next bunch of includes pull in what's used of the STL
// note that STL types used in template instantiations, where these
// types which are result of the instantiation are pulled in by
// means of an empty class definition, have to be present here 

%include "std_vector.i"
%include "std_pair.i"
%include "std_map.i"
%include "std_set.i"
%include "std_string.i"

// many aguments in hugin are C++ streams, so we need some
// support for them. SWIG has stream support, but not for
// fstreams. We only need to open them, though. Inheriting
// the fstreams from i/o stream gives us all the infrastructure

%include "std_ios.i"
%include "std_iostream.i"

namespace std {

// we wrap opening fstreams with a filename

class ifstream: public std::istream
{
  public: ifstream(char* filename) ;
};

class ofstream: public std::ostream
{
  public: ofstream(char* filename) ;
};

} // namespace std

// types used for groups of uints, like image numbers

%template(UIntVector) vector<unsigned int>;
%template(UIntSet)    set<unsigned int>;
%template(DoubleVector) vector<double>;

// we need a few declarations to make swig generate
// proxy classes for some types in the headers which we
// don't want to fully pull in:

// we don't want to pull in the full vigra interface,
// but just defining these classes is enough in this context
// since the vigra types are used only to pass in and out
// 2D coordinates:

namespace vigra
{
 class Size2D
  {
   public:
   int x, y;
   Size2D() ;
   Size2D ( int ix , int iy ) : x(ix) , y(iy) ;
   int width() const ;
   int height() const ;
   void setWidth(int w) ;
   void setHeight(int h) ;
  } ;

 class Point2D
  {
   public:
   int x, y;
   Point2D() ;
   Point2D ( int ix , int iy ) : x(ix) , y(iy) ;
  } ;

  class Rect2D
  {
   public:
    Rect2D() ;
    Rect2D ( int l , int t , int r , int b ) ;
    Point2D upperLeft() ;
    Point2D lowerRight() ;
    int left() ;
    int top() ;
    int right() ;
    int bottom() ;
    int width() ;
    int height() ;
   } ;
} ;

// same for this type, which is used for a pair of double coordinates

namespace hugin_utils
{
 class FDiff2D
  {
   public:
   double x, y;
   FDiff2D() ;
   FDiff2D ( double ix , double iy ) : x(ix) , y(iy) ;
  } ;
} ;

// in SrcPanoImage.h, there is a facility
// to read out EXIV time stamps into struct tm
// this definition makes the individual fields accessible:

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
// for them. The section is ordered roughly by header file, but it's
// been an iterative process, so the sequence might be changed.
	 
%import <hugin_shared.h>

%include <appbase/DocumentData.h>
%include <hsi_PanoramaData.h>

using namespace HuginBase;

// some types which are derived from templates need explicit
// instatiation to be wrapped by SWIG.

%template(CPoint)            std::pair<unsigned int, ControlPoint>;
%template(CPointVector)      std::vector< std::pair<unsigned int, ControlPoint> >;
%template(CPVector)          std::vector<ControlPoint>;
%template(ImageVector)       std::vector<SrcPanoImage>;
%template(LensVarMap)        std::map<std::string,LensVariable> ;
%template(OptimizeVector)    std::vector<std::set<std::string> > ;
%template(VariableMap)       std::map< std::string, Variable>;
%template(VariableMapVector) std::vector< std::map<std::string, Variable> > ;

%template(MaskPolygonVector) std::vector< MaskPolygon >;

%include <panodata/ImageVariable.h>
%include <panodata/ImageVariableTranslate.h>
%include <hsi_ImageVariableGroup.h>
%include <panodata/StandardImageVariableGroups.h>

%include <panodata/PanoramaVariable.h>
%include <panodata/PanoramaOptions.h>

%include <hsi_SrcPanoImage.h>
// %include <panodata/DestPanoImage.h>
%include <panodata/ControlPoint.h>
%include <panodata/Lens.h>
%include <panodata/Mask.h>

%include <hsi_Panorama.h>

%include <panotools/PanoToolsInterface.h>
%include <panotools/PanoToolsOptimizerWrapper.h>
%include <panotools/PanoToolsUtils.h>

// %include <algorithm/ControlPointCreatorAlgorithm.h>
%include <algorithms/PanoramaAlgorithm.h>
%include <algorithms/StitcherAlgorithm.h>
%include <algorithms/basic/CalculateCPStatistics.h>
%include <algorithms/basic/CalculateMeanExposure.h>
%include <algorithms/basic/CalculateOptimalROI.h>
%include <algorithms/basic/CalculateOptimalScale.h>
%include <algorithms/basic/CalculateOverlap.h>
%include <algorithms/basic/RotatePanorama.h>
%include <algorithms/basic/LayerStacks.h>
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
%include <algorithms/optimizer/PTOptimizer.h>
%include <algorithms/optimizer/PhotometricOptimizer.h>
%include <algorithms/point_sampler/PointSampler.h>
