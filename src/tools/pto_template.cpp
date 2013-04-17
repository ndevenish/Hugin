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

static void usage(const char* name)
{
    cout << name << ": apply template" << endl
         << name << " version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] input.pto" << endl
         << endl
         << "  Options:" << endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << endl
         << "                            Default: <filename>_template.pto" << endl
         << "     --template=template.pto   Apply the given template file" << endl
         << "     -h, --help             Shows this help" << endl
         << endl;
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
    string output;
    string templateFile;
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
                    cerr << "Error: Template \"" << templateFile << "\" not found." << endl;
                    return 1;
                };
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            case ':':
                cerr <<"Option " << longOptions[optionIndex].name << " requires a parameter" << endl;
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
        cout << "Error: No project file given." << endl;
        return 1;
    };
    if (argc - optind != 1)
    {
        cout << "Error: pto_template can only work on one project file at one time" << endl;
        return 1;
    };
    if (templateFile.length()==0)
    {
        cerr << "Error: No template given." << endl;
        return 1;
    };

    string input=argv[optind];
    // read panorama
    Panorama pano;
    ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        cerr << "Error: could not open script : " << input << endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != DocumentData::SUCCESSFUL)
    {
        cerr << "Error while parsing panos tool script: " << input << endl;
        cerr << "DocumentData::ReadWriteError code: " << err << endl;
        return 1;
    }

    if(pano.getNrOfImages()==0)
    {
        cerr << "Error: project file does not contains any image" << endl;
        cerr << "aborting processing" << endl;
        return 1;
    };

    Panorama newPano;
    ifstream templateStream(templateFile.c_str());
    if (!templateStream.good())
    {
        cerr << "Error: could not open template script : " << templateFile << endl;
        return 1;
    }
    newPano.setFilePrefix(hugin_utils::getPathPrefix(templateFile));
    err = newPano.readData(templateStream);
    if (err != DocumentData::SUCCESSFUL)
    {
        cerr << "Error while parsing template script: " << templateFile << endl;
        cerr << "DocumentData::ReadWriteError code: " << err << endl;
        return 1;
    }

    if (pano.getNrOfImages() != newPano.getNrOfImages())
    {
        cerr << "Error: template expects " << newPano.getNrOfImages() << " images," << endl
             << "       current project contains " << pano.getNrOfImages() << " images" << endl
             << "       Could not apply template" << endl;
        return false;
    }

    // check image sizes, and correct parameters if required.
    for (unsigned int i = 0; i < newPano.getNrOfImages(); i++)
    {
        // check if image size is correct
        const SrcPanoImage & oldSrcImg = pano.getImage(i);
        SrcPanoImage newSrcImg = newPano.getSrcImage(i);

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
    UIntSet imgs;
    fill_set(imgs, 0, newPano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_template.pto");
    }
    ofstream of(output.c_str());
    newPano.printPanoramaScript(of, newPano.getOptimizeVector(), newPano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    cout << endl << "Written output to " << output << endl;
    return 0;
}
