// -*- c-basic-offset: 4 -*-

/** @file align_image_stack.cpp
 *
 *  @brief program to align a set of well overlapping images (~90%)
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

#include <config.h>
#include <fstream>
#include <sstream>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>
#include <vigra/cornerdetection.hxx>
#include <vigra/localminmax.hxx>

#include <common/utils.h>

#include <vigra_ext/Pyramid.h>
#include <vigra_ext/Correlation.h>

#include <PT/PTOptimise.h>


#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif

#include "panoinc.h"
#include "PT/Stitcher.h"

#include <tiff.h>

using namespace vigra;
using namespace PT;
using namespace std;
using namespace vigra_ext;
using namespace utils;
using namespace PTools;

int g_verbose = 0;

static void usage(const char * name)
{
    cerr << name << ": align overlapping images for HDR creation" << std::endl
         << std::endl
         << "Usage: " << name  << " [options] input files" << std::endl
         << "Valid options are:" << std::endl
         << "  -o prefix save aligned images as prefix0000.tif and so on (default: aligned_)" << std::endl
         << "  -p file   Output .pto file (useful for debugging, or further refinement)" << std::endl
         << "  -v        Verbose, print progress messages" << std::endl
         << "  -t num    Remove all control points with an error higher than num (default: 3)" << std::endl
         << "  -c num    Harris corner threshold (default: 5)" << std::endl
         << "  -h        Display help (this text)" << std::endl
         << std::endl;
}


template <class ImageType>
void createCtrlPoints(Panorama & pano, int img1, const ImageType & leftImg, int img2, const ImageType & rightImg, double scale, double cornerThreshold)
{
    //////////////////////////////////////////////////
    // find interesting corners using harris corner detector

    BImage leftCorners(leftImg.size(), vigra::UInt8(0));
    FImage leftCornerResponse(leftImg.size());

    DEBUG_DEBUG("running corner detector threshold: " << cornerThreshold << "  scale: " << scale );

    // find corner response at scale scale
    vigra::cornerResponseFunction(srcImageRange(leftImg, GreenAccessor<typename ImageType::value_type>()),
                                  destImage(leftCornerResponse),
                                  scale);

    //saveScaledImage(leftCornerResponse,"corner_response.png");
    DEBUG_DEBUG("finding local maxima");
    // find local maxima of corner response, mark with 1
    vigra::localMaxima(srcImageRange(leftCornerResponse), destImage(leftCorners), 255);

    if (g_verbose > 5)
        exportImage(srcImageRange(leftCorners), vigra::ImageExportInfo("corner_response_maxima.png"));

    DEBUG_DEBUG("thresholding corner response");
    // threshold corner response to keep only strong corners (above 400.0)
    transformImage(srcImageRange(leftCornerResponse), destImage(leftCornerResponse),
                   vigra::Threshold<double, double>(
                           cornerThreshold, DBL_MAX, 0.0, 1.0));

    vigra::combineTwoImages(srcImageRange(leftCorners), srcImage(leftCornerResponse),
                            destImage(leftCorners), std::multiplies<float>());

    if (g_verbose > 5)
        exportImage(srcImageRange(leftCorners), vigra::ImageExportInfo("corner_response_threshold.png"));


    int nGood = 0;
    int nBad = 0;
    // sample grid on img1 and try to add ctrl points
    for (int x=0; x < leftImg.size().x; x++ ) {
        for (int y=0; y < leftImg.size().y; y++ ) {
            if (leftCorners(x,y) > 0) {
                // we found a corner. correlate with second image

                // load parameters

                long templWidth = 20;
                long sWidth = 100;
                double corrThresh = 0.9;
                double curvThresh = 0.0;

                vigra_ext::CorrelationResult res;
                vigra::Diff2D roundP1(x, y);
                vigra::Diff2D roundP2(x, y);

                res = vigra_ext::PointFineTune(leftImg,
                                               roundP1,
                                               templWidth,
                                               rightImg,
                                               roundP2,
                                               sWidth
                                              );

                if (g_verbose > 1) {
                    cout << "corr coeff: " <<  res.maxi << " curv:" <<  res.curv.x << " " << res.curv.y << std::endl;
                }
                if (res.maxi < corrThresh )
                {
                    nBad++;
                    DEBUG_DEBUG("low correlation: " << res.maxi << " curv: " << res.curv);
                } else {
                    nGood++;
                    // add control point
                    ControlPoint p(img1, roundP1.x, roundP1.y, img2, res.maxpos.x, res.maxpos.y);
                    pano.addCtrlPoint(p);
                }
            }
        }
    }
    if (g_verbose > 0) {
        cout << "Number of good matches: " << nGood << ", bad matches: " << nBad << std::endl;
    }
};


int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "hp:vo:t:c:o:";
    int c;

    opterr = 0;

    g_verbose = 0;
    int nPoints = 100;
    double cpErrorThreshold = 3;
    double cornerThreshold = 5;
    std::string outputPrefix = "aligned_";
    std::string ptoFile;
    string basename;
    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'c':
            cornerThreshold = atof(optarg);
            break;
        case 't':
            cpErrorThreshold = atof(optarg);
            break;
        case 'p':
            ptoFile = optarg;
            break;
        case 'o':
            outputPrefix = optarg;
            break;
        case 'v':
            g_verbose++;
            break;
        case 'h':
            usage(argv[0]);
            return 1;
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

    // extract file names
    std::vector<std::string> files;
    for (size_t i=0; i < nFiles; i++)
        files.push_back(argv[optind+i]);

    // suppress tiff warnings
    TIFFSetWarningHandler(0);

//    utils::StreamMultiProgressDisplay pdisp(cout);
    //utils::MultiProgressDisplay pdisp;

    typedef vigra::BRGBImage ImageType;
    try {
        // load first image
        vigra::ImageImportInfo firstImgInfo(files[0].c_str());
        ImageType * leftImg = new BRGBImage(firstImgInfo.size());
        vigra::importImage(firstImgInfo, destImage(*leftImg));

        ImageType * rightImg = new BRGBImage(firstImgInfo.size());

        Panorama pano;
        Lens l;
        pano.addLens(l);

        // add the first image.to the panorama object
        // default settings
        double focalLength = 50;
        double cropFactor = 1;
        VariableMap defaultVars;
        fillVariableMap(defaultVars);

        SrcPanoImage srcImg;
        srcImg.setFilename(files[0]);
        bool ok = initImageFromFile(srcImg, focalLength, cropFactor);
        if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
            cerr << "Could not decode image: " << files[0] << "Unsupported image file format";
            return 1;
        }
        PanoImage panoImg(files[0], srcImg.getSize().x, srcImg.getSize().y, 0);
        int imgNr = pano.addImage(panoImg, defaultVars);
        pano.setSrcImage(imgNr, srcImg);

        // setup output to be exactly similar to input image
        PanoramaOptions opts;
        opts.setWidth(srcImg.getSize().x, false);
        opts.setHeight(srcImg.getSize().y);
        opts.setHFOV(srcImg.getHFOV(), false);
        opts.setProjection(PanoramaOptions::RECTILINEAR);
            // output to tiff format
        opts.outputFormat = PanoramaOptions::TIFF_m;
        opts.outfile = outputPrefix;
        // m estimator, to be more robust against points on moving objects
        opts.huberSigma = 2;
        pano.setOptions(opts);

        // variables that should be optimized
        // optimize nothing in the first image
        OptimizeVector optvars(1);

        // loop to add images and control points between them.
        for (int i = 1; i < nFiles; i++) {
            if (g_verbose > 0) {
                cout << "Creating control points between " << files[i-1] << " and " << files[i] << endl;
            }
            // add next image.
            srcImg.setFilename(files[i]);
            bool ok = initImageFromFile(srcImg, focalLength, cropFactor);
            if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
                cerr << "Could not decode image: " << files[i] << "Unsupported image file format";
                return 1;
            }
            PanoImage panoImg(files[i], srcImg.getSize().x, srcImg.getSize().y, 0);
            int imgNr = pano.addImage(panoImg, defaultVars);
            pano.setSrcImage(imgNr, srcImg);

            // load the actual image data of the next image
            vigra::ImageImportInfo nextImgInfo(files[i].c_str());
            assert(nextImgInfo.size() == firstImgInfo.size());
            vigra::importImage(nextImgInfo, destImage(*rightImg));

            // add control points.
            // TODO: work on smaller images, or use a fast descriptor, based on
            // integral images.
            createCtrlPoints(pano, i-1, *leftImg, i, *rightImg, 8, cornerThreshold);

            // swap images;
            delete leftImg;
            leftImg = rightImg;
            rightImg = new ImageType(leftImg->size());

            // optimize yaw, roll, pitch
            std::set<std::string> vars;
            vars.insert("y");
            vars.insert("p");
            vars.insert("r");
            optvars.push_back(vars);
        }
        delete leftImg;
        delete rightImg;

        // optimize everything.
        pano.setOptimizeVector(optvars);
        PTools::optimize(pano);

        // need to do some basic outlier pruning.
        // remove all points with error higher than a specified threshold
        CPVector cps = pano.getCtrlPoints();
        CPVector newCPs;
        for (int i=0; i < cps.size(); i++) {
            if (cps[i].error < cpErrorThreshold) {
                newCPs.push_back(cps[i]);
            }
        }
        if (g_verbose > 0) {
            cout << "Ctrl points before pruning: " << cps.size() << ", after: " << newCPs.size() << std::endl;
        }
        pano.setCtrlPoints(newCPs);
        // reoptimize
        PTools::optimize(pano);

        UIntSet imgs = pano.getActiveImages();

        if (ptoFile.size() > 0) {
            std::ofstream script(ptoFile.c_str());
            pano.printPanoramaScript(script, optvars, pano.getOptions(), imgs, false, "");
        }

        // remap all images
        utils::StreamMultiProgressDisplay progress(cout);

        PT::stitchPanorama(pano, opts,
                           progress, opts.outfile, imgs);

    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
