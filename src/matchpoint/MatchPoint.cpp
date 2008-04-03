/***************************************************************************
 *   Copyright (C) 2007 by Zoran Mesec   *
 *   zoran.mesec@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
//#define USE_OPENCV
//#define USE_QT
#define USE_VIGRA

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <ctime>
#include <algorithm>

#include <string>
#include "APImage.h"
#include "HessianDetector.h"
#include "Descriptor.h"

#include "getopt.h"

#include <limits>

#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif


using namespace std;

void usage()
{
    cout << "MatchPoint: run feature detection and extraction" << endl
         << endl
	 << "Usage:  MatchPoint [options] image1.jpg output.key" << endl
	 << "Options:" << endl

    /*    // << "  -x              generate xml suitable for Autopano-SIFT feature matcher" << endl
	 << "  -o file          generate XML file with interest points and descriptors(image1.jpg.xml, image2.jpg.xml)" << endl
	 << "  -n num          extract num interest points an descriptors" << endl*/
     << "  -v              verbose output" << endl
     << "  -t              generate keypoint file for matlab test suite(file name is generated using formula: image1.jpg.key)" << endl
	 //<< "                  TODO: replace with threshold, default 1000" << endl
	 << "Arguments:" << endl
	 << "  image1.jpg      Path to image to be analyzed." << endl
	 << "  output.key      Output keypoint file.." << endl
	 << endl;
}


int main(int argc, char *argv[])
{



    const char * optstring = "hvt";
    char c;
    clock_t start,finish;
    double time;

    bool verbose = false;
    bool testFileOutput = false;
    string input1;
    string output;

    // parse arguments
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch(c) {
            case 'v':
                verbose = true;
                break;
            case 't':
                testFileOutput=true;
                break;
            /*case 'o':
                output = optarg;
            break;
        case 'n':
            nrPoints = atoi(optarg);
            break;*/
            case 'h':
                usage();
                //return 0;
                break;
            default:
                usage();
                return 0;
        }
        //optind++;
    }

    if (optind+2 !=  argc) {
        usage();
       return 1;
    }

    input1=argv[optind];
    output=argv[optind+1];
    if (verbose) cerr << "Input image: " <<input1 << "; Output key file: "<<output<<endl;

    APImage im1(input1);
    if(!im1.open()) {
        cerr<< "Error! Image can not be opened"<<"\n";
        return 0;
    }

    //need to integrate the image before the detection process(only if using box filter approximation)
    im1.integrate();

    start = clock();

    HessianDetector hd1(&im1,0, HD_BOX_FILTERS,1);
    if(!hd1.detect()) {
        cerr << "Detection of points failed!";
        return 1;
    }

    finish = clock();

    time = (double(finish)-double(start))/CLOCKS_PER_SEC;
    cout << "Measured time:"<<time<<"\n";

    vector<vector<int> >* interestPoints1=hd1.getPoints();

    Descriptor d1(&im1,&hd1);
    d1.setPoints(interestPoints1);
    //d.orientate();
    d1.createDescriptors();

    if(testFileOutput) {
//        string testOutputPath = input1.append(".key");
        if (verbose) cerr << "Generating output file for matlab test suite: " << output << endl;
        d1.printDescriptors(output);   //for matlab test suite
    } else {
        d1.generateAutopanoXML(output);
    }

    return EXIT_SUCCESS;
}
