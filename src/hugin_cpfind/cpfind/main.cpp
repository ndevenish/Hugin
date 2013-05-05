// -*- c-basic-offset: 4 ; tab-width: 4 -*-
/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>
#include <vector>
#include <string>
#include <boost/foreach.hpp>
#include "Utils.h"
#include <getopt.h>

using namespace std;

#include "PanoDetector.h"

void printVersion()
{
    std::cout << "Hugin's cpfind " << DISPLAY_VERSION << endl;
    std::cout << "based on Pan-o-matic by Anael Orlinski" << endl;
};

void printUsage()
{
    printVersion();
    cout << endl
        << "Basic usage: " << endl
        << "  cpfind -o output_project project.pto" << endl
        << "  cpfind -k i0 -k i1 ... -k in project.pto" << endl
        << "  cpfind --kall project.pto" << endl
        << endl << "The input project file is required." << endl
        << endl << "General options" << endl
        << "  -q|--quiet   Do not output progress" << endl
        << "  -v|--verbose  Verbose output" << endl
        << "  -h|--help     Shows this help screen" << endl
        << "  --version     Prints the version number and exits then" << endl
        << "  -o|--output=<string>  Sets the filename of the output file" << endl
        << "                        (default: default.pto)" << endl
        << endl << "Matching strategy (these options are mutually exclusive)" << endl
        << "  --linearmatch   Enable linear images matching" << endl
        << "                  Can be fine tuned with" << endl
        << "      --linearmatchlen=<int>  Number of images to match (default: 1)" << endl
        << "  --multirow      Enable heuristic multi row matching" << endl
        << "  --prealigned    Match only overlapping images," << endl
        << "                  requires a rough aligned panorama" << endl
        << endl << "Feature description options" << endl
        << "  --sieve1width=<int>    Sieve 1: Number of buckets on width (default: 10)" << endl
        << "  --sieve1height=<int>   Sieve 1: Number of buckets on height (default: 10)" << endl
        << "  --sieve1size=<int>     Sieve 1: Max points per bucket (default: 100)" << endl
        << "  --kdtreesteps=<int>          KDTree: search steps (default: 200)" << endl
        << "  --kdtreeseconddist=<double>  KDTree: distance of 2nd match (default: 0.25)" << endl
        << endl << "Feature matching options" << endl
        << "  --ransaciter=<int>     Ransac: iterations (default: 1000)" << endl
        << "  --ransacdist=<int>     Ransac: homography estimation distance threshold" << endl
        << "                                 (in pixels) (default: 25)" << endl
        << "  --ransacmode=<string>  Ransac: Select the mode used in the ransac step." << endl
        << "                                 Possible values: auto, hom, rpy, rpyv, rpyb" << endl
        << "                                 (default: auto)" << endl
        << "  --minmatches=<int>     Minimum matches (default: 6)" << endl
        << "  --sieve2width=<int>    Sieve 2: Number of buckets on width (default: 5)" << endl
        << "  --sieve2height=<int>   Sieve 2: Number of buckets on height (default: 5)" << endl
        << "  --sieve2size=<int>     Sieve 2: Max points per bucket (default: 1)" << endl
        << endl << "Caching options" << endl
        << "  -c|--cache    Caches automaticall keypoints to external file" << endl
        << "  --clean       Clean up cached keyfiles" << endl
        << "  -p|--keypath=<string>    Store keyfiles in given path" << endl
        << "  -k|--writekeyfile=<int>  Write a keyfile for this image number" << endl
        << "  --kall                   Write keyfiles for all images in the project" << endl
        << endl << "Advanced options" << endl
        << "  --celeste       Masks area with clouds before running feature descriptor" << endl
        << "                  Celeste can be fine tuned with the following parameters" << endl
        << "      --celestethreshold=<int>  Threshold for celeste (default 0.5)" << endl
        << "      --celesteradius=<int>     Radius for celeste (in pixels, default 20)" << endl
        << "  --ncores=<int>  Number of threads to use (default: autodetect number of cores)" << endl;
};

bool parseOptions(int argc, char** argv, PanoDetector& ioPanoDetector)
{
    enum
    {
        SIEVE1WIDTH=256,
        SIEVE1HEIGHT,
        SIEVE1SIZE,
        LINEARMATCH,
        LINEARMATCHLEN,
        MULTIROW,
        PREALIGNED,
        KDTREESTEPS,
        KDTREESECONDDIST,
        MINMATCHES,
        RANSACMODE,
        RANSACITER,
        RANSACDIST,
        SIEVE2WIDTH,
        SIEVE2HEIGHT,
        SIEVE2SIZE,
        KALL,
        CLEAN,
        CELESTE,
        CELESTETHRESHOLD,
        CELESTERADIUS,
        CPFINDVERSION
    };
    const char* optstring = "qvftn:o:k:cp:h";
    static struct option longOptions[] =
    {
        {"quiet", no_argument, NULL, 'q' },
        {"verbose", no_argument, NULL, 'v'},
        {"fullscale", no_argument, NULL, 'f'},
        {"sieve1width", required_argument, NULL, SIEVE1WIDTH},
        {"sieve1height", required_argument, NULL, SIEVE1HEIGHT},
        {"sieve1size", required_argument, NULL, SIEVE1SIZE},
        {"linearmatch", no_argument, NULL, LINEARMATCH},
        {"linearmatchlen", required_argument, NULL, LINEARMATCHLEN},
        {"multirow", no_argument, NULL, MULTIROW},
        {"prealigned", no_argument, NULL, PREALIGNED},
        {"kdtreesteps", required_argument, NULL, KDTREESTEPS},
        {"kdtreeseconddist", required_argument, NULL, KDTREESECONDDIST},
        {"minmatches", required_argument, NULL, MINMATCHES},
        {"ransacmode", required_argument, NULL, RANSACMODE},
        {"ransaciter", required_argument, NULL, RANSACITER},
        {"ransacdist", required_argument, NULL, RANSACDIST},
        {"sieve2width", required_argument, NULL, SIEVE2WIDTH},
        {"sieve2height", required_argument, NULL, SIEVE2HEIGHT},
        {"sieve2size", required_argument, NULL, SIEVE2SIZE},
        {"test", no_argument, NULL, 't'},
        {"ncores", required_argument, NULL, 'n'},
        {"output", required_argument, NULL, 'o'},
        {"writekeyfile", required_argument, NULL, 'k'},
        {"kall", no_argument, NULL, KALL},
        {"cache", no_argument, NULL, 'c'},
        {"clean", no_argument, NULL, CLEAN},
        {"keypath", required_argument, NULL, 'p'},
        {"celeste", no_argument, NULL, CELESTE},
        {"celestethreshold", required_argument, NULL, CELESTETHRESHOLD},
        {"celesteradius", required_argument, NULL, CELESTERADIUS},
        {"version", no_argument, NULL, CPFINDVERSION},
        {"help", no_argument, NULL, 'h'},
        0
    };

    int c;
    int optionIndex = 0;
    int number;
    double floatNumber;
    string ransacMode;
    vector<int> keyfilesIndex;
    int doLinearMatch=0;
    int doMultirow=0;
    int doPrealign=0;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'q':
                ioPanoDetector.setVerbose(0);
                break;
            case 'v':
                ioPanoDetector.setVerbose(2);
                break;
            case 'f':
                ioPanoDetector.setDownscale(false);
                break;
            case SIEVE1WIDTH:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setSieve1Width(number);
                };
                break;
            case SIEVE1HEIGHT:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setSieve1Height(number);
                };
                break;
            case SIEVE1SIZE:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setSieve1Size(number);
                };
                break;
            case LINEARMATCH:
                doLinearMatch=1;
                break;
            case LINEARMATCHLEN:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setLinearMatchLen(number);
                };
                break;
            case MULTIROW:
                doMultirow=1;
                break;
            case PREALIGNED:
                doPrealign=1;
                break;
            case KDTREESTEPS:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setKDTreeSearchSteps(number);
                };
                break;
            case KDTREESECONDDIST:
                floatNumber=atof(optarg);
                if(floatNumber>0)
                {
                    ioPanoDetector.setKDTreeSecondDistance(floatNumber);
                };
                break;
            case MINMATCHES:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setMinimumMatches(number);
                };
                break;
            case RANSACMODE:
                ransacMode=optarg;
                cout << "Ransac: " << ransacMode << endl;
                transform(ransacMode.begin(), ransacMode.end(), ransacMode.begin(),(int(*)(int)) tolower);
                cout << "Ransac: " << ransacMode << endl;
                if(ransacMode=="auto")
                {
                    ioPanoDetector.setRansacMode(RANSACOptimizer::AUTO);
                }
                else
                {
                    if(ransacMode=="hom")
                    {
                        ioPanoDetector.setRansacMode(RANSACOptimizer::HOMOGRAPHY);
                    }
                    else
                    {
                        if(ransacMode=="rpy")
                        {
                            ioPanoDetector.setRansacMode(RANSACOptimizer::RPY);
                        }
                        else
                        {
                            if(ransacMode=="rpyv")
                            {
                                ioPanoDetector.setRansacMode(RANSACOptimizer::RPYV);
                            }
                            else
                            {
                                if(ransacMode=="rpyvb")
                                {
                                    ioPanoDetector.setRansacMode(RANSACOptimizer::RPYVB);
                                }
                                else
                                {
                                    cout << "Warning: Invalid parameter in --ransacmode." << endl;
                                };
                            };
                        };
                    };
                };
                break;
            case RANSACITER:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setRansacIterations(number);
                };
                break;
            case RANSACDIST:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setRansacDistanceThreshold(number);
                };
                break;
            case SIEVE2WIDTH:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setSieve2Width(number);
                };
                break;
            case SIEVE2HEIGHT:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setSieve2Height(number);
                };
                break;
            case SIEVE2SIZE:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setSieve2Size(number);
                };
                break;
            case 't':
                ioPanoDetector.setTest(true);
                break;
            case 'n':
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setCores(number);
                };
                break;
            case 'o':
                ioPanoDetector.setOutputFile(optarg);
                break;
            case 'k':
                number=atoi(optarg);
                if((number==0) && (strcmp(optarg,"0")!=0))
                {
                    cout << "Warning: " << optarg << " is not a valid image number of writekeyfile." << endl;
                }
                else
                {
                    keyfilesIndex.push_back(number);
                };
                break;
            case KALL:
                ioPanoDetector.setWriteAllKeyPoints();
                break;
            case 'c':
                ioPanoDetector.setCached(true);
                break;
            case CLEAN:
                ioPanoDetector.setCleanup(true);
                break;
            case 'p':
                ioPanoDetector.setKeyfilesPath(optarg);
                break;
            case CELESTE:
                ioPanoDetector.setCeleste(true);
                break;
            case CELESTETHRESHOLD:
                floatNumber=atof(optarg);
                if(floatNumber>0.0)
                {
                    ioPanoDetector.setCelesteThreshold(floatNumber);
                };
                break;
            case CELESTERADIUS:
                number=atoi(optarg);
                if(number>0)
                {
                    ioPanoDetector.setCelesteRadius(number);
                };
                break;
            case CPFINDVERSION:
                printVersion();
                return false;
                break;
            case 'h':
                printUsage();
                return false;
                break;
            case ':':
                cerr <<"Option " << longOptions[optionIndex].name << " requires an argument" << endl;
                return false;
                break;
            case '?':
            default:
                break;
        };
    };
    
    if (argc - optind != 1)
    {
        cout << "Error: cpfind requires at least an input project file." << endl;
        return false;
    };
    ioPanoDetector.setInputFile(argv[optind]);
    if(doLinearMatch + doMultirow + doPrealign>1)
    {
        cout << "Error: The arguments --linearmatch, --multirow and --prealigned are" << endl
             << "       mutually exclusive. Use only one of them." << endl;
        return false;
    };
    if(doLinearMatch)
    {
        ioPanoDetector.setMatchingStrategy(PanoDetector::LINEAR);
    };
    if(doMultirow)
    {
        ioPanoDetector.setMatchingStrategy(PanoDetector::MULTIROW);
    };
    if(doPrealign)
    {
        ioPanoDetector.setMatchingStrategy(PanoDetector::PREALIGNED);
    };
    if(keyfilesIndex.size()>0)
    {
        ioPanoDetector.setKeyPointsIdx(keyfilesIndex);
    };
    return true;
};

int main(int argc, char** argv)
{
    // create a panodetector object
    PanoDetector aPanoDetector;
    if(!parseOptions(argc, argv, aPanoDetector))
    {
        return 0;
    }

    if (!aPanoDetector.checkData())
    {
        return 0;
    }

    printVersion();
    if (aPanoDetector.getVerbose() > 1)
    {
        aPanoDetector.printDetails();
    }

    TIMETRACE("Detection",aPanoDetector.run());

    return 0;

}
