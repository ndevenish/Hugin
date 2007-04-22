// -*- c-basic-offset: 4 -*-

/** @file hugin_merge_images.cpp
 *
 *  @brief merge images
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
    cerr << name << ": merge overlapping images" << std::endl
         << std::endl
         << "Usage: " << name  << " [options] -o output.exr input files" << std::endl
         << "Valid options are:" << std::endl
         << "  -o prefix output file" << std::endl
         << "  -m mode   merge mode, can be one of: hdr" << std::endl
         << "  -v        Verbose, print progress messages" << std::endl
         << "  -h        Display help (this text)" << std::endl
         << std::endl;
}


int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "hvo:m:";
    int c;

    opterr = 0;

    g_verbose = 0;
    std::string output = "merged.exr";
    std::string mode = "hdr";
    string basename;
    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'm':
            mode = optarg;
            break;
        case 'o':
            output = optarg;
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
        mergeFunctor
        reduceOpenEXRFiles(files, output,);
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
        double cropFactor = 0;
        VariableMap defaultVars;
        fillVariableMap(defaultVars);

        SrcPanoImage srcImg;
        srcImg.setFilename(files[0]);
        initImageFromFile(srcImg, focalLength, cropFactor);
        if (srcImg.getSize().x == 0 || srcImg.getSize().y == 0) {
            cerr << "Could not decode image: " << files[0] << "Unsupported image file format";
            return 1;
        }

        // use hfov specified by user.
        if (hfov > 0) {
            srcImg.setHFOV(hfov);
        } else if (cropFactor == 0) {
            // could not read HFOV, assuming default: 50
            srcImg.setHFOV(50);
        }

        PanoImage panoImg(files[0], srcImg.getSize().x, srcImg.getSize().y, 0);
        int imgNr = pano.addImage(panoImg, defaultVars);
        pano.setSrcImage(imgNr, srcImg);

        // setup output to be exactly similar to input image
        PanoramaOptions opts;
        opts.setHFOV(srcImg.getHFOV(), false);
        opts.setWidth(srcImg.getSize().x, false);
        opts.setHeight(srcImg.getSize().y);
        opts.setProjection(PanoramaOptions::RECTILINEAR);
            // output to tiff format
        opts.outputFormat = PanoramaOptions::TIFF_m;
        opts.tiff_saveROI = false;
        opts.outfile = outputPrefix;
        // m estimator, to be more robust against points on moving objects
        opts.huberSigma = 2;
        pano.setOptions(opts);

        // variables that should be optimized
        // optimize nothing in the first image
        OptimizeVector optvars(1);

        // loop to add images and control points between them.
        for (int i = 1; i < (int) nFiles; i++) {
            if (g_verbose > 0) {
                cout << "Creating control points between " << files[i-1] << " and " << files[i] << endl;
            }
            // add next image.
            srcImg.setFilename(files[i]);
            initImageFromFile(srcImg, focalLength, cropFactor);
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
        for (int i=0; i < (int)cps.size(); i++) {
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
