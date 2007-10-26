// -*- c-basic-offset: 4 -*-

/** @file khanSupport.cpp
 *
 *  @brief support functions related to the khan algorithm
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
  
#include <hugin_config.h>
#include <fstream>
#include <sstream>
#include <math.h>
#include <algorithm>

#include <boost/shared_ptr.hpp>

#include <hugin_utils/utils.h>

// include vigra image processing library
#include <vigra/error.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/combineimages.hxx>
#include <vigra/resizeimage.hxx>

#include <vigra_ext/impexalpha.hxx>
#include <vigra_ext/utils.h>

#include "khan.h"

using namespace std;
using namespace hugin_utils;
using namespace vigra;
using namespace vigra::functor;

extern int g_verbose;

/** Mexican hat function, eq. 5 from Khan paper */
float weightMexicanHat(unsigned char x)
{
    double t = (x/127.5 -1);
    t *= t; // ^2
    t *= t; // ^4
    t *= t; // ^8
	t *= t; // ^16
    return 1.0 - t; 
}

/** favor high SNR function 
 *  f(x) = 1 - (x-220/255)^4
*/
float favorHighSNR(unsigned char x)
{
	double t = x/255 - 0.8627;
	t *= t; // ^2
	t *= t; // ^4
	t *= t; // ^8
	return 1.0 - x;
}

/** compute output image when given source images */
bool weightedAverageOfImageFiles(const vector<ImageImportInfo> &inputInfo,
									int width, int height,
									const vector<FImagePtr> &weights,
									FRGBImage *output,
									BImage *mask)
{
	if(g_verbose > 0) {
		cout << "Merging input images" << endl;
	}
	
	assert(inputInfo.size() == weights.size());
	output->resize(width, height);
	mask->resize(width, height);
	
	for(unsigned i = 0; i < inputInfo.size(); i++) {
		const ImageImportInfo curr_info = inputInfo.at(i);
		Rect2D bounds(Point2D(curr_info.getPosition()), curr_info.size());
		FRGBImage img(inputInfo.at(i).size());
		BImage tmpMask(inputInfo.at(i).size());
		
		//load image
		importImageAlpha(inputInfo.at(i), destImage(img), destImage(tmpMask));
		
		//combine with weight
		combineTwoImages(srcImageRange(img), 
							srcImage(*(weights.at(i))),
							destImage(img), Arg1() * Arg2());
		//combine with output img
		combineTwoImages(srcImageRange(*output, bounds), srcImage(img), 
							destImage(*output, bounds.upperLeft()), 
							Arg1() + Arg2());
		//combine masks
		combineTwoImages(srcImageRange(*mask, bounds), srcImage(tmpMask),
							destImage(*mask), max(Arg2() + Arg1(), Param(255)));
	}
	return true;
}

/** merges images into a better memory layout 
return width of final image (can calc height from this)

output_bounds are needed to convert the vectors back into images
*/
int Fimages2Vectors(const vector<ImageImportInfo> &file_info, 
					const vector<BImagePtr> &alpha_images,
					const vector<FImagePtr> &input_images,
					vector<vector<float> > *return_image, 
					vector<Rect2D> *output_bounds)
{

	int width = 0;
	int height = 0;
	int total_layers = file_info.size();
	bool ignore_alpha = !(alpha_images.size());

	//error checking
	if((unsigned)total_layers != input_images.size())
			return -1;
				
	for(int i = 0; i < total_layers; i++) {
		ImageImportInfo f = file_info.at(i);
		int end_x = f.getPosition().x + f.width();
		int end_y = f.getPosition().y + f.height();
		
		if(end_x > width)
			width = end_x;
		if(end_y > height)
			height = end_y;
	}
	
	return_image->resize(width * height, vector<float>());
	
	for(int i = 0; i < total_layers; i++) {
		ImageImportInfo f = file_info.at(i);
		Diff2D pos = f.getPosition();
		assert(pos.x >= 0 && pos.y >= 0);
		Size2D size = f.size();
		float *data = (float *) input_images.at(i)->data();
		char *alpha;
		if(!ignore_alpha)
			alpha = (char *) alpha_images.at(i)->data();
		for(int y = 0; y < size.height(); y++) {
			int real_y = y + pos.y;
			for(int x = 0; x < size.width(); x++) {
				if(ignore_alpha || *(alpha + y * size.width() + x)) {
					int real_x = x + pos.x;
					return_image->
						at(real_y * width + real_x).push_back(
							*(data + y * size.width() + x));
				}
			} //end for x
		} //end for y
		if(output_bounds) {
			output_bounds->push_back(Rect2D(*(new Point2D(pos)), size));
		}
	}
	
	return width;
}

int Bimages2Vectors(const vector<ImageImportInfo> &file_info, 
						const vector<BImagePtr> &alpha_images,
						const vector<BImagePtr> &input_images,
						vector<vector<char> > *return_image, 
						vector<Rect2D> *output_bounds)
{
	int width = 0;
	int height = 0;
	int total_layers = file_info.size();
	bool ignore_alpha = !(alpha_images.size());

	//error checking
	if((unsigned)total_layers != input_images.size())
			return -1;
				
	for(int i = 0; i < total_layers; i++) {
		ImageImportInfo f = file_info.at(i);
		int end_x = f.getPosition().x + f.width();
		int end_y = f.getPosition().y + f.height();
		
		if(end_x > width)
			width = end_x;
		if(end_y > height)
			height = end_y;
	}
	
	return_image->resize(width * height, vector<char>());
	
	for(int i = 0; i < total_layers; i++) {
		ImageImportInfo f = file_info.at(i);
		Diff2D pos = f.getPosition();
		assert(pos.x >= 0 && pos.y >= 0);
		Size2D size = f.size();
		char *data = (char *) input_images.at(i)->data();
		char *alpha;
		if(!ignore_alpha)
			alpha = (char *) alpha_images.at(i)->data();
		for(int y = 0; y < size.height(); y++) {
			int real_y = y + pos.y;
			for(int x = 0; x < size.width(); x++) {
				if(ignore_alpha || *(alpha + y * size.width() + x)) {
					int real_x = x + pos.x;
					return_image->
						at(real_y * width + real_x).push_back(
							*(data + y * size.width() + x));
				}
			} //end for x
		} //end for y
		if(output_bounds) {
			output_bounds->push_back(Rect2D(*(new Point2D(pos)), size));
		}
	}
	
	return width;
}

/** converts from a vector of vectors to a vector of images */
void Fvectors2Images(const vector<Rect2D> &bounds,
						const vector<BImagePtr> &alpha_images,
						const vector<vector<float> > &input_images,
						const int width,
						vector<FImagePtr> *output_images)
{
	int total_layers = bounds.size();
	bool ignore_alpha = !(alpha_images.size());
	if(!ignore_alpha)
		assert((unsigned)total_layers == alpha_images.size());
	
	output_images->resize(total_layers);
	
	//duplicate input_images
	vector<vector<float> > input;
	for(unsigned i = 0; i < input_images.size(); i++) {
		input.push_back(vector<float>(input_images.at(i)));
	}
	
	//go in reverse order so can delete in constant time
	for(int i = total_layers - 1; i >= 0; i--) {
		FImagePtr img = FImagePtr(new FImage(bounds.at(i).size()));
		const unsigned char *alpha;
		if(!ignore_alpha)
			alpha = alpha_images.at(i)->data();
		
		for(int y = 0; y < img->height(); y++) {
			int src_y = y + bounds.at(i).top();
			for(int x = 0; x < img->width(); x++) {
				int src_x = x + bounds.at(i).left();
				
				//if this px not transparent
				if(ignore_alpha || *(alpha + y * img->width() + x)) {
					vector<float> *src = &(input.at(src_y * width + src_x));
					assert(src->size());
					//pop last layer from this px in input
					*((float *)img->data() + y * img->width() + x) = *(src->end()-1);
					src->pop_back();
				}
			}//end for x
		}//end for y
		
		output_images->at(i) = img;
	}
}

void Bvectors2Images(const vector<Rect2D> &bounds,
						const vector<BImagePtr> &alpha_images,
						const vector<vector<char> > &input_images,
						const int width,
						vector<BImagePtr> *output_images)
{
	int total_layers = bounds.size();
	int height = input_images.size() / width;
	bool ignore_alpha = !(alpha_images.size());
	if(!ignore_alpha)
		assert((unsigned)total_layers == alpha_images.size());
	
	for(int i = 0; i < total_layers; i++) {
		output_images->push_back(BImagePtr(new BImage(bounds.at(i).size())));
	}
	
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			Point2D pt(x, y);
			unsigned ind = 0;
			vector<char> pixel_layers = input_images.at(y * width + x);
			
			for(int i = 0; i < total_layers && ind < pixel_layers.size();	i++) {
				Rect2D img = bounds.at(i);
				if(img.contains(pt)) {
					int real_x = x - img.left();
					int real_y = y - img.top();
					
					if(ignore_alpha || *(alpha_images.at(i)->data() + 
						real_y * img.width() + real_x)) {
						
						char *realpt = (char *)(output_images->at(i)->data());
						realpt += real_y * img.width() + real_x;
						*realpt = input_images.at(y * img.width() + x).at(ind++);
					}
				}
			}//end for i
		}//end for x
	}//end for y
}
