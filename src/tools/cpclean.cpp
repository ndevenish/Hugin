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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <hugin_version.h>

#include <fstream>
#include <sstream>
#ifdef WIN32
 #include <getopt.h>
#else
 #include <unistd.h>
#endif

#include <algorithms/optimizer/PTOptimizer.h>
#include <algorithms/control_points/CleanCP.h>

using namespace std;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char * name)
{
    cout << name << ": remove wrong control points by statistic method" << endl
         << "cpclean version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] input.pto" << endl
         << endl
         << "CPClean uses statistical methods to remove wrong control points" << endl << endl
         << "Step 1 optimises all images pairs, calculates for each pair mean " << endl
         << "       and standard deviation and removes all control points " << endl
         << "       with error bigger than mean+n*sigma" << endl
         << "Step 2 optimises the whole panorama, calculates mean and standard deviation" << endl
         << "       for all control points and removes all control points with error" << endl
         << "       bigger than mean+n*sigma" << endl << endl
         << "  Options:" << endl
         << "     -o file.pto  Output Hugin PTO file. Default: '<filename>_clean.pto'." << endl
         << "     -n num   distance factor for checking (default: 2)" << endl
         << "     -p       do only pairwise optimisation (skip step 2)" << endl
         << "     -w       do optimise whole panorama (skip step 1)" << endl
         << "     -s       skip optimisation step when optimisation the whole panorama" << endl
         << "     -h       shows help" << endl
         << endl;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "o:hn:pws";

    int c;
    string output;
    bool onlyPair = false;
    bool wholePano = false;
    bool skipOptimisation = false;
    double n = 2.0;
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch (c) {
        case 'o':
            output = optarg;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        case 'n':
            n = atof(optarg);
            if(n==0)
            {
                cerr <<"Invalid parameter: " << optarg << " is not valid real number" << endl;
                return 1;
            };
	        if (n<1.0) 
            {
		        cerr << "Invalid parameter: n must be at least 1" << endl;
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
        case ':':
            cerr <<"Option -n requires a number" << endl;
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
        usage(argv[0]);
        return 1;
    };
    
    if (onlyPair && wholePano)
    {
        cerr << "Options -p and -w can't used together" << endl;
        return 1;
    };

    string input=argv[optind];

    Panorama pano;
    ifstream prjfile(input.c_str());
    if (!prjfile.good()) {
        cerr << "could not open script : " << input << endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != DocumentData::SUCCESSFUL) {
        cerr << "error while parsing panos tool script: " << input << endl;
        cerr << "DocumentData::ReadWriteError code: " << err << endl;
        return 1;
    }

    size_t nrImg=pano.getNrOfImages();
    if (nrImg < 2) 
    {
        cerr << "Panorama should consist of at least two images" << endl;
        return 1;
    }

    if (pano.getNrOfCtrlPoints() < 3)
    {
        cerr << "Panorama should contain at least 3 control point" << endl;
    };
    
    size_t cpremoved1=0;
    UIntSet CPtoRemove;
    // step 1 with pairwise optimisation
    if(!wholePano)
    {
        CPtoRemove=getCPoutsideLimit_pair(pano,n);
        if (CPtoRemove.size()>0)
            for(UIntSet::reverse_iterator it = CPtoRemove.rbegin(); it != CPtoRemove.rend(); ++it)
                pano.removeCtrlPoint(*it);
        cpremoved1=CPtoRemove.size();
    };

    // step 2 with optimisation of whole panorama
    bool unconnected=false;
    if(!onlyPair)
    {
        //check for unconnected images
        CPGraph graph;
        createCPGraph(pano, graph);
        CPComponents comps;
        int parts=findCPComponents(graph, comps);
        if (parts > 1) 
        {
            unconnected=true;
        }
        else
        {
            CPtoRemove.clear();
            if(skipOptimisation)
            {
                std::cout << endl << "Skipping optimisation, current image positions will be used." << endl;
            };
            CPtoRemove=getCPoutsideLimit(pano,n,skipOptimisation);
            if (CPtoRemove.size()>0)
                for(UIntSet::reverse_iterator it = CPtoRemove.rbegin(); it != CPtoRemove.rend(); ++it)
                    pano.removeCtrlPoint(*it);
        };
    };

    cout << endl;
    if(!wholePano)
        cout << "Removed " << cpremoved1 << " control points in step 1" << endl;
    if(!onlyPair)
        if(unconnected)
            cout <<"Skipped step 2 because of unconnected image pairs" << endl;
        else
            cout << "Removed " << CPtoRemove.size() << " control points in step 2" << endl;

    //write output
    OptimizeVector optvec = pano.getOptimizeVector();
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
 	// Set output .pto filename if not given
	if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_clean.pto");
	}
    ofstream of(output.c_str());
    pano.printPanoramaScript(of, optvec, pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));
    
    cout << endl << "Written output to " << output << endl;
    return 0;
}
