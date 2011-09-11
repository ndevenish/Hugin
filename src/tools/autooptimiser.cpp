// -*- c-basic-offset: 4 -*-

/** @file autooptimiser.cpp
 *
 *  @brief a smarter PTOptimizer, with pairwise optimisation
 *         before global optimisation starts
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

#include <hugin_config.h>
#include <hugin_version.h>

#include <fstream>
#include <sstream>
#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif

#include <hugin_basic.h>
#include <hugin_utils/stl_utils.h>
#include <appbase/ProgressDisplayOld.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include <algorithms/nona/CenterHorizontally.h>
#include <algorithms/basic/StraightenPanorama.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/basic/CalculateOptimalScale.h>
#include <algorithms/optimizer/PhotometricOptimizer.h>
#include <panodata/ImageVariableGroup.h>
#include <panodata/StandardImageVariableGroups.h>
#include "ExtractPoints.h"

using namespace std;
using namespace hugin_utils;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char * name)
{
    cerr << name << ": optimize image positions" << endl
         << "autooptimiser version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] input.pto" << endl
         << "   To read a project from stdio, specify - as input file." << endl
         << endl
         << "  Options:" << endl
         << "     -o file.pto  output file. If obmitted, stdout is used." << endl
         << endl
         << "    Optimisation options (if not specified, no optimisation takes place)" << std::endl
         << "     -a       auto align mode, includes various optimisation stages, depending" << endl
         << "               on the amount and distribution of the control points" << endl
         << "     -p       pairwise optimisation of yaw, pitch and roll, starting from" << endl
         << "              first image" << endl
         << "     -m       Optimise photometric parameters" << endl
         << "     -n       Optimize parameters specified in script file (like PTOptimizer)" << endl
         << endl
         << "    Postprocessing options:" << endl
         << "     -l       level horizon (works best for horizontal panos)" << endl
         << "     -s       automatically select a suitable output projection and size" << endl
         << "    Other options:" << endl
         << "     -q       quiet operation (no progress is reported)" << endl
         << "     -v HFOV  specify horizontal field of view of input images." << endl
         << "               Used if the .pto file contains invalid HFOV values" << endl
         << "               (autopano-SIFT writes .pto files with invalid HFOV)" << endl
         << endl
         << "   When using -a -l -m and -s options together, a similar operation to the \"Align\"" << endl
         << "    button in hugin is performed." << endl
         << endl;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "alho:npqsv:m";
    int c;
    string output;
    bool doPairwise = false;
    bool doAutoOpt = false;
    bool doNormalOpt = false;
    bool doLevel = false;
    bool chooseProj = false;
    bool quiet = false;
    bool doPhotometric = false;
    double hfov = 0.0;
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch (c) {
        case 'o':
            output = optarg;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        case 'p':
            doPairwise = true;
            break;
        case 'a':
            doAutoOpt = true;
            break;
        case 'n':
            doNormalOpt = true;
            break;
        case 'l':
            doLevel = true;
            break;
        case 's':
            chooseProj = true;
            break;
        case 'q':
            quiet = true;
            break;
        case 'v':
            hfov = atof(optarg);
            break;
        case 'm':
            doPhotometric = true;
            break;
        default:
            abort ();
        }
    }

    if (argc - optind != 1) {
        usage(argv[0]);
        return 1;
    }

    const char * scriptFile = argv[optind];

    Panorama pano;
    if (scriptFile[0] == '-') {
        DocumentData::ReadWriteError err = pano.readData(std::cin);
        if (err != DocumentData::SUCCESSFUL) {
            cerr << "error while reading script file from stdin." << endl;
            cerr << "DocumentData::ReadWriteError code: " << err << endl;
            return 1;
        }
    } else {
        ifstream prjfile(scriptFile);
        if (!prjfile.good()) {
            cerr << "could not open script : " << scriptFile << endl;
            return 1;
        }
        pano.setFilePrefix(hugin_utils::getPathPrefix(scriptFile));
        DocumentData::ReadWriteError err = pano.readData(prjfile);
        if (err != DocumentData::SUCCESSFUL) {
            cerr << "error while parsing panos tool script: " << scriptFile << endl;
            cerr << "DocumentData::ReadWriteError code: " << err << endl;
            return 1;
        }
    }

    if (pano.getNrOfImages() == 0) {
        cerr << "Panorama should consist of at least one image" << endl;
        return 1;
    }

    // for bad HFOV (from autopano-SIFT)
    for (unsigned i=0; i < pano.getNrOfImages(); i++) {
        SrcPanoImage img = pano.getSrcImage(i);
        if (img.getProjection() == SrcPanoImage::RECTILINEAR
            && img.getHFOV() >= 180)
        {
            // something is wrong here, try to read from exif data
            double focalLength = 0;
            double cropFactor = 0;
            cerr << "HFOV of image " << img.getFilename() << " invalid, trying to read EXIF tags" << endl;
            bool ok = img.readEXIF(focalLength, cropFactor, true, false);
            if (! ok) {
                if (hfov) {
                    img.setHFOV(hfov);
                } else {
                    cerr << "EXIF reading failed, please specify HFOV with -v" << endl;
                    return 1;
                }
            }
            pano.setSrcImage(i, img);
        }
    }

    if(pano.getNrOfCtrlPoints()==0 && (doPairwise || doAutoOpt || doNormalOpt))
    {
        cerr << "Panorama have to have control points to optimise positions" << endl;
        return 1;
    };
    if (doPairwise && ! doAutoOpt) {
        // do pairwise optimisation
        set<string> optvars;
        optvars.insert("r");
        optvars.insert("p");
        optvars.insert("y");
        AutoOptimise::autoOptimise(pano);

        // do global optimisation
        if (!quiet) std::cerr << "*** Pairwise position optimisation" << endl;
        PTools::optimize(pano);
    } else if (doAutoOpt) {
        if (!quiet) std::cerr << "*** Adaptive geometric optimisation" << endl;
        SmartOptimise::smartOptimize(pano);
    } else if (doNormalOpt) {
        if (!quiet) std::cerr << "*** Optimising parameters specified in PTO file" << endl;
        PTools::optimize(pano);
    } else {
        if (!quiet) std::cerr << "*** Geometric parameters not optimized" << endl;
    }

    if (doLevel)
    {
        bool hasVerticalLines=false;
        CPVector allCP=pano.getCtrlPoints();
        if(allCP.size()>0 && (doPairwise || doAutoOpt || doNormalOpt))
        {
            for(size_t i=0;i<allCP.size() && !hasVerticalLines;i++)
            {
                hasVerticalLines=(allCP[i].mode==ControlPoint::X);
            };
        };
        // straighten only if there are no vertical control points
        if(hasVerticalLines)
        {
            cout << "Skipping automatic leveling because of existing vertical control points." << endl;
        }
        else
        {
            StraightenPanorama(pano).run();
            CenterHorizontally(pano).run();
        };
    }

    if (chooseProj) {
        PanoramaOptions opts = pano.getOptions();
        double hfov, vfov;
        CalculateFitPanorama fitPano = CalculateFitPanorama(pano);
        fitPano.run();
        opts.setHFOV(fitPano.getResultHorizontalFOV());
        opts.setHeight(roundi(fitPano.getResultHeight()));
        vfov = opts.getVFOV();
        hfov = opts.getHFOV();
        // avoid perspective projection if field of view > 100 deg
        double mf = 100;
        if (vfov < mf) {
            // cylindrical or rectilinear
            if (hfov < mf) {
                opts.setProjection(PanoramaOptions::RECTILINEAR);
            } else {
                opts.setProjection(PanoramaOptions::CYLINDRICAL);
            }
        }

        // downscale pano a little
        double sizeFactor = 0.7;

        pano.setOptions(opts);
        double w = CalculateOptimalScale::calcOptimalScale(pano);
        opts.setWidth(roundi(opts.getWidth()*w*sizeFactor), true);
        pano.setOptions(opts);
    }

    if(doPhotometric)
    {
        // photometric estimation
        PanoramaOptions opts = pano.getOptions();
        int nPoints = 200;
        int pyrLevel=3;
        bool randomPoints = true;
        nPoints = nPoints * pano.getNrOfImages();
 
        std::vector<vigra_ext::PointPairRGB> points;
        ProgressDisplay *progressDisplay;
        if(!quiet)
            progressDisplay=new StreamProgressDisplay(std::cout);
        else
            progressDisplay=new DummyProgressDisplay();
        try 
        {
            loadImgsAndExtractPoints(pano, nPoints, pyrLevel, randomPoints, *progressDisplay, points, !quiet);
        } 
        catch (std::exception & e)
        {
            cerr << "caught exception: " << e.what() << endl;
            return 1;
        };
        if(!quiet)
            cout << "\rSelected " << points.size() << " points" << endl;

        if (points.size() == 0)
        {
            cerr << "Error: no overlapping points found, exiting" << endl;
            return 1;
        }

        progressDisplay->startSubtask("Photometric Optimization", 0.0);
        // first, ensure that vignetting and response coefficients are linked
        const HuginBase::ImageVariableGroup::ImageVariableEnum vars[] = {
                HuginBase::ImageVariableGroup::IVE_EMoRParams,
                HuginBase::ImageVariableGroup::IVE_ResponseType,
                HuginBase::ImageVariableGroup::IVE_VigCorrMode,
                HuginBase::ImageVariableGroup::IVE_RadialVigCorrCoeff,
                HuginBase::ImageVariableGroup::IVE_RadialVigCorrCenterShift
        };
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        HuginBase::ImageVariableGroup & lenses = variable_groups.getLenses();
        for (size_t i = 0; i < lenses.getNumberOfParts(); i++)
        {
            std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> links_needed;
            links_needed.clear();
            for (int v = 0; v < 5; v++)
            {
                if (!lenses.getVarLinkedInPart(vars[v], i))
                {
                    links_needed.insert(vars[v]);
                }
            };
            if (!links_needed.empty())
            {
                std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator it;
                for (it = links_needed.begin(); it != links_needed.end(); it++)
                {
                    lenses.linkVariablePart(*it, i);
                }
            }
        }

        HuginBase::SmartPhotometricOptimizer::PhotometricOptimizeMode optmode = 
            HuginBase::SmartPhotometricOptimizer::OPT_PHOTOMETRIC_LDR;
        if (opts.outputMode == PanoramaOptions::OUTPUT_HDR)
        {
            optmode = HuginBase::SmartPhotometricOptimizer::OPT_PHOTOMETRIC_HDR;
        }
        SmartPhotometricOptimizer photoOpt(pano, progressDisplay, pano.getOptimizeVector(), points, optmode);
        photoOpt.run();

        // calculate the mean exposure.
        opts.outputExposureValue = CalculateMeanExposure::calcMeanExposure(pano);
        pano.setOptions(opts);
        progressDisplay->finishSubtask();
        delete progressDisplay;
    };

    // write result
    OptimizeVector optvec = pano.getOptimizeVector();
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    if (output != "") {
        ofstream of(output.c_str());
        pano.printPanoramaScript(of, optvec, pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(scriptFile));
    } else {
        pano.printPanoramaScript(cout, optvec, pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(scriptFile));
    }
    return 0;
}
