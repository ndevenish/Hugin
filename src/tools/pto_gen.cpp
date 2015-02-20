// -*- c-basic-offset: 4 -*-

/** @file pto_gen.cpp
 *
 *  @brief program to generate a pto file from given image files
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

#include <fstream>
#include <getopt.h>
#ifdef _WINDOWS
#include <io.h>
#endif
#include <panodata/Panorama.h>
#include <panodata/StandardImageVariableGroups.h>
#include <panodata/OptimizerSwitches.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include "hugin_utils/alphanum.h"
#include <vigra/impex.hxx>
#include <lensdb/LensDB.h>

#ifdef __APPLE__
#include <hugin_config.h>
#include <mach-o/dyld.h>    /* _NSGetExecutablePath */
#include <limits.h>         /* PATH_MAX */
#include <libgen.h>         /* dirname */
#endif

using namespace std;
using namespace HuginBase;

static void usage(const char* name)
{
    cout << name << ": generate project file from images" << endl
         << name << " version " << hugin_utils::GetHuginVersion() << endl
         << endl
         << "Usage:  " << name << " [options] image1 [...]" << endl
         << endl
         << "  Options:" << endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << endl
         << "     -p, --projection=INT   Projection type (default: 0)" << endl
         << "     -f, --fov=FLOAT        Horizontal field of view of images (default: 50)" << endl
         << "     -c, --crop=left,right,top,bottom        Sets the crop of input" << endl
         << "                            images (especially for fisheye lenses)" << endl
         << "     -s, --stacklength=INT  Number of images in stack" << endl
         << "                            (default: automatic detection)" << endl
         << "     -l, --linkstacks       Link image positions in stacks" << endl
         << "     --distortion           Try to load distortion information from" << endl
         << "                            lens database" << endl
         << "     --vignetting           Try to load vignetting information from" << endl
         << "                            lens database" << endl
         << "     -h, --help             Shows this help" << endl
         << endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:p:f:c:s:lh";

    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"projection", required_argument, NULL, 'p' },
        {"fov", required_argument, NULL, 'f' },
        {"crop", required_argument, NULL, 'c' },
        {"stacklength", required_argument, NULL, 's' },
        {"linkstacks", no_argument, NULL, 'l' },
        {"distortion", no_argument, NULL, 300 },
        {"vignetting", no_argument, NULL, 301 },
        {"help", no_argument, NULL, 'h' },

        0
    };

    int c;
    int optionIndex = 0;
    string output;
    int projection=-1;
    float fov=-1;
    int stackLength=0;
    bool linkStacks=false;
    vigra::Rect2D cropRect(0,0,0,0);
    bool loadDistortion=false;
    bool loadVignetting=false;
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
            case 'p':
                {
                    projection=atoi(optarg);
                    if((projection==0) && (strcmp(optarg,"0")!=0))
                    {
                        cerr << "Could not parse image number.";
                        return 1;
                    };
                    if(projection<0)
                    {
                        cerr << "Invalid projection number." << endl;
                        return 1;
                    };
                };
                break;
            case 'f':
                fov=atof(optarg);
                if(fov<1 || fov>360)
                {
                    cerr << "Invalid field of view";
                    return 1;
                };
                break;
            case 'c':
                {
                    int left, right, top, bottom;
                    int n=sscanf(optarg, "%d,%d,%d,%d", &left, &right, &top, &bottom);
                    if (n==4)
                    {
                        if(right>left && bottom>top)
                        {
                            cropRect.setUpperLeft(vigra::Point2D(left,top));
                            cropRect.setLowerRight(vigra::Point2D(right,bottom));
                        }
                        else
                        {
                            cerr << "Invalid crop area" << endl;
                            return 1;
                        };
                    }
                    else
                    {
                        cerr << "Could not parse crop values" << endl;
                        return 1;
                    };
                };
                break;
            case 's':
                stackLength=atoi(optarg);
                if ((stackLength == 0) && (strcmp(optarg, "0") != 0))
                {
                    cerr << "Could not parse stack length.";
                    return 1;
                };
                break;
            case 'l':
                linkStacks=true;
                break;
            case 300:
                loadDistortion=true;
                break;
            case 301:
                loadVignetting=true;
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

    if (argc - optind < 1)
    {
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return 1;
    };

    cout << "Generating pto file..." << endl;
    cout.flush();

    std::vector<string> filelist;
    while(optind<argc)
    {
        string input;
#ifdef _WINDOWS
        //do globbing
        input=GetAbsoluteFilename(argv[optind]);
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        char newFile[_MAX_PATH];

        _splitpath(input.c_str(), drive, dir, NULL, NULL);

        struct _finddata_t finddata;
        intptr_t findhandle = _findfirst(input.c_str(), &finddata);
        if (findhandle != -1)
        {
            do
            {
                //ignore folder, can be happen when using *.*
                if((finddata.attrib & _A_SUBDIR)==0)
                {
                    _splitpath(finddata.name, NULL, NULL, fname, ext);
                    _makepath(newFile, drive, dir, fname, ext);
                    //check if valid image file
                    if(vigra::isImage(newFile))
                    {
                        filelist.push_back(std::string(newFile));
                    };
                };
            }
            while (_findnext(findhandle, &finddata) == 0);
            _findclose(findhandle);
        }
#else
        input=argv[optind];
        if(hugin_utils::FileExists(input))
        {
            if(vigra::isImage(input.c_str()))
            {
                filelist.push_back(GetAbsoluteFilename(input));
            };
        };
#endif
        optind++;
    };

    if(filelist.size()==0)
    {
        cerr << "No valid image files given." << endl;
        return 1;
    };

    //sort filenames
    sort(filelist.begin(),filelist.end(),doj::alphanum_less());

    Panorama pano;
    for(size_t i=0; i<filelist.size(); i++)
    {
        SrcPanoImage srcImage;
        cout << "Reading " << filelist[i] << "..." << endl;
        srcImage.setFilename(filelist[i]);
        try
        {
            vigra::ImageImportInfo info(filelist[i].c_str());
            if(info.width()==0 || info.height()==0)
            {
                cerr << "ERROR: Could not decode image " << filelist[i] << endl
                     << "Skipping this image." << endl << endl;
                continue;
            }
            srcImage.setSize(info.size());
            // check for black/white images
            const std::string pixelType=info.getPixelType();
            if (pixelType == "BILEVEL")
            {
                cerr << "ERROR: Image " << filelist[i] << " is a black/white images." << endl
                    << "       This is not supported. Convert to grayscale image and try again." << endl
                    << "       Skipping this image." << endl;
                continue;
            }
            if((pixelType=="UINT8") || (pixelType=="UINT16") || (pixelType=="INT16"))
            {
                srcImage.setResponseType(HuginBase::SrcPanoImage::RESPONSE_EMOR);
            }
            else
            {
                srcImage.setResponseType(HuginBase::SrcPanoImage::RESPONSE_LINEAR);
            };
        }
        catch(std::exception& e)
        {
            cerr << "ERROR: caught exception: " << e.what() << endl;
            cerr << "Could not read image information for file " << filelist[i] << endl;
            cerr << "Skipping this image." << endl << endl;
            continue;
        };

        srcImage.readEXIF();
        bool fovOk=srcImage.applyEXIFValues();
        if(projection>=0)
        {
            srcImage.setProjection((HuginBase::BaseSrcPanoImage::Projection)projection);
        }
        else
        {
            srcImage.readProjectionFromDB();
        };
        if(fov>0)
        {
            srcImage.setHFOV(fov);
            if(srcImage.getCropFactor()==0)
            {
                srcImage.setCropFactor(1.0);
            };
        }
        else
        {
            //set plausible default value if they could not read from exif
            if(!fovOk)
            {
                cout << "\tNo value for field of view found in EXIF data. " << endl
                     << "\tAssuming a HFOV of 50 degrees. " << endl;
                srcImage.setHFOV(50);
                srcImage.setCropFactor(1.0);
            };
        };
        if(cropRect.width()>0 && cropRect.height()>0)
        {
            if(srcImage.isCircularCrop())
            {
                srcImage.setCropMode(SrcPanoImage::CROP_CIRCLE);
            }
            else
            {
                srcImage.setCropMode(SrcPanoImage::CROP_RECTANGLE);
            };
            srcImage.setAutoCenterCrop(false);
            srcImage.setCropRect(cropRect);
        };
        if(loadDistortion)
        {
            if(srcImage.readDistortionFromDB())
            {
                cout << "\tRead distortion data from lens database." << endl;
            }
            else
            {
                cout << "\tNo valid distortion data found in lens database." << endl;
            };
        };
        if(loadVignetting)
        {
            if(srcImage.readVignettingFromDB())
            {
                cout << "\tRead vignetting data from lens database." << endl;
            }
            else
            {
                cout << "\tNo valid vignetting data found in lens database." << endl;
            };
        };

        pano.addImage(srcImage);
    };

    if(pano.getNrOfImages()==0)
    {
        cerr << "Adding images to project files failed." << endl;
        HuginBase::LensDB::LensDB::Clean();
        return 1;
    };

    //link lenses
    if(pano.getNrOfImages()>1)
    {
        double redBalanceAnchor=pano.getImage(pano.getOptions().colorReferenceImage).getExifRedBalance();
        double blueBalanceAnchor=pano.getImage(pano.getOptions().colorReferenceImage).getExifBlueBalance();
        if(fabs(redBalanceAnchor)<1e-2)
        {
            redBalanceAnchor=1;
        };
        if(fabs(blueBalanceAnchor)<1e-2)
        {
            blueBalanceAnchor=1;
        };
        StandardImageVariableGroups variable_groups(pano);
        ImageVariableGroup& lenses = variable_groups.getLenses();

        for(size_t i=1; i<pano.getNrOfImages(); i++)
        {
            int image=-1;
            const SrcPanoImage& srcImg=pano.getImage(i);
            for(size_t j=0; j<i; j++)
            {
                const SrcPanoImage& compareImg=pano.getImage(j);
                if(srcImg.getHFOV()==compareImg.getHFOV() &&
                        srcImg.getProjection()==compareImg.getProjection() &&
                        srcImg.getExifModel()==compareImg.getExifModel() &&
                        srcImg.getExifMake()==compareImg.getExifMake() &&
                        srcImg.getSize()==compareImg.getSize())
                {
                    image=j;
                    break;
                };
            };
            if(image!=-1)
            {
                SrcPanoImage img=pano.getSrcImage(i);
                double ev=img.getExposureValue();
                lenses.switchParts(i,lenses.getPartNumber(image));
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_ExposureValue, i);
                img.setExposureValue(ev);
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceRed, i);
                lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceBlue, i);
                img.setWhiteBalanceRed(img.getExifRedBalance()/redBalanceAnchor);
                img.setWhiteBalanceBlue(img.getExifBlueBalance()/blueBalanceAnchor);
                pano.setSrcImage(i, img);
            };
        };
        cout << endl << "Assigned " << lenses.getNumberOfParts() << " lenses." << endl;
        if(lenses.getNumberOfParts()>1 && stackLength!=1)
        {
            cout << "Project contains more than one lens, but you requested to assign" << endl
                 << "stacks. This is not supported. Therefore stacks will not be" << endl
                 << "assigned." << endl << endl;
            stackLength=1;
        };

        if (stackLength == 0)
        {
            // automatic detection
            if (pano.hasPossibleStacks())
            {
                pano.linkPossibleStacks(linkStacks);
            };
        }
        else
        {
            if (stackLength > 1)
            {
                stackLength = std::min<int>(stackLength, pano.getNrOfImages());
                int stackCount = pano.getNrOfImages() / stackLength;
                if (pano.getNrOfImages() % stackLength > 0)
                {
                    stackCount++;
                };
                if (stackCount < pano.getNrOfImages())
                {
                    for (size_t stackNr = 0; stackNr < stackCount; stackNr++)
                    {
                        size_t firstImgStack = stackNr*stackLength;
                        for (size_t i = 0; i < stackLength; i++)
                        {
                            if (firstImgStack + i < pano.getNrOfImages())
                            {
                                pano.linkImageVariableStack(firstImgStack, firstImgStack + i);
                                if (linkStacks)
                                {
                                    pano.linkImageVariableYaw(firstImgStack, firstImgStack + i);
                                    pano.linkImageVariablePitch(firstImgStack, firstImgStack + i);
                                    pano.linkImageVariableRoll(firstImgStack, firstImgStack + i);
                                };
                            };
                        };
                    };
                };
            };
        };

        variable_groups.update();
        const size_t stackCount = variable_groups.getStacks().getNumberOfParts();
        if (stackCount != pano.getNrOfImages())
        {
            std::cout << "Assigned " << stackCount << " stacks: " << std::endl
                << "\t" << (linkStacks ? "Linking position of images in stacks" : "Use individual positions of images in stacks") << std::endl;
        };
    };

    //set output exposure value
    PanoramaOptions opt = pano.getOptions();
    opt.outputExposureValue = CalculateMeanExposure::calcMeanExposure(pano);
    pano.setOptions(opt);
    // set optimizer switches
    pano.setOptimizerSwitch(HuginBase::OPT_PAIR);
    pano.setPhotometricOptimizerSwitch(HuginBase::OPT_EXPOSURE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE);

    //output
    if(output=="")
    {
        output=hugin_utils::stripExtension(pano.getImage(0).getFilename());
        if(pano.getNrOfImages()>1)
        {
            output.append("-");
            output.append(hugin_utils::stripExtension(hugin_utils::stripPath(pano.getImage(pano.getNrOfImages()-1).getFilename())));
        };
        output=output.append(".pto");
    };
    output=GetAbsoluteFilename(output);
    //write output
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    ofstream of(output.c_str());
    pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(output));

    cout << endl << "Written output to " << output << endl;
    HuginBase::LensDB::LensDB::Clean();
    return 0;
}
