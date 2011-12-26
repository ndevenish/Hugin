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
         << "  -t num    Remove all control points with an error higher than num pixels (default: 3)" << std::endl
         << "  -f HFOV   approximate horizontal field of view of input images, use if EXIF info not complete" << std::endl
         << "  -m        Optimize field of view for all images, except for first." << std::endl
         << "             Useful for aligning focus stacks with slightly different magnification." << std::endl
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
         << "  -c num    number of control points (per grid) to create between adjacent images (default: 8)" << std::endl
         << "  -l        Assume linear input files" << std::endl
	 << "  -s scale  Scale down image by 2^scale (default: 1 [2x downsampling])" << std::endl
	 << "  -g gsize  Break image into a rectangular grid (gsize x gsize) and attempt to find " << std::endl 
	 << "             num control points in each section (default: 5 [5x5 grid] )" << std::endl
         << "  -h        Display help (this text)" << std::endl
         << std::endl;
}


#if 0
template <class VALUETYPE>
class InterestPointSelector
{
public:

        /** the functor's argument type
        */
    typedef VALUETYPE argument_type;

        /** the functor's result type
        */
    typedef VALUETYPE result_type;

        /** \deprecated use argument_type
        */
    typedef VALUETYPE value_type;


    InterestPointSelector(int nrPoints)
    {
        minResponse = 0;
        nPoints = nrPoints;
    }

    void operator()(argument_type const & resp)
    {
        //double resp = leftCornerResponse(x,y);
	if (resp > minResponse) {
	    // add to point map
	    points.insert(make_pair(resp,Diff2D(x,y)));
	    // if we have reached the maximum
	    // number of points, increase the threshold, to avoid
	    // growing the points map too much.
	    // extract more than nPoints, because some might be bad
	    // and cannot be matched with the other image.
	    if (points.size() > 5*nPoints) {
		// remove the point with the lowest corner response.
		leftCorners(points.begin()->second.x,points.begin()->second.y)=0;
		points.erase(points.begin());
		// use new threshold for next selection.
		minResponse = points.begin()->first;
	    }
	}
    }

    argument_type minResponse;
    std::multimap<argument_type, vigra::Diff2D> points;
    int nPoints;
}

#endif

template <class ImageType>
void createCtrlPoints(Panorama & pano, int img1, const ImageType & leftImg, const ImageType & leftImgOrig, int img2, const ImageType & rightImg, const ImageType & rightImgOrig, int pyrLevel, double scale, unsigned nPoints, unsigned grid, bool stereo = false)
{
    typedef typename ImageType::value_type VT;
    //////////////////////////////////////////////////
    // find interesting corners using harris corner detector
    typedef std::vector<std::multimap<double, vigra::Diff2D> > MapVector;

    if (stereo)
    {
        // add one vertical control point to keep the images aligned vertically
        ControlPoint p(img1, 0, 0, img2, 0, 0, ControlPoint::X);
        pano.addCtrlPoint(p);
    }
    std::vector<std::multimap<double, vigra::Diff2D> >points;
    if (g_verbose > 0) {
        std::cout << "Trying to find " << nPoints << " corners... ";
    }

    vigra_ext::findInterestPointsOnGrid(srcImageRange(leftImg, GreenAccessor<VT>()), scale, 5*nPoints, grid, points);

    if (stereo)
    {
        // add some additional control points around image edges
        // this is useful for better results - images are more distorted around edges
        // and also for stereoscopic window adjustment - it must be alligned according to
        // the nearest object which crosses the edge and these control points helps to find it.
        std::multimap<double, vigra::Diff2D> up;
        std::multimap<double, vigra::Diff2D> down;
        std::multimap<double, vigra::Diff2D> left;
        std::multimap<double, vigra::Diff2D> right;
        int xstep = leftImg.size().x / (nPoints + 1);
        int ystep = leftImg.size().y / (nPoints + 1);
        for (int k = 6; k >= 0; --k) 
            for (int j = 0; j < 2; ++j) 
            	for (int i = 0; i < nPoints; ++i) { 
                    up.insert(   std::make_pair(0, vigra::Diff2D(j * xstep / 2 + i * xstep ,    1 + k * 10)));
                    down.insert( std::make_pair(0, vigra::Diff2D(j * xstep / 2 + i * xstep ,    leftImg.size().y - 2 - k * 10)));
                    left.insert( std::make_pair(0, vigra::Diff2D(1 + k * 10,                    j * ystep / 2 + i * ystep)));
                    right.insert(std::make_pair(0, vigra::Diff2D(leftImg.size().x - 2 - k * 10, j * ystep / 2 + i * ystep)));
        }
        points.push_back(up);
        points.push_back(down);
        points.push_back(left);
        points.push_back(right);
    }  

    double scaleFactor = 1<<pyrLevel;

    for (MapVector::iterator mit = points.begin(); mit != points.end(); ++mit) {

        unsigned nGood = 0;
        unsigned nBad = 0;
        // loop over all points, starting with the highest corner score
        for (multimap<double, vigra::Diff2D>::reverse_iterator it = (*mit).rbegin();
             it != (*mit).rend();
             ++it)
        {
            if (nGood >= nPoints) {
                // we have enough points, stop
                break;
            }

            long templWidth = 20;
            long sWidth = 100;
            long sWidth2 = scaleFactor;
            double corrThresh = 0.9;
            //double curvThresh = 0.0;

            vigra_ext::CorrelationResult res;

            res = vigra_ext::PointFineTune(leftImg,
                                            (*it).second,
                                            templWidth,
                                            rightImg,
                                            (*it).second,
                                            sWidth
                                            );
            if (g_verbose > 2) {
                cout << "I :" << (*it).second.x * scaleFactor << "," << (*it).second.y * scaleFactor << " -> "
                        << res.maxpos.x * scaleFactor << "," << res.maxpos.y * scaleFactor << ":  corr coeff: " <<  res.maxi
                        << " curv:" <<  res.curv.x << " " << res.curv.y << std::endl;
            }
            if (res.maxi < corrThresh )
            {
                nBad++;
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
                                            sWidth2
                                            );

                if (g_verbose > 2) {
                    cout << "II>" << (*it).second.x * scaleFactor << "," << (*it).second.y * scaleFactor << " -> "
                            << res.maxpos.x << "," << res.maxpos.y << ":  corr coeff: " <<  res.maxi
                            << " curv:" <<  res.curv.x << " " << res.curv.y << std::endl;
                }
                if (res.maxi < corrThresh )
                {
                    nBad++;
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
            pano.addCtrlPoint(p);
            
        }
        if (g_verbose > 0) {
            cout << "Number of good matches: " << nGood << ", bad matches: " << nBad << std::endl;
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
    }

    double cpErrorThreshold;
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
    int pyrLevel;
    std::string alignedPrefix;
    std::string ptoFile;
    std::string hdrFile;
    string basename;
};

template <class PixelType>
int main2(std::vector<std::string> files, Parameters param)
{
    typedef vigra::BasicImage<PixelType> ImageType;
    try {
        // load first image
        vigra::ImageImportInfo firstImgInfo(files[0].c_str());

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


        Panorama pano;
        Lens l;

        // add the first image.to the panorama object
        // default settings
        double focalLength = 50;
        double cropFactor = 0;
        
        SrcPanoImage srcImg;
        srcImg.setFilename(files[0]);
        
        if (param.fisheye) {
            srcImg.setProjection(SrcPanoImage::FULL_FRAME_FISHEYE);
        }
        srcImg.readEXIF(focalLength, cropFactor, true, true);
        // disable autorotate
        srcImg.setRoll(0);
        if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
            cerr << "Could not decode image: " << files[0] << "Unsupported image file format";
            return 1;
        }

        // use hfov specified by user.
        if (param.hfov > 0) {
            srcImg.setHFOV(param.hfov);
        } else if (cropFactor == 0) {
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

        if (srcImg.getRoll() == 0.0 || srcImg.getRoll() == 180.0) {
            opts.setWidth(srcImg.getSize().x, false);
            opts.setHeight(srcImg.getSize().y);
        } else {
            opts.setWidth(srcImg.getSize().y, false);
            opts.setHeight(srcImg.getSize().x);
        }
            // output to tiff format
        opts.outputFormat = PanoramaOptions::TIFF_m;
        opts.tiff_saveROI = false;
        // m estimator, to be more robust against points on moving objects
        opts.huberSigma = 2;
        pano.setOptions(opts);

        // variables that should be optimized
        // optimize nothing in the first image
        OptimizeVector optvars(1);

        ImageType * rightImg = new ImageType(leftImg->size());
        ImageType * rightImgOrig = new ImageType(leftImgOrig->size());
        StandardImageVariableGroups variable_groups(pano);

        // loop to add images and control points between them.
        for (int i = 1; i < (int) files.size(); i++) {
            if (g_verbose > 0) {
                cout << "Creating control points between " << files[i-1] << " and " << files[i] << std::endl;
            }
            // add next image.
            srcImg.setFilename(files[i]);
            srcImg.readEXIF(focalLength, cropFactor, true, true);
            if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
                cerr << "Could not decode image: " << files[i] << "Unsupported image file format";
                return 1;
            }
            if (param.hfov > 0) {
                srcImg.setHFOV(param.hfov);
            } else if (cropFactor == 0) {
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
            
            // load the actual image data of the next image
            vigra::ImageImportInfo nextImgInfo(files[i].c_str());
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
            createCtrlPoints(pano, i-1, *leftImg, *leftImgOrig, i, *rightImg, *rightImgOrig, param.pyrLevel, 2, param.nPoints, param.grid, param.stereo);

            // swap images;
            delete leftImg;
            delete leftImgOrig;
            leftImg = rightImg;
            leftImgOrig = rightImgOrig;
            rightImg = new ImageType(leftImg->size());
            rightImgOrig = new ImageType(leftImgOrig->size());

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
        }
        delete leftImg;
        delete rightImg;
        delete leftImgOrig;
        delete rightImgOrig;

        // optimize everything.
        pano.setOptimizeVector(optvars);
        bool optimizeError = false;
        optimizeError = (PTools::optimize(pano) > 0);

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
            StreamMultiProgressDisplay progress(cout);
            stitchPanorama(pano, pano.getOptions(),
                           progress, opts.outfile, imgs);
        }
        if (param.alignedPrefix.size()) {
            // disable all exposure compensation stuff.
            PanoramaOptions opts = pano.getOptions();
            opts.outputExposureValue = 0;
            opts.outputMode = PanoramaOptions::OUTPUT_LDR;
            opts.outputFormat = PanoramaOptions::TIFF_m;
            opts.outputPixelType = "";
            opts.outfile = param.alignedPrefix;
            pano.setOptions(opts);
            for (unsigned i=0; i < pano.getNrOfImages(); i++) {
                SrcPanoImage img = pano.getSrcImage(i);
                img.setExposureValue(0);
                pano.setSrcImage(i, img);
            }
            // remap all images
            StreamMultiProgressDisplay progress(cout);
            stitchPanorama(pano, pano.getOptions(),
                           progress, opts.outfile, imgs);

        }

        // At this point we have panorama options set according to the output
        if (param.ptoFile.size() > 0) {
            std::ofstream script(param.ptoFile.c_str());
            pano.printPanoramaScript(script, optvars, pano.getOptions(), imgs, false, "");
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
//    // use to override exposure value on the command line?
//    std::map<std::string, double> exposureValueMap;
    while ((c = getopt (argc, argv, optstring)) != -1)
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
        default:
            cerr << "Invalid parameter: " << optarg << std::endl;
            usage(argv[0]);
            return 1;
        }

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

    // TODO: sort images in pano by exposure

    std::string pixelType;

    try {
        vigra::ImageImportInfo firstImgInfo(files[0].c_str());
        pixelType = firstImgInfo.getPixelType();
    } catch (std::exception & e) {
        cerr << "ERROR: caught exception: " << e.what() << std::endl;
        return 1;
    }

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
        return 1;
    }

    return 0;
}
