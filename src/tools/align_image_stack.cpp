// -*- c-basic-offset: 4 -*-

/** @file align_image_stack.cpp
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
#include <hugin_version.h>
#include <fstream>
#include <sstream>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>
#include <vigra/cornerdetection.hxx>
#include <vigra/localminmax.hxx>
#include <hugin_utils/utils.h>

#include <vigra_ext/Pyramid.h>
#include <vigra_ext/Correlation.h>
#include <vigra_ext/InterestPoints.h>

#include <panodata/Panorama.h>
#include <panotools/PanoToolsOptimizerWrapper.h>
#include <panodata/StandardImageVariableGroups.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include <nona/Stitcher.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <lensdb/LensDB.h>

#include <getopt.h>
#ifndef WIN32
 #include <unistd.h>
#endif

#include <hugin_utils/openmp_lock.h>
#include <tiff.h>

#ifdef __APPLE__
#include <hugin_config.h>
#include <mach-o/dyld.h>    /* _NSGetExecutablePath */
#include <limits.h>         /* PATH_MAX */
#include <libgen.h>         /* dirname */
#endif

using namespace vigra;
using namespace HuginBase;
using namespace AppBase;
using namespace std;
using namespace vigra_ext;
using namespace HuginBase::PTools;
using namespace HuginBase::Nona;

int g_verbose = 0;

static void usage(const char * name)
{
    cerr << name << ": align overlapping images for HDR creation" << std::endl
         << "align_image_stack version " << DISPLAY_VERSION << std::endl
         << std::endl
         << "Usage: " << name  << " [options] input files" << std::endl
         << "Valid options are:" << std::endl
         << " Modes of operation:" << std::endl
         << "  -p file   Output .pto file (useful for debugging, or further refinement)" << std::endl
         << "  -a prefix align images, output as prefix_xxxx.tif" << std::endl
         << "  -o output merge images to HDR, generate output.hdr" << std::endl
         << " Modifiers" << std::endl
         << "  -v        Verbose, print progress messages.  Repeat for higher verbosity" << std::endl
         << "  -e        Assume input images are full frame fish eye (default: rectilinear)" << std::endl
         << "  -t num    Remove all control points with an error higher than num pixels" << std::endl
         << "             (default: 3)" << std::endl
         << "  --corr=num  Correlation threshold for identifing control points" << std::endl
         << "               (default: 0.9)" << std::endl
         << "  -f HFOV   approximate horizontal field of view of input images," << std::endl
         << "             use if EXIF info not complete" << std::endl
         << "  -m        Optimize field of view for all images, except for first." << std::endl
         << "             Useful for aligning focus stacks with slightly" << std::endl
         << "             different magnification." << std::endl
         << "  -d        Optimize radial distortion for all images, except for first." << std::endl
         << "  -i        Optimize image center shift for all images, except for first." << std::endl
         << "  -x        Optimize X coordinate of the camera position." << std::endl
         << "  -y        Optimize Y coordinate of the camera position." << std::endl
         << "  -z        Optimize Z coordinate of the camera position." << std::endl
         << "             Useful for aligning more distorted images." << std::endl
         << "  -S        Assume stereo images - allow horizontal shift of control points." << std::endl
         << "  -A        Align stereo window - assumes -S." << std::endl
         << "  -P        Align stereo window with pop-out effect - assumes -S." << std::endl
         << "  -C        Auto crop the image to the area covered by all images." << std::endl
         << "  -c num    number of control points (per grid) to create" << std::endl
         << "             between adjacent images (default: 8)" << std::endl
         << "  -l        Assume linear input files" << std::endl
         << "  -s scale  Scale down image by 2^scale (default: 1 [2x downsampling])" << std::endl
         << "  -g gsize  Break image into a rectangular grid (gsize x gsize) and attempt" << std::endl
         << "             to find num control points in each section" << std::endl 
         << "             (default: 5 [5x5 grid] )" << std::endl
         << "  --distortion Try to load distortion information from lensfun database" << std::endl
         << "  --use-given-order  Use the image order as given from command line" << std::endl
         << "                     (By default images will be sorted by exposure values.)" << std::endl
         << "  --threads=NUM  Use NUM threads" << std::endl
         << "  --gpu     Use GPU for remapping" << std::endl
         << "  -h        Display help (this text)" << std::endl
         << std::endl;
}

typedef std::multimap<double, vigra::Diff2D> MapPoints;
static hugin_omp::Lock lock;

template <class ImageType>
void FineTuneInterestPoints(Panorama & pano, 
    int img1, const ImageType & leftImg, const ImageType & leftImgOrig, 
    int img2, const ImageType & rightImg, const ImageType & rightImgOrig,
    const MapPoints & points, unsigned nPoints, int pyrLevel, int templWidth, int sWidth, double scaleFactor, double corrThresh, bool stereo)
{
    unsigned nGood = 0;
    // loop over all points, starting with the highest corner score
    for (MapPoints::const_reverse_iterator it = points.rbegin(); it != points.rend(); ++it)
    {
        if (nGood >= nPoints)
        {
            // we have enough points, stop
            break;
        }

        vigra_ext::CorrelationResult res = vigra_ext::PointFineTune(leftImg,
            (*it).second,
            templWidth,
            rightImg,
            (*it).second,
            sWidth
            );
        if (g_verbose > 2)
        {
            std::ostringstream buf;
            buf << "I :" << (*it).second.x * scaleFactor << "," << (*it).second.y * scaleFactor << " -> "
                << res.maxpos.x * scaleFactor << "," << res.maxpos.y * scaleFactor << ":  corr coeff: " << res.maxi
                << " curv:" << res.curv.x << " " << res.curv.y << std::endl;
            cout << buf.str();
        }
        if (res.maxi < corrThresh)
        {
            DEBUG_DEBUG("low correlation: " << res.maxi << " curv: " << res.curv);
            continue;
        }

        if (pyrLevel > 0)
        {
            res = vigra_ext::PointFineTune(leftImgOrig,
                Diff2D((*it).second.x * scaleFactor, (*it).second.y * scaleFactor),
                templWidth,
                rightImgOrig,
                Diff2D(res.maxpos.x * scaleFactor, res.maxpos.y * scaleFactor),
                scaleFactor
                );

            if (g_verbose > 2)
            {
                std::ostringstream buf;
                buf << "II>" << (*it).second.x * scaleFactor << "," << (*it).second.y * scaleFactor << " -> "
                    << res.maxpos.x << "," << res.maxpos.y << ":  corr coeff: " << res.maxi
                    << " curv:" << res.curv.x << " " << res.curv.y << std::endl;
                cout << buf.str();
            }
            if (res.maxi < corrThresh)
            {
                DEBUG_DEBUG("low correlation in pass 2: " << res.maxi << " curv: " << res.curv);
                continue;
            }
        }

        nGood++;
        // add control point
        ControlPoint p(img1, (*it).second.x * scaleFactor,
            (*it).second.y * scaleFactor,
            img2, res.maxpos.x,
            res.maxpos.y,
            stereo ? ControlPoint::Y : ControlPoint::X_Y);
        {
            hugin_omp::ScopedLock sl(lock);
            pano.addCtrlPoint(p);
        };
    }
    if (g_verbose > 0)
    {
        std::ostringstream buf;
        buf << "Number of good matches: " << nGood << ", bad matches: " << points.size() - nGood << std::endl;
        cout << buf.str();
    }
};

template <class ImageType>
void createCtrlPoints(Panorama & pano, int img1, const ImageType & leftImg, const ImageType & leftImgOrig, int img2, const ImageType & rightImg, const ImageType & rightImgOrig, int pyrLevel, double scale, unsigned nPoints, unsigned grid, double corrThresh = 0.9, bool stereo = false)
{
    typedef typename ImageType::value_type VT;
    //////////////////////////////////////////////////
    // find interesting corners using harris corner detector
    if (stereo)
    {
        // add one vertical control point to keep the images aligned vertically
        ControlPoint p(img1, 0, 0, img2, 0, 0, ControlPoint::X);
        pano.addCtrlPoint(p);
    }

    if (g_verbose > 0) {
        std::cout << "Trying to find " << nPoints << " corners... " << std::endl;
    }

    vigra::Size2D size(leftImg.width(), leftImg.height());
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

    const double scaleFactor = 1 << pyrLevel;
    const long templWidth = 20;
    const long sWidth = 100;

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < rects.size(); ++i)
    {
        MapPoints points;
        vigra::Rect2D rect(rects[i]);
        findInterestPointsPartial(srcImageRange(leftImg, RGBToGrayAccessor<VT>()), rect, scale, 5 * nPoints, points);
        FineTuneInterestPoints(pano, img1, leftImg, leftImgOrig, img2, rightImg, rightImgOrig, points, nPoints, pyrLevel, templWidth, sWidth, scaleFactor, corrThresh, stereo);
    };

    if (stereo)
    {
        // add some additional control points around image edges
        // this is useful for better results - images are more distorted around edges
        // and also for stereoscopic window adjustment - it must be alligned according to
        // the nearest object which crosses the edge and these control points helps to find it.
        MapPoints up;
        MapPoints down;
        MapPoints left;
        MapPoints right;
        int xstep = leftImg.size().x / (nPoints + 1);
        int ystep = leftImg.size().y / (nPoints + 1);
        for (int k = 6; k >= 0; --k)
        for (int j = 0; j < 2; ++j)
        for (int i = 0; i < nPoints; ++i) {
            up.insert(std::make_pair(0, vigra::Diff2D(j * xstep / 2 + i * xstep, 1 + k * 10)));
            down.insert(std::make_pair(0, vigra::Diff2D(j * xstep / 2 + i * xstep, leftImg.size().y - 2 - k * 10)));
            left.insert(std::make_pair(0, vigra::Diff2D(1 + k * 10, j * ystep / 2 + i * ystep)));
            right.insert(std::make_pair(0, vigra::Diff2D(leftImg.size().x - 2 - k * 10, j * ystep / 2 + i * ystep)));
        }
        // execute parallel
#pragma omp parallel sections
        {
#pragma omp section
            {
                FineTuneInterestPoints(pano, img1, leftImg, leftImgOrig, img2, rightImg, rightImgOrig, up, nPoints, pyrLevel, templWidth, sWidth, corrThresh, scaleFactor, stereo);
            }
#pragma omp section
            {
                FineTuneInterestPoints(pano, img1, leftImg, leftImgOrig, img2, rightImg, rightImgOrig, down, nPoints, pyrLevel, templWidth, sWidth, corrThresh, scaleFactor, stereo);
            }
#pragma omp section
            {
                FineTuneInterestPoints(pano, img1, leftImg, leftImgOrig, img2, rightImg, rightImgOrig, left, nPoints, pyrLevel, templWidth, sWidth, corrThresh, scaleFactor, stereo);
            }
#pragma omp section
            {
                FineTuneInterestPoints(pano, img1, leftImg, leftImgOrig, img2, rightImg, rightImgOrig, right, nPoints, pyrLevel, templWidth, sWidth, corrThresh, scaleFactor, stereo);
            }
        }
    }
};

void alignStereoWindow(Panorama & pano, bool pop_out)
{
    CPVector cps = pano.getCtrlPoints();
    std::vector<PTools::Transform *> transTable(pano.getNrOfImages());

    std::vector<int> max_i(pano.getNrOfImages() - 1, -1); // index of a point with biggest shift
    std::vector<int> max_i_b(pano.getNrOfImages() - 1, -1); // the same as above, only border points considered
    std::vector<double> max_dif(pano.getNrOfImages() - 1, -1000000000); // value of the shift
    std::vector<double> max_dif_b(pano.getNrOfImages() - 1, -1000000000); // the same as above, only border points considered

    for (int i=0; i < pano.getNrOfImages(); i++)
    {
        transTable[i] = new PTools::Transform();
        transTable[i]->createInvTransform(pano.getImage(i), pano.getOptions());
    }

    double rbs = 0.1; // relative border size
    
    for (int i=0; i < (int)cps.size(); i++) {
        if (cps[i].mode == ControlPoint::X) {
            if (max_i[cps[i].image1Nr] < 0) // first control point for given pair
                max_i[cps[i].image1Nr] = i; // use it as a fallback in case on other points exist
            continue;
        }
        
        vigra::Size2D size1 = pano.getImage(cps[i].image1Nr).getSize();
        vigra::Size2D size2 = pano.getImage(cps[i].image2Nr).getSize();
        
        vigra::Rect2D rect1(size1);
        vigra::Rect2D rect2(size2);
        
        rect1.addBorder(-size1.width() * rbs, -size1.height() * rbs);
        rect2.addBorder(-size2.width() * rbs, -size2.height() * rbs);
        

        double xt1, yt1, xt2, yt2; 
        if(!transTable[cps[i].image1Nr]->transformImgCoord(xt1, yt1, cps[i].x1, cps[i].y1)) continue;        
        if(!transTable[cps[i].image2Nr]->transformImgCoord(xt2, yt2, cps[i].x2, cps[i].y2)) continue;        

        double dif = xt2 - xt1;
        if (dif > max_dif[cps[i].image1Nr]) {
            max_dif[cps[i].image1Nr] = dif;
            max_i[cps[i].image1Nr] = i;
        }

        if (!(rect1.contains(Point2D(cps[i].x1, cps[i].y1)) &&
            rect2.contains(Point2D(cps[i].x2, cps[i].y2)))) {
            // the same for border points only
            if (dif > max_dif_b[cps[i].image1Nr]) {
                max_dif_b[cps[i].image1Nr] = dif;
                max_i_b[cps[i].image1Nr] = i;
            }
        }
    }

    for (int i=0; i < pano.getNrOfImages(); i++)
    {
        delete transTable[i];
    }
    
    for (int i=0; i < (int)max_i.size(); i++) {
        if (pop_out && (max_i_b[i] >= 0)) // check points at border
            cps[max_i_b[i]].mode = ControlPoint::X_Y;
        else if (max_i[i] >= 0) // no points at border - use any point
            cps[max_i[i]].mode = ControlPoint::X_Y;
        else {
            //no points at all - should not happen
        }
        
    }

    CPVector newCPs;
    for (int i=0; i < (int)cps.size(); i++) {
        if (cps[i].mode != ControlPoint::X) { // remove the vertical control lines, X_Y points replaces them
            newCPs.push_back(cps[i]);
        }
    }

    pano.setCtrlPoints(newCPs);
}

void autoCrop(Panorama & pano)
{
    CalculateOptimalROI cropPano(pano, true);
    cropPano.run();
        
    vigra::Rect2D roi=cropPano.getResultOptimalROI();
    //set the ROI - fail if the right/bottom is zero, meaning all zero
    if(roi.right() != 0 && roi.bottom() != 0)
    {
        PanoramaOptions opt = pano.getOptions();
        opt.setROI(roi);
        pano.setOptions(opt);
        cout << "Set crop size to " << roi.left() << "," << roi.top() << "," << roi.right() << "," << roi.bottom() << endl;
    }
    else {
        cout << "Could not find best crop rectangle for image" << endl;
    };
}

struct Parameters
{
    Parameters()
    {
        cpErrorThreshold = 3;
        corrThresh = 0.9;
        nPoints = 8;
        grid = 5;
        hfov = 0;
        pyrLevel = 1;
        linear = false;   // Assume linear input files if true
        optHFOV = false;
        optDistortion = false;
        optCenter = false;
        optX = false;
        optY = false;
        optZ = false;
        stereo = false;
        stereo_window = false;
        pop_out = false;
        crop = false;
        fisheye = false;
        gpu = false;
        loadDistortion = false;
        sortImagesByEv = true;
    }

    double cpErrorThreshold;
    double corrThresh;
    int nPoints;
    int grid;		// Partition images into grid x grid subregions, each with npoints
    double hfov;
    bool linear;
    bool optHFOV;
    bool optDistortion;
    bool optCenter;
    bool optX;
    bool optY;
    bool optZ;
    bool fisheye;
    bool stereo;
    bool stereo_window;
    bool pop_out;
    bool crop;
    bool gpu;
    bool loadDistortion;
    bool sortImagesByEv;
    int pyrLevel;
    std::string alignedPrefix;
    std::string ptoFile;
    std::string hdrFile;
    string basename;
};

// dummy panotools progress functions
static int ptProgress(int command, char* argument)
{
    return 1;
}
static int ptinfoDlg(int command, char* argument)
{
    return 1;
}

struct SortImageVectorEV
{
public:
    SortImageVectorEV(const Panorama* pano) : m_pano(pano) {};
    bool operator()(const unsigned int i, const unsigned int j)
    {
        return m_pano->getImage(i).getExposureValue() > m_pano->getImage(j).getExposureValue();
    };
private:
    const Panorama* m_pano;
};

template <class PixelType>
int main2(std::vector<std::string> files, Parameters param)
{
    typedef vigra::BasicImage<PixelType> ImageType;
    try {
        Panorama pano;
        Lens l;

        // add the first image.to the panorama object
        SrcPanoImage srcImg;
        srcImg.setFilename(files[0]);
        
        if (param.fisheye) {
            srcImg.setProjection(SrcPanoImage::FULL_FRAME_FISHEYE);
        }
        srcImg.readEXIF();
        srcImg.applyEXIFValues();
        if (param.sortImagesByEv)
        {
            if (fabs(srcImg.getExposureValue()) < 1E-6)
            {
                // no exposure values found in file, don't sort images
                param.sortImagesByEv = false;
            }
        };
        // disable autorotate
        srcImg.setRoll(0);
        if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
            cerr << "Could not decode image: " << files[0] << "Unsupported image file format";
            return 1;
        }

        if(param.loadDistortion)
        {
            if(srcImg.readDistortionFromDB())
            {
                cout << "\tRead distortion data from lensfun database." << endl;
            }
            else
            {
                cout << "\tNo valid distortion data found in lensfun database." << endl;
            }
        }

        // use hfov specified by user.
        if (param.hfov > 0) {
            srcImg.setHFOV(param.hfov);
        } else if (srcImg.getCropFactor() == 0) {
            // could not read HFOV, assuming default: 50
            srcImg.setHFOV(50);
        }
        
        if (param.linear) {
            srcImg.setResponseType(SrcPanoImage::RESPONSE_LINEAR);
            if (g_verbose>0) {
                cout << "Using linear response" << std::endl;
            }
        }
        
        pano.addImage(srcImg);
        
        // setup output to be exactly similar to input image
        PanoramaOptions opts;

        if (param.fisheye) {
            opts.setProjection(PanoramaOptions::FULL_FRAME_FISHEYE);
        } else {
            opts.setProjection(PanoramaOptions::RECTILINEAR);
        }
        opts.setHFOV(srcImg.getHFOV(), false);
        opts.setWidth(srcImg.getSize().x, false);
        opts.setHeight(srcImg.getSize().y);
        // output to tiff format
        opts.outputFormat = PanoramaOptions::TIFF_m;
        opts.tiff_saveROI = false;
        // m estimator, to be more robust against points on moving objects
        opts.huberSigma = 2;
        pano.setOptions(opts);

        // variables that should be optimized
        // optimize nothing in the first image
        OptimizeVector optvars(1);
        std::vector<unsigned int> images;
        images.push_back(0);

        StandardImageVariableGroups variable_groups(pano);

        // loop to add images.
        for (int i = 1; i < (int) files.size(); i++) {
            // add next image.
            srcImg.setFilename(files[i]);
            srcImg.readEXIF();
            srcImg.applyEXIFValues();
            if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
                cerr << "Could not decode image: " << files[i] << "Unsupported image file format";
                return 1;
            }

            if(param.loadDistortion)
            {
                if(srcImg.readDistortionFromDB())
                {
                    cout << "\tRead distortion data from lensfun database." << endl;
                }
                else
                {
                    cout << "\tNo valid distortion data found in lensfun database." << endl;
                }
            }

            if (param.hfov > 0) {
                srcImg.setHFOV(param.hfov);
            } else if (srcImg.getCropFactor() == 0) {
                // could not read HFOV, assuming default: 50
                srcImg.setHFOV(50);
            }
            
            int imgNr = pano.addImage(srcImg);
            variable_groups.update();
            // each image shares the same lens.
            variable_groups.getLenses().switchParts(imgNr, 0);
            // unlink HFOV?
            if (param.optHFOV) {
                pano.unlinkImageVariableHFOV(0);
            }
            if (param.optDistortion) {
                pano.unlinkImageVariableRadialDistortion(0);
            }
            if (param.optCenter) {
                pano.unlinkImageVariableRadialDistortionCenterShift(0);
            }
            // All images are in the same stack: Link the stack variable.
            pano.linkImageVariableStack(imgNr, 0);
            // exposure value is linked, reset to value found in EXIF
            pano.unlinkImageVariableExposureValue(0);
            srcImg.setExposureValue(srcImg.calcExifExposureValue());
            pano.setSrcImage(i, srcImg);
            images.push_back(i);
            // optimize yaw, roll, pitch
            std::set<std::string> vars;
            vars.insert("y");
            vars.insert("p");
            vars.insert("r");
            if (param.optHFOV) {
                vars.insert("v");
            }
            if (param.optDistortion) {
                vars.insert("a");
                vars.insert("b");
                vars.insert("c");
            }
            if (param.optCenter) {
                vars.insert("d");
                vars.insert("e");
            }
            if (param.optX) {
                vars.insert("TrX");
            }
            if (param.optY) {
                vars.insert("TrY");
            }
            if (param.optZ) {
                vars.insert("TrZ");
            }
            optvars.push_back(vars);

        };
            
        // sort now all images by exposure value
        if (param.sortImagesByEv)
        {
            std::sort(images.begin(), images.end(), SortImageVectorEV(&pano));
        };

        // load first image
        vigra::ImageImportInfo firstImgInfo(pano.getSrcImage(images[0]).getFilename().c_str());

        // original size
        ImageType * leftImgOrig = new ImageType(firstImgInfo.size());
        // rescale image
        ImageType * leftImg = new ImageType();
        {
            if(firstImgInfo.numExtraBands() == 1) {
                vigra::BImage alpha(firstImgInfo.size());
                vigra::importImageAlpha(firstImgInfo, destImage(*leftImgOrig), destImage(alpha));
            } else if (firstImgInfo.numExtraBands() == 0) {
                vigra::importImage(firstImgInfo, destImage(*leftImgOrig));
            } else {
                vigra_fail("Images with multiple extra (alpha) channels not supported");
            }
            reduceNTimes(*leftImgOrig, *leftImg, param.pyrLevel);
        }


        ImageType * rightImg = new ImageType(leftImg->size());
        ImageType * rightImgOrig = new ImageType(leftImgOrig->size());
        // loop to add control points between them.
        for (int i = 1; i < (int) images.size(); i++) {
            if (g_verbose > 0) {
                cout << "Creating control points between " << pano.getSrcImage(images[i-1]).getFilename().c_str() << " and " << 
                    pano.getSrcImage(images[i]).getFilename().c_str() << std::endl;
            }

            // load the actual image data of the next image
            vigra::ImageImportInfo nextImgInfo(pano.getSrcImage(images[i]).getFilename().c_str());
            assert(nextImgInfo.size() == firstImgInfo.size());
            {
                if (nextImgInfo.numExtraBands() == 1) {
                    vigra::BImage alpha(nextImgInfo.size());
                    vigra::importImageAlpha(nextImgInfo, destImage(*rightImgOrig), destImage(alpha));
                } else if (nextImgInfo.numExtraBands() == 0) {
                    vigra::importImage(nextImgInfo, destImage(*rightImgOrig));
                } else {
                    vigra_fail("Images with multiple extra (alpha) channels not supported");
                }
                reduceNTimes(*rightImgOrig, *rightImg, param.pyrLevel);
            }

            // add control points.
            // work on smaller images
            // TODO: or use a fast interest point operator.
            createCtrlPoints(pano, images[i-1], *leftImg, *leftImgOrig, images[i], *rightImg, *rightImgOrig, param.pyrLevel, 2, param.nPoints, param.grid, param.corrThresh, param.stereo);

            // swap images;
            delete leftImg;
            delete leftImgOrig;
            leftImg = rightImg;
            leftImgOrig = rightImgOrig;
            rightImg = new ImageType(leftImg->size());
            rightImgOrig = new ImageType(leftImgOrig->size());
        }
        delete leftImg;
        delete rightImg;
        delete leftImgOrig;
        delete rightImgOrig;

        // optimize everything.
        pano.setOptimizeVector(optvars);
        // disable optimizer progress messages if -v not given
        if (g_verbose == 0)
        {
            PT_setProgressFcn(ptProgress);
            PT_setInfoDlgFcn(ptinfoDlg);
        };
        bool optimizeError = (PTools::optimize(pano) > 0);

        // need to do some basic outlier pruning.
        // remove all points with error higher than a specified threshold
        if (param.cpErrorThreshold > 0) {
            CPVector cps = pano.getCtrlPoints();
            CPVector newCPs;
            for (int i=0; i < (int)cps.size(); i++) {
                if (cps[i].error < param.cpErrorThreshold ||
                    cps[i].mode == ControlPoint::X) { // preserve the vertical control point for stereo alignment
                    newCPs.push_back(cps[i]);
                }
            }
            if (g_verbose > 0) {
                cout << "Ctrl points before pruning: " << cps.size() << ", after: " << newCPs.size() << std::endl;
            }
            pano.setCtrlPoints(newCPs);
            if (param.stereo_window) alignStereoWindow(pano, param.pop_out);
            // reoptimize
            optimizeError = (PTools::optimize(pano) > 0) ;
        }
        
        if (param.crop) autoCrop(pano);

        UIntSet imgs = pano.getActiveImages();
        if (optimizeError)
        {
            if (param.ptoFile.size() > 0) {
                std::ofstream script(param.ptoFile.c_str());
                pano.printPanoramaScript(script, optvars, pano.getOptions(), imgs, false, "");
            }
            cerr << "An error occured during optimization." << std::endl;
            cerr << "Try adding \"-p debug.pto\" and checking output." << std::endl;
            cerr << "Exiting..." << std::endl;
            return 1;
        }

        if (param.hdrFile.size()) {
            // TODO: photometric alignment (HDR, fixed white balance)
            //utils::StreamProgressReporter progress(2.0);
            //loadImgsAndExtractPoints(pano, nPoints, pyrLevel, randomPoints, progress, points);
            //smartOptimizePhotometric

            // switch to HDR output mode
            PanoramaOptions opts = pano.getOptions();
            opts.outputFormat = PanoramaOptions::HDR;
            opts.outputPixelType = "FLOAT";
            opts.outputMode = PanoramaOptions::OUTPUT_HDR;
            opts.outfile = param.hdrFile;
            pano.setOptions(opts);

            // remap all images
            MultiProgressDisplay* progress;
            if(g_verbose > 0)
            {
                progress = new StreamMultiProgressDisplay(cout);
            }
            else
            {
                progress = new DummyMultiProgressDisplay();
            };
            stitchPanorama(pano, pano.getOptions(),
                           *progress, opts.outfile, imgs);
            std::cout << "Written HDR output to " << opts.outfile << std::endl;
            delete progress;
        }
        if (param.alignedPrefix.size()) {
            // disable all exposure compensation stuff.
            PanoramaOptions opts = pano.getOptions();
            opts.outputExposureValue = 0;
            opts.outputMode = PanoramaOptions::OUTPUT_LDR;
            opts.outputFormat = PanoramaOptions::TIFF_m;
            opts.outputPixelType = "";
            opts.outfile = param.alignedPrefix;
            opts.remapUsingGPU = param.gpu;
            pano.setOptions(opts);
            for (unsigned i=0; i < pano.getNrOfImages(); i++) {
                SrcPanoImage img = pano.getSrcImage(i);
                img.setExposureValue(0);
                pano.setSrcImage(i, img);
            }
            // remap all images
            MultiProgressDisplay* progress;
            if(g_verbose > 0)
            {
                progress = new StreamMultiProgressDisplay(cout);
            }
            else
            {
                progress = new DummyMultiProgressDisplay();
            };
            stitchPanorama(pano, pano.getOptions(),
                           *progress, opts.outfile, imgs);
            delete progress;
            std::cout << "Written aligned images to files with prefix \"" << opts.outfile << "\"" << std::endl;
        }

        // At this point we have panorama options set according to the output
        if (param.ptoFile.size() > 0) {
            std::ofstream script(param.ptoFile.c_str());
            pano.printPanoramaScript(script, optvars, pano.getOptions(), imgs, false, "");
            std::cout << "Written project file " << param.ptoFile << std::endl;
        }


    } catch (std::exception & e) {
        cerr << "ERROR: caught exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "a:ef:g:hlmdiSAPCp:vo:s:t:c:xyz";
    int c;

    opterr = 0;

    g_verbose = 0;

    Parameters param;
    enum
    {
        CORRTHRESH=1000,
        THREADS,
        GPU,
        LENSFUN,
        USEGIVENORDER,
    };

    static struct option longOptions[] =
    {
        {"corr", required_argument, NULL, CORRTHRESH },
        {"threads", required_argument, NULL, THREADS },
        {"gpu", no_argument, NULL, GPU },
        {"distortion", no_argument, NULL, LENSFUN },
        {"use-given-order", no_argument, NULL, USEGIVENORDER },
        {"help", no_argument, NULL, 'h' },
        0
    };
    int optionIndex = 0;
    int nThread = getCPUCount();
    if (nThread < 0) nThread = 1;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
        switch (c) {
        case 'a':
            param.alignedPrefix = optarg;
            break;
        case 'c':
            param.nPoints = atoi(optarg);
            if (param.nPoints<1) {
                cerr << "Invalid parameter: Number of points/grid (-c) must be at least 1" << std::endl;
                return 1;
            }
            break;
        case 'e':
            param.fisheye = true;
            break;
        case 'f':
            param.hfov = atof(optarg);
            if (param.hfov<=0) {
                cerr << "Invalid parameter: HFOV (-f) must be greater than 0" << std::endl;
                return 1;
            }
            break;
        case 'g':
            param.grid = atoi(optarg);
            if (param.grid <1 || param.grid>50) {
                cerr << "Invalid parameter: number of grid cells (-g) should be between 1 and 50" << std::endl;
                return 1;
            }
            break;
        case 'l':
            param.linear = true;
            break;
        case 'm':
            param.optHFOV = true;
            break;
        case 'd':
            param.optDistortion = true;
            break;
        case 'i':
            param.optCenter = true;
            break;
        case 'x':
            param.optX = true;
            break;
        case 'y':
            param.optY = true;
            break;
        case 'z':
            param.optZ = true;
            break;
        case 'S':
            param.stereo = true;
            break;
        case 'A':
            param.stereo = true;
            param.stereo_window = true;
            break;
        case 'P':
            param.stereo = true;
            param.stereo_window = true;
            param.pop_out = true;
            break;
        case 'C':
            param.crop = true;
            break;
        case 't':
            param.cpErrorThreshold = atof(optarg);
            if (param.cpErrorThreshold <= 0) {
                cerr << "Invalid parameter: control point error threshold (-t) must be greater than 0" << std::endl;
                return 1;
            }
            break;
        case 'p':
            param.ptoFile = optarg;
            break;
        case 'o':
            param.hdrFile = optarg;
            break;
        case 'v':
            g_verbose++;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        case 's':
            param.pyrLevel = atoi(optarg);
            if (param.pyrLevel<0 || param.pyrLevel >8) {
            cerr << "Invalid parameter: scaling (-s) should be between 0 and 8" << std::endl;
            return 1;
            }
            break;
        case CORRTHRESH:
            param.corrThresh = atof(optarg);
            if(param.corrThresh<=0 || param.corrThresh>1.0)
            {
                cerr << "Invalid correlation value. Should be between 0 and 1" << endl;
                return 1;
            };
            break;
        case THREADS:
            nThread = atoi(optarg);
            break;
        case GPU:
            param.gpu = true;
            break;
        case LENSFUN:
            param.loadDistortion = true;
            break;
        case USEGIVENORDER:
            param.sortImagesByEv = false;
            break;
        default:
            cerr << "Invalid parameter: " << optarg << std::endl;
            usage(argv[0]);
            return 1;
        }

    // use always given image order for stereo options
    if (param.stereo)
    {
        param.sortImagesByEv = false;
    };
    unsigned nFiles = argc - optind;
    if (nFiles < 2) {
        std::cerr << std::endl << "Error: at least two files need to be specified" << std::endl <<std::endl;
        usage(argv[0]);
        return 1;
    }

    if (param.hdrFile.size() == 0 && param.ptoFile.size() == 0 && param.alignedPrefix.size() == 0) {
        std::cerr << std::endl
                  << "ERROR: Please specify at least one of the -p, -o or -a options." << std::endl
                  << std::endl;
        usage(argv[0]);
        return 1;
    }

    // extract file names
    std::vector<std::string> files;
    for (size_t i=0; i < nFiles; i++)
        files.push_back(argv[optind+i]);

    if (nThread == 0) nThread = 1;
    vigra_ext::ThreadManager::get().setNThreads(nThread);

    std::string pixelType;

    try {
        vigra::ImageImportInfo firstImgInfo(files[0].c_str());
        pixelType = firstImgInfo.getPixelType();
    } catch (std::exception & e) {
        cerr << "ERROR: caught exception: " << e.what() << std::endl;
        return 1;
    }

    if(param.gpu)
    {
        param.gpu=hugin_utils::initGPU(&argc, argv);
    };
    if (pixelType == "UINT8") {
        return main2<RGBValue<UInt8> >(files, param);
    } else if (pixelType == "INT16") {
        return main2<RGBValue<Int16> >(files, param);
    } else if (pixelType == "UINT16") {
        return main2<RGBValue<UInt16> >(files, param);
    } else if (pixelType == "FLOAT") {
        return main2<RGBValue<float> >(files, param);
    } else {
        cerr << " ERROR: unsupported pixel type: " << pixelType << std::endl;
        if(param.gpu)
        {
            hugin_utils::wrapupGPU();
        };
        HuginBase::LensDB::LensDB::Clean();
        return 1;
    }

    if(param.gpu)
    {
        hugin_utils::wrapupGPU();
    };
    HuginBase::LensDB::LensDB::Clean();
    return 0;
}
