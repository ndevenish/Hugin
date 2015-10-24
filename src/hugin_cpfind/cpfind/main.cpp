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
* <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <vector>
#include <string>
#include "Utils.h"
#include <getopt.h>
#include "hugin_utils/stl_utils.h"

#include "PanoDetector.h"

void printVersion()
{
    std::cout << "Hugin's cpfind " << hugin_utils::GetHuginVersion() << std::endl;
    std::cout << "based on Pan-o-matic by Anael Orlinski" << std::endl;
};

void printUsage()
{
    printVersion();
    std::cout << std::endl
        << "Basic usage: " << std::endl
        << "  cpfind -o output_project project.pto" << std::endl
        << "  cpfind -k i0 -k i1 ... -k in project.pto" << std::endl
        << "  cpfind --kall project.pto" << std::endl
        << std::endl << "The input project file is required." << std::endl
        << std::endl << "General options" << std::endl
        << "  -q|--quiet   Do not output progress" << std::endl
        << "  -v|--verbose  Verbose output" << std::endl
        << "  -h|--help     Shows this help screen" << std::endl
        << "  --version     Prints the version number and exits then" << std::endl
        << "  -o|--output=<string>  Sets the filename of the output file" << std::endl
        << "                        (default: default.pto)" << std::endl
        << std::endl << "Matching strategy (these options are mutually exclusive)" << std::endl
        << "  --linearmatch   Enable linear images matching" << std::endl
        << "                  Can be fine tuned with" << std::endl
        << "      --linearmatchlen=<int>  Number of images to match (default: 1)" << std::endl
        << "  --multirow      Enable heuristic multi row matching" << std::endl
        << "  --prealigned    Match only overlapping images," << std::endl
        << "                  requires a rough aligned panorama" << std::endl
        << std::endl << "Feature description options" << std::endl
        << "  --sieve1width=<int>    Sieve 1: Number of buckets on width (default: 10)" << std::endl
        << "  --sieve1height=<int>   Sieve 1: Number of buckets on height (default: 10)" << std::endl
        << "  --sieve1size=<int>     Sieve 1: Max points per bucket (default: 100)" << std::endl
        << "  --kdtreesteps=<int>          KDTree: search steps (default: 200)" << std::endl
        << "  --kdtreeseconddist=<double>  KDTree: distance of 2nd match (default: 0.25)" << std::endl
        << std::endl << "Feature matching options" << std::endl
        << "  --ransaciter=<int>     Ransac: iterations (default: 1000)" << std::endl
        << "  --ransacdist=<int>     Ransac: homography estimation distance threshold" << std::endl
        << "                                 (in pixels) (default: 25)" << std::endl
        << "  --ransacmode=<string>  Ransac: Select the mode used in the ransac step." << std::endl
        << "                                 Possible values: auto, hom, rpy, rpyv, rpyb" << std::endl
        << "                                 (default: auto)" << std::endl
        << "  --minmatches=<int>     Minimum matches (default: 6)" << std::endl
        << "  --sieve2width=<int>    Sieve 2: Number of buckets on width (default: 5)" << std::endl
        << "  --sieve2height=<int>   Sieve 2: Number of buckets on height (default: 5)" << std::endl
        << "  --sieve2size=<int>     Sieve 2: Max points per bucket (default: 1)" << std::endl
        << std::endl << "Caching options" << std::endl
        << "  -c|--cache    Caches automaticall keypoints to external file" << std::endl
        << "  --clean       Clean up cached keyfiles" << std::endl
        << "  -p|--keypath=<string>    Store keyfiles in given path" << std::endl
        << "  -k|--writekeyfile=<int>  Write a keyfile for this image number" << std::endl
        << "  --kall                   Write keyfiles for all images in the project" << std::endl
        << std::endl << "Advanced options" << std::endl
        << "  --celeste       Masks area with clouds before running feature descriptor" << std::endl
        << "                  Celeste can be fine tuned with the following parameters" << std::endl
        << "      --celestethreshold=<int>  Threshold for celeste (default 0.5)" << std::endl
        << "      --celesteradius=<int>     Radius for celeste (in pixels, default 20)" << std::endl
        << "  --ncores=<int>  Number of threads to use (default: autodetect number of cores)" << std::endl;
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
    std::string ransacMode;
    std::vector<int> keyfilesIndex;
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
                ransacMode = optarg;
                std::cout << "Ransac: " << ransacMode << std::endl;
                ransacMode=hugin_utils::tolower(ransacMode);
                std::cout << "Ransac: " << ransacMode << std::endl;
                if(ransacMode=="auto")
                {
                    ioPanoDetector.setRansacMode(HuginBase::RANSACOptimizer::AUTO);
                }
                else
                {
                    if(ransacMode=="hom")
                    {
                        ioPanoDetector.setRansacMode(HuginBase::RANSACOptimizer::HOMOGRAPHY);
                    }
                    else
                    {
                        if(ransacMode=="rpy")
                        {
                            ioPanoDetector.setRansacMode(HuginBase::RANSACOptimizer::RPY);
                        }
                        else
                        {
                            if(ransacMode=="rpyv")
                            {
                                ioPanoDetector.setRansacMode(HuginBase::RANSACOptimizer::RPYV);
                            }
                            else
                            {
                                if(ransacMode=="rpyvb")
                                {
                                    ioPanoDetector.setRansacMode(HuginBase::RANSACOptimizer::RPYVB);
                                }
                                else
                                {
                                    std::cout << "Warning: Invalid parameter in --ransacmode." << std::endl;
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
                    std::cout << "Warning: " << optarg << " is not a valid image number of writekeyfile." << std::endl;
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
                std::cerr <<"Option " << longOptions[optionIndex].name << " requires an argument" << std::endl;
                return false;
                break;
            case '?':
            default:
                break;
        };
    };
    
    if (argc - optind != 1)
    {
        std::cout << "Error: cpfind requires at least an input project file." << std::endl;
        return false;
    };
    ioPanoDetector.setInputFile(argv[optind]);
    if(doLinearMatch + doMultirow + doPrealign>1)
    {
        std::cout << "Error: The arguments --linearmatch, --multirow and --prealigned are" << std::endl
             << "       mutually exclusive. Use only one of them." << std::endl;
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
