// -*- c-basic-offset: 4 -*-

/** @file panotrafo.cpp
 *
 *  @brief Transform between image <-> panorama coordinates
 *
 *  @author Pablo d'Angelo
 *
 *  $Id: cpclean.cpp 4822 2009-12-19 23:17:06Z brunopostle $
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
#include <panotools/PanoToolsInterface.h>

using namespace std;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char * name)
{
    cout << name << ": transform pixel coordinates" << endl
         << "pano_trafo version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " input.pto image_nr" << endl
         << endl
         << "pano_trafo reads pixel coordinates from standard input and prints" << endl
	 << "the transformed coordinates to standard out." << endl
         << "     -r       Transform from panorama to image coordinates" << endl
         << "     -h       shows help" << endl
         << endl;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "hr";

    int c;
    bool reverse = false;
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch (c) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'r':
            reverse = true;
            break;
        case '?':
            break;
        default:
            abort ();
        }
    }

    if (argc - optind != 2) 
    {
        usage(argv[0]);
        return 1;
    };
    
    string input=argv[optind];
    int imageNumber = atoi(argv[optind+1]);

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

    if (imageNumber >= pano.getNrOfImages()) {
	cerr << "Not enough images in panorama" << endl;
	return 1;
    }
    
    // pano tools interface
    HuginBase::PTools::Transform trafo;
    if (reverse) {
	trafo.createTransform(pano.getSrcImage(imageNumber), pano.getOptions());
    } else {
	trafo.createInvTransform(pano.getSrcImage(imageNumber), pano.getOptions());
    }

    double xin,yin,xout,yout;
    while( fscanf(stdin, "%lf %lf", &xin, &yin) == 2 ) {
	trafo.transformImgCoord(xout, yout, xin, yin);
	fprintf(stdout,"%f %f\n",xout, yout);
    }
}
