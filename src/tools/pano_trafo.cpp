// -*- c-basic-offset: 4 -*-

/** @file pano_trafo.cpp
 *
 *  @brief Transform between image <-> panorama coordinates
 *
 *  @author Pablo d'Angelo, Kay F. Jahnke
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *  KFJ 2010-12-28 I've thrown out C fprintf style IO, now
 *  it's all done with streams, removing the problem I had with
 *  unflushed output.
 *  I've also added a new mode where no image number is passed on
 *  the command line and instead the image number is passed with
 *  each coordinate pair.
 *
 */

#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include <panodata/Panorama.h>
#include <panotools/PanoToolsInterface.h>

static void usage(const char* name)
{
    std::cout << name << ": transform pixel coordinates" << std::endl
         << "pano_trafo version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " input.pto [ image_nr ]" << std::endl
         << std::endl
         << "pano_trafo reads pixel coordinates from standard input and" << std::endl
         << "prints the transformed coordinates to standard output." << std::endl
         << "If you pass an image number on the command line," << std::endl
         << "it reads pairs of coordinates and transforms them." << std::endl
         << "If you don't pass an image number, it will read triplets" << std::endl
         << "of the form <image number> <x coordinate> <y coordinate>" << std::endl
         << "and output the transformed coordinates." << std::endl
         << std::endl
         << "     -r       Transform from panorama to image coordinates" << std::endl
         << "     -h       shows help" << std::endl
         << std::endl;
}

// alternative behaviour if no image number is passed.
// main() with the original behaviour follows below.

void work_on_triplets(const HuginBase::Panorama& pano, bool reverse)
{
    // pano tools interface
    int images = pano.getNrOfImages() ;
    int image = 0 ;

    // instead of just one transform, create one for each image

    HuginBase::PTools::Transform* trafo_set =
        new HuginBase::PTools::Transform [ images ] ;

    if ( ! trafo_set )
    {
        std::cerr << "not enough memory" ; // very unlikely...
        exit ( -1 ) ;
    }

    for ( image = 0 ; image < images ; image++ )
    {
        if (reverse)
        {
            trafo_set[image].createTransform(pano.getSrcImage(image), pano.getOptions());
        }
        else
        {
            trafo_set[image].createInvTransform(pano.getSrcImage(image), pano.getOptions());
        }
    }

    // now we can process data triplets from cin

    double xin , yin , xout , yout ;

    while ( std::cin >> image >> xin >> yin )
    {
        if ( image < 0 || image >= images )
        {
            std::cerr << "no image " << image << " in pano" << std::endl ;
            exit ( 1 ) ; // we don't want an index out of range
        }
        trafo_set[image].transformImgCoord(xout, yout, xin, yin);
        std::cout << xout << " " << yout << std::endl ;
    }
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "hr";

    int c;
    bool reverse = false;
    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch (c)
        {
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
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

    if (argc - optind < 1 || argc - optind > 2)
    {
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return 1;
    }

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
        std::cerr << "AppBase::DocumentData::ReadWriteError code: " << err << std::endl;
        return 1;
    }

    // set up output format
    std::cout.setf ( std::ios::fixed ) ;
    std::cout.precision ( 6 ) ; // should be ample

    if ( argc - optind == 1 )
    {
        // no image number was passed. This triggers the new
        // behaviour to accept triplets on cin
        work_on_triplets ( pano , reverse ) ;
        return 0;
    }

    // an image number was passed, so proceed
    // as in the original version

    int imageNumber = atoi(argv[optind+1]);
    if (imageNumber >= pano.getNrOfImages())
    {
        std::cerr << "Not enough images in panorama" << std::endl;
        return 1;
    }

    // pano tools interface
    HuginBase::PTools::Transform trafo;
    if (reverse)
    {
        trafo.createTransform(pano.getSrcImage(imageNumber), pano.getOptions());
    }
    else
    {
        trafo.createInvTransform(pano.getSrcImage(imageNumber), pano.getOptions());
    }

    double xin , yin , xout , yout ;

    // here's where the old-style IO was, now it's all streams.
    // It's also format-free input, so newlines don't matter
    while ( std::cin >> xin >> yin )
    {
        trafo.transformImgCoord(xout, yout, xin, yin);
        std::cout << xout << " " << yout << std::endl ;
    }
}
