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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <hugin_config.h>
#include <fstream>
#include <sstream>
#include <iostream>

#include <vigra/error.hxx>
#include <vigra_ext/impexalpha.hxx>
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
#include <foreign/levmar/levmar.h>
#include <hugin_utils/openmp_lock.h>
#include <lensdb/LensDB.h>

#include <getopt.h>
#ifndef WIN32
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

struct Parameters
{
    Parameters()
    {
        cpErrorThreshold = 1.5;
        optMethod = 0;
        load = false;
        reset = false;
        saveDB = false;
        scale=2;
        nPoints=10;
        grid = 10;
        verbose = 0;
    }

    double cpErrorThreshold;
    int optMethod;
    bool load;
    bool reset;
    bool saveDB;
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
    unsigned grid;
    int verbose;
};

Parameters g_param;

// Optimiser code
struct OptimData
{
    PanoramaData& m_pano;
    double huberSigma;
    const OptimizeVector& m_optvars;

    double m_dist[3][3]; // a,b,c for all imgs
    double m_shift[2];   // x,y shift
    double m_hfov[3];
    double m_center[2];  // center of image (without shift)
    std::vector<double*> m_mapping;

    int m_maxIter;

    OptimData(PanoramaData& pano, const OptimizeVector& optvars,
        double mEstimatorSigma, int maxIter)
        : m_pano(pano), huberSigma(mEstimatorSigma), m_optvars(optvars), m_maxIter(maxIter)
    {
        assert(m_pano.getNrOfImages() == m_optvars.size());
        assert(m_pano.getNrOfImages() == 3);
        LoadFromImgs();

        for (unsigned int i = 0; i<3; i++)
        {
            const std::set<std::string> vars = m_optvars[i];
            for (std::set<std::string>::const_iterator it = vars.begin(); it != vars.end(); ++it)
            {
                const char var = (*it)[0];
                if ((var >= 'a') && (var <= 'c'))
                {
                    m_mapping.push_back(&(m_dist[i][var - 'a']));
                }
                else if ((var == 'd') || (var == 'e'))
                {
                    m_mapping.push_back(&(m_shift[var - 'd']));
                }
                else if (var == 'v')
                {
                    m_mapping.push_back(&(m_hfov[i]));
                }
                else
                {
                    cerr << "Unknown parameter detected, ignoring!" << std::endl;
                }
            }
        }
    }

    /// copy internal optimization variables into x
    void ToX(double* x)
    {
        for (size_t i=0; i < m_mapping.size(); i++)
        {
            x[i] = *(m_mapping[i]);
        }
    }

    /// copy new values from x to internal optimization variables
    void FromX(double* x)
    {
        for (size_t i=0; i < m_mapping.size(); i++)
        {
            *(m_mapping[i]) = x[i];
        }
    }

    void LoadFromImgs()
    {
        for (unsigned int i = 0; i < 3; i++)
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
                m_center[0] = img.getSize().width() / 2.0;
                m_center[1] = img.getSize().height() / 2.0;
            }
        }
    };
    void SaveToImgs()
    {
        for (unsigned int i = 0; i < 3; i++)
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
    };
};

void get_optvars(OptimizeVector& _retval)
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

// dummy panotools progress functions
static int ptProgress(int command, char* argument)
{
    return 1;
}
static int ptinfoDlg(int command, char* argument)
{
    return 1;
}

// Method 0: using PTOptimizer
//   PTOptimizer minimizes the tangential and sagittal distance between the points
int optimize_old(Panorama& pano)
{
    if (g_param.verbose == 0)
    {
        // deactive PTOptimizer status information if -v is not given
        PT_setProgressFcn(ptProgress);
        PT_setInfoDlgFcn(ptinfoDlg);
    };

    OptimizeVector optvars;
    get_optvars(optvars);
    pano.setOptimizeVector(optvars);
    PTools::optimize(pano);
    return 0;
}

inline double weightHuber(double x, double sigma)
{
    if (fabs(x) > sigma)
    {
        x = sqrt(sigma*(2.0*fabs(x) - sigma));
    }
    return x;
}

void optGetError(double* p, double* x, int m, int n, void* data)
{
    OptimData* dat = static_cast<OptimData*>(data);
    dat->FromX(p);

    /* compute new a,b,c,d from a,b,c,v */
    double dist[3][4];
    for (unsigned int i = 0; i<3; i++)
    {
        double scale = dat->m_hfov[1] / dat->m_hfov[i];
        for (unsigned int j = 0; j<3; j++)
        {
            dist[i][j] = dat->m_dist[i][j] * pow(scale, (int)(4 - j));
        }
        dist[i][3] = scale*(1 - dat->m_dist[i][0] - dat->m_dist[i][1] - dat->m_dist[i][2]);
    }

    double center[2];
    center[0] = dat->m_center[0] + dat->m_shift[0];
    center[1] = dat->m_center[1] + dat->m_shift[1];

    double base_size = std::min(dat->m_center[0], dat->m_center[1]);

    CPVector newCPs;

    unsigned int noPts = dat->m_pano.getNrOfCtrlPoints();
    // loop over all points to calculate the error
    for (unsigned int ptIdx = 0; ptIdx < noPts; ptIdx++)
    {
        const ControlPoint& cp = dat->m_pano.getCtrlPoint(ptIdx);

        double dist_p1 = vigra::hypot(cp.x1 - center[0], cp.y1 - center[1]);
        double dist_p2 = vigra::hypot(cp.x2 - center[0], cp.y2 - center[1]);

        if (cp.image1Nr == 1)
        {
            double base_dist = dist_p1 / base_size;
            double corr_dist_p1 = dist[cp.image2Nr][0] * pow(base_dist, 4) +
                dist[cp.image2Nr][1] * pow(base_dist, 3) +
                dist[cp.image2Nr][2] * pow(base_dist, 2) +
                dist[cp.image2Nr][3] * base_dist;
            corr_dist_p1 *= base_size;
            x[ptIdx] = corr_dist_p1 - dist_p2;
        }
        else
        {
            double base_dist = dist_p2 / base_size;
            double corr_dist_p2 = dist[cp.image1Nr][0] * pow(base_dist, 4) +
                dist[cp.image1Nr][1] * pow(base_dist, 3) +
                dist[cp.image1Nr][2] * pow(base_dist, 2) +
                dist[cp.image1Nr][3] * base_dist;
            corr_dist_p2 *= base_size;
            x[ptIdx] = corr_dist_p2 - dist_p1;
        }

        ControlPoint newcp = cp;
        newcp.error = fabs(x[ptIdx]);
        newCPs.push_back(newcp);

        dat->m_pano.getCtrlPoint(ptIdx);
        // use huber robust estimator
        if (dat->huberSigma > 0)
        {
            x[ptIdx] = weightHuber(x[ptIdx], dat->huberSigma);
        }
    }

    dat->m_pano.updateCtrlPointErrors(newCPs);
}

int optVis(double* p, double* x, int m, int n, int iter, double sqerror, void* data)
{
    return 1;
    /*    OptimData * dat = (OptimData *) data;
    char tmp[200];
    tmp[199] = 0;
    double error = sqrt(sqerror/n)*255;
    snprintf(tmp,199, "Iteration: %d, error: %f", iter, error);
    return dat->m_progress.increaseProgress(0.0, tmp) ? 1 : 0 ; */
}

// Method 1: minimize only the center distance difference (sagittal distance) of the points
//   the tangential distance is not of interest for TCA correction, 
//   and is caused by the limited accuracy of the fine tune function, especially close the the edge of the fisheye image
void optimize_new(PanoramaData& pano)
{
    OptimizeVector optvars;
    get_optvars(optvars);

    int nMaxIter = 1000;
    OptimData data(pano, optvars, 0.5, nMaxIter);

    int ret;
    double info[LM_INFO_SZ];

    // parameters
    int m = data.m_mapping.size();
    vigra::ArrayVector<double> p(m, 0.0);

    // vector for errors
    int n = pano.getNrOfCtrlPoints();
    vigra::ArrayVector<double> x(n, 0.0);

    data.ToX(p.begin());
    if (g_param.verbose > 0)
    {
        fprintf(stderr, "Parameters before optimization: ");
        for (int i = 0; i<m; ++i)
        {
            fprintf(stderr, "%.7g ", p[i]);
        }
        fprintf(stderr, "\n");
    }

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

    ret = dlevmar_dif(&optGetError, &optVis, &(p[0]), &(x[0]), m, n, nMaxIter, NULL, info, NULL, NULL, &data);  // no jacobian
    // copy to source images (data.m_imgs)
    data.SaveToImgs();
    // calculate error at solution
    data.huberSigma = 0;
    optGetError(&(p[0]), &(x[0]), m, n, &data);
    double error = 0;
    for (int i = 0; i<n; i++)
    {
        error += x[i] * x[i];
    }
    error = sqrt(error / n);

    if (g_param.verbose > 0)
    {
        fprintf(stderr, "Levenberg-Marquardt returned %d in %g iter, reason %g\nSolution: ", ret, info[5], info[6]);
        for (int i = 0; i<m; ++i)
        {
            fprintf(stderr, "%.7g ", p[i]);
        }
        fprintf(stderr, "\n\nMinimization info:\n");
        for (int i = 0; i<LM_INFO_SZ; ++i)
        {
            fprintf(stderr, "%g ", info[i]);
        }
        fprintf(stderr, "\n");
    }
}

static void usage(const char* name)
{
    cerr << name << ": Parameter estimation of transverse chromatic abberations" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << endl
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
         << "    --save-into-database  Saves the tca data into Hugin lens database" << std::endl
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

static hugin_omp::Lock lock;
typedef std::multimap<double, vigra::Diff2D> MapPoints;

template <class ImageType>
void createCtrlPoints(Panorama& pano, const ImageType& img, int imgRedNr, int imgGreenNr, int imgBlueNr, double scale, int nPoints, unsigned grid)
{
    vigra::BasicImage<RGBValue<UInt8> > img8(img.size());

    double ratio = 255.0/vigra_ext::LUTTraits<typename ImageType::value_type>::max();
    transformImage(srcImageRange(img), destImage(img8),
                   Arg1()*Param(ratio));
    if (g_param.verbose > 0)
    {
        std::cout << "image8 size:" << img8.size() << std::endl;
    };
    //////////////////////////////////////////////////
    // find interesting corners using harris corner detector
    typedef std::vector<std::multimap<double, vigra::Diff2D> > MapVector;

    if (g_param.verbose > 0)
    {
        std::cout << "Finding control points... " << std::endl;
    }
    const long templWidth = 29;
    const long sWidth = 29 + 11;


    vigra::Size2D size(img8.width(), img8.height());
    std::vector<vigra::Rect2D> rects;
    for (unsigned party = 0; party < grid; party++)
    {
        for (unsigned partx = 0; partx < grid; partx++)
        {
            // run corner detector only in current sub-region (saves a lot of memory for big images)
            vigra::Rect2D rect(partx*size.x / grid, party*size.y / grid,
                (partx + 1)*size.x / grid, (party + 1)*size.y / grid);
            rect &= vigra::Rect2D(size);
            if (rect.width()>0 && rect.height()>0)
            {
                rects.push_back(rect);
            };
        };
    };

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < rects.size(); ++i)
    {
        MapPoints points;
        vigra::Rect2D rect(rects[i]);
        vigra_ext::findInterestPointsPartial(srcImageRange(img8, GreenAccessor<RGBValue<UInt8> >()), rect, scale, 5 * nPoints, points);

        // loop over all points, starting with the highest corner score
        CPVector cps;
        size_t nBad = 0;
        for (MapPoints::const_reverse_iterator it = points.rbegin(); it != points.rend(); ++it)
        {
            if (cps.size() >= nPoints)
            {
                // we have enough points, stop
                break;
            };

            // Green <-> Red
            ControlPoint p1(imgGreenNr, it->second.x, it->second.y, imgRedNr, it->second.x, it->second.y);
            vigra::Diff2D roundP1(hugin_utils::roundi(p1.x1), hugin_utils::roundi(p1.y1));
            vigra::Diff2D roundP2(hugin_utils::roundi(p1.x2), hugin_utils::roundi(p1.y2));

            vigra_ext::CorrelationResult res = PointFineTune(
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
                cps.push_back(p1);
            }
            else
            {
                ++nBad;
            };

            // Green <-> Blue
            ControlPoint p2(imgGreenNr, it->second.x, it->second.y, imgBlueNr, it->second.x, it->second.y);
            roundP1 = vigra::Diff2D(hugin_utils::roundi(p2.x1), hugin_utils::roundi(p2.y1));
            roundP2 = vigra::Diff2D(hugin_utils::roundi(p2.x2), hugin_utils::roundi(p2.y2));

            res = PointFineTune(
                img8, GreenAccessor<RGBValue<UInt8> >(), roundP1, templWidth,
                img8, BlueAccessor<RGBValue<UInt8> >(), roundP2, sWidth);

            if (res.maxi > 0.98)
            {
                p2.x1 = roundP1.x;
                p2.y1 = roundP1.y;
                p2.x2 = res.maxpos.x;
                p2.y2 = res.maxpos.y;
                p2.error = res.maxi;
                cps.push_back(p2);
            }
            else
            {
                ++nBad;
            };
        }
        if (g_param.verbose > 0)
        {
            std::ostringstream buf;
            buf << "Number of good matches: " << cps.size() << ", bad matches: " << nBad << std::endl;
            cout << buf.str();
        }
        if (!cps.empty())
        {
            hugin_omp::ScopedLock sl(lock);
            for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it)
            {
                pano.addCtrlPoint(*it);
            };
        };
    };
};

int main2(Panorama& pano);

template <class PixelType>
int processImg(const char* filename)
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

        if (!(bands == 3 || (bands == 4 && extraBands == 1)))
        {
            cerr << "Unsupported number of bands!";
            exit(-1);
        }

        vigra::importImageAlpha(imgInfo, destImage(imgOrig), destImage(alpha));
        Panorama pano;
        // add the first image to the panorama object
        StandardImageVariableGroups variable_groups(pano);
        ImageVariableGroup& lenses = variable_groups.getLenses();

        string red_name;
        if( g_param.red_name.size())
        {
            red_name=g_param.red_name;
        }
        else
        {
            red_name=std::string("red_")+filename;
        }

        SrcPanoImage srcRedImg;
        srcRedImg.setSize(imgInfo.size());
        srcRedImg.setProjection(SrcPanoImage::RECTILINEAR);
        srcRedImg.setHFOV(10);
        srcRedImg.setCropFactor(1);
        srcRedImg.setFilename(red_name);
        int imgRedNr = pano.addImage(srcRedImg);
        lenses.updatePartNumbers();
        lenses.switchParts(imgRedNr, 0);

        string green_name;
        if( g_param.green_name.size())
        {
            green_name=g_param.green_name;
        }
        else
        {
            green_name=std::string("green_")+filename;
        }

        SrcPanoImage srcGreenImg;
        srcGreenImg.setSize(imgInfo.size());
        srcGreenImg.setProjection(SrcPanoImage::RECTILINEAR);
        srcGreenImg.setHFOV(10);
        srcGreenImg.setCropFactor(1);
        srcGreenImg.setFilename(green_name);
        int imgGreenNr = pano.addImage(srcGreenImg);
        lenses.updatePartNumbers();
        lenses.switchParts(imgGreenNr, 0);

        string blue_name;
        if( g_param.blue_name.size())
        {
            blue_name=g_param.blue_name;
        }
        else
        {
            blue_name=std::string("blue_")+filename;
        }

        SrcPanoImage srcBlueImg;
        srcBlueImg.setSize(imgInfo.size());
        srcBlueImg.setProjection(SrcPanoImage::RECTILINEAR);
        srcBlueImg.setHFOV(10);
        srcBlueImg.setCropFactor(1);
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
    }
    catch (std::exception& e)
    {
        cerr << "ERROR: caught exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int processPTO(const char* filename)
{
    Panorama pano;

    ifstream ptofile(filename);
    if (ptofile.bad())
    {
        cerr << "could not open script : " << filename << std::endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(filename));
    AppBase::DocumentData::ReadWriteError err = pano.readData(ptofile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        cerr << "error while parsing script: " << filename << std::endl;
        return 1;
    }

    return main2(pano);
}

void resetValues(Panorama& pano)
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

void print_result(Panorama& pano)
{
    double dist[3][3]; // a,b,c for all imgs
    double shift[2];   // x,y shift
    double hfov[3];

    for (unsigned int i=0; i < 3; i++)
    {
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
    for (unsigned int i=0 ; i<3 ; i++)
    {
        double scale = hfov[1] / hfov[i];
        for (unsigned int j=0 ; j<3 ; j++)
        {
            distnew[i][j] = dist[i][j]*pow(scale, (int)(4-j));
        }
        distnew[i][3] = scale*(1 - dist[i][0] - dist[i][1] - dist[i][2]);
    }

    if (hugin_utils::roundi(shift[0]) == 0 && hugin_utils::roundi(shift[1]) == 0)
    {
        fprintf(stdout, "-r %.7f:%.7f:%.7f:%.7f "
            "-b %.7f:%.7f:%.7f:%.7f ",
            distnew[0][0], distnew[0][1], distnew[0][2], distnew[0][3],
            distnew[2][0], distnew[2][1], distnew[2][2], distnew[2][3]);
    }
    else
    {
        fprintf(stdout, "-r %.7f:%.7f:%.7f:%.7f "
            "-b %.7f:%.7f:%.7f:%.7f "
            "-x %d:%d\n",
            distnew[0][0], distnew[0][1], distnew[0][2], distnew[0][3],
            distnew[2][0], distnew[2][1], distnew[2][2], distnew[2][3],
            hugin_utils::roundi(shift[0]), hugin_utils::roundi(shift[1]));
    };
    if (g_param.saveDB)
    {
        // save information into database
        // read EXIF data, using routines of HuginBase::SrcPanoImage
        SrcPanoImage img;
        img.setFilename(g_param.basename);
        img.readEXIF();
        std::string lensname = img.getDBLensName();
        if (lensname.empty())
        {
            // no suitable lensname found in exif data, ask user
            std::cout << std::endl << "For saving information tca data into database no suitable information" << std::endl
                << "found in EXIF data." << std::endl
                << "For fixed lens cameras leave lensname empty." << std::endl << std::endl << "Lensname: ";
            char input[256];
            std::cin.getline(input, 255);
            lensname = hugin_utils::StrTrim(std::string(input));
            if (lensname.empty())
            {
                std::string camMaker;
                std::string camModel;
                std::cout << "Camera maker: ";
                while (camMaker.empty())
                {
                    std::cin.getline(input, 255);
                    camMaker = hugin_utils::StrTrim(std::string(input));
                };
                std::cout << "Camera model: ";
                while (camModel.empty())
                {
                    std::cin.getline(input, 255);
                    camModel = hugin_utils::StrTrim(std::string(input));
                };
                lensname = camMaker.append("|").append(camModel);
            };
        };
        double focal = img.getExifFocalLength();
        if (fabs(focal) < 0.1f)
        {
            std::cout << "Real focal length (in mm): ";
            while (fabs(focal) < 0.1f)
            {
                std::cin >> focal;
                focal = fabs(focal);
            };
        };
        HuginBase::LensDB::LensDB& lensDB = HuginBase::LensDB::LensDB::GetSingleton();
        std::vector<double> redDistData(4, 0.0f);
        std::vector<double> blueDistData(4, 0.0f);
        redDistData[0] = distnew[0][0];
        redDistData[1] = distnew[0][1];
        redDistData[2] = distnew[0][2];
        redDistData[3] = distnew[0][3];
        blueDistData[0] = distnew[2][0];
        blueDistData[1] = distnew[2][1];
        blueDistData[2] = distnew[2][2];
        blueDistData[3] = distnew[2][3];
        if (lensDB.SaveTCA(lensname, focal, redDistData, blueDistData))
        {
            std::cout << std::endl << std::endl << "TCA data for " << lensname << " @ " << focal << " mm successful saved into database." << std::endl;
        }
        else
        {
            std::cout << std::endl << std::endl << "Could not save data into database." << std::endl;
        };
        HuginBase::LensDB::LensDB::Clean();
    }
}

int main2(Panorama& pano)
{
    if (g_param.reset)
    {
        resetValues(pano);
    }

    for (int i = 0; i < 10; i++)
    {
        if (g_param.optMethod == 0)
        {
            optimize_old(pano);
        }
        else if (g_param.optMethod == 1)
        {
            optimize_new(pano);
        }
        else
        {
            cerr << "Unknown optimizer strategy." << endl
                << "Using newfit method." << endl;
            g_param.optMethod = 1;
            optimize_new(pano);
        };

        CPVector cps = pano.getCtrlPoints();
        CPVector newCPs;
        for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it)
        {
            if (it->error < g_param.cpErrorThreshold)
            {
                newCPs.push_back(*it);
            }
        }
        if (g_param.verbose > 0)
        {
            cerr << "Ctrl points before pruning: " << cps.size() << ", after: " << newCPs.size() << std::endl;
        }
        pano.setCtrlPoints(newCPs);

        if (cps.size() == newCPs.size())
        {
            // no points were removed, do not re-optimize
            break;
        }
    }

    if (!g_param.ptoOutputFile.empty())
    {
        OptimizeVector optvars;
        get_optvars(optvars);
        UIntSet allImgs;
        fill_set(allImgs, 0, pano.getNrOfImages() - 1);
        std::ofstream script(g_param.ptoOutputFile.c_str());
        pano.printPanoramaScript(script, optvars, pano.getOptions(), allImgs, true, "");
    }

    print_result(pano);
    return 0;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "hlm:o:rt:vw:R:G:B:s:g:n:";
    enum
    {
        SAVEDATABASE=1000
    };
    static struct option longOptions[] =
    {
        { "save-into-database", no_argument, NULL, SAVEDATABASE },
        { "help", no_argument, NULL, 'h' },
        0
    };
    int c;
    bool parameter_request_seen=false;
    int optionIndex = 0;
    opterr = 0;

    while ((c = getopt_long(argc, argv, optstring, longOptions, &optionIndex)) != -1)
    {
        switch (c)
        {
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case 'l':
                g_param.load = true;
                break;
            case 'm':
                g_param.optMethod = atoi(optarg);
                break;
            case 'o':
                {
                    char* optptr = optarg;
                    while (*optptr != 0)
                    {
                        if ((*optptr == 'a') || (*optptr == 'b') ||
                                (*optptr == 'c') || (*optptr == 'v') ||
                                (*optptr == 'd') || (*optptr == 'e'))
                        {
                            g_param.optvars.insert(std::string(optptr, 1));
                        }
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
                if (g_param.cpErrorThreshold <= 0)
                {
                    cerr << "Invalid parameter: control point error threshold (-t) must be greater than 0" << std::endl;
                    return 1;
                }
                break;
            case 'v':
                g_param.verbose++;
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
            case SAVEDATABASE:
                g_param.saveDB = true;
                break;
            default:
                cerr << "Invalid parameter: '" << argv[optind-1] << " " << optarg << "'" << std::endl;
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 1;
        }
    };

    if ((argc - optind) != 1)
    {
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return 1;
    }

    // If no parameters were requested to be optimised, we optimize the
    // default parameters.
    if ( !parameter_request_seen)
    {
        for ( const char* dop=DEFAULT_OPTIMISATION_PARAMETER;
                *dop != 0; ++dop)
        {
            g_param.optvars.insert( std::string( dop, 1));
        }
    }

    // Program will crash if nothing is to be optimised.
    if ( g_param.optvars.empty())
    {
        cerr << "No parameters to optimize." << endl;
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return 1;
    }

    if (!g_param.load)
    {
        g_param.basename = argv[optind];
        vigra::ImageImportInfo firstImgInfo(argv[optind]);
        std::string pixelType = firstImgInfo.getPixelType();
        if (pixelType == "UINT8")
        {
            return processImg<RGBValue<UInt8> >(argv[optind]);
        }
        else if (pixelType == "INT16")
        {
            return processImg<RGBValue<Int16> >(argv[optind]);
        }
        else if (pixelType == "UINT16")
        {
            return processImg<RGBValue<UInt16> >(argv[optind]);
        }
        else
        {
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

