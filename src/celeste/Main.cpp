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
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <fstream>
#include <sstream>
#include <string>
#include "Celeste.h"
#include <sys/stat.h> 
#include "hugin_config.h" 
#include <vigra_ext/impexalpha.hxx>
#include <vigra_ext/cms.h>
#include <panodata/Panorama.h>
#include <hugin_utils/utils.h>

#ifdef _WIN32
#include <getopt.h>
#else
 #include <unistd.h>
#endif

static void usage(){

	// Print usage and exit
	std::cout << std::endl << "Celeste: Removes cloud-like control points from Hugin project files and creates image masks" << std::endl;
	std::cout << "using Support Vector Machines." << std::endl;
    std::cout << std::endl << "Version " << hugin_utils::GetHuginVersion() << std::endl;
	std::cout << std::endl << "Usage: celeste_standalone [options] image1 image2 [..]" << std::endl << std::endl;
	std::cout << "Options:" << std::endl << std::endl;
	std::cout << "  -i <filename>   Input Hugin PTO file. Control points over SVM threshold will" << std::endl;
	std::cout << "                  be removed before being written to the output file. If -m is" << std::endl;
	std::cout << "                  set to 1, images in the file will be also be masked." << std::endl;
	std::cout << "  -o <filename>   Output Hugin PTO file. Default: '<filename>_celeste.pto'" << std::endl;
	std::cout << "  -d <filename>   SVM model file. Default: 'data/celeste.model'" << std::endl;
	std::cout << "  -s <int>        Maximum dimension for re-sized image prior to processing. A" << std::endl;
	std::cout << "                  higher value will increase the resolution of the mask but is" << std::endl;
	std::cout << "                  significantly slower. Default: 800" << std::endl;
	std::cout << "  -t <float>      SVM threshold. Raise this value to remove fewer control points," << std::endl;
	std::cout << "                  lower it to remove more. Range 0 to 1. Default: 0.5" << std::endl;
	std::cout << "  -m <1|0>        Create masks when processing Hugin PTO file. Default: 0" << std::endl;
	std::cout << "  -f <std::string>     Mask file format. Options are PNG, JPEG, BMP, GIF and TIFF." << std::endl;
	std::cout << "                  Default: PNG" << std::endl;
	std::cout << "  -r <1|0>        Filter radius. 0 = large (more accurate), 1 = small (higher" << std::endl;
	std::cout << "                  resolution mask, slower, less accurate). Default: 0" << std::endl;
	std::cout << "  -h              Print usage." << std::endl; 
	std::cout << "  image1 image2.. Image files to be masked." << std::endl << std::endl;
	exit(1);

}

vigra::UInt16RGBImage loadAndConvertImage(std::string imagefile)
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
    // convert to sRGB colorspace if images contains icc profile
    if (!info.getICCProfile().empty())
    {
        HuginBase::Color::ApplyICCProfile(image, info.getICCProfile(), TYPE_RGB_16);
    }
    return image;
};

std::string generateMaskName(std::string imagefile,std::string mask_format)
{
    std::string mask_name = ("");
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

    int mask = 0;
    double threshold = 0.5;
    std::vector<std::string> images_to_mask;
    std::string pto_file = (""),output_pto = ("");
    std::string mask_format = ("PNG");
    std::string model_file = ("celeste.model");
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
                    std::cerr << "Invalid parameter: threshold (-t) should be between 0 and 1" << std::endl;
                    return 1;
                };
                break;
            case 'm': 
                mask = atoi(optarg);
                if(mask<0 || mask>1)
                {
                    std::cerr << "Invalid parameter: mask parameter (-m) can only be 0 or 1" << std::endl;
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
                    std::cerr << "Invalid parameter: maximum dimension (-s) should be bigger than 100" << std::endl;
                    return 1;
                };
                break;
            case ':':
                std::cerr <<"Missing parameter for parameter " << argv[optind] << std::endl;
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
        std::cout << "No project file or image files given."<< std::endl;
        return 1;
    };

    
	// Check model file
    if (!hugin_utils::FileExists(model_file))
    {
        std::string install_path_model=hugin_utils::GetDataDir();
		install_path_model.append(model_file);
		
		if (!hugin_utils::FileExists(install_path_model)){
		
    			std::cout << std::endl << "Couldn't open SVM model file " << model_file << std::endl;
			std::cout << "Also tried " << install_path_model << std::endl << std::endl; 
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
    mask_format=hugin_utils::toupper(mask_format);
	if (mask_format == ("JPG")){
		mask_format = ("JPEG");
	}
	if (mask_format != ("PNG") &&mask_format != ("BMP") && mask_format != ("GIF") && mask_format !=	("JPEG") && mask_format != ("TIFF")){
		mask_format = ("TIFF");
	}

	// Print some stuff out
	std::cout << std::endl << "Celeste: Removes cloud-like control points from Hugin project files and creates image masks" << std::endl;
	std::cout << "using Support Vector Machines." << std::endl;
    std::cout << std::endl << "Version " << hugin_utils::GetHuginVersion() << std::endl << std::endl;
	std::cout << "Arguments:" << std::endl;
	std::cout << "Input Hugin file:\t" << pto_file << std::endl;
	std::cout << "Output Hugin file:\t" << output_pto << std::endl;
	std::cout << "SVM model file:\t\t" << model_file << std::endl;
	std::cout << "Max dimension:\t\t" << resize_dimension << std::endl;
	std::cout << "SVM threshold:\t\t" << threshold << std::endl;
	std::cout << "Create PTO masks:\t";
	if (mask){
		std::cout << "Yes" << std::endl;
	}else{
		std::cout << "No" << std::endl;
	} 
	std::cout << "Mask format:\t\t" << mask_format << std::endl;
	std::cout << "Filter radius:\t\t";

	// Mask resolution
    int radius;
	if (course_fine)
    {
        radius = 10;
        std::cout << "Small" << std::endl << std::endl;
	}
    else
    {
        radius=20;
        std::cout << "Large" << std::endl << std::endl;
	} 
	
	// Convert mask format to lower case
    mask_format=hugin_utils::tolower(mask_format);

    struct celeste::svm_model* model;
    if(!celeste::loadSVMmodel(model,model_file))
    {
        return 1;
    };

    // Mask images
	if (images_to_mask.size())
    {
        std::cout << "Masking images..." << std::endl << std::endl;
        for (unsigned int l = 0; l < images_to_mask.size(); l++)
        {
            std::string imagefile=images_to_mask[l];
            try
            {
                std::cout << "Opening image file:\t" << imagefile << std::endl;
                // Read image given and convert to UInt16
                vigra::UInt16RGBImage in=loadAndConvertImage(imagefile);
                if(in.width()==0 || in.height()==0)
                {
                    continue;
                };

                // Create mask file name
                std::string mask_name = generateMaskName(imagefile,mask_format);

                std::cout << "Generating mask:\t" << mask_name << std::endl;				
                // Create mask
                vigra::BImage* mask=celeste::getCelesteMask(model,in,radius,threshold,resize_dimension);
                exportImage(srcImageRange(*mask), vigra::ImageExportInfo(mask_name.c_str()).setPixelType("UINT8"));
                delete mask;
            }
            catch (vigra::StdException & e)
            {
                // catch any errors that might have occurred and print their reason
                std::cout << "Unable to open file:\t" << imagefile << std::endl << std::endl;
                std::cout << e.what() << std::endl << std::endl;
    		};
        };
	};

    // Process PTO file
    if (pto_file != (""))
    {
  		std::cout << "Parsing Hugin project file " << pto_file << std::endl << std::endl;

        HuginBase::Panorama pano;
        std::ifstream prjfile(pto_file.c_str());
        if (!prjfile.good())
        {
            std::cerr << "could not open script : " << pto_file << std::endl;
            celeste::destroySVMmodel(model);
            return 1;
        }
        pano.setFilePrefix(hugin_utils::getPathPrefix(pto_file));
        AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
        if (err != AppBase::DocumentData::SUCCESSFUL)
        {
            std::cerr << "error while parsing panos tool script: " << pto_file << std::endl;
            std::cerr << "DocumentData::ReadWriteError code: " << err << std::endl;
            celeste::destroySVMmodel(model);
            return 1;
        }

        for(unsigned int i=0;i<pano.getNrOfImages();i++)
        {
            HuginBase::CPointVector cps = pano.getCtrlPointsVectorForImage(i);
            if(cps.size()>0 || mask)
            {
                try
                {
                    std::string imagefile=pano.getImage(i).getFilename();
                    vigra::UInt16RGBImage in=loadAndConvertImage(imagefile);
                    if(in.width()==0 || in.height()==0)
                    {
                        continue;
                    };
                    if(cps.size()>0)
                    {
                        HuginBase::UIntSet cloudCP = celeste::getCelesteControlPoints(model, in, cps, radius, threshold, resize_dimension);
                        if(cloudCP.size()>0)
                        {
                            for (HuginBase::UIntSet::reverse_iterator it = cloudCP.rbegin(); it != cloudCP.rend(); ++it)
                            {
                                pano.removeCtrlPoint(*it);
                            };
                        };
                    };
                    if(mask)
                    {
                        std::string mask_name = generateMaskName(imagefile,mask_format);
                        // Create mask
                        vigra::BImage* mask=celeste::getCelesteMask(model,in,radius,threshold,resize_dimension);
                        exportImage(srcImageRange(*mask), vigra::ImageExportInfo(mask_name.c_str()).setPixelType("UINT8"));
                        delete mask;
                    };
                }
                catch (vigra::StdException & e)
                {
                    // catch any errors that might have occurred and print their reason
                    std::cout << "Unable to open file:\t" << pano.getImage(i).getFilename() << std::endl << std::endl;
                    std::cout << e.what() << std::endl << std::endl;
    		    };
            };
        };

		// write new pto file
        std::ofstream of(output_pto.c_str());
        HuginBase::UIntSet imgs;
        fill_set(imgs,0, pano.getNrOfImages()-1);
        pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(pto_file));
    
        std::cout << std::endl << "Written file " << output_pto << std::endl << std::endl;
	}
    celeste::destroySVMmodel(model);
	return(0);
	
}	
