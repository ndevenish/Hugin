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

#include <hugin_version.h>

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

using namespace std;
using namespace HuginBase;
using namespace AppBase;

static void usage(const char * name)
{
    cout << name << ": change output parameters of project file" << endl
         << "pano_modify version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] input.pto" << endl
         << endl
         << "  Options:" << endl
         << "     -o, --output=file.pto  Output Hugin PTO file. Default: <filename>_mod.pto" << endl
         << "     -p, --projection=x     Sets the output projection to number x" << endl
         << "     --fov=AUTO|HFOV|HFOVxVFOV   Sets field of view" << endl
         << "                                   AUTO: calculates optimal fov" << endl
         << "                                   HFOV|HFOVxVFOV: set to given fov" << endl
         << "     -s, --straighten       Straightens the panorama" << endl
         << "     -c, --center           Centers the panorama" << endl
         << "     --canvas=AUTO|num%|WIDTHxHEIGHT  Sets the output canvas size" << endl
         << "                                   AUTO: calculate optimal canvas size" << endl
         << "                                   num%: scales the optimal size by given percent" << endl
         << "                                   WIDTHxHEIGHT: set to given size" << endl
         << "     --crop=AUTO|AUTOHDR|left,right,top,bottom  Sets the crop rectangle" << endl
         << "                                   AUTO: autocrop panorama" << endl
         << "                                   AUTOHDR: autocrop HDR panorama" << endl
         << "                                   left,right,top,bottom: to given size" << endl
         << "     --rotate=yaw,pitch,roll  Rotates the whole panorama with the given angles" << endl
         << "     --translate=x,y,z        Translate the whole panorama with the given values" << endl
         << "     -h, --help             Shows this help" << endl
         << endl;
}

// toupper is overloaded in <locale>, but we want to use it as a unary function.
inline char toupper_2(char c)
{
    return toupper(c);
}

/** convert string to uppercase letters. */
std::string strToUpper(const std::string& aString)
{
    std::string result(aString);
    std::transform(aString.begin(), aString.end(), result.begin(), toupper_2);
    return result;
};

int main(int argc, char *argv[])
{
    // parse arguments
    const char * optstring = "o:p:sch";

    enum
    {
        SWITCH_FOV=1000,
        SWITCH_CANVAS=1001,
        SWITCH_CROP=1002,
        SWITCH_ROTATE=1003,
        SWITCH_TRANSLATE=1004
    };
    static struct option longOptions[] = {
        {"output", required_argument, NULL, 'o' },
        {"projection", required_argument, NULL, 'p' },
        {"fov", optional_argument, NULL, SWITCH_FOV },
        {"straighten", no_argument, NULL, 's' },
        {"center", no_argument, NULL, 'c' },
        {"canvas", optional_argument, NULL, SWITCH_CANVAS },
        {"crop", optional_argument, NULL, SWITCH_CROP },
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
                usage(argv[0]);
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
                param=strToUpper(param);
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
                param=strToUpper(param);
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
                param=strToUpper(param);
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
        usage(argv[0]);
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
    if (!prjfile.good()) {
        cerr << "could not open script : " << input << endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != DocumentData::SUCCESSFUL) {
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
            cout << "Setting projection to " << proj.name << endl;
        pano.setOptions(opt);
    };
    if(abs(yaw) + abs(pitch) + abs(roll) > 0.0)
    {
        cout << "Rotate panorama (yaw=" << yaw << ", pitch= " << pitch << ", roll=" << roll << ")" << endl;
        RotatePanorama(pano, yaw, pitch, roll).run();
    };
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
            opt.setVFOV(newVFOV);
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
            cout << "Could not find best crop rectangle" << endl;
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
