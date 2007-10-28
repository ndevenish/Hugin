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

#include <config.h>
#include <fstream>
#include <sstream>

#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif


#include <panoinc.h>
#include "common/stl_utils.h"
#include "PT/ImageGraph.h"
#include "PT/PTOptimise.h"

//using namespace vigra;
//using namespace vigra_ext;
using namespace PT;
using namespace std;
using namespace utils;

static void usage(const char * name)
{
    cerr << name << ": optimize image positions" << endl
         << endl
         << "Usage:  " << name << " [options] input.pto" << endl
         << "   To read a project from stdio, specify - as input file." << endl
         << endl
         << "  Options:" << endl
         << "     -o file.pto  output file. If obmitted, stdout is used." << endl
         << "     -p       pairwise optimisation of yaw, pitch and roll, starting from" << endl
         << "              first image" << endl
         << "     -a       auto align mode, includes various optimisation stages, depending" << endl
         << "               on the amount and distribution of the control points" << endl
         << "     -l       level horizon" << endl
         << "     -s       automatically select a suitable output projection and size" << endl
         << "     -q       quiet operation (no progress is reported)" << endl
         << "     -v HFOV  specify horizontal field of view of input images." << endl
         << "               Used if the .pto file contains invalid HFOV values" << endl
         << "               (autopano-SIFT writes .pto files with invalid HFOV)" << endl
         << endl
         << "   When using -a -l and -s options together, a similar operation to the \"Align\"" << endl
         << "    button in hugin is performed." << endl
         << endl;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "alho:pqsv:";
    int c;
    string output;
    bool doPairwise = false;
    bool doAutoOpt = false;
    bool doLevel = false;
    bool chooseProj = false;
    bool quiet = false;
    double hfov = 0.0;
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch (c) {
        case 'o':
            output = optarg;
            break;
        case 'h':
            usage(argv[0]);
            return 1;
        case 'p':
            doPairwise = true;
            break;
        case 'a':
            doAutoOpt = true;
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
        default:
            abort ();
        }
    }

    if (argc - optind != 1) {
        usage(argv[0]);
        return 1;
    }

    const char * scriptFile = argv[optind];

    utils::MultiProgressDisplay * pdisp;
    if (!quiet) {
        pdisp = new utils::StreamMultiProgressDisplay(cerr);
    } else {
        pdisp = new utils::MultiProgressDisplay;
    }
//    utils::CoutProgressDisplay pdisp;

    Panorama pano;
    PanoramaMemento newPano;
    if (scriptFile == "-") {
        if (newPano.loadPTScript(cin)) {
            pano.setMemento(newPano);
        } else {
            cerr << "error while reading script file from stdin" << endl;
            return 1;
        }
    } else {
        ifstream prjfile(scriptFile);
        if (prjfile.bad()) {
            cerr << "could not open script : " << scriptFile << endl;
            return 1;
        }
        if (newPano.loadPTScript(prjfile)) {
            pano.setMemento(newPano);
        } else {
            cerr << "error while parsing panos tool script: " << scriptFile << endl;
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
            bool ok = initImageFromFile(img, focalLength, cropFactor);
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

    if (doPairwise && ! doAutoOpt) {
        // do pairwise optimisation
        set<string> optvars;
        optvars.insert("r");
        optvars.insert("p");
        optvars.insert("y");
        PTools::autoOptimise(pano);

        // do global optimisation
        PTools::optimize(pano);
    } else if (doAutoOpt) {
        PTools::smartOptimize(pano);
    }

    if (doLevel) {
        // straighten
        pano.straighten();
        pano.centerHorizontically();
    }

    if (chooseProj) {
        PanoramaOptions opts = pano.getOptions();
        double hfov, vfov, height;
        pano.fitPano(hfov, height);
        opts.setHFOV(hfov);
        opts.setHeight(roundi(height));
        vfov = opts.getVFOV();
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
        int w = pano.calcOptimalWidth();
        opts.setWidth(roundi(w*sizeFactor), true);
        pano.setOptions(opts);
    }

    OptimizeVector optvec = pano.getOptimizeVector();
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    if (output != "") {
        ofstream of(output.c_str());
        pano.printPanoramaScript(of, optvec, pano.getOptions(), imgs, false);
    } else {
        pano.printPanoramaScript(cout, optvec, pano.getOptions(), imgs, false);
    }
    return 0;
}
