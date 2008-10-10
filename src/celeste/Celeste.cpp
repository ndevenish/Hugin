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

#include <iostream>
#include "vigra/stdimage.hxx"
#include "vigra/resizeimage.hxx"
#include "vigra/impex.hxx"
#include "vigra/colorconversions.hxx"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "Gabor.h"
#include "Utilities.h"
#include "CelesteGlobals.h"
#include "svm.h"

using namespace vigra; 
using namespace std; 

typedef vigra::BRGBImage::PixelType RGB;

void get_gabor_response(string& imagefile, unsigned int& mask, string& model_file, double& threshold,string&
mask_format,vector<double>& svm_responses){
	
	// Open SVM model file
	struct svm_model* model;
	
	if((model = svm_load_model(model_file.c_str())) == 0){
		cout << "Couldn't load model file '" << model_file << "'" << endl << endl;
		for (int j = 0; j < gNumLocs; j++){
			svm_responses.push_back(0);
		}		
		return;
	}else{
		//cout << "Loaded model file " << model_file << endl;
	}	
	
	// Integers and containers for libsvm
	int svm_type=svm_get_svm_type(model);
	int nr_class=svm_get_nr_class(model);
	int max_nr_attr = 56;
	struct svm_node *gabor_responses = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
	double *prob_estimates = (double *) malloc(nr_class*sizeof(double));
	
	// Open image using Vigra
	try{

		cout << "Generating feature vector by Gabor filtering..." << endl;
		cout << "Opening image file:\t" << imagefile << endl;

        	// Read image given as first argument
        	// File type is determined automatically
        	vigra::ImageImportInfo info(imagefile.c_str());

		// Create RGB images of appropriate size    	
		vigra::FRGBImage in(info.width(), info.height());

        	// Import the image
        	importImage(info, destImage(in));

		// Max dimension
		double sizefactor = 1;
        	int nw = info.width(), nh = info.height();

		// In case we want to save filters
		// Create this directory and change option in Globals.cpp
		char basename[] = "gabor_filters/celeste"; 
				
		if (info.width() >= info.height()){
			if (resize_dimension >= info.width() ){
				resize_dimension = info.width();
			}
		}else{
			if (resize_dimension >= info.height()){
				resize_dimension = info.height();
			}		
		}
		//cout << "Re-size dimenstion:\t" << resize_dimension << endl;
		
		cout << "Image dimensions:\t" << info.width() << " x " << info.height() << endl;
		
        	// Re-size to max dimension
		if (info.width() > resize_dimension || info.height() > resize_dimension){
				
			if (info.width() >= info.height()){


				sizefactor = (double)resize_dimension/info.width();

        			// calculate new image size
        			nw = resize_dimension;
        			nh = static_cast<int>(0.5 + (sizefactor*info.height()));
							
			}else{
				sizefactor = (double)resize_dimension/info.height();


       				// calculate new image size
        			nw = static_cast<int>(0.5 + (sizefactor*info.width()));
        			nh = resize_dimension;
				
			}
         			
			cout << "Scaling by:\t\t" << sizefactor << endl;
			cout << "New dimensions:\t\t" << nw << " x " << nh << endl;
	
        		// create a RGB image of appropriate size    		
        		vigra::FRGBImage out(nw, nh);
	
			// resize the image, using a bi-cubic spline algorithm
			resizeImageNoInterpolation(srcImageRange(in),destImageRange(out));

			in = out;

		}
	
		// Convert to LUV colour space
		FRGBImage luv(in.width(),in.height());
		transformImage(srcImageRange(in), destImage(luv), RGBPrime2LuvFunctor<double>() );

		// Prepare Gabor API array
		float *frameBuf = new float[in.width()*in.height()];
		float *u_values = new float[in.width()*in.height()];
		float *v_values = new float[in.width()*in.height()];
		float** pixels = CreateMatrix( (float)0, in.height(), in.width() );

		// Do something with each pixel...
		unsigned int counter = 0;
		vigra::FRGBImage::iterator img_iter(luv.begin()),end(luv.end());
         	for(; img_iter != end; ++img_iter) {

			// [0] is L, [1] is U, [2] is V
			// We only want L for Gabor filtering
 	    		frameBuf[counter] = (*img_iter)[0];
			
			u_values[counter] = (*img_iter)[1];
			v_values[counter] = (*img_iter)[2];
		
 	    		//cout << "Pixel " << counter << " - L: " << (*img_iter)[0] << endl;
 	    		//cout << "Pixel " << counter << " - U: " << (*img_iter)[1] << endl;
	    		//cout << "Pixel " << counter << " - V: " << (*img_iter)[2] << endl;
			counter++;
 		}

		// Prepare framebuf for Gabor API
		unsigned int k = 0;
		for (int i = 0; i < in.height(); i++ ){
			for (int j = 0; j < in.width(); j++ ){
				pixels[i][j] = frameBuf[k];
				//cout << i << " " << j << " = " << k << " - " << frameBuf[k] << endl;
				k++;
			}
		}

		if (gNumLocs){

			float *response = NULL;
			int len = 0;
	
			// Scale control points by sizefactor
			for (int j = 0; j < gNumLocs; j++){
		
				//cout << sizefactor << ": " << gLocations[j][0] << "," << gLocations[j][1] << " ---> ";
				gLocations[j][0] = int(gLocations[j][0] * sizefactor);
				gLocations[j][1] = int(gLocations[j][1] * sizefactor);
				//cout << gLocations[j][0] << "," << gLocations[j][1] << endl;

				// Move CPs to border if the filter radius is out of bounds
				if (gLocations[j][0] <= gRadius){
					//cout << "Moving CP to border" << endl;
					gLocations[j][0] = gRadius + 1;
				}	
				if (gLocations[j][1] <= gRadius){
					//cout << "Moving CP to border" << endl;
					gLocations[j][1] = gRadius + 1;
				}
				if (gLocations[j][0] >= nw - gRadius){
					//cout << "Moving CP to border" << endl;
					gLocations[j][0] = nw - gRadius - 1;
				}
				if (gLocations[j][1] >= nh - gRadius){
					//cout << "Moving CP to border" << endl;
					gLocations[j][1] = nh - gRadius - 1;
				}
			}

			// Do Gabor filtering
			response = ProcessChannel( pixels, in.height(), in.width(), response, &len, basename );

			// Turn the response into SVM vector, and add colour features					
			int vector_length = (int)len/gNumLocs;
		
			for (int j = 0; j < gNumLocs; j++){

				int pixel_number = gLocations[j][0] + (in.width() * (gLocations[j][1] - 1)) - 1;
				unsigned int feature = 1;
				double score = 0;
							
				//cout << "0 ";				
				for ( int v = (j * vector_length); v < ((j + 1) * vector_length); v++){
					gabor_responses[feature-1].index = feature;
					gabor_responses[feature-1].value = response[v];					
					//cout << feature << ":" << response[v] << " ";
					feature++;
				}

				// Work out average colour - U + V channels			
				float u_sum = 0, v_sum = 0;
				unsigned int pixels_in_region = (gRadius * 2)*(gRadius * 2);
			
				for (int t = 1 - gRadius; t <= gRadius; t++){			
			
					unsigned int this_y_pixel = pixel_number + (t * in.width());

					for (int r = 1 - gRadius; r <= gRadius; r++){			
			
						unsigned int this_x_pixel = this_y_pixel + r;
						u_sum += u_values[this_x_pixel];
						v_sum += v_values[this_x_pixel];			
					}
				}


				float u_ave = (float)u_sum/pixels_in_region;
				float v_ave = (float)v_sum/pixels_in_region;

				// Now work out standard deviation for U and V channels
				u_sum = 0, v_sum = 0;

				for (int t = 1 - gRadius; t <= gRadius; t++){			
					
					unsigned int this_y_pixel = pixel_number + (t * in.width());
				
					for (int r = 1 - gRadius; r <= gRadius; r++){			
			
						unsigned int this_x_pixel = this_y_pixel + r;
					
						u_sum +=  pow(u_values[this_x_pixel]-u_ave,2);
						v_sum +=  pow(v_values[this_x_pixel]-v_ave,2);

					}
				}
			
			
		     		float std_u = sqrt(u_sum/(pixels_in_region-1));
    	 			float std_v = sqrt(v_sum/(pixels_in_region-1));	
			
				// Add these colour features to feature vector							
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = u_ave;
				//cout << feature << ":" << u_ave << " ";
				feature++;
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = std_u;
				//cout << feature << ":" << std_u << " ";
				feature++;
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = v_ave;
				//cout << feature << ":" << v_ave << " ";
				feature++;								
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = std_v;
				//cout << feature << ":" << std_v << " ";
				feature++;
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = u_values[pixel_number];
				//cout << feature << ":" << u_values[pixel_number] << " ";
				feature++;								
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = v_values[pixel_number];
				//cout << feature << ":" << v_values[pixel_number] << " " << endl;
				gabor_responses[feature].index = -1;				
							
				score = svm_predict_probability(model,gabor_responses,prob_estimates);
				//cout << score << " " << prob_estimates[0] << endl;

				svm_responses.push_back(prob_estimates[0]);		

			}

			delete[] response;
		}
		
		// Create mask
		if (mask){

			// Create mask file name
			string mask_name = ("");
			if (imagefile.substr(imagefile.length()-4,1) == (".")){
			
				mask_name.append(imagefile.substr(0,imagefile.length()-4));
								
			}else{
			
				mask_name.append(imagefile.substr(0,imagefile.length()-4));
			}
			mask_name.append("_mask.");
			mask_name.append(mask_format);

			cout << "Generating mask:\t" << mask_name << endl;				
			// Create mask of same dimensions
			vigra::BRGBImage mask_out(nw, nh);
			
			// Set mask to white
			vigra::initImage(srcIterRange(mask_out.upperLeft(),
			mask_out.upperLeft() + vigra::Diff2D(nw,nh)),
                	RGB(255,255,255) );

			float *mask_response = NULL;
			gLocations = NULL;
			gNumLocs = 0;

			// Create grid of fiducial points
			for (int i = gRadius; i < in.height() - gRadius; i += spacing ){
				for (int j = gRadius; j < in.width() - gRadius; j += spacing ){
					gNumLocs++;
				}
				// Add extra FP at the end of each row in case nw % gRadius
				gNumLocs++;
			}

			// Add extra FP at the end of each row in case nh % gRadius	
			for (int j = gRadius; j < in.width() - gRadius; j += spacing ){
				gNumLocs++;
			}

			// Create the storage matrix
			gLocations = CreateMatrix( (int)0, gNumLocs, 2);
			gNumLocs = 0;
			for (int i = gRadius; i < in.height() - gRadius; i += spacing ){
				for (int j = gRadius; j < in.width() - gRadius; j += spacing ){
			
					gLocations[gNumLocs][0] = j;
					gLocations[gNumLocs][1] = i;
					//cout << "fPoint " << gNumLocs << ":\t" << i << " " << j << endl;
					gNumLocs++;
				}
				
				// Add extra FP at the end of each row in case nw % spacing
				if (nw % spacing){
				
					gLocations[gNumLocs][0] = nw - gRadius - 1;
					gLocations[gNumLocs][1] = i;
					//cout << "efPoint " << gNumLocs << ":\t" << i << " " << nw - gRadius - 1 << endl;
					gNumLocs++;
				}
				
			}
				
			// Add extra FP at the end of each row in case nh % spacing
			if (nh % spacing){
						
				for (int j = gRadius; j < in.width() - gRadius; j += spacing ){
			
					gLocations[gNumLocs][0] = j;
					gLocations[gNumLocs][1] = nh - gRadius - 1;
					//cout << "efPoint " << gNumLocs << ":\t" << nh - gRadius - 1 << " " << j << endl;
					gNumLocs++;
				
				}
			}

			//cout << "Total FPs:\t" << gNumLocs << endl;

			int len = 0;		
			//cout << "Pre-response " << in.height() << ","<<  in.width() << endl;	
			mask_response = ProcessChannel( pixels, in.height(), in.width(), mask_response, &len, basename );
			//cout << "Post-response" << endl;

			// Turn the response into SVM vector, and add colour features
		
			int vector_length = (int)len/gNumLocs;

			for ( int j = 0; j < gNumLocs; j++ ){

				//cout << j << ":"<<endl;

				unsigned int pixel_number = gLocations[j][0] + (in.width() * (gLocations[j][1] - 1)) - 1;
				unsigned int feature = 1;
				double score = 0;
			
				// need one more for index = -1
				if(j >= max_nr_attr - 1){
					max_nr_attr *= 2;
					gabor_responses = (struct svm_node *) realloc(gabor_responses,max_nr_attr*sizeof(struct svm_node));
				}	
			
			
				for ( int v = (j * vector_length); v < ((j + 1) * vector_length); v++){					
					gabor_responses[feature-1].index = feature;
					gabor_responses[feature-1].value = mask_response[v];					
					//cout << feature << ":" << mask_response[v] << " ";
					feature++;
				}

				// Work out average colour - U + V channels			
				float u_sum = 0, v_sum = 0;
				int pixels_in_region = (gRadius * 2)*(gRadius * 2);
			
				for (int t = 1 - gRadius; t <= gRadius; t++){			
			
					unsigned int this_y_pixel = pixel_number + (t * in.width());

					for (int r = 1 - gRadius; r <= gRadius; r++){			
			
						unsigned int this_x_pixel = this_y_pixel + r;
					
						u_sum += u_values[this_x_pixel];
						v_sum += v_values[this_x_pixel];

					}
				}

				float u_ave = (float)u_sum/pixels_in_region;
				float v_ave = (float)v_sum/pixels_in_region;

				// Now work out standard deviation for U and V channels
				u_sum = 0, v_sum = 0;

				for (int t = 1 - gRadius; t <= gRadius; t++){	
						
					unsigned int this_y_pixel = pixel_number + (t * in.width());
				
					for (int r = 1 - gRadius; r <= gRadius; r++){			
			
						unsigned int this_x_pixel = this_y_pixel + r;
					
						u_sum +=  pow(u_values[this_x_pixel]-u_ave,2);
						v_sum +=  pow(v_values[this_x_pixel]-v_ave,2);

					}
				}			

		     		float std_u = sqrt(u_sum/(pixels_in_region-1));
    	 			float std_v = sqrt(v_sum/(pixels_in_region-1));	

				// Add these colour features to feature vector							
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = u_ave;
				//cout << feature << ":" << u_ave << " ";
				feature++;
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = std_u;
				//cout << feature << ":" << std_u << " ";
				feature++;
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = v_ave;
				//cout << feature << ":" << v_ave << " ";
				feature++;								
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = std_v;
				//cout << feature << ":" << std_v << " ";
				feature++;
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = u_values[pixel_number];
				//cout << feature << ":" << u_values[pixel_number] << " ";
				feature++;								
				gabor_responses[feature-1].index = feature;
				gabor_responses[feature-1].value = v_values[pixel_number];
				//cout << feature << ":" << v_values[pixel_number] << " " << endl;
				gabor_responses[feature].index = -1;	

				score = svm_predict_probability(model,gabor_responses,prob_estimates);
				//cout << score << " " << prob_estimates[0] << endl;

				if (prob_estimates[0] >= threshold){
						
					//cout << "Cloud\t\t(score " << m.classify(feature_vector.c_str()) << " > " << threshold << ")" << endl;	
							
					unsigned int sub_x0 = gLocations[j][0] - gRadius;
        				unsigned int sub_y0 = gLocations[j][1] - gRadius;
        				unsigned int sub_x1 = gLocations[j][0] + gRadius + 1;
        				unsigned int sub_y1 = gLocations[j][1] + gRadius + 1;
					
					//cout << sub_x0 << ","<< sub_y0 << " - " << sub_x1 << "," << sub_y1 << endl;
					
					// Set region to black
					vigra::initImage(srcIterRange(mask_out.upperLeft() + vigra::Diff2D(sub_x0, sub_y0),
					mask_out.upperLeft() + vigra::Diff2D(sub_x1, sub_y1)),
                			RGB(0,0,0) );				
				
				}else{				
					//cout << "Non-cloud\t(score " << m.classify(feature_vector.c_str()) << " <= " << threshold << ")" << endl;	
								
				}						
				//cout << feature_vector << endl;			

			}
	
			delete [] mask_response;

			// Re-size mask to match original image
        		vigra::BRGBImage mask_resize(info.width(),info.height());			   
			resizeImageNoInterpolation(srcImageRange(mask_out),destImageRange(mask_resize));		
			exportImage(srcImageRange(mask_resize), ImageExportInfo(mask_name.c_str()).setPixelType("UINT8"));
		
		}

		DisposeMatrix( pixels, in.height() );
		delete[] frameBuf;
		delete[] u_values;
		delete[] v_values;
		gNumLocs = 0;		
		cout << endl;

	}catch (vigra::StdException & e){
        
		// catch any errors that might have occured and print their reason
        	cout << "Unable to open file:\t" << imagefile << endl << endl;
		cout << e.what() << endl << endl;
		for (int j = 0; j < gNumLocs; j++){
			svm_responses.push_back(0);
		}
        	return;
		
    	}
	
	// Free up libsvm stuff
	free(gabor_responses);
	free(prob_estimates);
	svm_destroy_model(model);
		
}

