// -*- c-basic-offset: 4 -*-

/** @file jbu.cpp
 *
 *  @brief Implementation of joint bilateral upsampling
 *
 *  @author Jing Jin <jingidy@gmail.com>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
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
 
#include <cmath>

#include "khan.h"
#include "jbu.h"


using namespace std;
using namespace hugin_utils;
using namespace vigra;
using namespace vigra::functor;

extern int g_verbose;

/** given n layers of source images and n layers of references, upscales all
 *  source images with respect to their individual references
 *  destImgs is ptr to an empty vector to be filled in
 */
void jointBilateralUpsampling(vector<FImagePtr> *sources, 
								vector<FImagePtr> *refs,
								vector<FImagePtr> *destImgs, int num_neighbors)
{
	int num_layers = sources->size();
	bool destEmpty = destImgs->size() == 0;
	for(int i = 0; i < num_layers; i++) {
		FImagePtr tmp = jbuImage(sources->at(i), refs->at(i), num_neighbors);
		if(!destEmpty)
			destImgs->at(i) = tmp;
		else
			destImgs->push_back(tmp);
	}
}


/** upsamples a given source image to the size of the given reference 
 *  creates the resulting FImagePtr and returns it
 */
FImagePtr jbuImage(FImagePtr source, FImagePtr refIm, int num_neighbors)
{		
	int width = refIm->width();
	int height = refIm->height();
	int s_width = source->width();
	int s_height = source->height();
	float scale = s_width / width;
		
	FImagePtr dest = FImagePtr(new FImage(width, height));
	float sigma = 0.5;
	int mu = 0;
	
	for(int y = 0; y < height; y++) {
		float *refPixLoc = (float *)(refIm->data()) + y * width;
		
		if(g_verbose > 2)
			cout << "y: " << y << "\n"
				<< "\trefPixLoc: " << hex << refPixLoc << endl;
		
		for(int x = 0; x < width; x++) {
			float refPix = *(refPixLoc + x);
			
			float total_val = 0;
			float normalizing_factor = 0;
			
			//prevent black areas fro all rgausses being 0
			float norgauss = 0; 
			float norgauss_normalize = 0;
			
			//find coords in source
			float o_x = x * scale;
			float o_y = y * scale;
			
			if(g_verbose > 2)
				cout << "x: " << x << "\n"
				<< "\trefPixLoc+x: " << hex << refPixLoc + x << "\n"
				<< "\to_x: " << o_x << "\n"
				<< "\to_y: " << o_y << endl;
				
			//for each neighbor for the source
			for(int j = -num_neighbors; j <= num_neighbors; j++) {
				int r_y = (int)round(o_y + j);
				r_y = (r_y > 0 ? (r_y < s_height ? r_y : s_height-1) : 0);
				float *srcPixLoc = (float *)source->data() + r_y * s_width;
				//corresponding reference pixel
				float *neighborPixLoc = (float *)refIm->data() + 
											(int) (r_y * scale * width);
				
				if(g_verbose > 3)
					cout << "\tj: " << j << "\n"
					<< "\t\tr_y: " << r_y << "\n"
					<< "\t\tsrcPixLoc: " << hex << srcPixLoc << "\n"
					<< "\t\tneighborPixLoc: " << hex << neighborPixLoc << endl;
								
				for(int i = -num_neighbors; i <= num_neighbors; i++) {
					//find coords in source
					int r_x = (int)round(o_x + i);
					r_x = (r_x > 0 ? (r_x < s_width ? r_x : s_width-1) : 0);
					float srcPix = *(srcPixLoc + r_x);
					//in ref img
					neighborPixLoc += int(r_x * scale);
					float neighborPix = *neighborPixLoc;
					
					if(g_verbose > 3)
						cout << "\ti: " << i << "\n"
						<< "\t\tr_x: " << r_x << "\n"
						<< "\t\tsrcPixLoc+r_x: " << hex << srcPixLoc + r_x << "\n"
						<< "\t\tsrcPix: " << srcPix << "\n"
						<< "\t\tneighborPixLoc: " << hex << neighborPixLoc << "\n"
						<< " \t\tneighborPix: " << neighborPix << endl;
					
					//gauss dist to center
					float sgauss = simpleGauss(dist(o_x, o_y, r_x, r_y),
												sigma, mu);
					//gauss radiance diff to center in ref
					float rgauss = simpleGauss(abs(refPix - neighborPix),
												sigma, mu);
					
					//multiply gausses by value in source and add to total val
					norgauss = srcPix * sgauss;
					norgauss_normalize += sgauss;
					float totalgauss = sgauss * rgauss;
					normalizing_factor += totalgauss;
					total_val += srcPix * totalgauss;
					
					if(g_verbose > 3)
						cout << "\t\tsgauss: " << sgauss << "\n"
								<< "\t\trgauss: " << rgauss << "\n"
								<< "\t\ttotalgauss+srcPix: " << totalgauss + 
																srcPix << endl;
					
				} //end for i
			}//end for j
			
			//normalize and store
			
			if(g_verbose > 2)
				cout << "\tnormalizing_factor: " << normalizing_factor << "\n"
						<< "\ttotal_val(before): " << total_val << endl;

			if(total_val) {
				total_val /= normalizing_factor;
				*((float *)dest->data() + y * width + x) = total_val;
			}
			else {
				total_val = norgauss/norgauss_normalize;
			}
			
			if(g_verbose > 3)
				cout << "\ttotal_val(after): " << total_val << "\n"
						<< "stored in: " << hex << dest->data() + 
												y * width + x << endl; 
		}//end for x
	}//end for y
		
	return dest; 
}

/** given 2 floating point pixel locations, computes the distance */
float dist(float x1, float y1, float x2, float y2)
{
	return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

/** gaussian applied to a single variable */
float simpleGauss(float x, float sigma, float mu)
{
	if(sigma == 0) {
		cerr << "simpleGauss: sigma must be non-zero" << endl;
		exit(1);
	}
	float pi = 3.1415926;
	float x_p = x - mu;
	float exponent = std::exp(x_p * x_p / (-2 * sigma * sigma));
	exponent /= sqrt(2 * pi) * sigma;
	return exponent;
}
