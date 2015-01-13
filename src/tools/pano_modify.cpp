// -*- c-basic-offset: 4 -*-

/** @file pano_modify.cpp
 *
 *  @brief program to modify some aspects of a project file on the command line
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

#include <fstream>
#include <sstream>
#include <getopt.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <panodata/Panorama.h>
#include <algorithms/nona/CenterHorizontally.h>
#include <algorithms/basic/StraightenPanorama.h>
#include <algorithms/basic/RotatePanorama.h>
#include <algorithms/basic/TranslatePanorama.h>
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/basic/CalculateOptimalScale.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <algorithms/basic/LayerStacks.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include "hugin_utils/utils.h"

using namespace std;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char* name)
{
    cout << name << ": change output parameters of project file" << endl
         << "pano_modify version " << hugin_utils::GetHuginVersion() << endl
         << endl
         << "Usage:  " << name << " [options] input.pto" << endl
         << endl
         << "  Options:" << endl
         << "    -o, --output=file.pto  Output Hugin PTO file. Default: <filename>_mod.pto" << endl
         << "    -p, --projection=x     Sets the output projection to number x" << endl
         << "    --fov=AUTO|HFOV|HFOVxVFOV   Sets field of view" << endl
         << "                                AUTO: calculates optimal fov" << endl
         << "                                HFOV|HFOVxVFOV: set to given fov" << endl
         << "    -s, --straighten       Straightens the panorama" << endl
         << "    -c, --center           Centers the panorama" << endl
         << "    --canvas=AUTO|num%|WIDTHxHEIGHT  Sets the output canvas size" << endl
         << "                                AUTO: calculate optimal canvas size" << endl
         << "                                num%: scales the optimal size by given percent" << endl
         << "                                WIDTHxHEIGHT: set to given size" << endl
         << "    --crop=AUTO|AUTOHDR|left,right,top,bottom  Sets the crop rectangle" << endl
         << "                                AUTO: autocrop panorama" << endl
         << "                                AUTOHDR: autocrop HDR panorama" << endl
         << "                                left,right,top,bottom: to given size" << endl
         << "    --output-exposure=AUTO|num  Sets the output exposure value to mean" << endl
         << "                                exposure (AUTO) or to given value" << endl
         << "    --output-type=str       Sets the type of output" << endl
         << "                              Valid items are" << endl
         << "                                NORMAL|N: normal panorama" << endl
         << "                                STACKSFUSEDBLENDED|BF: LDR panorama with" << endl
         << "                                    blended stacks" << endl
         << "                                EXPOSURELAYERSFUSED|FB: LDR panorama with" << endl
         << "                                    fused exposure layers (any arrangement)" << endl
         << "                                HDR: HDR panorama" << endl
         << "                                REMAP: remapped images with corrected exposure" << endl
         << "                                REMAPORIG: remapped images with" << endl
         << "                                    uncorrected exposure" << endl
         << "                                HDRREMAP: remapped images in linear color space" << endl
         << "                                FUSEDSTACKS: exposure fused stacks" << endl
         << "                                HDRSTACKS: HDR stacks" << endl
         << "                                EXPOSURELAYERS: blended exposure layers" << endl
         << "                              and separated by a comma." << endl
         << "    --ldr-file=JPG|TIF|PNG  Sets the filetype for LDR panorama output" << endl
         << "    --ldr-compression=str   Sets the compression for LDR panorama output" << endl
         << "                              For TIF: NONE|PACKBITS|LZW|DEFLATE" << endl
         << "                              For JPEG: quality as number" << endl
         << "    --hdr-file=EXR|TIF      Sets the filetype for HDR panorama output" << endl
         << "    --hdr-compression=str   Sets the compression for HDR panorama output" << endl
         << "                              For TIF: NONE|PACKBITS|LZW|DEFLATE" << endl
         << "    --rotate=yaw,pitch,roll Rotates the whole panorama with the given angles" << endl
         << "    --translate=x,y,z       Translate the whole panorama with the given values" << endl
         << "    -h, --help             Shows this help" << endl
         << endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:p:sch";

    enum
    {
        SWITCH_FOV=1000,
        SWITCH_CANVAS,
        SWITCH_CROP,
        SWITCH_ROTATE,
        SWITCH_TRANSLATE,
        SWITCH_EXPOSURE,
        SWITCH_OUTPUT_TYPE,
        SWITCH_LDRFILETYPE,
        SWITCH_LDRCOMPRESSION,
        SWITCH_HDRFILETYPE,
        SWITCH_HDRCOMPRESSION
    };
    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"projection", required_argument, NULL, 'p' },
        {"fov", required_argument, NULL, SWITCH_FOV },
        {"straighten", no_argument, NULL, 's' },
        {"center", no_argument, NULL, 'c' },
        {"canvas", required_argument, NULL, SWITCH_CANVAS },
        {"crop", required_argument, NULL, SWITCH_CROP },
        {"output-exposure", required_argument, NULL, SWITCH_EXPOSURE },
        {"output-type", required_argument, NULL, SWITCH_OUTPUT_TYPE },
        {"ldr-file", required_argument,NULL, SWITCH_LDRFILETYPE },
        {"ldr-compression", required_argument, NULL, SWITCH_LDRCOMPRESSION },
        {"hdr-file", required_argument, NULL, SWITCH_HDRFILETYPE },
        {"hdr-compression", required_argument, NULL, SWITCH_HDRCOMPRESSION },
        {"rotate", required_argument, NULL, SWITCH_ROTATE },
        {"translate", required_argument, NULL, SWITCH_TRANSLATE },
        {"help", no_argument, NULL, 'h' },
        0
    };

    int projection=-1;
    double newHFOV=-1;
    double newVFOV=-1;
    int scale=100;
    int newWidth=-1;
    int newHeight=-1;
    vigra::Rect2D newROI(0,0,0,0);
    bool doFit=false;
    bool doStraighten=false;
    bool doCenter=false;
    bool doOptimalSize=false;
    bool doAutocrop=false;
    bool autocropHDR=false;
    int c;
    int optionIndex = 0;
    double yaw = 0;
    double pitch = 0;
    double roll = 0;
    double x = 0;
    double y = 0;
    double z = 0;
    double outputExposure = -1000;
    bool calcMeanExposure = false;
    std::string outputType;
    std::string ldrfiletype;
    std::string ldrcompression;
    std::string hdrfiletype;
    std::string hdrcompression;
    string output;
    string param;
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
                //projection
                projection=atoi(optarg);
                if((projection==0) && (strcmp(optarg,"0")!=0))
                {
                    cerr << "Could not parse projection number.";
                    return 1;
                };
                if(projection>=panoProjectionFormatCount())
                {
                    cerr << "projection " << projection << " is an invalid projection number.";
                    return 1;
                };
                break;
            case SWITCH_FOV:
                //field of view
                param=optarg;
                param=hugin_utils::toupper(param);
                if(param=="AUTO")
                {
                    doFit=true;
                }
                else
                {
                    int hfov, vfov;
                    int n=sscanf(optarg, "%dx%d", &hfov, &vfov);
                    if(n==1)
                    {
                        if(hfov>0)
                        {
                            newHFOV=hfov;
                        }
                        else
                        {
                            cerr << "Invalid field of view" << endl;
                            return 1;
                        };
                    }
                    else
                    {
                        if (n==2)
                        {
                            if(hfov>0 && vfov>0)
                            {
                                newHFOV=hfov;
                                newVFOV=vfov;
                            }
                            else
                            {
                                cerr << "Invalid field of view" << endl;
                                return 1;
                            };
                        }
                        else
                        {
                            cerr << "Could not parse field of view" << endl;
                            return 1;
                        };
                    };
                };
                break;
            case 's':
                doStraighten=true;
                break;
            case 'c':
                doCenter=true;
                break;
            case SWITCH_CANVAS:
                //canvas size
                param=optarg;
                param=hugin_utils::toupper(param);
                if(param=="AUTO")
                {
                    doOptimalSize=true;
                }
                else
                {
                    int pos=param.find("%");
                    if(pos!=string::npos)
                    {
                        param=param.substr(0,pos);
                        scale=atoi(param.c_str());
                        if(scale==0)
                        {
                            cerr << "No valid scale factor given." << endl;
                            return 1;
                        };
                        doOptimalSize=true;
                    }
                    else
                    {
                        int width, height;
                        int n=sscanf(optarg, "%dx%d", &width, &height);
                        if (n==2)
                        {
                            if(width>0 && height>0)
                            {
                                newWidth=width;
                                newHeight=height;
                            }
                            else
                            {
                                cerr << "Invalid canvas size" << endl;
                                return 1;
                            };
                        }
                        else
                        {
                            cerr << "Could not parse canvas size" << endl;
                            return 1;
                        };
                    };
                };
                break;
            case SWITCH_CROP:
                //crop
                param=optarg;
                param=hugin_utils::toupper(param);
                if(param=="AUTO" || param=="AUTOHDR")
                {
                    doAutocrop=true;
                    if(param=="AUTOHDR")
                    {
                        autocropHDR=true;
                    };
                }
                else
                {
                    int left, right, top, bottom;
                    int n=sscanf(optarg, "%d,%d,%d,%d", &left, &right, &top, &bottom);
                    if (n==4)
                    {
                        if(right>left && bottom>top && left>=0 && top>=0)
                        {
                            newROI.setUpperLeft(vigra::Point2D(left,top));
                            newROI.setLowerRight(vigra::Point2D(right,bottom));
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
            case SWITCH_EXPOSURE:
                param = optarg;
                param = hugin_utils::toupper(param);
                if (param == "AUTO")
                {
                    calcMeanExposure = true;
                }
                else
                {
                    int n = sscanf(optarg, "%lf", &outputExposure);
                    if (n != 1)
                    {
                        cerr << "Could not parse output exposure value.";
                        return 1;
                    };
                };
                break;
            case SWITCH_OUTPUT_TYPE:
                if (!outputType.empty())
                {
                    outputType.append(",");
                };
                outputType.append(optarg);
                break;
            case SWITCH_LDRFILETYPE:
                ldrfiletype = hugin_utils::tolower(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_LDRCOMPRESSION:
                ldrcompression = hugin_utils::toupper(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_HDRFILETYPE:
                hdrfiletype = hugin_utils::tolower(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_HDRCOMPRESSION:
                hdrcompression = hugin_utils::toupper(hugin_utils::StrTrim(optarg));
                break;
            case SWITCH_ROTATE:
                {
                    int n=sscanf(optarg, "%lf,%lf,%lf", &yaw, &pitch, &roll);
                    if(n!=3)
                    {
                        cerr << "Could not parse rotate angles values. Given: \"" << optarg << "\"" << endl;
                        return 1;
                    };
                };
                break;
            case SWITCH_TRANSLATE:
                {
                    int n=sscanf(optarg, "%lf,%lf,%lf", &x, &y, &z);
                    if(n!=3)
                    {
                        cerr << "Could not parse translation values. Given: \"" << optarg << "\"" << endl;
                        return 1;
                    };
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
        cout << "Warning: pano_modify can only work on one project file at one time" << endl << endl;
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return 1;
    };

    // set some options which depends on each other
    if(doStraighten)
    {
        doCenter=false;
        doFit=true;
    };
    if(doCenter)
    {
        doFit=true;
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

    // sets the projection
    if(projection!=-1)
    {
        PanoramaOptions opt=pano.getOptions();
        opt.setProjection((PanoramaOptions::ProjectionFormat)projection);
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(projection, &proj))
        {
            cout << "Setting projection to " << proj.name << endl;
        }
        pano.setOptions(opt);
    };
    // output exposure value
    if (outputExposure > -1000 || calcMeanExposure)
    {
        PanoramaOptions opt = pano.getOptions();
        if (calcMeanExposure)
        {
            opt.outputExposureValue = HuginBase::CalculateMeanExposure::calcMeanExposure(pano);
        }
        else
        {
            opt.outputExposureValue = outputExposure;
        };
        std::cout << "Setting output exposure value to " << opt.outputExposureValue << std::endl;
        pano.setOptions(opt);
    };
    // output type: normal, fused, hdr pano..
    if (!outputType.empty())
    {
        PanoramaOptions opt = pano.getOptions();
        // reset all output
        // final pano
        opt.outputLDRBlended = false;
        opt.outputLDRExposureBlended = false;
        opt.outputLDRExposureLayersFused = false;
        opt.outputHDRBlended = false;
        // remapped images
        opt.outputLDRLayers = false;
        opt.outputLDRExposureRemapped = false;
        opt.outputHDRLayers = false;
        // stacks
        opt.outputLDRStacks = false;
        opt.outputHDRStacks = false;
        // exposure layers
        opt.outputLDRExposureLayers = false;
        // now parse string and set corresponding options
        std::vector<std::string> tokens = hugin_utils::SplitString(outputType, ",");
        size_t counter = 0;
        for (size_t i = 0; i < tokens.size(); i++)
        {
            std::string s = hugin_utils::toupper(hugin_utils::StrTrim(tokens[i]));
            if (s == "NORMAL" || s=="N")
            {
                opt.outputLDRBlended = true;
                std::cout << "Activate output of normal panorma." << std::endl;
                counter++;
                continue;
            };
            if (s == "STACKSFUSEDBLENDED" || s == "FB")
            {
                opt.outputLDRExposureBlended = true;
                std::cout << "Activate output of ldr panorma: Exposure fused from stacks." << std::endl;
                counter++;
                continue;
            };
            if (s == "EXPOSURELAYERSFUSED" || s == "BF")
            {
                opt.outputLDRExposureLayersFused = true;
                std::cout << "Activate output of ldr panorma: Exposure fused from any arrangement." << std::endl;
                counter++;
                continue;
            };
            if (s == "HDR")
            {
                opt.outputHDRBlended = true;
                std::cout << "Activate output of hdr panorma." << std::endl;
                counter++;
                continue;
            };
            // single remapped images
            if (s == "REMAP")
            {
                opt.outputLDRLayers = true;
                std::cout << "Activate output of remapped, exposure corrected images." << std::endl;
                counter++;
                continue;
            };
            if (s == "REMAPORIG")
            {
                opt.outputLDRExposureRemapped = true;
                std::cout << "Activate output of remapped images with unmodified exposure." << std::endl;
                counter++;
                continue;
            };
            if (s == "HDRREMAP")
            {
                opt.outputHDRLayers = true;
                std::cout << "Activate output of remapped hdr images." << std::endl;
                counter++;
                continue;
            };
            //stacks
            if (s == "FUSEDSTACKS")
            {
                opt.outputLDRStacks = true;
                std::cout << "Activate output of exposure fused stacks." << std::endl;
                counter++;
                continue;
            };
            if (s == "HDRSTACKS")
            {
                opt.outputHDRStacks = true;
                std::cout << "Activate output of hdr stacks." << std::endl;
                counter++;
                continue;
            };
            //exposure layers
            if (s == "EXPOSURELAYERS")
            {
                opt.outputLDRExposureLayers = true;
                std::cout << "Activate output of exposure layers." << std::endl;
                counter++;
                continue;
            };
            std::cout << "Unknown parameter \"" << s << "\" found in --output-type." << std::endl
                << "Ignoring this parameter." << std::endl;
        }
        if (counter > 0)
        {
            pano.setOptions(opt);
        }
        else
        {
            std::cout << "No matching output type given. The whole output-type is ignored." << std::endl;
        };
    };
    // ldr output file type
    if (!ldrfiletype.empty())
    {
        PanoramaOptions opt = pano.getOptions();
        if (ldrfiletype == "jpg" || ldrfiletype == "png" || ldrfiletype == "tif")
        {
            opt.outputImageType = ldrfiletype;
            std::cout << "Setting ldr output to filetype \"" << ldrfiletype << "\"." << std::endl;
            pano.setOptions(opt);
        }
        else
        {
            std::cout << "LDR file format \"" << ldrfiletype << "\" is not a valid LDR output filetype." << std::endl
                << "Ignoring parameter." << std::endl;
        };
    };
    // ldr compression
    if (!ldrcompression.empty())
    {
        PanoramaOptions opt = pano.getOptions();
        if (opt.outputImageType == "tif")
        {
            if (ldrcompression == "NONE" || ldrcompression == "PACKBITS" || ldrcompression == "LZW" || ldrcompression == "DEFLATE")
            {
                opt.outputImageTypeCompression = ldrcompression;
                std::cout << "Setting TIF compression to \"" << ldrcompression << "\"." << std::endl;
                opt.tiffCompression = ldrcompression;
            }
            else
            {
                std::cout << "LDR compression \"" << ldrcompression << "\" is not a valid compression value for TIF files." << std::endl
                    << "Ignoring compression." << std::endl;
            }
        }
        else
        {
            if (opt.outputImageType == "jpg")
            {
                int quality = 0;
                quality = atoi(ldrcompression.c_str());
                if (quality != 0)
                {
                    if (quality>0 && quality <=100)
                    { 
                        opt.quality = quality;
                        std::cout << "Setting JPEG quality to " << quality << "." << std::endl;
                    }
                    else
                    {
                        std::cout << "Given value for JPEG quality is outside the valid range 1..100." << std::endl
                            << "Ignoring value." << std::endl;
                    };
                }
                else
                {
                    std::cout << "Could not parse \"" << ldrcompression << "\" as a valid JPEG quality number." << std::endl
                        << "Ignoring value." << std::endl;
                };
            }
            else
            {
                if (opt.outputImageType == "png")
                {
                    std::cout << "Setting compression for PNG images is not supported." << std::endl;
                }
                else
                {
                    // this should never happen
                    std::cout << "Unknown image format" << std::endl;
                };
            };
        };
        pano.setOptions(opt);
    };
    // hdr output file type
    if (!hdrfiletype.empty())
    {
        PanoramaOptions opt = pano.getOptions();
        if (hdrfiletype == "exr" || hdrfiletype == "tif")
        {
            opt.outputImageTypeHDR = hdrfiletype;
            std::cout << "Setting hdr output to filetype \"" << ldrfiletype << "\"." << std::endl;
            pano.setOptions(opt);
        }
        else
        {
            std::cout << "HDR file format \"" << ldrfiletype << "\" is not a valid HDR output filetype." << std::endl
                << "Ignoring parameter." << std::endl;
        };
    };
    // hdr compression
    if (!hdrcompression.empty())
    {
        PanoramaOptions opt = pano.getOptions();
        if (opt.outputImageTypeHDR == "tif")
        {
            if (hdrcompression == "NONE" || hdrcompression == "PACKBITS" || hdrcompression == "LZW" || hdrcompression == "DEFLATE")
            {
                opt.outputImageTypeHDRCompression = hdrcompression;
                std::cout << "Setting HDR-TIF compression to \"" << hdrcompression << "\"." << std::endl;
            }
            else
            {
                std::cout << "HDR compression \"" << ldrcompression << "\" is not a valid compression value for TIF files." << std::endl
                    << "Ignoring compression." << std::endl;
            }
        }
        else
        {
            if (opt.outputImageTypeHDR == "exr")
            {
                std::cout << "Setting compression for EXR images is not supported." << std::endl;
            }
            else
            {
                // this should never happen
                std::cout << "Unknown HDR image format" << std::endl;
            };
        };
        pano.setOptions(opt);
    };
    // rotate complete pano
    if (abs(yaw) + abs(pitch) + abs(roll) > 0.0)
    {
        cout << "Rotate panorama (yaw=" << yaw << ", pitch= " << pitch << ", roll=" << roll << ")" << endl;
        RotatePanorama(pano, yaw, pitch, roll).run();
    };
    // translate complete pano
    if(abs(x) + abs(y) + abs(z) > 0.0)
    {
        cout << "Translate panorama (x=" << x << ", y=" << y << ", z=" << z << ")" << endl;
        TranslatePanorama(pano, x, y, z).run();
    };
    // straighten
    if(doStraighten)
    {
        cout << "Straighten panorama" << endl;
        StraightenPanorama(pano).run();
        CenterHorizontally(pano).run();
    };
    // center
    if(doCenter)
    {
        cout << "Center panorama" << endl;
        CenterHorizontally(pano).run();
    }
    //fit fov
    if(doFit)
    {
        cout << "Fit panorama field of view to best size" << endl;
        PanoramaOptions opt=pano.getOptions();
        CalculateFitPanorama fitPano = CalculateFitPanorama(pano);
        fitPano.run();
        opt.setHFOV(fitPano.getResultHorizontalFOV());
        opt.setHeight(roundi(fitPano.getResultHeight()));
        cout << "Setting field of view to " << opt.getHFOV() << " x " << opt.getVFOV() << endl;
        pano.setOptions(opt);
    };
    //set field of view manually
    if(newHFOV>0)
    {
        PanoramaOptions opt=pano.getOptions();
        opt.setHFOV(newHFOV);
        if(opt.fovCalcSupported(opt.getProjection()) && newVFOV>0)
        {
            opt.setVFOV(newVFOV);
        }
        cout << "Setting field of view to " << opt.getHFOV() << " x " << opt.getVFOV() << endl;
        pano.setOptions(opt);
    };
    // calc optimal size
    if(doOptimalSize)
    {
        cout << "Calculate optimal size of panorama" << endl;
        double s = CalculateOptimalScale::calcOptimalScale(pano);
        PanoramaOptions opt=pano.getOptions();
        opt.setWidth(roundi(opt.getWidth()*s*scale/100), true);
        cout << "Setting canvas size to " << opt.getWidth() << " x " << opt.getHeight() << endl;
        pano.setOptions(opt);
    };
    // set canvas size
    if(newWidth>0 && newHeight>0)
    {
        PanoramaOptions opt=pano.getOptions();
        opt.setWidth(newWidth);
        opt.setHeight(newHeight);
        cout << "Setting canvas size to " << opt.getWidth() << " x " << opt.getHeight() << endl;
        pano.setOptions(opt);
    };
    // auto crop
    if(doAutocrop)
    {
        cout << "Searching for best crop rectangle" << endl;
        CalculateOptimalROI cropPano(pano);
        if(autocropHDR)
        {
            cropPano.setStacks(getHDRStacks(pano,pano.getActiveImages(), pano.getOptions()));
        }
        cropPano.run();

        vigra::Rect2D roi=cropPano.getResultOptimalROI();
        PanoramaOptions opt = pano.getOptions();
        //set the ROI - fail if the right/bottom is zero, meaning all zero
        if(roi.right() != 0 && roi.bottom() != 0)
        {
            opt.setROI(roi);
            cout << "Set crop size to " << roi.left() << "," << roi.top() << "," << roi.right() << "," << roi.bottom() << endl;
            pano.setOptions(opt);
        }
        else
        {
            cout << "Could not find best crop rectangle" << endl;
        }
    };
    //setting crop rectangle manually
    if(newROI.right() != 0 && newROI.bottom() != 0)
    {
        PanoramaOptions opt = pano.getOptions();
        opt.setROI(newROI);
        cout << "Set crop size to " << newROI.left() << "," << newROI.right() << "," << newROI.top() << "," << newROI.bottom() << endl;
        pano.setOptions(opt);
    };

    //write output
    OptimizeVector optvec = pano.getOptimizeVector();
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_mod.pto");
    }
    ofstream of(output.c_str());
    pano.printPanoramaScript(of, optvec, pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    cout << endl << "Written output to " << output << endl;
    return 0;
}
