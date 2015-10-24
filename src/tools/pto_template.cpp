// -*- c-basic-offset: 4 -*-

/** @file pto_template.cpp
 *
 *  @brief program to apply template
 *
 *  @author T. Modes
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
    std::cout << name << ": apply template" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] input.pto" << std::endl
         << std::endl
         << "  Options:" << std::endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << std::endl
         << "                            Default: <filename>_template.pto" << std::endl
         << "     --template=template.pto   Apply the given template file" << std::endl
         << "     -h, --help             Shows this help" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:t:h";
    enum
    {
        MINOVERLAP=1000
    };

    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"template", required_argument, NULL, 't'},
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    int optionIndex = 0;
    std::string output;
    std::string templateFile;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case 't':
                templateFile = optarg;
                if(!hugin_utils::FileExists(templateFile))
                {
                    std::cerr << "Error: Template \"" << templateFile << "\" not found." << std::endl;
                    return 1;
                };
                break;
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case ':':
                std::cerr <<"Option " << longOptions[optionIndex].name << " requires a parameter" << std::endl;
                return 1;
                break;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (argc - optind == 0)
    {
        std::cout << "Error: No project file given." << std::endl;
        return 1;
    };
    if (argc - optind != 1)
    {
        std::cout << "Error: pto_template can only work on one project file at one time" << std::endl;
        return 1;
    };
    if (templateFile.length()==0)
    {
        std::cerr << "Error: No template given." << std::endl;
        return 1;
    };

    std::string input=argv[optind];
    // read panorama
    HuginBase::Panorama pano;
    std::ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        std::cerr << "Error: could not open script : " << input << std::endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "Error while parsing panos tool script: " << input << std::endl;
        std::cerr << "AppBase::DocumentData::ReadWriteError code: " << err << std::endl;
        return 1;
    }

    if(pano.getNrOfImages()==0)
    {
        std::cerr << "Error: project file does not contains any image" << std::endl;
        std::cerr << "aborting processing" << std::endl;
        return 1;
    };

    HuginBase::Panorama newPano;
    std::ifstream templateStream(templateFile.c_str());
    if (!templateStream.good())
    {
        std::cerr << "Error: could not open template script : " << templateFile << std::endl;
        return 1;
    }
    newPano.setFilePrefix(hugin_utils::getPathPrefix(templateFile));
    err = newPano.readData(templateStream);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "Error while parsing template script: " << templateFile << std::endl;
        std::cerr << "AppBase::DocumentData::ReadWriteError code: " << err << std::endl;
        return 1;
    }

    if (pano.getNrOfImages() != newPano.getNrOfImages())
    {
        std::cerr << "Error: template expects " << newPano.getNrOfImages() << " images," << std::endl
             << "       current project contains " << pano.getNrOfImages() << " images" << std::endl
             << "       Could not apply template" << std::endl;
        return false;
    }

    // check image sizes, and correct parameters if required.
    for (unsigned int i = 0; i < newPano.getNrOfImages(); i++)
    {
        // check if image size is correct
        const HuginBase::SrcPanoImage& oldSrcImg = pano.getImage(i);
        HuginBase::SrcPanoImage newSrcImg = newPano.getSrcImage(i);

        // just keep the file name
        newSrcImg.setFilename(oldSrcImg.getFilename());
        if (oldSrcImg.getSize() != newSrcImg.getSize())
        {
            // adjust size properly.
            newSrcImg.resize(oldSrcImg.getSize());
        }
        newPano.setSrcImage(i, newSrcImg);
    }
    // keep old control points.
    newPano.setCtrlPoints(pano.getCtrlPoints());

    //write output
    HuginBase::UIntSet imgs;
    fill_set(imgs, 0, newPano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_template.pto");
    }
    std::ofstream of(output.c_str());
    newPano.printPanoramaScript(of, newPano.getOptimizeVector(), newPano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    std::cout << std::endl << "Written output to " << output << std::endl;
    return 0;
}
