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
#include <algorithms/optimizer/PTOptimizer.h>
#include <nona/Stitcher.h>

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
         << "align_image_stack version " << PACKAGE_VERSION << std::endl
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
void createCtrlPoints(Panorama & pano, int img1, const ImageType & leftImg, int img2, const ImageType & rightImg, int pyrLevel, double scale, unsigned nPoints, unsigned grid)
{
    typedef typename ImageType::value_type VT;
    //////////////////////////////////////////////////
    // find interesting corners using harris corner detector
    typedef std::vector<std::multimap<double, vigra::Diff2D> > MapVector;

    std::vector<std::multimap<double, vigra::Diff2D> >points;
    if (g_verbose > 0) {
        std::cout << "Trying to find " << nPoints << " corners... ";
    }

    vigra_ext::findInterestPointsOnGrid(srcImageRange(leftImg, GreenAccessor<VT>()), scale, 5*nPoints, grid, points);

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
                cout << (*it).second.x << "," << (*it).second.y << " -> "
                        << res.maxpos.x << "," << res.maxpos.y << ":  corr coeff: " <<  res.maxi
                        << " curv:" <<  res.curv.x << " " << res.curv.y << std::endl;
            }
            if (res.maxi < corrThresh )
            {
                nBad++;
                DEBUG_DEBUG("low correlation: " << res.maxi << " curv: " << res.curv);
            } else {
                nGood++;
                // add control point
                ControlPoint p(img1, (*it).second.x * scaleFactor,
                                (*it).second.y * scaleFactor,
                                img2, res.maxpos.x * scaleFactor,
                                res.maxpos.y * scaleFactor);
                pano.addCtrlPoint(p);
            }
        }
        if (g_verbose > 0) {
            cout << "Number of good matches: " << nGood << ", bad matches: " << nBad << std::endl;
        }
    }
};


struct Parameters
{
    Parameters()
    {
        cpErrorThreshold = 3;
        nPoints = 8;
	grid = 5;
        hfov = 0;
        pyrLevel = 1;
	linear = false;	   // Assume linear input files if true
        optHFOV = false;
        fisheye = false;
    }

    double cpErrorThreshold;
    int nPoints;
    int grid;		// Partition images into grid x grid subregions, each with npoints
    double hfov;
    bool linear;
    bool optHFOV;
    bool fisheye;
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

        // rescale image
        ImageType * leftImg = new ImageType();
        {
            ImageType leftImgOrig(firstImgInfo.size());
            if(firstImgInfo.numExtraBands() == 1) {
                vigra::BImage alpha(firstImgInfo.size());
                vigra::importImageAlpha(firstImgInfo, destImage(leftImgOrig), destImage(alpha));
            } else if (firstImgInfo.numExtraBands() == 0) {
                vigra::importImage(firstImgInfo, destImage(leftImgOrig));
            } else {
                vigra_fail("Images with multiple extra (alpha) channels not supported");
            }
            reduceNTimes(leftImgOrig, *leftImg, param.pyrLevel);
        }


        Panorama pano;
        Lens l;
        pano.addLens(l);

        // add the first image.to the panorama object
        // default settings
        double focalLength = 50;
        double cropFactor = 0;
        VariableMap defaultVars;
        fillVariableMap(defaultVars);

        SrcPanoImage srcImg;
        srcImg.setFilename(files[0]);

        if (param.fisheye) {
            srcImg.setProjection(SrcPanoImage::FULL_FRAME_FISHEYE);
        }
        srcImg.readEXIF(focalLength, cropFactor, true);
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

        PanoImage panoImg(files[0], srcImg.getSize().x, srcImg.getSize().y, 0);
        int imgNr = pano.addImage(panoImg, defaultVars);
        // unlink HFOV?
        if (param.optHFOV) {
            l = pano.getLens(0);
            LensVariable lv = map_get(l.variables, "v");
            lv.setLinked(false);
            pano.updateLensVariable(0, lv);
        }
	if (param.linear) {
	    srcImg.setResponseType(SrcPanoImage::RESPONSE_LINEAR);
	    if (g_verbose>0) {
		cout << "Using linear response" << std::endl;
	    }
	}
        pano.setSrcImage(imgNr, srcImg);

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

        // loop to add images and control points between them.
        for (int i = 1; i < (int) files.size(); i++) {
            if (g_verbose > 0) {
                cout << "Creating control points between " << files[i-1] << " and " << files[i] << std::endl;
            }
            // add next image.
            srcImg.setFilename(files[i]);
            srcImg.readEXIF(focalLength, cropFactor, true);
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

            PanoImage panoImg(files[i], srcImg.getSize().x, srcImg.getSize().y, 0);
            pano.addLens(l);
            int imgNr = pano.addImage(panoImg, defaultVars);
            pano.setSrcImage(imgNr, srcImg);

            // load the actual image data of the next image
            vigra::ImageImportInfo nextImgInfo(files[i].c_str());
            assert(nextImgInfo.size() == firstImgInfo.size());
            {
                ImageType rightImgOrig(nextImgInfo.size());
                if (nextImgInfo.numExtraBands() == 1) {
                    vigra::BImage alpha(nextImgInfo.size());
                    vigra::importImageAlpha(nextImgInfo, destImage(rightImgOrig), destImage(alpha));
                } else if (nextImgInfo.numExtraBands() == 0) {
                    vigra::importImage(nextImgInfo, destImage(rightImgOrig));
                } else {
                    vigra_fail("Images with multiple extra (alpha) channels not supported");
                }
                reduceNTimes(rightImgOrig, *rightImg, param.pyrLevel);
            }

            // add control points.
            // work on smaller images
            // TODO: or use a fast interest point operator.
            createCtrlPoints(pano, i-1, *leftImg, i, *rightImg, param.pyrLevel, 2, param.nPoints, param.grid);

            // swap images;
            delete leftImg;
            leftImg = rightImg;
            rightImg = new ImageType(leftImg->size());

            // optimize yaw, roll, pitch
            std::set<std::string> vars;
            vars.insert("y");
            vars.insert("p");
            vars.insert("r");
            if (param.optHFOV) {
                vars.insert("v");
            }
            optvars.push_back(vars);
        }
        delete leftImg;
        delete rightImg;

        // optimize everything.
        pano.setOptimizeVector(optvars);
        PTools::optimize(pano);

        // need to do some basic outlier pruning.
        // remove all points with error higher than a specified threshold
        if (param.cpErrorThreshold > 0) {
            CPVector cps = pano.getCtrlPoints();
            CPVector newCPs;
            for (int i=0; i < (int)cps.size(); i++) {
                if (cps[i].error < param.cpErrorThreshold) {
                    newCPs.push_back(cps[i]);
                }
            }
            if (g_verbose > 0) {
                cout << "Ctrl points before pruning: " << cps.size() << ", after: " << newCPs.size() << std::endl;
            }
            pano.setCtrlPoints(newCPs);
            // reoptimize
            PTools::optimize(pano);
        }

        UIntSet imgs = pano.getActiveImages();

        if (param.ptoFile.size() > 0) {
            std::ofstream script(param.ptoFile.c_str());
            pano.printPanoramaScript(script, optvars, pano.getOptions(), imgs, false, "");
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
    } catch (std::exception & e) {
        cerr << "ERROR: caught exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "a:ef:g:hlmp:vo:s:t:c:";
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

    vigra::ImageImportInfo firstImgInfo(files[0].c_str());

    std::string pixelType = firstImgInfo.getPixelType();
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
