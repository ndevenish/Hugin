// -*- c-basic-offset: 4 -*-

/** @file cpclean.cpp
 *
 *  @brief program to remove wrong control points by statistical method
 *
 *  the algorithm is based on ptoclean by Bruno Postle
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */

/*  This program is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <fstream>
#include <sstream>
#include <getopt.h>

#include <algorithms/optimizer/PTOptimizer.h>
#include <algorithms/optimizer/ImageGraph.h>
#include <algorithms/control_points/CleanCP.h>
#include "panotools/PanoToolsInterface.h"

static void usage(const char* name)
{
    std::cout << name << ": remove wrong control points by statistic method" << std::endl
        << "cpclean version " << hugin_utils::GetHuginVersion() << std::endl
        << std::endl
        << "Usage:  " << name << " [options] input.pto" << std::endl
        << std::endl
        << "CPClean uses statistical methods to remove wrong control points" << std::endl << std::endl
        << "Step 1 optimises all images pairs, calculates for each pair mean " << std::endl
        << "       and standard deviation and removes all control points " << std::endl
        << "       with error bigger than mean+n*sigma" << std::endl
        << "Step 2 optimises the whole panorama, calculates mean and standard deviation" << std::endl
        << "       for all control points and removes all control points with error" << std::endl
        << "       bigger than mean+n*sigma" << std::endl << std::endl
        << "  Options:" << std::endl
        << "     --output|-o file.pto     Output Hugin PTO file." << std::endl
        << "                              Default: '<filename>_clean.pto'." << std::endl
        << "     --max-distance|-n num    distance factor for checking (default: 2)" << std::endl
        << "     --pairwise-checking|-p   do only pairwise optimisation (skip step 2)" << std::endl
        << "     --whole-pano-checking|-w do optimise whole panorama (skip step 1)" << std::endl
        << "     --dont-optimize|-s       skip optimisation step when optimisation the" << std::endl
        << "                              whole panorama" << std::endl
        << "     --check-line-cp|-l       also include line control points for calculation" << std::endl
        << "                              and filtering in step 2" << std::endl
        << "     --help|-h                 shows help" << std::endl
        << std::endl;
}

// dummy panotools progress functions
static int ptProgress(int command, char* argument)
{
    return 1;
}

static int ptinfoDlg(int command, char* argument)
{
    return 1;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:hn:pwslv";
    static struct option longOptions[] =
    {
        { "output", required_argument, NULL, 'o'},
        { "max-distance", required_argument, NULL, 'n'},
        { "pairwise-checking", no_argument, NULL, 'p'},
        { "whole-pano-checking", no_argument, NULL, 'w'},
        { "dont-optimize", no_argument, NULL, 's'},
        { "check-line-cp", no_argument, NULL, 'l' },
        { "help", no_argument, NULL, 'h' },
        0
    };
    int c;
    std::string output;
    bool onlyPair = false;
    bool wholePano = false;
    bool skipOptimisation = false;
    bool includeLineCp = false;
    bool verbose = false;
    double n = 2.0;
    int optionIndex = 0;
    while ((c = getopt_long(argc, argv, optstring, longOptions, &optionIndex)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case 'n':
                n = atof(optarg);
                if(n==0)
                {
                    std::cerr <<"Invalid parameter: " << optarg << " is not valid real number" << std::endl;
                    return 1;
                };
                if (n<1.0)
                {
                    std::cerr << "Invalid parameter: n must be at least 1" << std::endl;
                    return 1;
                };
                break;
            case 'p':
                onlyPair= true;
                break;
            case 'w':
                wholePano = true;
                break;
            case 's':
                skipOptimisation = true;
                break;
            case 'l':
                includeLineCp = true;
                break;
            case 'v':
                verbose = true;
                break;
            case ':':
                std::cerr <<"Option -n requires a number" << std::endl;
                return 1;
                break;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (argc - optind != 1)
    {
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return 1;
    };

    if (onlyPair && wholePano)
    {
        std::cerr << "Options -p and -w can't used together" << std::endl;
        return 1;
    };

    std::string input=argv[optind];

    HuginBase::Panorama pano;
    std::ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        std::cerr << "could not open script : " << input << std::endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "error while parsing panos tool script: " << input << std::endl;
        std::cerr << "DocumentData::ReadWriteError code: " << err << std::endl;
        return 1;
    }

    const size_t nrImg=pano.getNrOfImages();
    if (nrImg < 2)
    {
        std::cerr << "Panorama should consist of at least two images" << std::endl;
        return 1;
    }

    if (pano.getNrOfCtrlPoints() < 3)
    {
        std::cerr << "Panorama should contain at least 3 control point" << std::endl;
    };

    if (!verbose)
    {
        PT_setProgressFcn(ptProgress);
        PT_setInfoDlgFcn(ptinfoDlg);
    };

    size_t cpremoved1 = 0;
    HuginBase::UIntSet CPtoRemove;
    // step 1 with pairwise optimisation
    if(!wholePano)
    {
        AppBase::DummyProgressDisplay dummy;
        CPtoRemove=getCPoutsideLimit_pair(pano, dummy, n);
        if (CPtoRemove.size()>0)
            for (HuginBase::UIntSet::reverse_iterator it = CPtoRemove.rbegin(); it != CPtoRemove.rend(); ++it)
            {
                pano.removeCtrlPoint(*it);
            }
        cpremoved1=CPtoRemove.size();
    };

    // step 2 with optimisation of whole panorama
    bool unconnected=false;
    if(!onlyPair)
    {
        //check for unconnected images
        HuginGraph::ImageGraph graph(pano);
        unconnected = !graph.IsConnected();
        if (!unconnected)
        {
            CPtoRemove.clear();
            if(skipOptimisation)
            {
                std::cout << std::endl << "Skipping optimisation, current image positions will be used." << std::endl;
            };
            CPtoRemove=getCPoutsideLimit(pano, n, skipOptimisation, includeLineCp);
            if (CPtoRemove.size()>0)
                for (HuginBase::UIntSet::reverse_iterator it = CPtoRemove.rbegin(); it != CPtoRemove.rend(); ++it)
                {
                    pano.removeCtrlPoint(*it);
                }
        };
    };

    std::cout << std::endl;
    if(!wholePano)
    {
        std::cout << "Removed " << cpremoved1 << " control points in step 1" << std::endl;
    }
    if (!onlyPair)
    {
        if (unconnected)
        {
            std::cout << "Skipped step 2 because of unconnected image pairs" << std::endl;
        }
        else
        {
            std::cout << "Removed " << CPtoRemove.size() << " control points in step 2" << std::endl;
        };
    };

    //write output
    HuginBase::OptimizeVector optvec = pano.getOptimizeVector();
    HuginBase::UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_clean.pto");
    }
    std::ofstream of(output.c_str());
    pano.printPanoramaScript(of, optvec, pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    std::cout << std::endl << "Written output to " << output << std::endl;
    return 0;
}
