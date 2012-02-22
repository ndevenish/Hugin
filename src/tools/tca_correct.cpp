// -*- c-basic-offset: 4 -*-

/** @file tca_correct.cpp
 *
 *  @brief program to align a set of well overlapping images (~90%)
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: align_image_stack.cpp 2493 2007-10-24 20:26:26Z dangelo $
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
#include <fstream>
#include <sstream>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>
#include <vigra/cornerdetection.hxx>
#include <vigra/localminmax.hxx>
#include <hugin_utils/utils.h>
#include <hugin_math/hugin_math.h>

#include "vigra/stdimage.hxx"
#include "vigra/stdimagefunctions.hxx"
#include "vigra/functorexpression.hxx"
#include "vigra/transformimage.hxx"

#include <vigra_ext/Pyramid.h>
#include <vigra_ext/Correlation.h>
#include <vigra_ext/InterestPoints.h>
#include <vigra_ext/utils.h>

#include <panodata/Panorama.h>
#include <panodata/StandardImageVariableGroups.h>
#include <panotools/PanoToolsOptimizerWrapper.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include <nona/Stitcher.h>
#include <foreign/levmar/lm.h>

#include <hugin_version.h>

#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif


#include <tiff.h>

using namespace vigra;
using namespace HuginBase;
using namespace AppBase;
using namespace std;
using namespace vigra_ext;
using namespace HuginBase::PTools;
using namespace HuginBase::Nona;
using namespace vigra::functor;

#define DEFAULT_OPTIMISATION_PARAMETER "abcvde"

int g_verbose = 0;

struct Parameters
{
    Parameters()
    {
        cpErrorThreshold = 1.5;
	optMethod = 0;
	load = false;
	reset = false;
        scale=2;
        nPoints=10;
        grid = 10;
    }

    double cpErrorThreshold;
    int optMethod;
    bool load;
    bool reset;
    std::set<std::string> optvars;
    
    std::string alignedPrefix;
    std::string ptoFile;
    std::string ptoOutputFile;
    string basename;

    string red_name;
    string green_name;
    string blue_name;

    double scale;
    int nPoints;
    int grid;
};

Parameters g_param;

struct OptimData
{
    PanoramaData& m_pano;
    double huberSigma;
    const OptimizeVector& m_optvars;

    double m_dist[3][3]; // a,b,c for all imgs
    double m_shift[2];   // x,y shift
    double m_hfov[3];
    double m_center[2];  // center of image (without shift)
    std::vector<double *> m_mapping;

    int m_maxIter;

    OptimData(PanoramaData& pano, const OptimizeVector& optvars,
              double mEstimatorSigma, int maxIter);

    /// copy internal optimization variables into x
    void ToX(double * x)
    {
	for (size_t i=0; i < m_mapping.size(); i++)
    	    x[i] = *(m_mapping[i]);
    }

    /// copy new values from x to internal optimization variables
    void FromX(double * x)
    {
	for (size_t i=0; i < m_mapping.size(); i++)
    	    *(m_mapping[i]) = x[i];
    }
    
    void LoadFromImgs();
    void SaveToImgs();
};

OptimData::OptimData(PanoramaData & pano, const OptimizeVector & optvars,
                     double mEstimatorSigma, int maxIter)
  : m_pano(pano), huberSigma(mEstimatorSigma), m_optvars(optvars), m_maxIter(maxIter)
{
    assert(m_pano.getNrOfImages() == m_optvars.size());
    assert(m_pano.getNrOfImages() == 3);
    LoadFromImgs();

    for (unsigned int i=0 ; i<3 ; i++)
    {
        const std::set<std::string> vars = m_optvars[i];
        for (std::set<std::string>::const_iterator it = vars.begin(); it != vars.end(); ++it)
	{
	    const char var = (*it)[0];
	    if ((var >= 'a') && (var <= 'c'))
		m_mapping.push_back(&(m_dist[i][var - 'a']));
	    else if ((var == 'd') || (var == 'e'))
		m_mapping.push_back(&(m_shift[var - 'd']));
	    else if (var == 'v')
		m_mapping.push_back(&(m_hfov[i]));
	    else
	    {
		cerr << "Unknown parameter detected, ignoring!" << std::endl;
	    }
	}
    }
}

void OptimData::LoadFromImgs()
{
    for (unsigned int i=0; i < 3; i++)
    {
        SrcPanoImage img = m_pano.getSrcImage(i);
	m_hfov[i] = img.getHFOV();
	m_dist[i][0] = img.getRadialDistortion()[0];
	m_dist[i][1] = img.getRadialDistortion()[1];
	m_dist[i][2] = img.getRadialDistortion()[2];
	if (i == 0)
	{
	    m_shift[0] = img.getRadialDistortionCenterShift().x;
	    m_shift[1] = img.getRadialDistortionCenterShift().y;

	    m_center[0] = img.getSize().width()/2.0;
	    m_center[1] = img.getSize().height()/2.0;
	}
    }
}

void OptimData::SaveToImgs()
{
    for (unsigned int i=0; i < 3; i++)
    {
	SrcPanoImage img = m_pano.getSrcImage(i);

	img.setHFOV(m_hfov[i]);

	std::vector<double> radialDist(4);
        radialDist[0] = m_dist[i][0];
        radialDist[1] = m_dist[i][1];
        radialDist[2] = m_dist[i][2];
        radialDist[3] = 1 - radialDist[0] - radialDist[1] - radialDist[2];
        img.setRadialDistortion(radialDist);
                        
        img.setRadialDistortionCenterShift(hugin_utils::FDiff2D(m_shift[0], m_shift[1]));

	m_pano.setSrcImage(i, img);
    }
}

static void usage(const char * name)
{
    cerr << name << ": Parameter estimation of transverse chromatic abberations" << std::endl
         << name << " version " << DISPLAY_VERSION << endl
         << std::endl
         << "Usage: " << name  << " [options] <inputfile>" << std::endl
         << "  option are: " << std::endl
         << "    -h            Display help (this text)" << std::endl
         << "    -l            input file is PTO file instead of image" << std::endl
         << "    -m method     optimization method (0 normal, 1 newfit)" << std::endl
         << "    -o optvars    string of variables to optimize (\"abcvde\")" << std::endl
         << "    -r            reset values (this will zero a,b,c,d,e params and set v to 10)" << std::endl
         << "                  makes sense only with -l option" << std::endl
         << "    -s <scale>    Scale for corner detection" << endl
         << "    -n <number>   number of points per grid cell (default: 10)" << endl
         << "    -g <number>   divide image in <number>x<number> grid cells (default: 10)" << endl
         << "    -t num        Remove all control points with an error higher than num pixels (default: 1.5)" << std::endl
         << "    -v            Verbose" << std::endl
         << "    -w filename   write PTO file" << std::endl 
         << "    -R <r>        Use this file as red channel" << endl
         << "    -G <g>        Use this file as green channel" << endl
         << "    -B <b>        Use this file as blue channel" << endl
         << endl
         << "  <inputfile> is the base name of 4 image files:" << endl
         << "    <inputfile>        Colour file to compute TCA parameters" << endl 
         << "    red_<inputfile>    Red channel of <inputfile>" << endl
         << "    green_<inputfile>  Green channel of <inputfile>" << endl
         << "    blue_<inputfile>   Blue channel of <inputfile>" << endl
         << "    The channel images must be colour images with 3 identical channels." << endl 
         << "    If any of -R, -G, or -B is given, this file name is used instead of the derived name." << endl
         << endl
         << "  Output:" << endl
         << "    commandline arguments for fulla" << endl;
}

/** fine tune a point with normalized cross correlation
 *
 *  takes a patch of \p templSize by \p templSize from \p templImg
 *  images at \p tmplPos and searches it on the \p searchImg, at
 *  \p searchPos, in a neighbourhood of \p sWidth by \p sWidth.
 *
 *  The result in returned in @p tunedPos
 *
 *  @return correlation value
 */
template <class IMAGET, class ACCESSORT, class IMAGES, class ACCESSORS>
CorrelationResult PointFineTune2(const IMAGET & templImg,
				ACCESSORT access_t,
                                vigra::Diff2D templPos,
                                int templSize,
                                const IMAGES & searchImg,
				ACCESSORS access_s,
                                vigra::Diff2D searchPos,
                                int sWidth)
{
//    DEBUG_TRACE("templPos: " vigra::<< templPos << " searchPos: " vigra::<< searchPos);

    // extract patch from template

    int templWidth = templSize/2;
    vigra::Diff2D tmplUL(templPos.x - templWidth, templPos.y-templWidth);
    // lower right iterators "are past the end"
    vigra::Diff2D tmplLR(templPos.x + templWidth + 1, templPos.y + templWidth + 1);
    // clip corners to ensure the template is inside the image.
    vigra::Diff2D tmplImgSize(templImg.size());
    tmplUL = hugin_utils::simpleClipPoint(tmplUL, vigra::Diff2D(0,0), tmplImgSize);
    tmplLR = hugin_utils::simpleClipPoint(tmplLR, vigra::Diff2D(0,0), tmplImgSize);
    vigra::Diff2D tmplSize = tmplLR - tmplUL;
    DEBUG_DEBUG("template position: " << templPos << "  tmplUL: " << tmplUL
		    << "  templLR:" << tmplLR << "  size:" << tmplSize);

    // extract patch from search region
    // make search region bigger, so that interpolation can always be done
    int swidth = sWidth/2 +(2+templWidth);
//    DEBUG_DEBUG("search window half width/height: " << swidth << "x" << swidth);
//    Diff2D subjPoint(searchPos);
    // clip search window
    if (searchPos.x < 0) searchPos.x = 0;
    if (searchPos.x > (int) searchImg.width()) searchPos.x = searchImg.width()-1;
    if (searchPos.y < 0) searchPos.y = 0;
    if (searchPos.y > (int) searchImg.height()) searchPos.x = searchImg.height()-1;

    vigra::Diff2D searchUL(searchPos.x - swidth, searchPos.y - swidth);
    // point past the end
    vigra::Diff2D searchLR(searchPos.x + swidth+1, searchPos.y + swidth+1);
    // clip search window
    vigra::Diff2D srcImgSize(searchImg.size());
    searchUL = hugin_utils::simpleClipPoint(searchUL, vigra::Diff2D(0,0), srcImgSize);
    searchLR = hugin_utils::simpleClipPoint(searchLR, vigra::Diff2D(0,0), srcImgSize);
//    DEBUG_DEBUG("search borders: " << searchLR.x << "," << searchLR.y);

    vigra::Diff2D searchSize = searchLR - searchUL;
    // create output image

//#ifdef VIGRA_EXT_USE_FAST_CORR
    // source input
    vigra::FImage srcImage(searchLR-searchUL);
    vigra::copyImage(vigra::make_triple(searchImg.upperLeft() + searchUL,
                                        searchImg.upperLeft() + searchLR,
                                         access_s),
                     destImage(srcImage) );

    vigra::FImage templateImage(tmplSize);
    vigra::copyImage(vigra::make_triple(templImg.upperLeft() + tmplUL,
                                        templImg.upperLeft() + tmplLR,
                                        access_t),
                     destImage(templateImage));
#ifdef DEBUG_WRITE_FILES
    vigra::ImageExportInfo tmpli("hugin_templ.tif");
    vigra::exportImage(vigra::srcImageRange(templateImage), tmpli);

    vigra::ImageExportInfo srci("hugin_searchregion.tif");
    vigra::exportImage(vigra::srcImageRange(srcImage), srci);
#endif

//#endif

    vigra::FImage dest(searchSize);
    dest.init(-1);
    // we could use the multiresolution version as well.
    // but usually the region is quite small.
    CorrelationResult res;
#ifdef VIGRA_EXT_USE_FAST_CORR
    DEBUG_DEBUG("+++++ starting fast correlation");
    res = correlateImageFast(srcImage,
                             dest,
                             templateImage,
                             tmplUL-templPos, tmplLR-templPos - vigra::Diff2D(1,1),
                             -1);
#else
    DEBUG_DEBUG("+++++ starting normal correlation");
    res = correlateImage(srcImage.upperLeft(),
                         srcImage.lowerRight(),
                         srcImage.accessor(),
                         dest.upperLeft(),
                         dest.accessor(),
                         templateImage.upperLeft() + templPos,
                         templateImage.accessor(),
                         tmplUL, tmplLR, -1);

//     res = correlateImage(searchImg.upperLeft() + searchUL,
//                          searchImg.upperLeft() + searchLR,
//                          searchImg.accessor(),
//                          dest.upperLeft(),
//                          dest.accessor(),
//                          templImg.upperLeft() + templPos,
//                          templImg.accessor(),
//                          tmplUL, tmplLR, -1);
#endif
    DEBUG_DEBUG("normal search finished, max:" << res.maxi
                << " at " << res.maxpos);
    // do a subpixel maxima estimation
    // check if the max is inside the pixel boundaries,
    // and there are enought correlation values for the subpixel
    // estimation, (2 + templWidth)
    if (res.maxpos.x > 2 + templWidth && res.maxpos.x < 2*swidth+1-2-templWidth
        && res.maxpos.y > 2+templWidth && res.maxpos.y < 2*swidth+1-2-templWidth)
    {
        // subpixel estimation
        res = subpixelMaxima(vigra::srcImageRange(dest), res.maxpos.toDiff2D());
        DEBUG_DEBUG("subpixel position: max:" << res.maxi
                    << " at " << res.maxpos);
    } else {
        // not enough values for subpixel estimation.
        DEBUG_DEBUG("subpixel estimation not done, maxima too close to border");
    }

    res.maxpos = res.maxpos + searchUL;
    return res;
}

template <class ImageType>
void createCtrlPoints(Panorama & pano, const ImageType & img, int imgRedNr, int imgGreenNr, int imgBlueNr, double scale, int nPoints, int grid)

//template <class ImageType>
//void createCtrlPoints(Panorama & pano, const ImageType & img, double scale, unsigned nPoints, unsigned grid)
{
    vigra::BasicImage<RGBValue<UInt8> > img8(img.size());

    double ratio = 255.0/vigra_ext::LUTTraits<typename ImageType::value_type>::max();
    transformImage(srcImageRange(img), destImage(img8),
                   Arg1()*Param(ratio));

    std::cout << "image8 size:" << img8.size() << std::endl;
    //////////////////////////////////////////////////
    // find interesting corners using harris corner detector
    typedef std::vector<std::multimap<double, vigra::Diff2D> > MapVector;

    std::vector<std::multimap<double, vigra::Diff2D> >points;
    if (g_verbose > 0) {
        std::cout << "Finding interest points for matching... ";
    }

    vigra_ext::findInterestPointsOnGrid(srcImageRange(img8, GreenAccessor<RGBValue<UInt8> >()),
                                        scale, 5*nPoints, grid, points);

    if (g_verbose > 0) {
        std::cout << "Matching interest points..." << std::endl;
    }
    long templWidth = 29;
    long sWidth = 29 + 11;
    for (MapVector::iterator mit = points.begin(); mit != points.end(); ++mit) {

        int nGood = 0;
        int nBad = 0;
        // loop over all points, starting with the highest corner score
        for (multimap<double, vigra::Diff2D>::reverse_iterator it = (*mit).rbegin();
             it != (*mit).rend();
             ++it)
        {
            if (nGood >= nPoints) {
                // we have enough points, stop
                break;
            }

	    // Green <-> Red

            ControlPoint p1(imgGreenNr, it->second.x, it->second.y, imgRedNr, it->second.x, it->second.y);

            vigra_ext::CorrelationResult res;
            vigra::Diff2D roundP1(hugin_utils::roundi(p1.x1), hugin_utils::roundi(p1.y1));
            vigra::Diff2D roundP2(hugin_utils::roundi(p1.x2), hugin_utils::roundi(p1.y2));

	    res = PointFineTune2(
                img8, GreenAccessor<RGBValue<UInt8> >(),
                roundP1, templWidth,
                img8, RedAccessor<RGBValue<UInt8> >(),
                roundP2, sWidth);

	    if (res.maxi > 0.98)
	    {
        	p1.x1 = roundP1.x;
    		p1.y1 = roundP1.y;
    		p1.x2 = res.maxpos.x;
    		p1.y2 = res.maxpos.y;
                p1.error = res.maxi;
    		pano.addCtrlPoint(p1);
                nGood++;
	    } else {
                nBad++;
            }

	    // Green <-> Blue
            ControlPoint p2(imgGreenNr, it->second.x, it->second.y, imgBlueNr, it->second.x, it->second.y);

            roundP1 = vigra::Diff2D(hugin_utils::roundi(p2.x1), hugin_utils::roundi(p2.y1));
            roundP2 = vigra::Diff2D(hugin_utils::roundi(p2.x2), hugin_utils::roundi(p2.y2));

	    res = PointFineTune2(
                img8, GreenAccessor<RGBValue<UInt8> >(), roundP1, templWidth,
                img8, BlueAccessor<RGBValue<UInt8> >(), roundP2, sWidth);

	    if (res.maxi > 0.98)
	    {
        	p2.x1 = roundP1.x;
    		p2.y1 = roundP1.y;
    		p2.x2 = res.maxpos.x;
    		p2.y2 = res.maxpos.y;
                p2.error = res.maxi;
    		pano.addCtrlPoint(p2);
                nGood++;
	    } else {
                nBad++;
            }
        }
        if (g_verbose > 0) {
            cout << "Number of good matches: " << nGood << ", bad matches: " << nBad << std::endl;
        }
    }
};



template <class ImageType>
void createCtrlPointsOld(Panorama & pano, const ImageType & img, int imgRedNr, int imgGreenNr, int imgBlueNr, double scale, double cornerThreshold)
{
    vigra::BasicImage<RGBValue<UInt8> > img8(img.size());

    double ratio = 255.0/vigra_ext::LUTTraits<typename ImageType::value_type>::max();
    transformImage(srcImageRange(img), destImage(img8),
                   Arg1()*Param(ratio));

    BImage greenCorners(img.size(), vigra::UInt8(0));
    FImage greenCornerResponse(img.size());

    //DEBUG_DEBUG("running corner detector. scale: " << scale << "cornerThreshold" << cornerTreshold);

    // find corner response at scale scale
    vigra::cornerResponseFunction(srcImageRange(img8, GreenAccessor<RGBValue<UInt8> >()),
                                  destImage(greenCornerResponse),
                                  scale);

    //saveScaledImage(leftCornerResponse,"corner_response.png");
    DEBUG_DEBUG("finding local maxima");
    // find local maxima of corner response, mark with 1
    vigra::localMaxima(srcImageRange(greenCornerResponse), destImage(greenCorners), 255);

    if (g_verbose > 1)
        exportImage(srcImageRange(greenCorners), vigra::ImageExportInfo("corner_response_maxima.png"));

    DEBUG_DEBUG("thresholding corner response");
    // threshold corner response to keep only strong corners (above 400.0)
    transformImage(srcImageRange(greenCornerResponse), destImage(greenCornerResponse),
    		   vigra::Threshold<double, double>(
	           cornerThreshold, DBL_MAX, 0.0, 1.0));

    vigra::combineTwoImages(srcImageRange(greenCorners), srcImage(greenCornerResponse),
                            destImage(greenCorners), std::multiplies<float>());

    AppBase::StreamMultiProgressDisplay progress(std::cerr);
    progress.pushTask(AppBase::ProgressTask("finding and tuning points", ""));
    progress.pushTask(AppBase::ProgressTask("progress", "", 1.0/img.size().x, 0.001));
    
    long templWidth = 29;
    long sWidth = 29 + 11;
    DEBUG_DEBUG("selecting points");
    for (int x=0; x < img.size().x; x++ )
    {
        progress.setProgress((double)x/img.size().x);
        for (int y=0; y < img.size().y; y++ )
        {
            if (greenCorners(x,y) == 0) {
                continue;
            }

	    // Green <-> Red
            ControlPoint p1(imgGreenNr, x, y, imgRedNr, x, y);

            vigra_ext::CorrelationResult res;
            vigra::Diff2D roundP1(hugin_utils::roundi(p1.x1), hugin_utils::roundi(p1.y1));
            vigra::Diff2D roundP2(hugin_utils::roundi(p1.x2), hugin_utils::roundi(p1.y2));

	    res = PointFineTune2(
                img8, GreenAccessor<RGBValue<UInt8> >(),
                roundP1, templWidth,
                img8, RedAccessor<RGBValue<UInt8> >(),
                roundP2, sWidth);

	    if (res.maxi > 0.98)
	    {
        	p1.x1 = roundP1.x;
    		p1.y1 = roundP1.y;
    		p1.x2 = res.maxpos.x;
    		p1.y2 = res.maxpos.y;
                p1.error = res.maxi;
    		pano.addCtrlPoint(p1);
	    }

	    // Green <-> Blue
            ControlPoint p2(imgGreenNr, x, y, imgBlueNr, x, y);

            roundP1 = vigra::Diff2D(hugin_utils::roundi(p2.x1), hugin_utils::roundi(p2.y1));
            roundP2 = vigra::Diff2D(hugin_utils::roundi(p2.x2), hugin_utils::roundi(p2.y2));

	    res = PointFineTune2(
                img8, GreenAccessor<RGBValue<UInt8> >(), roundP1, templWidth,
                img8, BlueAccessor<RGBValue<UInt8> >(), roundP2, sWidth);

	    if (res.maxi > 0.98)
	    {
        	p2.x1 = roundP1.x;
    		p2.y1 = roundP1.y;
    		p2.x2 = res.maxpos.x;
    		p2.y2 = res.maxpos.y;
                p2.error = res.maxi;
    		pano.addCtrlPoint(p2);
	    }
        }
    }
    progress.popTask();
    progress.popTask();
}

void get_optvars(OptimizeVector &_retval)
{
    OptimizeVector optvars;
    std::set<std::string> vars = g_param.optvars;
    optvars.push_back(vars);
    optvars.push_back(std::set<std::string>());
    /* NOTE: delete "d" and "e" if they should be optimized,
             they are linked and always will be */
    vars.erase("d");
    vars.erase("e");
    optvars.push_back(vars);
    _retval = optvars;
}

int optimize_old(Panorama & pano)
{
    OptimizeVector optvars;
    get_optvars(optvars);
    pano.setOptimizeVector(optvars);
    PTools::optimize(pano);
    return 0;
}

inline double weightHuber(double x, double sigma)
{
    if (fabs(x) > sigma) {
        x = sqrt(sigma*(2.0*fabs(x) - sigma));
    }
    return x;
}
                    
void optGetError(double *p, double *x, int m, int n, void * data)
{
    int xi = 0 ;

    OptimData * dat = (OptimData *) data;
    dat->FromX(p);

    /* compute new a,b,c,d from a,b,c,v */
    double dist[3][4];
    for (unsigned int i=0 ; i<3 ; i++)
    {
	double scale = dat->m_hfov[1] / dat->m_hfov[i];
	for (unsigned int j=0 ; j<3 ; j++)
	    dist[i][j] = dat->m_dist[i][j]*pow(scale, (int)(4-j));
	dist[i][3] = scale*(1 - dat->m_dist[i][0] - dat->m_dist[i][1] - dat->m_dist[i][2]);
    }

    double center[2];
    center[0] = dat->m_center[0] + dat->m_shift[0];
    center[1] = dat->m_center[1] + dat->m_shift[1];

    double base_size = std::min(dat->m_center[0], dat->m_center[1]);

    double sqerror=0;

    CPVector newCPs;

    unsigned int noPts = dat->m_pano.getNrOfCtrlPoints();
    // loop over all points to calculate the error
    for (unsigned int ptIdx = 0 ; ptIdx < noPts ; ptIdx++)
    {
        const ControlPoint & cp = dat->m_pano.getCtrlPoint(ptIdx);

	double dist_p1 = vigra::hypot(cp.x1 - center[0], cp.y1 - center[1]);
	double dist_p2 = vigra::hypot(cp.x2 - center[0], cp.y2 - center[1]);

	if (cp.image1Nr == 1)
	{
	    double base_dist = dist_p1 / base_size;
	    double corr_dist_p1 = dist[cp.image2Nr][0]*pow(base_dist, 4) +
				  dist[cp.image2Nr][1]*pow(base_dist, 3) +
				  dist[cp.image2Nr][2]*pow(base_dist, 2) +
				  dist[cp.image2Nr][3]*base_dist;
	    corr_dist_p1 *= base_size;
	    x[ptIdx] = corr_dist_p1 - dist_p2;
	}
	else
	{
	    double base_dist = dist_p2 / base_size;
	    double corr_dist_p2 = dist[cp.image1Nr][0]*pow(base_dist, 4) +
				  dist[cp.image1Nr][1]*pow(base_dist, 3) +
				  dist[cp.image1Nr][2]*pow(base_dist, 2) +
				  dist[cp.image1Nr][3]*base_dist;
	    corr_dist_p2 *= base_size;
	    x[ptIdx] = corr_dist_p2 - dist_p1;
	}

        ControlPoint newcp = cp;
	newcp.error = fabs(x[ptIdx]);
        newCPs.push_back(newcp);
	
	dat->m_pano.getCtrlPoint(ptIdx);
	sqerror += x[ptIdx]*x[ptIdx];
        // use huber robust estimator
        if (dat->huberSigma > 0)
    	    x[ptIdx] = weightHuber(x[ptIdx], dat->huberSigma);
    }

    dat->m_pano.updateCtrlPointErrors(newCPs);
}

int optVis(double *p, double *x, int m, int n, int iter, double sqerror, void * data)
{
    return 1;
/*    OptimData * dat = (OptimData *) data;
    char tmp[200];
    tmp[199] = 0;
    double error = sqrt(sqerror/n)*255;
    snprintf(tmp,199, "Iteration: %d, error: %f", iter, error);
    return dat->m_progress.increaseProgress(0.0, tmp) ? 1 : 0 ; */
}

void optimize_new(PanoramaData & pano)
{
    OptimizeVector optvars;
    get_optvars(optvars);

    int nMaxIter = 1000;
    OptimData data(pano, optvars, 0.5, nMaxIter);

    int ret;
    //double opts[LM_OPTS_SZ];
    double info[LM_INFO_SZ];

    // parameters
    int m=data.m_mapping.size();
    vigra::ArrayVector<double> p(m, 0.0);

    // vector for errors
    int n=pano.getNrOfCtrlPoints();
    vigra::ArrayVector<double> x(n, 0.0);

    data.ToX(p.begin());
    if (g_verbose > 0) {
        fprintf(stderr, "Parameters before optimization: ");
        for(int i=0; i<m; ++i)
            fprintf(stderr, "%.7g ", p[i]);
        fprintf(stderr, "\n");
    }

    // covariance matrix at solution
    vigra::DImage cov(m,m);
    // TODO: setup optimization options with some good defaults.
    double optimOpts[5];
    
    optimOpts[0] = 1e-5;   // init mu
    // stop thresholds
    optimOpts[1] = 1e-7;   // ||J^T e||_inf
    optimOpts[2] = 1e-10;   // ||Dp||_2
    optimOpts[3] = 1e-3;   // ||e||_2
    // difference mode
    optimOpts[4] = LM_DIFF_DELTA;

//    data.huberSigma = 0;
    
    ret=dlevmar_dif(&optGetError, &optVis, &(p[0]), &(x[0]), m, n, nMaxIter, /*optimOpts*/ NULL, info, NULL, &(cov(0,0)), &data);  // no jacobian
    // copy to source images (data.m_imgs)
    data.SaveToImgs();
    // calculate error at solution
    data.huberSigma = 0;
    optGetError(&(p[0]), &(x[0]), m, n, &data);
    double error = 0;
    for (int i=0; i<n; i++) {
        error += x[i]*x[i];
    }
    error = sqrt(error/n);

    if (g_verbose > 0) {
        fprintf(stderr, "Levenberg-Marquardt returned %d in %g iter, reason %g\nSolution: ", ret, info[5], info[6]);
        for(int i=0; i<m; ++i)
            fprintf(stderr, "%.7g ", p[i]);
        fprintf(stderr, "\n\nMinimization info:\n");
        for(int i=0; i<LM_INFO_SZ; ++i)
            fprintf(stderr, "%g ", info[i]);
        fprintf(stderr, "\n");
    }
}

int main2(Panorama &pano);

template <class PixelType>
int processImg(const char *filename)
{
    typedef vigra::BasicImage<PixelType> ImageType;
    try
    {
        // load first image
        vigra::ImageImportInfo imgInfo(filename);
        ImageType imgOrig(imgInfo.size());
        vigra::BImage alpha(imgInfo.size());

        int bands = imgInfo.numBands();
        int extraBands = imgInfo.numExtraBands();

	if (!(bands == 3 || bands == 4 && extraBands == 1))
	{
	    cerr << "Unsupported number of bands!";
	    exit(-1);
	}

//        ImageType * leftImg = new ImageType();
        {
            vigra::importImageAlpha(imgInfo, destImage(imgOrig), destImage(alpha));
//            reduceNTimes(leftImgOrig, *leftImg, g_param.pyrLevel);
        }

	Panorama pano;
        // add the first image.to the panorama object
        
        StandardImageVariableGroups variable_groups(pano);
        ImageVariableGroup & lenses = variable_groups.getLenses();
        
        
        string red_name;
        if( g_param.red_name.size())
          red_name=g_param.red_name;
        else
          red_name=std::string("red_")+filename;

        SrcPanoImage srcRedImg(filename);
        srcRedImg.setSize(imgInfo.size());
        srcRedImg.setProjection(SrcPanoImage::RECTILINEAR);
        srcRedImg.setHFOV(10);
        srcRedImg.setExifCropFactor(1);
        srcRedImg.setFilename(red_name);
        int imgRedNr = pano.addImage(srcRedImg);
        lenses.updatePartNumbers();
        lenses.switchParts(imgRedNr, 0);
        
        
        string green_name;
        if( g_param.green_name.size())
          green_name=g_param.green_name;
        else
          green_name=std::string("green_")+filename;

        SrcPanoImage srcGreenImg( filename);
        srcGreenImg.setSize(imgInfo.size());
        srcGreenImg.setProjection(SrcPanoImage::RECTILINEAR);
        srcGreenImg.setHFOV(10);
        srcGreenImg.setExifCropFactor(1);
        srcGreenImg.setFilename(green_name);
        int imgGreenNr = pano.addImage(srcGreenImg);
        lenses.updatePartNumbers();
        lenses.switchParts(imgGreenNr, 0);
        
        
        string blue_name;
        if( g_param.blue_name.size())
          blue_name=g_param.blue_name;
        else
          blue_name=std::string("blue_")+filename;

        SrcPanoImage srcBlueImg( filename);
        srcBlueImg.setSize(imgInfo.size());
        srcBlueImg.setProjection(SrcPanoImage::RECTILINEAR);
        srcBlueImg.setHFOV(10);
        srcBlueImg.setExifCropFactor(1);
        srcBlueImg.setFilename(blue_name);
        int imgBlueNr = pano.addImage(srcBlueImg);
        lenses.updatePartNumbers();
        lenses.switchParts(imgBlueNr, 0);
        
        // lens variables are linked by default. Unlink the field of view and
        // the radial distortion.
        lenses.unlinkVariablePart(ImageVariableGroup::IVE_HFOV, 0);
        lenses.unlinkVariablePart(ImageVariableGroup::IVE_RadialDistortion, 0);
 
        // setup output to be exactly similar to input image
        PanoramaOptions opts;
        opts.setProjection(PanoramaOptions::RECTILINEAR);
        opts.setHFOV(srcGreenImg.getHFOV(), false);
        opts.setWidth(srcGreenImg.getSize().x, false);
        opts.setHeight(srcGreenImg.getSize().y);

        // output to tiff format
        opts.outputFormat = PanoramaOptions::TIFF_m;
        opts.tiff_saveROI = false;
        // m estimator, to be more robust against points on moving objects
        opts.huberSigma = 0.5;
        pano.setOptions(opts);

	createCtrlPoints(pano, imgOrig, imgRedNr, imgGreenNr, imgBlueNr, g_param.scale, g_param.nPoints, g_param.grid);

	main2(pano);
    } catch (std::exception & e) {
        cerr << "ERROR: caught exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int processPTO(const char *filename)
{
    Panorama pano;

    ifstream ptofile(filename);
    if (ptofile.bad()) {
        cerr << "could not open script : " << filename << std::endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(filename));
    AppBase::DocumentData::ReadWriteError err = pano.readData(ptofile);
    if (err != AppBase::DocumentData::SUCCESSFUL) {
        cerr << "error while parsing script: " << filename << std::endl;
        return 1;
    }

    return main2(pano);
}

void resetValues(Panorama &pano)
{
    for (unsigned int i=0; i < 3; i++)
    {
	SrcPanoImage img = pano.getSrcImage(i);

	img.setHFOV(10);

	std::vector<double> radialDist(4);
        radialDist[0] = 0;
        radialDist[1] = 0;
        radialDist[2] = 0;
        radialDist[3] = 1;
        img.setRadialDistortion(radialDist);
                        
        img.setRadialDistortionCenterShift(hugin_utils::FDiff2D(0, 0));

	pano.setSrcImage(i, img);
    }
}

void print_result(Panorama &pano)
{
    double dist[3][3]; // a,b,c for all imgs
    double shift[2];   // x,y shift
    double hfov[3];

    for (unsigned int i=0; i < 3; i++) {
        SrcPanoImage img = pano.getSrcImage(i);
	hfov[i] = img.getHFOV();
	dist[i][0] = img.getRadialDistortion()[0];
	dist[i][1] = img.getRadialDistortion()[1];
	dist[i][2] = img.getRadialDistortion()[2];
	if (i == 0)
	{
	    shift[0] = img.getRadialDistortionCenterShift().x;
	    shift[1] = img.getRadialDistortionCenterShift().y;
	}
    }

    /* compute new a,b,c,d from a,b,c,v */
    double distnew[3][4];
    for (unsigned int i=0 ; i<3 ; i++) {
	double scale = hfov[1] / hfov[i];
	for (unsigned int j=0 ; j<3 ; j++)
	    distnew[i][j] = dist[i][j]*pow(scale, (int)(4-j));
	distnew[i][3] = scale*(1 - dist[i][0] - dist[i][1] - dist[i][2]);
    }

    if ((hugin_utils::roundi(shift[0]) == 0) &&
         hugin_utils::roundi(shift[1]) == 0)
        fprintf(stdout, "-r %.7f:%.7f:%.7f:%.7f "
                        "-b %.7f:%.7f:%.7f:%.7f ",
                distnew[0][0], distnew[0][1], distnew[0][2], distnew[0][3],
                distnew[2][0], distnew[2][1], distnew[2][2], distnew[2][3]);
    else
        fprintf(stdout, "-r %.7f:%.7f:%.7f:%.7f "
                        "-b %.7f:%.7f:%.7f:%.7f "
                        "-x %d:%d\n",
                distnew[0][0], distnew[0][1], distnew[0][2], distnew[0][3],
                distnew[2][0], distnew[2][1], distnew[2][2], distnew[2][3],
                hugin_utils::roundi(shift[0]), hugin_utils::roundi(shift[1]));
}

int main2(Panorama &pano)
{
    if (g_param.reset)
        resetValues(pano);

    for (int i=0 ; i < 10 ; i++) {
        if (g_param.optMethod == 0)
            optimize_old(pano);
        else if(g_param.optMethod == 1)
            optimize_new(pano);

        CPVector cps = pano.getCtrlPoints();
        CPVector newCPs;
        for (int i=0; i < (int)cps.size(); i++) {
            if (cps[i].error < g_param.cpErrorThreshold) {
                newCPs.push_back(cps[i]);
            }
        }
        if (g_verbose > 0) {
            cerr << "Ctrl points before pruning: " << cps.size() << ", after: " << newCPs.size() << std::endl;
        }
        pano.setCtrlPoints(newCPs);

	if (cps.size() == newCPs.size())
	  // no points were removed, do not re-optimize
	  break;
    }

    if (! g_param.ptoOutputFile.empty()) {
	OptimizeVector optvars;
	get_optvars(optvars);
	UIntSet allImgs;
	fill_set(allImgs, 0, pano.getNrOfImages()-1);
	std::ofstream script(g_param.ptoOutputFile.c_str());
	pano.printPanoramaScript(script, optvars, pano.getOptions(), allImgs, true, "");
    }

    print_result(pano);
    return 0;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "hlm:o:rt:vw:R:G:B:s:g:n:";
    int c;
    bool parameter_request_seen=false;

    opterr = 0;

    g_verbose = 0;

    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'l':
            g_param.load = true;
            break;
        case 'm':
            g_param.optMethod = atoi(optarg);
            break;
        case 'o':
	    {
        	char *optptr = optarg;
    		while (*optptr != 0)
        	{
            	    if ((*optptr == 'a') || (*optptr == 'b') ||
            	        (*optptr == 'c') || (*optptr == 'v') || 
            	        (*optptr == 'd') || (*optptr == 'e'))
                  	g_param.optvars.insert(std::string(optptr, 1));
            	    optptr++;
        	}
                parameter_request_seen=true;
    	    }
    	    break;
        case 'r':
            g_param.reset = true;
            break;
        case 't':
            g_param.cpErrorThreshold = atof(optarg);
	    if (g_param.cpErrorThreshold <= 0) {
		cerr << "Invalid parameter: control point error threshold (-t) must be greater than 0" << std::endl;
		return 1;
	    }
            break;
        case 'v':
            g_verbose++;
            break;
        case 'w':
            g_param.ptoOutputFile = optarg;
            break;
        case 'R':
            g_param.red_name = optarg;
            break;
        case 'G':
            g_param.green_name = optarg;
            break;
        case 'B':
            g_param.blue_name = optarg;
            break;
        case 's':
            g_param.scale=atof( optarg);
            break;
        case 'n':
            g_param.nPoints=atoi( optarg);
            break;
        case 'g':
            g_param.grid=atoi(optarg);
            break;
        default:
            cerr << "Invalid parameter: '" << argv[optind-1] << " " << optarg << "'" << std::endl;
            usage(argv[0]);
            return 1;
        }

    if ((argc - optind) != 1) {
        usage(argv[0]);
        return 1;
    }

    // If no parameters were requested to be optimised, we optimize the
    // default parameters.
    if ( !parameter_request_seen)
    {
        for ( const char * dop=DEFAULT_OPTIMISATION_PARAMETER; 
                *dop != 0; ++dop) {
            g_param.optvars.insert( std::string( dop, 1));
        }
    }

    // Program will crash if nothing is to be optimised.
    if ( g_param.optvars.empty()) {
        cerr << "No parameters to optimize." << endl;
        usage(argv[0]);
        return 1;
    }

    if (!g_param.load)
    {
	vigra::ImageImportInfo firstImgInfo(argv[optind]);
        std::string pixelType = firstImgInfo.getPixelType();
	if (pixelType == "UINT8") {
    	    return processImg<RGBValue<UInt8> >(argv[optind]);
	} else if (pixelType == "INT16") {
    	    return processImg<RGBValue<Int16> >(argv[optind]);
	} else if (pixelType == "UINT16") {
    	    return processImg<RGBValue<UInt16> >(argv[optind]);
	} else {
    	    cerr << " ERROR: unsupported pixel type: " << pixelType << std::endl;
    	    return 1;
	}
    }
    else
    {
	return processPTO(argv[optind]);
    }

    return 0;
}

