// -*- c-basic-offset: 4 -*-

/** @file vig_optimize.cpp
 *
 *  @brief a simple test stitcher
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

#include <algorithms/optimizer/PhotometricOptimizer.h>
#include "ExtractPoints.h"

#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif

#include <hugin_basic.h>
#include <nona/Stitcher.h>

#include <tiff.h>

using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace AppBase;
using namespace HuginBase;



static void usage(const char * name)
{
    cerr << name << ": Try to determine the radial vignetting" << std::endl
         << "vig_optimize version " << DISPLAY_VERSION << endl
         << std::endl
         << "Usage: " << name  << " [options] -o output.pto input.pto" << std::endl
         << "Valid options are:" << std::endl
         << "  -o file   write results to output project" << std::endl
         << "  -v        Verbose, print progress messages" << std::endl
         << "  -p n      Number of points to extract" << std::endl
       // random points is default, all points was only implemented for testing purposes
       // << "  -r        Extract random point (faster, but less accurate)" << std::endl
         << "  -s level  Work on downscaled images, every step halves width and height" << std::endl
         << "  -h        Display help (this text)" << std::endl
         << std::endl
         << " Expert and debugging options:" << std::endl
         << "  -i file   Read corresponding points from file" << std::endl
         << "  -w file   Dump corresponding points to file" << std::endl;
}


/*
template<class RIMG>
void importROIImage(ImageImportInfo & info, RIMG & img)
{ 
    vigra::Rect2D roi(Point2D(info.getPosition()), info.size());
    img.resize(roi);
    importImageAlpha(info, destImage(img.m_image), destImage(img.m_mask));
}
*/


void loadPointsC(FILE *f, std::vector<PointPairRGB> & vec)
{
    double dummy1, dummy2;
    PointPairRGB point;
    double i1, i2;

    int n=0;
    do {
        n = fscanf(f, " %lf %lf %lf %lf %lf %lf %f %f %f %f %f %f %lf %f %f %lf",
                    &i1, &i2, &(point.p1.x), &(point.p1.y),
                    &(point.p2.x), &(point.p2.y),
                    &(point.i1.red()), &(point.i1.green()), &(point.i1.blue()),
                    &(point.i2.red()), &(point.i2.green()), &(point.i2.blue()),
                                    // matlab index.. set to zero
                    &dummy1,
                    &(point.r1), &(point.r2),
                                    // edge score at this point, zero so far
                    &(dummy2) );


        point.imgNr1 = hugin_utils::roundi(i1);
        point.imgNr2 = hugin_utils::roundi(i2);
        if (n==16) {
            vec.push_back(point);
        }
    } while (n == 16);
}

void loadPoints(istream & i, std::vector<PointPairRGB> & vec )
{
    while(!i.eof() && i.good()) {
        double dummy1, dummy2;
        PointPairRGB point;
        double i1, i2;
        i >> i1 >> i2 
                >> point.p1.x >> point.p1.y 
                >> point.p2.x  >> point.p2.y 
                >> point.i1.red()  >> point.i1.green()  >> point.i1.blue() 
                >> point.i2.red()  >> point.i2.green()  >> point.i2.blue() 
                        // matlab index.. set to zero
                >> dummy1 
                >> point.r1  >> point.r2  
                        // edge score at this point, zero so far
                >> dummy2;
        point.imgNr1 = hugin_utils::roundi(i1);
        point.imgNr2 = hugin_utils::roundi(i2);
        if (i.good()) {
            vec.push_back(point);
        }
    }
}

bool hasphotometricParams(Panorama &pano) 
{
    OptimizeVector vars=pano.getOptimizeVector();

    for (OptimizeVector::const_iterator it=vars.begin(); it != vars.end(); ++it)
    {
        std::set<std::string> cvars;
        for (std::set<std::string>::const_iterator itv = (*it).begin();
             itv != (*it).end(); ++itv)
        {
            if ((*itv)[0] == 'E' || (*itv)[0] == 'R' || (*itv)[0] == 'V') {
                return true;
            }
        }
    }
    return false;
}


int main(int argc, char *argv[])
{
    StreamProgressDisplay progressDisplay(std::cout);
    
    // parse arguments
    const char * optstring = "hi:o:p:s:vw:";
    int c;

    opterr = 0;

    int pyrLevel=3;
    int verbose = 0;
    int nPoints = 200;
    bool randomPoints = true;
    std::string outputFile;
    std::string outputPointsFile;
    std::string inputPointsFile;
    string basename;
    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'i':
            inputPointsFile = optarg;
            break;
        case 'o':
            outputFile = optarg;
            break;
        case 'p':
            nPoints = atoi(optarg);
            break;
        /*case 'r':
            randomPoints = true;
            break; */
        case 's':
            pyrLevel=atoi(optarg);
            break;
        case 'v':
            verbose++;
            break;
        case 'h':
            usage(argv[0]);
            return 1;
        case 'w':
            outputPointsFile = optarg;
            break;
        default:
            cerr << "Invalid parameter: " << optarg << std::endl;
            usage(argv[0]);
            return 1;
        }

    unsigned nFiles = argc - optind;
    if (nFiles != 1) {
        std::cerr << std::endl << "Error: one pto file needs to be specified" << std::endl <<std::endl;
        usage(argv[0]);
        return 1;
    }

    const char * scriptFile = argv[optind];
    Panorama pano;
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

    // Ensure photometric parameters are selected for optimizaion
    if (!hasphotometricParams(pano)) {
        cerr << "ERROR:no photometric parameters were selected for optimization" << endl;
        cerr << "please update 'v' line in .pto script and try again." << endl;
        return 1;
    }

    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    // todo: handle images with alpha masks
    try {
        std::vector<vigra_ext::PointPairRGB> points;

        if (inputPointsFile != "" ) {
            //ifstream ifs(inputPointsFile.c_str());
            //loadPoints(ifs, points);
            FILE * f = fopen(inputPointsFile.c_str(), "r");
            if (f == 0) {
                perror("Could not open input point file");
                return 1;
            }
            loadPointsC(f, points);
            fclose(f);
        } else {
            loadImgsAndExtractPoints(pano, nPoints, pyrLevel, randomPoints, progressDisplay, points, verbose);
        }
        if (verbose)
            cout << "\rSelected " << points.size() << " points" << std::endl;
        if (points.size() == 0) {
            std::cerr << "Error: no overlapping points found, exiting" << std::endl;
            return 1;
        }

        if (outputPointsFile.size() > 0) {
            ofstream of(outputPointsFile.c_str());
            for (std::vector<PointPairRGB>::iterator it = points.begin(); it != points.end(); ++it) {
                of << (*it).imgNr1 << " " << (*it).imgNr2 << " " 
                        << (*it).p1.x << " " << (*it).p1.y<< " " 
                        << (*it).p2.x << " " << (*it).p2.y<< " " 
                        << (*it).i1.red() << " " << (*it).i1.green() << " " << (*it).i1.blue() << " "
                        << (*it).i2.red() << " " << (*it).i2.green() << " " << (*it).i2.blue() << " "
                        // matlab index.. set to zero
                        << 0 << " "
                        << (*it).r1 << " " << (*it).r2 << " " 
                        // edge score at this point, zero so far
                        << 0 << std::endl;
            }
        }

        
        progressDisplay.startSubtask("Vignetting Optimization", 0.0);
		PhotometricOptimizer photoopt(pano, &progressDisplay, pano.getOptimizeVector(), points);
		photoopt.run();
		//		double error = photoopt.getResultError();
        
        progressDisplay.finishSubtask();
        
        UIntSet allImgs;
        fill_set(allImgs,0, pano.getNrOfImages()-1);
        // save project
        if (outputFile == "" || outputFile == "-") {
            pano.printPanoramaScript(std::cout, pano.getOptimizeVector(), pano.getOptions(), allImgs, false, hugin_utils::getPathPrefix(scriptFile));
        } else {
            ofstream of(outputFile.c_str());
            pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), allImgs, false, hugin_utils::getPathPrefix(scriptFile));
        }

    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
