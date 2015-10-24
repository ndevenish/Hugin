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

#include <iostream>
#include <vigra/stdimage.hxx>
#include <vigra/resizeimage.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/copyimage.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/initimage.hxx>
#include "vigra/colorconversions.hxx"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "Gabor.h"
#include "Utilities.h"
#include "CelesteGlobals.h"
#include "Celeste.h"
#include "svm.h"
#include <stdio.h>

namespace celeste
{

typedef vigra::BRGBImage::PixelType RGB;

// load SVM model
bool loadSVMmodel(struct svm_model*& model, std::string& model_file)
{
    if((model = svm_load_model(model_file.c_str())) == 0)
    {
        std::cout << "Couldn't load model file '" << model_file << "'" << std::endl << std::endl;
        return false;
    }
    else
    {
        std::cout << "Loaded model file:\t" << model_file << std::endl;
        return true;
    }
};

// destroy SVM model
void destroySVMmodel(struct svm_model*& model)
{
    svm_free_and_destroy_model(&model);
};

// prepare image for use with celeste (downscale, converting in Luv)
void prepareCelesteImage(vigra::UInt16RGBImage& rgb,vigra::UInt16RGBImage& luv, int& resize_dimension, double& sizefactor,bool verbose)
{
    // Max dimension
    sizefactor = 1;
    int nw=rgb.width();
    int nh=rgb.height();

    if(nw >= nh)
    {
        if (resize_dimension >= nw )
        {
            resize_dimension = nw;
        }
    }
    else
    {
        if (resize_dimension >= nh)
        {
            resize_dimension = nh;
        }
    }
    //std::cout << "Re-size dimenstion:\t" << resize_dimension << std::endl;
    if(verbose)
        std::cout << "Image dimensions:\t" << rgb.width() << " x " << rgb.height() << std::endl;

    // Re-size to max dimension
    vigra::UInt16RGBImage temp;

    if (rgb.width() > resize_dimension || rgb.height() > resize_dimension)
    {
        if (rgb.width() >= rgb.height())
        {
            sizefactor = (double)resize_dimension/rgb.width();
            // calculate new image size
            nw = resize_dimension;
            nh = static_cast<int>(0.5 + (sizefactor*rgb.height()));
        }
        else
        {
            sizefactor = (double)resize_dimension/rgb.height();
            // calculate new image size
            nw = static_cast<int>(0.5 + (sizefactor*rgb.width()));
            nh = resize_dimension;
        }

        if(verbose)
        {
            std::cout << "Scaling by:\t\t" << sizefactor << std::endl;
            std::cout << "New dimensions:\t\t" << nw << " x " << nh << std::endl;
        };

        // create a RGB image of appropriate size
        temp.resize(nw,nh);

        // resize the image, using a bi-cubic spline algorithm
        vigra::resizeImageNoInterpolation(srcImageRange(rgb),destImageRange(temp));
        //exportImage(srcImageRange(out), ImageExportInfo("test.tif").setPixelType("UINT16"));
    }
    else
    {
        temp.resize(nw,nh);
        vigra::copyImage(srcImageRange(rgb),destImage(temp));
    };

    // Convert to Luv colour space
    luv.resize(temp.width(),temp.height());
    vigra::transformImage(srcImageRange(temp), destImage(luv), vigra::RGB2LuvFunctor<double>() );
    //exportImage(srcImageRange(luv), ImageExportInfo("test_luv.tif").setPixelType("UINT16"));
    temp.resize(0,0);
};

// converts the given Luv image into arrays for Gabors filtering
void prepareGaborImage(vigra::UInt16RGBImage& luv, float**& pixels)
{
    pixels = CreateMatrix( (float)0, luv.height(), luv.width() );
    // Prepare framebuf for Gabor API, we need only L channel
    for (int i = 0; i < luv.height(); i++ )
    {
        for (int j = 0; j < luv.width(); j++ )
        {
            pixels[i][j] = luv(j,i)[0];
            //std::cout << i << " " << j << " = " << k << " - " << frameBuf[k] << std::endl;
        }
    }
};

//classify the points with SVM
std::vector<double> classifySVM(struct svm_model* model, int gNumLocs,int**& gLocations,int width,int height,int vector_length, float*& response,int gRadius,vigra::UInt16RGBImage& luv,bool needsMoreIndex=false)
{
    std::vector<double> svm_response;
    // Integers and containers for libsvm
    int max_nr_attr = 56;
    int nr_class=svm_get_nr_class(model);
    struct svm_node *gabor_responses = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
    double *prob_estimates = (double *) malloc(nr_class*sizeof(double));

    for (int j = 0; j < gNumLocs; j++)
    {
        unsigned int feature = 1;

        if(needsMoreIndex)
        {
            if(j >= max_nr_attr - 1)
            {
                max_nr_attr *= 2;
                struct svm_node* newPointer = (struct svm_node *) realloc(gabor_responses,max_nr_attr*sizeof(struct svm_node));
                if(newPointer == NULL)
                {
                    svm_response.clear();
                    free(gabor_responses);
                    free(prob_estimates);
                    return svm_response;
                }
                gabor_responses=newPointer;
            }
        };

        for ( int v = (j * vector_length); v < ((j + 1) * vector_length); v++)
        {
            gabor_responses[feature-1].index = feature;
            gabor_responses[feature-1].value = response[v];					
            //std::cout << feature << ":" << response[v] << " ";
            feature++;
        }

        // Work out average colour and variance
        vigra::FindAverageAndVariance<vigra::UInt16RGBImage::PixelType> average;
        vigra::inspectImage(srcIterRange(
            luv.upperLeft()+vigra::Diff2D(gLocations[j][0]-gRadius,gLocations[j][1]-gRadius),
            luv.upperLeft()+vigra::Diff2D(gLocations[j][0]+gRadius,gLocations[j][1]+gRadius)
            ),average);
        // Add these colour features to feature vector							

        gabor_responses[feature-1].index = feature;
        gabor_responses[feature-1].value = average.average()[1];
        //std::cout << feature << ":" << u_ave << " ";
        feature++;
        gabor_responses[feature-1].index = feature;
        gabor_responses[feature-1].value = sqrt(average.variance()[1]);
        //std::cout << feature << ":" << std_u << " ";
        feature++;
        gabor_responses[feature-1].index = feature;
        gabor_responses[feature-1].value = average.average()[2];
        //std::cout << feature << ":" << v_ave << " ";
        feature++;
        gabor_responses[feature-1].index = feature;
        gabor_responses[feature-1].value = sqrt(average.variance()[2]);
        //std::cout << feature << ":" << std_v << " ";
        feature++;
        gabor_responses[feature-1].index = feature;
        gabor_responses[feature-1].value = luv(gLocations[j][0],gLocations[j][1])[1];
        //std::cout << feature << ":" << u_values[pixel_number] << " ";
        feature++;
        gabor_responses[feature-1].index = feature;
        gabor_responses[feature-1].value = luv(gLocations[j][0],gLocations[j][1])[2];
        //std::cout << feature << ":" << v_values[pixel_number] << " " << std::endl;
        gabor_responses[feature].index = -1;

        svm_predict_probability(model,gabor_responses,prob_estimates);	
        svm_response.push_back(prob_estimates[0]);
    }
    // Free up libsvm stuff
    free(gabor_responses);
    free(prob_estimates);
    return svm_response;
};

// create a grid of control points for creating masks
void createGrid(int& gNumLocs,int**& gLocations,int gRadius,int width, int height)
{
    int spacing=(gRadius*2)+1;
    for (int i = gRadius; i < height - gRadius; i += spacing )
    {
        for (int j = gRadius; j < width - gRadius; j += spacing )
        {
            gNumLocs++;
        }
        // Add extra FP at the end of each row in case width % gRadius
        gNumLocs++;
    }

    // Add extra FP at the end of each row in case nh % gRadius	
    for (int j = gRadius; j < width - gRadius; j += spacing )
    {
        gNumLocs++;
    }

    // Create the storage matrix
    gLocations = CreateMatrix( (int)0, gNumLocs, 2);
    gNumLocs = 0;
    for (int i = gRadius; i < height - gRadius; i += spacing )
    {
        for (int j = gRadius; j < width - gRadius; j += spacing )
        {
            gLocations[gNumLocs][0] = j;
            gLocations[gNumLocs][1] = i;
            //std::cout << "fPoint " << gNumLocs << ":\t" << i << " " << j << std::endl;
            gNumLocs++;
        }

        // Add extra FP at the end of each row in case width % spacing
        if (width % spacing)
        {
            gLocations[gNumLocs][0] = width - gRadius - 1;
            gLocations[gNumLocs][1] = i;
            //std::cout << "efPoint " << gNumLocs << ":\t" << i << " " << nw - gRadius - 1 << std::endl;
            gNumLocs++;
        }
    }

    // Add extra FP at the end of each row in case height % spacing
    if (height % spacing)
    {
        for (int j = gRadius; j < width - gRadius; j += spacing )
        {
            gLocations[gNumLocs][0] = j;
            gLocations[gNumLocs][1] = height - gRadius - 1;
            //std::cout << "efPoint " << gNumLocs << ":\t" << nh - gRadius - 1 << " " << j << std::endl;
            gNumLocs++;
        }
    }
};

//generates the celeste mask on base of given responses and locations
void generateMask(vigra::BImage& mask,int& gNumLocs, int**& gLocations,std::vector<double> svm_responses,int gRadius, double threshold)
{
    for ( int j = 0; j < gNumLocs; j++ )
    {
        if (svm_responses[j] >= threshold)
        {
            unsigned int sub_x0 = gLocations[j][0] - gRadius;
            unsigned int sub_y0 = gLocations[j][1] - gRadius;
            unsigned int sub_x1 = gLocations[j][0] + gRadius + 1;
            unsigned int sub_y1 = gLocations[j][1] + gRadius + 1;
            //std::cout << sub_x0 << ","<< sub_y0 << " - " << sub_x1 << "," << sub_y1 << std::endl;

            // Set region to black
            vigra::initImage(srcIterRange(mask.upperLeft() + vigra::Diff2D(sub_x0, sub_y0),
                        mask.upperLeft() + vigra::Diff2D(sub_x1, sub_y1)), 0);				
        }
        else
        {
            //std::cout << "Non-cloud\t(score " << prob_estimates[0] << " <= " << threshold << ")" << std::endl;	
        }
    }
};

vigra::BImage* getCelesteMask(struct svm_model* model, vigra::UInt16RGBImage& input, int radius, float threshold, int resize_dimension,bool adaptThreshold,bool verbose)
{
    vigra::UInt16RGBImage luv;
    double sizefactor=1.0;
    prepareCelesteImage(input,luv,resize_dimension,sizefactor,verbose);

    // Prepare Gabor API array
    float** pixels=NULL;
    prepareGaborImage(luv,pixels);

    int** gLocations = NULL;
    int gNumLocs = 0;

    // Create grid of fiducial points
    createGrid(gNumLocs,gLocations,radius,luv.width(),luv.height());

    int len = 0;
    float* mask_response=NULL;
    mask_response = ProcessChannel(pixels,luv.width(),luv.height(),gNumLocs,gLocations,radius,mask_response,&len);
    // Turn the response into SVM vector, and add colour features
    std::vector<double> svm_responses=classifySVM(model,gNumLocs,gLocations,luv.width(),luv.height(),(int)len/gNumLocs,mask_response,radius,luv);
    delete [] mask_response;

    if(adaptThreshold)
    {
        double minVal=1;
        for(unsigned int i=0;i<svm_responses.size();i++)
        {
            if(svm_responses[i]<minVal)
            {
                minVal=svm_responses[i];
            };
        };
        if(threshold<minVal)
        {
            threshold = std::min(minVal + 0.1, 1.0);
        };
    };
    // Create mask of same dimensions
    vigra::BImage mask_out(luv.width(), luv.height(),255);
    generateMask(mask_out,gNumLocs,gLocations,svm_responses,radius,threshold);
    // Re-size mask to match original image
    vigra::BImage* mask_resize = new vigra::BImage(input.size());
    vigra::resizeImageNoInterpolation(vigra::srcImageRange(mask_out), vigra::destImageRange(*mask_resize));		
    DisposeMatrix(pixels,luv.height());
    DisposeMatrix(gLocations,gNumLocs);
    mask_out.resize(0,0);
    return mask_resize;
};

HuginBase::UIntSet getCelesteControlPoints(struct svm_model* model, vigra::UInt16RGBImage& input, HuginBase::CPointVector cps, int radius, float threshold, int resize_dimension,bool verbose)
{
    HuginBase::UIntSet cloudCP;
    vigra::UInt16RGBImage luv;
    double sizefactor=1.0;
    prepareCelesteImage(input,luv,resize_dimension,sizefactor,verbose);

    // Prepare Gabor API array
    float** pixels=NULL;
    prepareGaborImage(luv,pixels);

    int gNumLocs = cps.size();
    int** gLocations = CreateMatrix( (int)0, gNumLocs, 2);
    for(unsigned int j=0;j<cps.size();j++)
    {
        HuginBase::ControlPoint cp=cps[j].second;
        gLocations[j][0] = int(cp.x1 * sizefactor);
        gLocations[j][1] = int(cp.y1 * sizefactor);
        // Move CPs to border if the filter radius is out of bounds
        if (gLocations[j][0] <= radius)
        {
            gLocations[j][0] = radius + 1;
        }
        if (gLocations[j][1] <= radius)
        {
            gLocations[j][1] = radius + 1;
        }
        if (gLocations[j][0] >= luv.width() - radius)
        {
            gLocations[j][0] = luv.width() - radius - 1;
        }
        if (gLocations[j][1] >= luv.height() - radius)
        {
            gLocations[j][1] = luv.height() - radius - 1;
        }
    };

    int len = 0;
    float* response=NULL;
    response = ProcessChannel(pixels,luv.width(),luv.height(),gNumLocs,gLocations,radius,response,&len);
    // Turn the response into SVM vector, and add colour features
    std::vector<double> svm_responses = classifySVM(model, gNumLocs, gLocations, luv.width(), luv.height(), (int)len / gNumLocs, response, radius, luv);
    delete [] response;

    for(unsigned int i=0;i<svm_responses.size();i++)
    {
        if(svm_responses[i]>=threshold)
        {
            cloudCP.insert(cps[i].first);
        };
    };
    DisposeMatrix(pixels,luv.height());
    DisposeMatrix(gLocations,gNumLocs);

    return cloudCP;
};

} // end of namespace
