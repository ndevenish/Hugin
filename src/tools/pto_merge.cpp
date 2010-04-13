// -*- c-basic-offset: 4 -*-

/** @file pto_merge.cpp
 *
 *  @brief program to merge several project files
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
#include <getopt.h>
#ifndef WIN32
 #include <unistd.h>
#endif
#include <panodata/Panorama.h>

using namespace std;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char * name)
{
    cout << name << ": merges several project files" << endl
         << "pto_merge version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] input.pto input2.pto ..." << endl
         << endl
         << "  Options:" << endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << endl
         << "                            Default: <filename>_merge.pto" << endl
         << "     -h, --help             Shows this help" << endl
         << endl;
}

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "o:h";

    static struct option longOptions[] = {
        {"output", required_argument, NULL, 'o' },
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    int optionIndex = 0;
    string output;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c) 
        {
            case 'o':
                output = optarg;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (argc - optind < 2) 
    {
        cout << "Warning: pto_merge requires at least 2 project files" << endl << endl;
        usage(argv[0]);
        return 1;
    };
    
    string input=argv[optind];
    // read panorama
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

    optind++;
    while(optind<argc)
    {
        Panorama pano2;
        string input2=argv[optind];
        ifstream prjfile2(input2.c_str());
        if (!prjfile2.good()) 
        {
            cerr << "could not open script : " << input << endl;
            return 1;
        }
        pano2.setFilePrefix(hugin_utils::getPathPrefix(input2));
        DocumentData::ReadWriteError err = pano2.readData(prjfile2);
        if (err != DocumentData::SUCCESSFUL) 
        {
            cerr << "error while parsing panos tool script: " << input << endl;
            cerr << "DocumentData::ReadWriteError code: " << err << endl;
            return 1;
        }
        pano.mergePanorama(pano2);
        optind++;
    };

    //write output
    OptimizeVector optvec = pano.getOptimizeVector();
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_merge.pto");
    }
    ofstream of(output.c_str());
    pano.printPanoramaScript(of, optvec, pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));
    
    cout << endl << "Written output to " << output << endl;
    return 0;
}
