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

#include <fstream>
#include <sstream>

#include <unistd.h>


#include <panoinc.h>
#include "PT/ImageGraph.h"
#include "PT/PTOptimise.h"

//using namespace vigra;
//using namespace vigra_ext;
using namespace PT;
using namespace std;


static void usage(const char * name)
{
    cerr << name << ": test program to do pairwise registration" << endl
         << endl
         << "Usage:  " << name << " -o output.pto input.pto"
         << endl;
}



int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "ho:";
    int c;
    string output;
    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'o':
            output = optarg;
            break;
        case 'h':
            usage(argv[0]);
            return 1;
        default:
            abort ();
        }

    if (argc - optind <1) {
        usage(argv[0]);
        return 1;
    }

    const char * scriptFile = argv[optind];

    utils::CoutProgressDisplay pdisp;

    Panorama pano;
    PanoramaMemento newPano;
    ifstream prjfile(scriptFile);
    if (prjfile.bad()) {
        cerr << "could not open script : " << scriptFile << endl;
        exit(1);
    }
    if (newPano.loadPTScript(prjfile)) {
        pano.setMemento(newPano);
    } else {
        cerr << "error while parsing panos tool script: " << scriptFile << endl;
        exit(1);
    }

    VariableMapVector newvars = PTools::autoOptimise(pano);

    // run a global optimisation...

//    pano.updateVariables(newvars);

    unsigned int nImages = pano.getNrOfImages();
    OptimizeVector optvec(nImages);
    // fill optimize vector, just anchor one image.
    for (unsigned int i=0; i<nImages; i++) {
        if (i != pano.getOptions().optimizeReferenceImage) {
            optvec[i].insert("y");
            optvec[i].insert("p");
            optvec[i].insert("r");
        }
    }

    if (output != "") {
        ofstream of(output.c_str());
        pano.printOptimizerScript(of, optvec, pano.getOptions());
    } else {
        pano.printOptimizerScript(cout, optvec, pano.getOptions());
    }
    return 0;
}
