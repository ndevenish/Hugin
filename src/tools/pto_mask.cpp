// -*- c-basic-offset: 4 -*-

/** @file pto_mask.cpp
 *
 *  @brief program to set mask from command line
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

struct MaskFiles
{
    size_t imageNr;
    std::string maskFile;
};

static void usage(const char* name)
{
    std::cout << name << ": add mask to pto project" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] input.pto" << std::endl
         << std::endl
         << "  Options:" << std::endl
         << "     -o, --output=file.pto  Output Hugin PTO file. Default: <filename>_mask.pto" << std::endl
         << "     --mask=filename@imgNr  Read the mask from the file and" << std::endl
         << "                            assign the mask to given image" << std::endl
         << "     --rotate=CLOCKWISE|90|COUNTERCLOCKWISE|-90" << std::endl
         << "                            Rotates the mask clock- or counterclockwise" << std::endl
         << "     --process==CLIP|SCALE|PROP_SCALE   Specify how the mask should be modified" << std::endl
         << "                            if the mask is create for an image with" << std::endl
         << "                            different size." << std::endl
         << "                            * CLIP: clipping (Default)" << std::endl
         << "                            * SCALE: Scaling width and height individually" << std::endl
         << "                            * PROP_SCALE: Proportional scale" << std::endl
         << "     -h, --help             Shows this help" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:h";
    enum
    {
        MASK_SWITCH=1000,
        ROTATE_SWITCH,
        PROC_SWITCH
    };
    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"mask", required_argument, NULL, MASK_SWITCH },
        {"rotate", required_argument, NULL, ROTATE_SWITCH},
        {"process", required_argument, NULL, PROC_SWITCH},
        {"help", no_argument, NULL, 'h' },
        0
    };

    int c;
    int optionIndex = 0;
    std::vector<MaskFiles> maskFiles;
    size_t rotate=0;
    size_t process=0;
    std::string output;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case MASK_SWITCH:
                {
                    std::string s=optarg;
                    size_t found=s.rfind('@');
                    MaskFiles mf;
                    if(found!=std::string::npos)
                    {
                        std::string s2=s.substr(found+1, std::string::npos);
                        mf.imageNr=atoi(s2.c_str());
                        if(mf.imageNr==0 && s2!="0")
                        {
                            std::cerr << "Error: Could not parse image number: \"" << s2 << "\"." << std::endl;
                            return 1;
                        };
                    }
                    else
                    {
                        std::cerr << "Error: No image number found in \"" << s << "\"." << std::endl;
                        return 1;
                    };
                    mf.maskFile=s.substr(0, found);
                    if(!hugin_utils::FileExists(mf.maskFile))
                    {
                        std::cerr << "Error: File \"" << mf.maskFile << "\" does not exists." << std::endl;
                        return 1;
                    };
                    maskFiles.push_back(mf);
                };
                break;
            case ROTATE_SWITCH:
                {
                    std::string s=optarg;
                    s=hugin_utils::toupper(s);
                    if(s=="CLOCKWISE" || s=="90")
                    {
                        rotate=1;
                    }
                    else
                    {
                        if(s=="COUNTERCLOCKWISE" || s=="-90")
                        {
                            rotate=2;
                        }
                        else
                        {
                            std::cerr << "Error:  Unknown rotate command (" << optarg << ") found." << std::endl;
                            return 1;
                        };
                    };
                };
                break;
            case PROC_SWITCH:
                {
                    std::string s=optarg;
                    s=hugin_utils::toupper(s);
                    if(s=="CLIP")
                    {
                        process=0;
                    }
                    else
                    {
                        if(s=="SCALE")
                        {
                            process=1;
                        }
                        else
                        {
                            if(s=="PROP_SCALE")
                            {
                                process=2;
                            }
                            else
                            {
                                std::cerr << "Error: Unknown process command (" << optarg << ") found." << std::endl;
                                return 1;
                            };
                        };
                    };
                }
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
        std::cout << "Error: No project file given." << std::endl << std::endl;
        return 1;
    };
    if (argc - optind != 1)
    {
        std::cout << "Error: pto_mask can only work on one project file at one time" << std::endl << std::endl;
        return 1;
    };

    if(maskFiles.size()==0)
    {
        std::cerr << "Error: No mask files given." << std::endl << std::endl;
        return 1;
    };

    std::string input=argv[optind];
    // read panorama
    HuginBase::Panorama pano;
    std::ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        std::cerr << "Error: could not open script " << input << std::endl;
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
        return 1;
    };

    //read masks and apply
    for(size_t i=0; i<maskFiles.size(); i++)
    {
        if(maskFiles[i].imageNr<pano.getNrOfImages())
        {
            std::ifstream in(maskFiles[i].maskFile.c_str());
            vigra::Size2D maskImageSize;
            HuginBase::MaskPolygonVector loadedMasks;
            LoadMaskFromStream(in, maskImageSize, loadedMasks, maskFiles[i].imageNr);
            in.close();
            if(maskImageSize.area()==0 || loadedMasks.size()==0)
            {
                std::cerr << "Error: Could not parse mask from file \"" << maskFiles[i].maskFile << "\"." << std::endl;
                return 1;
            };
            double maskWidth;
            double maskHeight;
            if(rotate==0)
            {
                maskWidth=maskImageSize.width();
                maskHeight=maskImageSize.height();
            }
            else
            {
                maskWidth=maskImageSize.height();
                maskHeight=maskImageSize.width();
                bool clockwise=(rotate==1);
                for(unsigned int i=0; i<loadedMasks.size(); i++)
                {
                    loadedMasks[i].rotate90(clockwise, maskImageSize.width(), maskImageSize.height());
                }
            };
            // compare image size from file with that of current image alert user
            // if different.
            vigra::Size2D imageSize=pano.getImage(maskFiles[i].imageNr).getSize();
            if (maskImageSize != imageSize)
            {
                switch(process)
                {
                    case 0:
                        // clip mask
                        std::cout << "Clipping mask" << std::endl;
                        for(unsigned int i=0; i<loadedMasks.size(); i++)
                            loadedMasks[i].clipPolygon(vigra::Rect2D(-0.5*HuginBase::maskOffset, -0.5*HuginBase::maskOffset,
                                                       imageSize.width()+0.5*HuginBase::maskOffset, imageSize.height()+0.5*HuginBase::maskOffset));
                        break;
                    case 1:
                        // scale mask
                        std::cout << "Scaling mask" << std::endl;
                        for(unsigned int i=0; i<loadedMasks.size(); i++)
                        {
                            loadedMasks[i].scale((double)imageSize.width()/maskWidth,(double)imageSize.height()/maskHeight);
                        }
                        break;
                    case 2:
                        // proportional scale mask
                        std::cout << "Propotional scale mask" << std::endl;
                        {
                            double factor=std::min((double)imageSize.width()/maskWidth, (double)imageSize.height()/maskHeight);
                            for(unsigned int i=0; i<loadedMasks.size(); i++)
                            {
                                loadedMasks[i].scale(factor);
                            }
                        };
                        break;
                };
            };
            HuginBase::MaskPolygonVector masks = pano.getImage(maskFiles[i].imageNr).getMasks();
            for(size_t j=0; j<loadedMasks.size(); j++)
            {
                masks.push_back(loadedMasks[j]);
            };
            pano.updateMasksForImage(maskFiles[i].imageNr, masks);
        }
        else
        {
            std::cout << "Warning: Invalid image number \"" << maskFiles[i].imageNr << "\"." << std::endl
                 << "         Project contains only " << pano.getNrOfImages()+1 << " images." << std::endl
                 << "         Ignoring this mask." << std::endl;
        };
    };

    //write output
    HuginBase::UIntSet imgs;
    fill_set(imgs, 0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_mask.pto");
    }
    std::ofstream of(output.c_str());
    pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    std::cout << std::endl << "Written output to " << output << std::endl;
    return 0;
}
