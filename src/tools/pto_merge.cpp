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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <fstream>
#include <sstream>
#include <getopt.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <panodata/Panorama.h>

static void usage(const char* name)
{
    std::cout << name << ": merges several project files" << std::endl
         << "pto_merge version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] input.pto input2.pto ..." << std::endl
         << std::endl
         << "  Options:" << std::endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << std::endl
         << "                            Default: <filename>_merge.pto" << std::endl
         << "     -h, --help             Shows this help" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:h";

    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    int optionIndex = 0;
    std::string output;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (argc - optind < 2)
    {
        std::cout << "Warning: pto_merge requires at least 2 project files" << std::endl << std::endl;
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return 1;
    };

    std::string input=argv[optind];
    // read panorama
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
        std::cerr << "AppBase::DocumentData::ReadWriteError code: " << err << std::endl;
        return 1;
    }

    optind++;
    while(optind<argc)
    {
        HuginBase::Panorama pano2;
        std::string input2=argv[optind];
        std::ifstream prjfile2(input2.c_str());
        if (!prjfile2.good())
        {
            std::cerr << "could not open script : " << input << std::endl;
            return 1;
        }
        pano2.setFilePrefix(hugin_utils::getPathPrefix(input2));
        AppBase::DocumentData::ReadWriteError err = pano2.readData(prjfile2);
        if (err != AppBase::DocumentData::SUCCESSFUL)
        {
            std::cerr << "error while parsing panos tool script: " << input << std::endl;
            std::cerr << "AppBase::DocumentData::ReadWriteError code: " << err << std::endl;
            return 1;
        }
        pano.mergePanorama(pano2);
        optind++;
    };

    //write output
    HuginBase::OptimizeVector optvec = pano.getOptimizeVector();
    HuginBase::UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_merge.pto");
    }
    std::ofstream of(output.c_str());
    pano.printPanoramaScript(of, optvec, pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    std::cout << std::endl << "Written output to " << output << std::endl;
    return 0;
}
