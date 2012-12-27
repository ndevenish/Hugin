// -*- c-basic-offset: 4 -*-

/** @file checkpto.cpp
 *
 *  @brief helper program for assistant makefile
 *  
 *
 *  @author Thomas Modes
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

#include <panodata/Panorama.h>
#include "algorithms/optimizer/ImageGraph.h"
#include "hugin_base/panotools/PanoToolsUtils.h"
#include "algorithms/basic/CalculateCPStatistics.h"

using namespace std;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char * name)
{
    cout << name << ": report the number of image groups in a project" << endl
         << name << " version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " input.pto" << endl
         << endl
         << name << " examins the connections between images in a project and" << endl
         << "reports back the number of parts or image groups in that project" << endl
         << endl
         << name << " is used by the assistant makefile" << endl
         << endl;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "h";

    int c;
    string output;
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch (c) {
        case 'h':
            usage(argv[0]);
            return 0;
        case '?':
            break;
        default:
            abort ();
        }
    }

    if (argc - optind != 1) 
    {
        usage(argv[0]);
        return -1;
    };
    
    string input=argv[optind];

    Panorama pano;
    ifstream prjfile(input.c_str());
    if (!prjfile.good()) {
        cerr << "could not open script : " << input << endl;
        return -1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != DocumentData::SUCCESSFUL) {
        cerr << "error while parsing panos tool script: " << input << endl;
        cerr << "DocumentData::ReadWriteError code: " << err << endl;
        return -1;
    }

    std::cout << endl 
              << "Opened project " << input << endl << endl
              << "Project contains" << endl
              << pano.getNrOfImages() << " images" << endl
              << pano.getNrOfCtrlPoints() << " control points" << endl << endl;
    //cp statistics
    if(pano.getNrOfCtrlPoints()>0)
    {
        double min;
        double max;
        double mean;
        double var;
        HuginBase::PTools::calcCtrlPointErrors(pano);
        CalculateCPStatisticsError::calcCtrlPntsErrorStats(pano, min, max, mean, var);
        if(max>0) 
        {
            std::cout << "Control points statistics" << std::endl
                << fixed << std::setprecision(2)
                << "\tMean error        : " << mean << std::endl
                << "\tStandard deviation: " << sqrt(var) << std::endl
                << "\tMinimum           : " << min << std::endl
                << "\tMaximum           : " << max << std::endl;
        };
    };
    CPGraph graph;
    createCPGraph(pano, graph);
    CPComponents comps;
    int n = findCPComponents(graph, comps);
    if(n==1)
    {
        std::cout << "All images are connected." << endl;
        // return value must be 0, otherwise the assistant does not continue
        return 0;
    }
    else
    {
        std::cout << "Found unconnected images!" << endl 
                  << "There are " << n << " image groups." << endl;

        std::cout << "Image groups: " << endl;
        for (unsigned i=0; i < comps.size(); i++)
        {
            std::cout << "[";
            CPComponents::value_type::const_iterator it;
            size_t c=0;
            for (it = comps[i].begin(); it != comps[i].end(); ++it)
            {
                std::cout << (*it);
                if (c+1 != comps[i].size())
                {
                    std::cout << ", ";
                }
                c++;
            }
            std::cout << "]";
            if (i+1 != comps.size())
            {
                std::cout << ", " << endl;
            }
        }
        return n;
    };
}
