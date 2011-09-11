// -*- c-basic-offset: 4 -*-

/** @file linefind.cpp
 *
 *  @brief program to find vertical lines in project
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
#include <lines/FindLines.h>
#include <vigra/impex.hxx>
extern "C"
{
#include <pano13/filter.h>
}
#if defined _MSC_VER && _MSC_VER>=1600
#include <ppl.h>
#define HAS_PPL
#endif

using namespace std;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char* name)
{
    cout << name << ": find vertical lines in images" << endl
         << name << " version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] input.pto" << endl
         << endl
         << "  Options:" << endl
         << "     -o, --output=file.pto  Output Hugin PTO file. Default: <filename>_line.pto" << endl
         << "     -i, --image=IMGNR      Work only on given image numbers" << endl
         << "     -l, --lines=COUNT      Save maximal COUNT lines (default: 5)" << endl
         << "     -h, --help             Shows this help" << endl
         << endl;
}

// dummy panotools progress functions
static int ptProgress( int command, char* argument )
{
    return 1;
}
static int ptinfoDlg( int command, char* argument )
{
    return 1;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:i:l:h";

    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"image", required_argument, NULL, 'i' },
        {"lines", required_argument, NULL, 'l' },
        {"help", no_argument, NULL, 'h' },
        0
    };

    UIntSet cmdlineImages;
    int c;
    int optionIndex = 0;
    int nrLines = 5;
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
            case 'i':
                {
                    int imgNr=atoi(optarg);
                    if((imgNr==0) && (strcmp(optarg,"0")!=0))
                    {
                        cerr << "Could not parse projection number.";
                        return 1;
                    };
                    cmdlineImages.insert(imgNr);
                };
                break;
            case 'l':
                nrLines=atoi(optarg);
                if(nrLines<1)
                {
                    cerr << "Cound not parse number of lines or invalid number.";
                    return 1;
                };
                break;
            case ':':
                cerr <<"Option " << longOptions[optionIndex].name << " requires a number" << endl;
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
        cout << "Warning: " << argv[0] << " can only work on one project file at one time" << endl << endl;
        usage(argv[0]);
        return 1;
    };

    string input=argv[optind];
    // read panorama
    Panorama pano;
    ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        cerr << "could not open script : " << input << endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != DocumentData::SUCCESSFUL)
    {
        cerr << "error while parsing panos tool script: " << input << endl;
        cerr << "DocumentData::ReadWriteError code: " << err << endl;
        return 1;
    }

    if(pano.getNrOfImages()==0)
    {
        cerr << "error: project file does not contains any image" << endl;
        cerr << "aborting processing" << endl;
        return 1;
    };

    std::vector<size_t> imagesToProcess;
    if(cmdlineImages.size()==0)
    {
        //no image given, process all
        for(size_t i=0;i<pano.getNrOfImages();i++)
        {
            imagesToProcess.push_back(i);
        };
    }
    else
    {
        //check, if given image numbers are valid
        for(UIntSet::const_iterator it=cmdlineImages.begin();it!=cmdlineImages.end();it++)
        {
            if((*it)>=0 && (*it)<pano.getNrOfImages())
            {
                imagesToProcess.push_back(*it);
            };
        };
    };

    if(imagesToProcess.size()==0)
    {
        cerr << "No image to process found" << endl << "Stopping processing" << endl;
        return 1;
    };

    PT_setProgressFcn(ptProgress);
    PT_setInfoDlgFcn(ptinfoDlg);

    cout << argv[0] << " is searching for vertical lines" << endl;
#ifdef HAS_PPL
    size_t nrCPS=pano.getNrOfCtrlPoints();
    Concurrency::parallel_for<size_t>(0,imagesToProcess.size(),[&pano,imagesToProcess,nrLines](size_t i)
#else
    for(size_t i=0;i<imagesToProcess.size();i++)
#endif
    {
        unsigned int imgNr=imagesToProcess[i];
        cout << "Working on image " << pano.getImage(imgNr).getFilename() << endl;
        vigra::ImageImportInfo imageInfo(pano.getImage(imgNr).getFilename().c_str());
        vigra::UInt8RGBImage image(imageInfo.width(),imageInfo.height());
        vigra::importImage(imageInfo,destImage(image));
        CPVector foundLines=HuginLines::GetVerticalLines(pano,imgNr,image,nrLines);
#ifndef HAS_PPL
        cout << "Found " << foundLines.size() << " vertical lines" << endl;
#endif
        if(foundLines.size()>0)
        {
            for(CPVector::const_iterator cpIt=foundLines.begin(); cpIt!=foundLines.end(); cpIt++)
            {
                pano.addCtrlPoint(*cpIt);
            };
        };
    }
#ifdef HAS_PPL
    );
    cout << endl << "Found " << pano.getNrOfCtrlPoints() - nrCPS << " vertical lines" << endl << endl;
#endif

    //write output
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_lines.pto");
    }
    ofstream of(output.c_str());
    pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    cout << endl << "Written output to " << output << endl;
    return 0;
}
