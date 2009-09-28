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
#include "Gabor.h"
#include "Utilities.h"
#include "CelesteGlobals.h"

using namespace vigra; 
using namespace std; 

void usage (char* filename){

	cout << "Usage: " << filename << " <image file> <mask file>" << endl;
	cout << "or for a positive example: " << filename << " -p <image file>" << endl;
	cout << "or for a negative example: " << filename << " -n <image file>" << endl;
	cout << "(supported formats: " << vigra::impexListFormats() << ")" << endl;
}

int main(int argc, char ** argv){

        unsigned int a = 1;
	unsigned int pos = 0;
	unsigned int neg = 0;
	string image_file,mask_file;
	vigra::FRGBImage mask_in;
	vigra::FRGBImage::iterator mask_iter;

	// Deal with arguments
    	if(argc < 3 || argc >= 4){
	
		usage(argv[0]); exit(1);
    	}

        while( a < argc ){

                if( argv[a][0] == '-'){
    
                        a++;
                        switch(argv[a-1][1]){

                                // Put some more argument options in here later
                                case 'h' : {usage(argv[0]); exit(1);}
				case 'p' : {pos = 1;
						cerr << "Ignoring mask. Image is a positive example." << endl;
						image_file = argv[argc-1];
						break;
					    }
				case 'n' : {neg = 1;
						cerr << "Ignoring mask. Image is a negative example." << endl;
						image_file = argv[argc-1];
						break;
						}
                                default  : {usage(argv[0]); exit(1);}
                        }

                }

                a++;
        }

	if (pos == 0 && neg == 0){

		image_file = argv[argc-2];
		mask_file = argv[argc-1];
		
		//cout << "Image is " << argv[argc-2] << endl;
		//cout << "Mask is " << argv[argc-1] << endl;
				
	}
    
	try{

		cerr << "Opening image file: " << image_file << endl;

        	// Read image given as first argument
        	// File type is determined automatically
        	vigra::ImageImportInfo info(image_file.c_str());

		// Create RGB images of appropriate size    	
		vigra::FRGBImage in(info.width(), info.height());

        	// Import the image
        	importImage(info, destImage(in));

		// Deal with mask unless -p || -n flags are on
		if (pos == 0 && neg == 0){
		
			cerr << "Opening mask file:  " << mask_file << endl;

        		// Read mask given as second argument
        		vigra::ImageImportInfo mask_info(mask_file.c_str());
		
			// Check dimensions are the same
			if (info.width() != mask_info.width() || info.height() != mask_info.height()){

    	    			cerr << "Error - image and mask files are different dimension:" << endl;
				cerr << "Image:\t" << info.width() << " x " << info.height() << endl;
				cerr << "Mask:\t" << mask_info.width() << " x " << mask_info.height() << endl << endl;
    	    			return 1;

			}
		
			// Create RGB images of appropriate size    	
			vigra::FRGBImage tmp_mask_in(info.width(), info.height());
			// Import the mask
			importImage(mask_info, destImage(tmp_mask_in));
			mask_in = tmp_mask_in;
		
		}
		

		// Max dimension
		const int resize_dimension = 800;
		
        	// Re-size to 800 max dimension
		if (info.width() >= resize_dimension || info.height() >= resize_dimension){

			// Method (0 - pixel repetition, 1 - linear, 2 - spline
			// Pixel repetition seems to be the fastest
			int method = 0;
			double sizefactor;
	
			if (info.width() >= info.height()){
				sizefactor = (double)resize_dimension/info.width();
			}else{
				sizefactor = (double)resize_dimension/info.height();
			}
        
        		// calculate new image size
        		int nw = (int)(sizefactor*info.width());
        		int nh = (int)(sizefactor*info.height());
 
 			cerr << "Image dimensions:\t" << info.width() << " x " << info.height() << endl;
			cerr << "Scaling by:\t\t" << sizefactor << endl;
			cerr << "New dimensions:\t\t" << nw << " x " << nh << endl;
	
        		// create a RGB image of appropriate size    		
        		vigra::FRGBImage out(nw, nh);
			            
        		switch(method){
              
				case 0:
				// resize the image, using a bi-cubic spline algorithms
				resizeImageNoInterpolation(srcImageRange(in),destImageRange(out));
                		break;
		
				case 1:
                		// resize the image, using a bi-cubic spline algorithms
                		resizeImageLinearInterpolation(srcImageRange(in),destImageRange(out));
                		break;
		
              			default:
                		// resize the image, using a bi-cubic spline algorithms
                		resizeImageSplineInterpolation(srcImageRange(in),destImageRange(out));
			}

			in = out;

			// Deal with mask unless -p || -n flags are on
			if (pos == 0 && neg == 0){

				vigra::FRGBImage mask_out(nw, nh);

        			switch(method){
              
					case 0:
					// resize the image, using a bi-cubic spline algorithms
					resizeImageNoInterpolation(srcImageRange(mask_in),destImageRange(mask_out));
                			break;
		
					case 1:
                			// resize the image, using a bi-cubic spline algorithms
					resizeImageLinearInterpolation(srcImageRange(mask_in),destImageRange(mask_out));
                			break;
		
              				default:
                			// resize the image, using a bi-cubic spline algorithms
 					resizeImageSplineInterpolation(srcImageRange(mask_in),destImageRange(mask_out));
					
				}
				
				mask_in = mask_out;
			}

		}else{
			cerr << "Image dimensions:\t" << info.width() << " x " << info.height() << endl;
		}

		cerr << "Total pixels:\t\t" << in.width()*in.height() << endl;
	
		// Convert to LUV colour space
		cerr << "Converting to LUV colourspace..." << endl;		
		
		FRGBImage luv(in.width(),in.height());
		transformImage(srcImageRange(in), destImage(luv), RGBPrime2LuvFunctor<double>() );
		
		// Create grid of fiducial points
		cerr << "Generating fiducial points..." << endl;

		for (int i = gRadius; i < in.height() - gRadius; i += spacing ){
			for (int j = gRadius; j < in.width() - gRadius; j += spacing ){
				gNumLocs++;
			}
		}

		// Create the storage matrix
		gLocations = CreateMatrix( (int)0, gNumLocs, 2);
		gNumLocs = 0;
		for (int i = gRadius; i < in.height() - gRadius; i += spacing ){
			for (int j = gRadius; j < in.width() - gRadius; j += spacing ){
			
				gLocations[gNumLocs][0] = j;
				gLocations[gNumLocs][1] = i;
				
				//cerr << "fPoint " << gNumLocs << ":\t" << i << " " << j << endl;
				
				gNumLocs++;
			}
		}
		cerr << "Total fiducial points: " << gNumLocs << endl;

		// Prepare Gabor API array
		float *frameBuf = new float[in.width()*in.height()];
		float *u_values = new float[in.width()*in.height()];
		float *v_values = new float[in.width()*in.height()];
		
		
		float** pixels = CreateMatrix( (float)0, in.height(), in.width() );

		// Do something with each pixel...
		int counter = 0;
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


		int k = 0;
		for (int i = 0; i < in.height(); i++ ){
			for (int j = 0; j < in.width(); j++ ){
				pixels[i][j] = frameBuf[k];
				//cout << i << " " << j << " = " << k << " - " << frameBuf[k] << endl;
				k++;
			}
		}

		char basename[] = "gabor_filters/celeste"; 

		float *response = NULL;
		int len = 0;

		response = ProcessChannel( pixels, in.height(), in.width(), response, &len, basename );

		// Deal with mask unless -p || -n flags are on
		if (pos == 0 && neg == 0){
		
			// Iterator for mask
			vigra::FRGBImage::iterator tmp_mask_iter(mask_in.begin()),mask_end(mask_in.end());
			mask_iter = tmp_mask_iter;
			mask_end = NULL;
		}
		
		cerr << "Generating feature vector by Gabor filtering..." << endl;
		
		// Do something nice with response
		int vector_length = (int)len/gNumLocs;
		for ( int j = 0; j < gNumLocs; j++ ){
			
			//cerr << gLocations[j][0] << "," << gLocations[j][1] << endl;
			
			int pixel_number = gLocations[j][0] + (in.width() * (gLocations[j][1] - 1)) - 1;

			// Apply label			
			if (pos){
				cout << "+1 ";
			}else if (neg){
				cout << "-1 ";
			}else{
				// In the mask, clouds are black, non-clouds are white
				if (mask_iter[pixel_number][0] != 0 || mask_iter[pixel_number][1] != 0 || mask_iter[pixel_number][2] != 0){
					// So this must be non-cloud = -ve example
					cout << "-1 ";
				}else{
					// So this must be cloud = +ve example
					cout << "+1 ";
				}			
			}

			int feature = 1;
			for ( int v = (j * vector_length); v < ((j + 1) * vector_length); v++){
				cout << feature << ":" << response[v] << " ";
				feature++;
			}

			// Work out average colour - U + V channels
			
			float u_sum = 0, v_sum = 0;
			int pixels_in_region = (gRadius * 2)*(gRadius * 2);
			
			// height
			//cout << endl << "Pixel " << pixel_number << " x:" << gLocations[j][0] << " y:" <<
			//gLocations[j][1] << " width: " <<  in.width() << endl;
			
			for (int t = 1 - gRadius; t <= gRadius; t++){			
			
				int this_y_pixel = pixel_number + (t * in.width());
				//cout << "t = " << t << " pixel= " << pixel_number << " + " << (t * in.width()) << endl;
				// width
				for (int r = 1 - gRadius; r <= gRadius; r++){			
			
					int this_x_pixel = this_y_pixel + r;
					
					//if (this_x_pixel == pixel_number){
					//	cout << "*" << this_x_pixel << "* ";
					//}else{
					//	cout << this_x_pixel << " ";
					//}
					
					u_sum += u_values[this_x_pixel];
					v_sum += v_values[this_x_pixel];
					
					//cout << "u = " << u_values[this_x_pixel] << "(" << u_sum << ")" << endl;
					//cout << "v = " << v_values[this_x_pixel] << "(" << v_sum << ")" << endl;
			
				}
				//cout << endl;
			}
			//cout << endl;

			float u_ave = (float)u_sum/pixels_in_region;
			float v_ave = (float)v_sum/pixels_in_region;

			// Now work out standard deviation for U and V channels
			u_sum = 0, v_sum = 0;

			for (int t = 1 - gRadius; t <= gRadius; t++){			
				int this_y_pixel = pixel_number + (t * in.width());
				
				for (int r = 1 - gRadius; r <= gRadius; r++){			
			
					int this_x_pixel = this_y_pixel + r;
					
					u_sum +=  pow(u_values[this_x_pixel]-u_ave,2);
					v_sum +=  pow(v_values[this_x_pixel]-v_ave,2);

				}
			}
			
			
		     	float std_u = sqrt(u_sum/(pixels_in_region-1));
    	 		float std_v = sqrt(v_sum/(pixels_in_region-1));	
			
			//cerr << "U sum = " << u_sum << " U ave = " << u_ave << " U std = " << std_u << endl;
			//cerr << "V sum = " << v_sum << " V ave = " << v_ave << " V std = " << std_v << endl << endl;
			
			cout << feature << ":" << u_ave << " ";
			feature++;
			cout << feature << ":" << std_u << " ";
			feature++;
			cout << feature << ":" << v_ave << " ";
			feature++;
			cout << feature << ":" << std_v << " ";
			feature++;				

			// Add the U/V values for this actual pixel

			cout << feature << ":" << u_values[pixel_number] << " ";
			feature++;
			cout << feature << ":" << v_values[pixel_number] << " ";
			feature++;					
				
				
			//cout << "# FP_" << j << "_Pixel_" << pixel_number << "_Pos_" << gLocations[j][0] << "-" << gLocations[j][1] <<
			//"_Mask_RGB_" << mask_iter[pixel_number][0] << "," << mask_iter[pixel_number][1] << "," <<
			//mask_iter[pixel_number][2] << endl;
			cout << endl;
		}

		delete[] response;
		delete[] frameBuf;
        delete[] u_values;
        delete[] v_values;

		// Export images
		//exportImage(srcImageRange(in), ImageExportInfo("output_images/image_reduced.jpg"));
		//exportImage(srcImageRange(luv), ImageExportInfo("output_images/luv_reduced.jpg"));
		//
		//if (pos == 0 && neg == 0){
		//	exportImage(srcImageRange(mask_in), ImageExportInfo("output_images/mask_reduced.jpg"));
		//}
			
		//cout << "Done." << endl;
        
	}catch (vigra::StdException & e){
        
		// catch any errors that might have occured and print their reason
        	std::cout << e.what() << std::endl;
        	return 1;
		
    	}
    
    	return 0;
	
}
