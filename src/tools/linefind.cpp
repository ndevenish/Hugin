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
#include <vigra_ext/impexalpha.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra_ext/utils.h>

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
         << "     -o, --output=file.pto  Output Hugin PTO file. Default: <filename>_lines.pto" << endl
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

/** converts the given image to UInt8RGBImage
 *  only this image is correctly processed by linefind
 *  @param src input image
 *  @param origType pixel type of input image
 *  @param dest converted image
 */
// 2 versions: one for color images, the other for gray images
template <class SrcIMG>
void convertToUInt8(SrcIMG & src, const std::string & origType, vigra::UInt8RGBImage & dest)
{
    dest.resize(src.size());
    long newMax=vigra_ext::getMaxValForPixelType("UINT8");
    // float needs to be from min ... max.
    if (origType == "FLOAT" || origType == "DOUBLE")
    {
        /** @TODO this convert routine scale the input values range into the full scale of UInt16
         *  this is not fully correct
         */
        vigra::RGBToGrayAccessor<vigra::RGBValue<float> > ga;
        vigra::FindMinMax<float> minmax;   // init functor
        vigra::inspectImage(srcImageRange(src, ga),
                            minmax);
        double minVal = minmax.min;
        double maxVal = minmax.max;
        vigra_ext::applyMapping(srcImageRange(src), destImage(dest), minVal, maxVal, 0);
    }
    else
    {
        vigra::transformImage(srcImageRange(src), destImage(dest),
            vigra::functor::Arg1()*vigra::functor::Param( newMax/ vigra_ext::getMaxValForPixelType(origType)));
    };
}

template <class SrcIMG>
void convertGrayToUInt8(SrcIMG & src, const std::string & origType, vigra::BImage & dest)
{
    dest.resize(src.size());
    long newMax=vigra_ext::getMaxValForPixelType("UINT8");
    // float needs to be from min ... max.
    if (origType == "FLOAT" || origType == "DOUBLE")
    {
        /** @TODO this convert routine scale the input values range into the full scale of UInt16
         *  this is not fully correct
         */
        vigra::FindMinMax<float> minmax;   // init functor
        vigra::inspectImage(srcImageRange(src), minmax);
        double minVal = minmax.min;
        double maxVal = minmax.max;
        vigra_ext::applyMapping(srcImageRange(src), destImage(dest), minVal, maxVal, 0);
    }
    else
    {
        vigra::transformImage(srcImageRange(src), destImage(dest),
            vigra::functor::Arg1()*vigra::functor::Param( newMax/ vigra_ext::getMaxValForPixelType(origType)));
    };
}

template <class SrcIMG>
vigra::BImage LoadGrayImageAndConvert(vigra::ImageImportInfo & info)
{
    vigra::BImage image;
    SrcIMG imageIn(info.width(),info.height());
    if(info.numExtraBands()==1)
    {
        vigra::BImage mask(info.size());
        vigra::importImageAlpha(info,destImage(imageIn),destImage(mask));
        mask.resize(0,0);
    }
    else
    {
        importImage(info,destImage(imageIn));
    };
    convertGrayToUInt8(imageIn,info.getPixelType(),image);
    imageIn.resize(0,0);
    return image;
};

template <class SrcIMG>
vigra::UInt8RGBImage LoadImageAndConvert(vigra::ImageImportInfo & info)
{
    vigra::UInt8RGBImage image;
    SrcIMG imageIn(info.width(),info.height());
    if(info.numExtraBands()==1)
    {
        vigra::BImage mask(info.size());
        vigra::importImageAlpha(info,destImage(imageIn),destImage(mask));
        mask.resize(0,0);
    }
    else
    {
        importImage(info,destImage(imageIn));
    };
    convertToUInt8(imageIn,info.getPixelType(),image);
    imageIn.resize(0,0);
    return image;
};

// loads the gray images and finds vertical lines, returns a CPVector with found vertical lines
HuginBase::CPVector LoadGrayImageAndFindLines(vigra::ImageImportInfo info, Panorama & pano, size_t imgNr, int nrLines)
{
    vigra::BImage image;
    HuginBase::CPVector lineCp;
    std::string pixelType=info.getPixelType();
    if(pixelType=="UINT8")
    {
        image.resize(info.width(),info.height());
        if(info.numExtraBands()==1)
        {
            vigra::BImage mask(info.size());
            vigra::importImageAlpha(info,destImage(image),destImage(mask));
            mask.resize(0,0);
        }
        else
        {
            importImage(info,destImage(image));
        };
    }
    else
    {
        if(pixelType=="UINT16" || pixelType=="INT16")
        {
            image=LoadGrayImageAndConvert<vigra::UInt16Image>(info);
        }
        else
        {
            if(pixelType=="INT32" || pixelType=="UINT32")
            {
                image=LoadGrayImageAndConvert<vigra::UInt32Image>(info);
            }
            else
            {
                if(pixelType=="FLOAT" || pixelType=="DOUBLE")
                {
                    image=LoadGrayImageAndConvert<vigra::FImage>(info);
                }
                else
                {
                    std::cerr << "Unsupported pixel type" << std::endl;
                };
            };
        };
    };
    if(image.width()>0 && image.height()>0)
    {
        lineCp=HuginLines::GetVerticalLines(pano, imgNr, image, nrLines);
    };
    return lineCp;
};

// loads the color images and finds vertical lines, returns a CPVector with found vertical lines
HuginBase::CPVector LoadImageAndFindLines(vigra::ImageImportInfo info, Panorama & pano, size_t imgNr, int nrLines)
{
    vigra::UInt8RGBImage image;
    HuginBase::CPVector lineCp;
    std::string pixelType=info.getPixelType();
    if(pixelType=="UINT8")
    {
        image.resize(info.width(),info.height());
        if(info.numExtraBands()==1)
        {
            vigra::BImage mask(info.size());
            vigra::importImageAlpha(info,destImage(image),destImage(mask));
            mask.resize(0,0);
        }
        else
        {
            importImage(info,destImage(image));
        };
    }
    else
    {
        if(pixelType=="UINT16" || pixelType=="INT16")
        {
            image=LoadImageAndConvert<vigra::UInt16RGBImage>(info);
        }
        else
        {
            if(pixelType=="INT32" || pixelType=="UINT32")
            {
                image=LoadImageAndConvert<vigra::UInt32RGBImage>(info);
            }
            else
            {
                if(pixelType=="FLOAT" || pixelType=="DOUBLE")
                {
                    image=LoadImageAndConvert<vigra::FRGBImage>(info);
                }
                else
                {
                    std::cerr << "Unsupported pixel type" << std::endl;
                };
            };
        };
    };
    if(image.width()>0 && image.height()>0)
    {
        lineCp=HuginLines::GetVerticalLines(pano, imgNr, image, nrLines);
    };
    return lineCp;
};

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
                        cerr << "Could not parse image number.";
                        return 1;
                    };
                    cmdlineImages.insert(imgNr);
                };
                break;
            case 'l':
                nrLines=atoi(optarg);
                if(nrLines<1)
                {
                    cerr << "Could not parse number of lines.";
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
#if _WINDOWS
    //multi threading of image loading results sometime in a race condition
    //try to prevent this by initialisation of codecManager before
    //running multi threading part
    std::string s=vigra::impexListExtensions();
#endif
#ifdef HAS_PPL
    size_t nrCPS=pano.getNrOfCtrlPoints();
    Concurrency::parallel_for<size_t>(0,imagesToProcess.size(),[&pano,imagesToProcess,nrLines](size_t i)
#else
    for(size_t i=0;i<imagesToProcess.size();i++)
#endif
    {
        unsigned int imgNr=imagesToProcess[i];
        cout << "Working on image " << pano.getImage(imgNr).getFilename() << endl;
        // now load and process all images
        vigra::ImageImportInfo info(pano.getImage(imgNr).getFilename().c_str());
        HuginBase::CPVector foundLines;
        if(info.isGrayscale())
        {
            foundLines=LoadGrayImageAndFindLines(info, pano, imgNr, nrLines);
        }
        else
        {
            if(info.isColor())
            {
                //colour images
                foundLines=LoadImageAndFindLines(info, pano, imgNr, nrLines);
            }
            else
            {
                std::cerr << "Image " << pano.getImage(imgNr).getFilename().c_str() << " has " 
                    << info.numBands() << " channels." << std::endl
                    << "Linefind works only with grayscale or color images." << std::endl
                    << "Skipping image." << std::endl;
            };
        };
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
