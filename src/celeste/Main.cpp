/***************************************************************************
 *   Copyright (C) 2008 by Tim Nugent                                      *
 *   timnugent@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <fstream>
#include <sstream>
#include <string>
#include "Celeste.h"
#include <sys/stat.h> 
#include "hugin_config.h" 
#include <hugin_version.h>
#include <vigra_ext/impexalpha.hxx>
#include <panodata/Panorama.h>

#ifdef _WINDOWS
#include <windows.h>
#include <getopt.h>
#else
 #include <unistd.h>
#endif

#ifdef __APPLE__
#include <hugin_config.h>
#include <mach-o/dyld.h>	/* _NSGetExecutablePath */
#include <limits.h>		/* PATH_MAX */
#include <libgen.h>		/* dirname */
#endif

using namespace std;
using namespace HuginBase;
using namespace AppBase;

bool fileexists(string strFilename) {

	struct stat stFileInfo;
	bool blnReturn;
	int intStat;

	// Attempt to get the file attributes
	intStat = stat(strFilename.c_str(),&stFileInfo);
	if(intStat == 0) {

		blnReturn = true;
  	}else{

    		blnReturn = false;
  	}
  
  	return(blnReturn);
}

static void usage(){

	// Print usage and exit
	cout << endl << "Celeste: Removes cloud-like control points from Hugin project files and creates image masks" << endl;
	cout << "using Support Vector Machines." << endl;
	cout << endl << "Version " << DISPLAY_VERSION << endl;
	cout << endl << "Usage: celeste_standalone [options] image1 image2 [..]" << endl << endl;
	cout << "Options:" << endl << endl;
	cout << "  -i <filename>   Input Hugin PTO file. Control points over SVM threshold will" << endl;
	cout << "                  be removed before being written to the output file. If -m is" << endl;
	cout << "                  set to 1, images in the file will be also be masked." << endl;
	cout << "  -o <filename>   Output Hugin PTO file. Default: '<filename>_celeste.pto'" << endl;
	cout << "  -d <filename>   SVM model file. Default: 'data/celeste.model'" << endl;
	cout << "  -s <int>        Maximum dimension for re-sized image prior to processing. A" << endl;
	cout << "                  higher value will increase the resolution of the mask but is" << endl;
	cout << "                  significantly slower. Default: 800" << endl;
	cout << "  -t <float>      SVM threshold. Raise this value to remove fewer control points," << endl;
	cout << "                  lower it to remove more. Range 0 to 1. Default: 0.5" << endl;
	cout << "  -m <1|0>        Create masks when processing Hugin PTO file. Default: 0" << endl;
	cout << "  -f <string>     Mask file format. Options are PNG, JPEG, BMP, GIF and TIFF." << endl;
	cout << "                  Default: PNG" << endl;
	cout << "  -r <1|0>        Filter radius. 0 = large (more accurate), 1 = small (higher" << endl;
	cout << "                  resolution mask, slower, less accurate). Default: 0" << endl;
	cout << "  -h              Print usage." << endl; 
	cout << "  image1 image2.. Image files to be masked." << endl << endl;
	exit(1);

}

vigra::UInt16RGBImage loadAndConvertImage(string imagefile)
{
    vigra::ImageImportInfo info(imagefile.c_str());
    std::string pixelType=info.getPixelType();
    vigra::UInt16RGBImage image;
    if(!info.isColor())
    {
        std::cerr << "Celeste works only on colour images, " << std::endl 
            << "but image " << imagefile << " has  " << info.numBands() << " channels." << std::endl 
            << "Skipping this image." << std::endl;
        return image;
    };
    if(pixelType=="UINT8" || pixelType=="INT16")
    {
        vigra::UInt16RGBImage imageIn(info.width(),info.height());
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
        celeste::convertToUInt16(imageIn,pixelType,image);
        imageIn.resize(0,0);
    }
    else
        if(pixelType=="UINT16")
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
            if(pixelType=="INT32" || pixelType=="UINT32")
            {
                vigra::UInt32RGBImage imageIn(info.width(),info.height());
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
                celeste::convertToUInt16(imageIn,pixelType,image);
                imageIn.resize(0,0);
            }
            else
                if(pixelType=="FLOAT" || pixelType=="DOUBLE")
                {
                    vigra::FRGBImage imagefloat(info.width(),info.height());
                    if(info.numExtraBands()==1)
                    {
                        vigra::BImage mask(info.size());
                        vigra::importImageAlpha(info,destImage(imagefloat),destImage(mask));
                        mask.resize(0,0);
                    }
                    else
                    {
                        importImage(info,destImage(imagefloat));
                    };
                    celeste::convertToUInt16(imagefloat,pixelType,image);
                    imagefloat.resize(0,0);
                }
                else
                {
                    std::cerr << "Unsupported pixel type" << std::endl;
                };
    return image;
};

std::string generateMaskName(string imagefile,string mask_format)
{
    string mask_name = ("");
    if (imagefile.substr(imagefile.length()-4,1)==("."))
    {
        mask_name.append(imagefile.substr(0,imagefile.length()-4));
    }
    else
    {
        mask_name.append(imagefile.substr(0,imagefile.length()-4));
    }
    mask_name.append("_mask.");
    mask_name.append(mask_format);
    return mask_name;
};


int main(int argc, char* argv[])
{

    // Exit with usage unless filename given as argument
    if (argc < 2)
    {
            usage();
    }

    unsigned int i = 1, mask = 0;
    double threshold = 0.5;
    vector<string> images_to_mask;
    string pto_file = (""),output_pto = ("");
    string mask_format = ("PNG");
    string model_file = ("celeste.model");
    int course_fine = 0;
    int resize_dimension=800;

    // Deal with arguments
    // parse arguments
    int c;
    const char * optstring = "i:o:d:s:t:m:f:r:h";

    while ((c = getopt (argc, argv, optstring)) != -1)
    {
        switch(c)
        {
            case 'h': 
                usage();
                break;
            case 'i':
                pto_file=optarg;
                break;
            case 'o':
                output_pto=optarg;
                break;
            case 't':
                threshold = atof(optarg);
                if(threshold<=0 || threshold>1)
                {
                    cerr << "Invalid parameter: threshold (-t) should be between 0 and 1" << std::endl;
                    return 1;
                };
                break;
            case 'm': 
                mask = atoi(optarg);
                if(mask<0 || mask>1)
                {
                    cerr << "Invalid parameter: mask parameter (-m) can only be 0 or 1" << std::endl;
                    return 1;
                };
                break;
            case 'f': 
                mask_format = optarg; 
                break;
            case 'd':
                model_file = optarg;
                break;
            case 'r': 
                course_fine = atoi(optarg);
                break;
            case 's': 
                resize_dimension = atoi(optarg);
                if(resize_dimension<100)
                {
                    cerr << "Invalid parameter: maximum dimension (-s) should be bigger than 100" << std::endl;
                    return 1;
                };
                break;
            case ':':
                cerr <<"Missing parameter for parameter " << argv[optind] << endl;
                return 1;
                break;
            case '?': /* invalid parameter */
                return 1;
                break;
            default: /* unknown */
                usage();
        };
    };

    while(optind<argc)
    {
        images_to_mask.push_back(argv[optind]);
        optind++;
    };
    
    if(images_to_mask.size()==0 && pto_file.empty())
    {
        cout << "No project file or image files given."<< endl;
        return 1;
    };

    
	// Check model file
	if (!fileexists(model_file)){
	
#if _WINDOWS
        char buffer[MAX_PATH];//always use MAX_PATH for filepaths
        GetModuleFileName(NULL,buffer,sizeof(buffer));
        string working_path=(buffer);
        string install_path_model="";
        //remove filename
        std::string::size_type pos=working_path.rfind("\\");
        if(pos!=std::string::npos)
        {
            working_path.erase(pos);
            //remove last dir: should be bin
            pos=working_path.rfind("\\");
            if(pos!=std::string::npos)
            {
                working_path.erase(pos);
                //append path delimiter and path
                working_path.append("\\share\\hugin\\data\\");
                install_path_model=working_path;
            }
        }
#elif defined MAC_SELF_CONTAINED_BUNDLE
		char path[PATH_MAX + 1];
		uint32_t size = sizeof(path);
		string install_path_model("");
		if (_NSGetExecutablePath(path, &size) == 0)
		{
			install_path_model=dirname(path);
			install_path_model.append("/../Resources/xrc/");
		}
#else
        string install_path_model = (INSTALL_DATA_DIR);
#endif

		install_path_model.append(model_file);
		
		if (!fileexists(install_path_model)){
		
    			cout << endl << "Couldn't open SVM model file " << model_file << endl;
			cout << "Also tried " << install_path_model << endl << endl; 
    			exit(1);

		}else{
		
			model_file = install_path_model;
		
		}
  	}

	// Set output .pto filename if not given
	if (output_pto == ("") && pto_file != ("")){
		output_pto = pto_file.substr(0,pto_file.length()-4).append("_celeste.pto");
	}

	// Convert mask format to upper case
	transform(mask_format.begin(), mask_format.end(), mask_format.begin(),(int(*)(int)) toupper);
	if (mask_format == ("JPG")){
		mask_format = ("JPEG");
	}
	if (mask_format != ("PNG") &&mask_format != ("BMP") && mask_format != ("GIF") && mask_format !=	("JPEG") && mask_format != ("TIFF")){
		mask_format = ("TIFF");
	}

	// Print some stuff out
	cout << endl << "Celeste: Removes cloud-like control points from Hugin project files and creates image masks" << endl;
	cout << "using Support Vector Machines." << endl;
	cout << endl << "Version " << DISPLAY_VERSION << endl << endl;
	cout << "Arguments:" << endl;
	cout << "Input Hugin file:\t" << pto_file << endl;
	cout << "Output Hugin file:\t" << output_pto << endl;
	cout << "SVM model file:\t\t" << model_file << endl;
	cout << "Max dimension:\t\t" << resize_dimension << endl;
	cout << "SVM threshold:\t\t" << threshold << endl;
	cout << "Create PTO masks:\t";
	if (mask){
		cout << "Yes" << endl;
	}else{
		cout << "No" << endl;
	} 
	cout << "Mask format:\t\t" << mask_format << endl;
	cout << "Filter radius:\t\t";

	// Mask resolution
    int radius;
	if (course_fine)
    {
        radius = 10;
        cout << "Small" << endl << endl;
	}
    else
    {
        radius=20;
        cout << "Large" << endl << endl;
	} 
	
	// Convert mask format to lower case
	transform(mask_format.begin(), mask_format.end(), mask_format.begin(),(int(*)(int)) tolower);

	// Vectors to store SVM responses and PTO file info etc
	vector<string> images,pto_file_top,pto_file_cps,pto_file_end;
	vector<double> svm_responses;

    struct celeste::svm_model* model;
    if(!celeste::loadSVMmodel(model,model_file))
    {
        return 1;
    };

    // Mask images
	if (images_to_mask.size())
    {
        cout << "Masking images..." << endl << endl;
        for (unsigned int l = 0; l < images_to_mask.size(); l++)
        {
            std::string imagefile=images_to_mask[l];
            try
            {
                cout << "Opening image file:\t" << imagefile << endl;
                // Read image given and convert to UInt16
                vigra::UInt16RGBImage in=loadAndConvertImage(imagefile);
                if(in.width()==0 || in.height()==0)
                {
                    continue;
                };

                // Create mask file name
                string mask_name = generateMaskName(imagefile,mask_format);

                cout << "Generating mask:\t" << mask_name << endl;				
                // Create mask
                vigra::BImage mask=celeste::getCelesteMask(model,in,radius,threshold,resize_dimension);
                exportImage(srcImageRange(mask), vigra::ImageExportInfo(mask_name.c_str()).setPixelType("UINT8"));
            }
            catch (vigra::StdException & e)
            {
                // catch any errors that might have occured and print their reason
                cout << "Unable to open file:\t" << imagefile << endl << endl;
                cout << e.what() << endl << endl;
    		};
        };
	};

    // Process PTO file
    if (pto_file != (""))
    {
  		cout << "Parsing Hugin project file " << pto_file << endl << endl;

        Panorama pano;
        ifstream prjfile(pto_file.c_str());
        if (!prjfile.good())
        {
            cerr << "could not open script : " << pto_file << endl;
            celeste::destroySVMmodel(model);
            return 1;
        }
        pano.setFilePrefix(hugin_utils::getPathPrefix(pto_file));
        DocumentData::ReadWriteError err = pano.readData(prjfile);
        if (err != DocumentData::SUCCESSFUL)
        {
            cerr << "error while parsing panos tool script: " << pto_file << endl;
            cerr << "DocumentData::ReadWriteError code: " << err << endl;
            celeste::destroySVMmodel(model);
            return 1;
        }

        for(unsigned int i=0;i<pano.getNrOfImages();i++)
        {
            CPointVector cps=pano.getCtrlPointsVectorForImage(i);
            if(cps.size()>0 || mask)
            {
                try
                {
                    string imagefile=pano.getImage(i).getFilename();
                    vigra::UInt16RGBImage in=loadAndConvertImage(imagefile);
                    if(in.width()==0 || in.height()==0)
                    {
                        continue;
                    };
                    if(cps.size()>0)
                    {
                        UIntSet cloudCP=celeste::getCelesteControlPoints(model,in,cps,radius,threshold,resize_dimension);
                        if(cloudCP.size()>0)
                        {
                            for(UIntSet::reverse_iterator it=cloudCP.rbegin();it!=cloudCP.rend();++it)
                            {
                                pano.removeCtrlPoint(*it);
                            };
                        };
                    };
                    if(mask)
                    {
                        string mask_name = generateMaskName(imagefile,mask_format);
                        // Create mask
                        vigra::BImage mask=celeste::getCelesteMask(model,in,radius,threshold,resize_dimension);
                        exportImage(srcImageRange(mask), vigra::ImageExportInfo(mask_name.c_str()).setPixelType("UINT8"));
                    };
                }
                catch (vigra::StdException & e)
                {
                    // catch any errors that might have occured and print their reason
                    cout << "Unable to open file:\t" << pano.getImage(i).getFilename() << endl << endl;
                    cout << e.what() << endl << endl;
    		    };
            };
        };

		// write new pto file
        ofstream of(output_pto.c_str());
        UIntSet imgs;
        fill_set(imgs,0, pano.getNrOfImages()-1);
        pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(pto_file));
    
        cout << endl << "Written file " << output_pto << endl << endl;
	}
    celeste::destroySVMmodel(model);
	return(0);
	
}	
