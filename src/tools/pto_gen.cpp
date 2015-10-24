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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <fstream>
#include <getopt.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <vigra/imageinfo.hxx>
#include <panodata/Panorama.h>
#include <panodata/StandardImageVariableGroups.h>
#include <panodata/OptimizerSwitches.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include "hugin_utils/alphanum.h"
#include <lensdb/LensDB.h>

#ifdef __APPLE__
#include <hugin_config.h>
#include <mach-o/dyld.h>    /* _NSGetExecutablePath */
#include <limits.h>         /* PATH_MAX */
#include <libgen.h>         /* dirname */
#endif

static void usage(const char* name)
{
    std::cout << name << ": generate project file from images" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] image1 [...]" << std::endl
         << std::endl
         << "  Options:" << std::endl
         << "     -o, --output=file.pto  Output Hugin PTO file." << std::endl
         << "     -p, --projection=INT   Projection type (default: 0)" << std::endl
         << "     -f, --fov=FLOAT        Horizontal field of view of images (default: 50)" << std::endl
         << "     -c, --crop=left,right,top,bottom        Sets the crop of input" << std::endl
         << "                            images (especially for fisheye lenses)" << std::endl
         << "     -s, --stacklength=INT  Number of images in stack" << std::endl
         << "                            (default: automatic detection)" << std::endl
         << "     -l, --linkstacks       Link image positions in stacks" << std::endl
         << "     --distortion           Try to load distortion information from" << std::endl
         << "                            lens database" << std::endl
         << "     --vignetting           Try to load vignetting information from" << std::endl
         << "                            lens database" << std::endl
         << "     -h, --help             Shows this help" << std::endl
         << std::endl;
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
    std::string output;
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
                        std::cerr << "Could not parse image number.";
                        return 1;
                    };
                    if(projection<0)
                    {
                        std::cerr << "Invalid projection number." << std::endl;
                        return 1;
                    };
                };
                break;
            case 'f':
                fov=atof(optarg);
                if(fov<1 || fov>360)
                {
                    std::cerr << "Invalid field of view";
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
                            std::cerr << "Invalid crop area" << std::endl;
                            return 1;
                        };
                    }
                    else
                    {
                        std::cerr << "Could not parse crop values" << std::endl;
                        return 1;
                    };
                };
                break;
            case 's':
                stackLength=atoi(optarg);
                if ((stackLength == 0) && (strcmp(optarg, "0") != 0))
                {
                    std::cerr << "Could not parse stack length.";
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
                std::cerr <<"Option " << longOptions[optionIndex].name << " requires a number" << std::endl;
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

    std::cout << "Generating pto file..." << std::endl;
    std::cout.flush();

    std::vector<std::string> filelist;
    while(optind<argc)
    {
        std::string input;
#ifdef _WIN32
        //do globbing
        input = hugin_utils::GetAbsoluteFilename(argv[optind]);
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
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
                    char fname[_MAX_FNAME];
                    char ext[_MAX_EXT];
                    char newFile[_MAX_PATH];
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
                filelist.push_back(hugin_utils::GetAbsoluteFilename(input));
            };
        };
#endif
        optind++;
    };

    if(filelist.size()==0)
    {
        std::cerr << "No valid image files given." << std::endl;
        return 1;
    };

    //sort filenames
    sort(filelist.begin(),filelist.end(),doj::alphanum_less());

    HuginBase::Panorama pano;
    for(size_t i=0; i<filelist.size(); i++)
    {
        HuginBase::SrcPanoImage srcImage;
        std::cout << "Reading " << filelist[i] << "..." << std::endl;
        srcImage.setFilename(filelist[i]);
        try
        {
            vigra::ImageImportInfo info(filelist[i].c_str());
            if(info.width()==0 || info.height()==0)
            {
                std::cerr << "ERROR: Could not decode image " << filelist[i] << std::endl
                     << "Skipping this image." << std::endl << std::endl;
                continue;
            }
            srcImage.setSize(info.size());
            // check for black/white images
            const std::string pixelType=info.getPixelType();
            if (pixelType == "BILEVEL")
            {
                std::cerr << "ERROR: Image " << filelist[i] << " is a black/white images." << std::endl
                    << "       This is not supported. Convert to grayscale image and try again." << std::endl
                    << "       Skipping this image." << std::endl;
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
            std::cerr << "ERROR: caught exception: " << e.what() << std::endl;
            std::cerr << "Could not read image information for file " << filelist[i] << std::endl;
            std::cerr << "Skipping this image." << std::endl << std::endl;
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
                std::cout << "\tNo value for field of view found in EXIF data. " << std::endl
                     << "\tAssuming a HFOV of 50 degrees. " << std::endl;
                srcImage.setHFOV(50);
                srcImage.setCropFactor(1.0);
            };
        };
        if(cropRect.width()>0 && cropRect.height()>0)
        {
            if(srcImage.isCircularCrop())
            {
                srcImage.setCropMode(HuginBase::SrcPanoImage::CROP_CIRCLE);
            }
            else
            {
                srcImage.setCropMode(HuginBase::SrcPanoImage::CROP_RECTANGLE);
            };
            srcImage.setAutoCenterCrop(false);
            srcImage.setCropRect(cropRect);
        };
        if(loadDistortion)
        {
            if(srcImage.readDistortionFromDB())
            {
                std::cout << "\tRead distortion data from lens database." << std::endl;
            }
            else
            {
                std::cout << "\tNo valid distortion data found in lens database." << std::endl;
            };
        };
        if(loadVignetting)
        {
            if(srcImage.readVignettingFromDB())
            {
                std::cout << "\tRead vignetting data from lens database." << std::endl;
            }
            else
            {
                std::cout << "\tNo valid vignetting data found in lens database." << std::endl;
            };
        };

        pano.addImage(srcImage);
    };

    if(pano.getNrOfImages()==0)
    {
        std::cerr << "Adding images to project files failed." << std::endl;
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
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        HuginBase::ImageVariableGroup& lenses = variable_groups.getLenses();

        for(size_t i=1; i<pano.getNrOfImages(); i++)
        {
            int image=-1;
            const HuginBase::SrcPanoImage& srcImg=pano.getImage(i);
            for(size_t j=0; j<i; j++)
            {
                const HuginBase::SrcPanoImage& compareImg=pano.getImage(j);
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
                HuginBase::SrcPanoImage img=pano.getSrcImage(i);
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
        std::cout << std::endl << "Assigned " << lenses.getNumberOfParts() << " lenses." << std::endl;
        if(lenses.getNumberOfParts()>1 && stackLength!=1)
        {
            std::cout << "Project contains more than one lens, but you requested to assign" << std::endl
                 << "stacks. This is not supported. Therefore stacks will not be" << std::endl
                 << "assigned." << std::endl << std::endl;
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
    HuginBase::PanoramaOptions opt = pano.getOptions();
    opt.outputExposureValue = HuginBase::CalculateMeanExposure::calcMeanExposure(pano);
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
    output = hugin_utils::GetAbsoluteFilename(output);
    //write output
    HuginBase::UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    std::ofstream of(output.c_str());
    pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(output));

    std::cout << std::endl << "Written output to " << output << std::endl;
    HuginBase::LensDB::LensDB::Clean();
    return 0;
}
