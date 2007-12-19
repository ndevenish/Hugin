// -*- c-basic-offset: 4 -*-

/** @file khan.cpp
 *
 *  @brief Implementation of the khan deghosting algorithm
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
#include <cmath>
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

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define ENABLE_SAME		0 /*-a s option currently doesn't work */
#define ENABLE_SCALING	0 /*-a m and u options currently don't work */

using namespace std;
using namespace hugin_utils;
using namespace vigra;
using namespace vigra::functor;

extern int g_verbose;


bool khanMain(vector<string> inputFiles, FRGBImage & output, BImage &mask, 
				int num_iters, char save_mode, char adv_mode, char ui_mode)
{
    //////////////////////////////////////////////////////////////////////////
    // 1. SETUP. load and prepare images

    // log grayscale images
    std::vector<FImagePtr> grayImages;
	
	// transparency images
	vector<BImagePtr> alpha_images;

    // initial weights (maybe 8 bit would be enough for them?)
    std::vector<FImagePtr> imgWeights;
	
	//file infos for images
	vector<ImageImportInfo> exrInfo;
	vector<ImageImportInfo> grayInfo;
	string exr = "EXR";

	//load images and prepare initial weights
    for (unsigned i = 0; i < inputFiles.size(); i++) {
        if (g_verbose > 0) {
            std::cout << "Loading and preparing " << inputFiles[i] << std::endl;
        }
		
        // load image from disk
        vigra::ImageImportInfo info(inputFiles[i].c_str());
		const char *type = info.getFileType();

		if(!exr.compare(type)) { //if is aligned input file
			if(g_verbose >1)
				cout << "Loading source image: " << inputFiles[i].c_str() << endl;
			exrInfo.push_back(info);
			BImagePtr origGray(new BImage(info.size()));
			FRGBImage img(info.size());
			FImagePtr gray(new FImage(info.size()));

			// load image
			vigra::importImageAlpha(info, vigra::destImage(img), 
				destImage(*origGray));
			
			// convert image to grayscale (and take logarithm)
			vigra::RGBToGrayAccessor<ImageType::value_type> color2gray;
			transformImage(srcImageRange(img, color2gray),
						   destImage(*gray),
						   log(Arg1()+Param(1.f)));
			
			// store for later use
			grayImages.push_back(gray);
			if(!(adv_mode & ADV_ALPHA))
				alpha_images.push_back(origGray);

			// for debugging purposes, save floating point tiff files
			if (save_mode & SAVE_SOURCES) {
				char tmpfn[100];
				snprintf(tmpfn, 99, "debug_init_gray_%d.tiff", i);
				ImageExportInfo exGray(tmpfn);
				exportImage(srcImageRange(*gray), exGray.setPixelType("UINT8"));
				if(!(adv_mode & ADV_ALPHA)) {
					snprintf(tmpfn, 99, "debug_init_alpha_%d.tiff", i);
					exGray = ImageExportInfo(tmpfn);
					exportImage(srcImageRange(*origGray), exGray.setPixelType("UINT8"));
				}
			}
		}
		else { //if is a grayscale luminance file
			if(g_verbose > 1)
				cout << "Loading luminance image: " << inputFiles[i].c_str() << endl;
			
			grayInfo.push_back(info);
			
			//if calculating weights from scratch
			if(!(ui_mode & UI_IMPORT_INIT_WEIGHTS) ||
					(ui_mode & UI_EXPORT_INIT_WEIGHTS)) {
				//load image
				BImagePtr origGray(new BImage(info.size()));
				vigra::importImage(info, destImage(*origGray));
				FImagePtr weight(new FImage(info.size()));
				
				// calculate initial weights, using mexican hat function
				transformImage(srcImageRange(*origGray),
							   destImage(*weight),
							   &weightMexicanHat);
				
				if(adv_mode & ADV_SNR) {
					if(g_verbose > 1)
						cout << "favoring high snr..." << endl;
					
					FImagePtr snrWeight(new FImage(info.size()));
					transformImage(srcImageRange(*origGray), destImage(*snrWeight),
									&favorHighSNR);
					
					//weight(x) = weightMexicanHat(x) * favorHighSNR(x)
					combineTwoImages(srcImageRange(*weight), srcImage(*snrWeight),
										destImage(*weight), Arg1() * Arg2());
				}
		
				imgWeights.push_back(weight);
	
				// for debugging purposes, save floating point tiff files
				if (save_mode & SAVE_WEIGHTS) {
					char tmpfn[100];
					snprintf(tmpfn, 99, "debug_init_weights_%d.tiff", i);
					ImageExportInfo exWeights(tmpfn);
					exportImage(srcImageRange(*weight), exWeights.setPixelType("UINT8"));
				}
			}
		}
    }
	
	if(ui_mode & UI_EXPORT_INIT_WEIGHTS) {
		if(!saveImages(inputFiles, "iw", imgWeights))
				cerr << "Cannot export initial weights" << endl;
		else if(g_verbose > 0)
			cout << "Saved initial weights in source folder" << endl;
	}
	
	if(ui_mode & UI_IMPORT_INIT_WEIGHTS) {
		string s;
		if(ui_mode & UI_EXPORT_INIT_WEIGHTS) { //just exported weights
			while(true) {
				cout << "Enter 'c' to load initial weights from source folder" << endl
					<< "Enter 'q' to cancel loading initial weights: ";
				cin >> s;
				
				if(s[0] == 'c' || s[0] == 'q')
					break;
			}
		}

		if(!s.length() || s[0] == 'c') {
			vector<FImagePtr> tmpWeights;
			if(!loadImages(inputFiles, "iw", &tmpWeights)) {
				cerr << "Cannot import initial weights" << endl;
				if(!imgWeights.size())
					return false;
			}
			else {
				imgWeights = tmpWeights;
				if(g_verbose > 0)
					cout << "Loaded initial weights from source folder" << endl;
			}
		}
	}

    //////////////////////////////////////////////////////////////////////////
    // 2. Estimation of weights

	// images used to compute weight
	vector<vector<float> > source_images;
	// final weights
	vector<vector<float> > weights;
	// alpha mask
	vector<vector<char> > alpha;

	//pos+size info for input images
	vector<Rect2D> img_bounds;
	
	//size of final composite
	int width, height;
	
	if(g_verbose > 1)
		cout << "Remappin input images" << endl;

	if((width = Fimages2Vectors(exrInfo, 
		alpha_images, grayImages, &source_images, &img_bounds)) == -1) {
		
		cerr << "Error converting images to vectors" << endl;
		abort();
	}
		
	if(!((adv_mode & ADV_JBU) || (adv_mode & ADV_MULTI))) {
		if(g_verbose > 1)
			cout << "Clearing gray input images" << endl;
		grayImages.clear();
	}
	height = source_images.size()/width;
	

	if(Fimages2Vectors(exrInfo, alpha_images, imgWeights, &weights) == -1) {
		cerr << "Error converting images to vectors" << endl;
		cerr << "Perhaps *_gray.pgm files missing from command line?" << endl;
		abort();
	}
	
	if(g_verbose > 3) { //check consistency
		cout << "checking image and weight vectors for consistent size" <<endl;
		assert(source_images.size() == weights.size());
		for(unsigned i = 0; i < source_images.size(); i++)
			assert(source_images.at(i).size() == weights.at(i).size());
	}
	
	if(!((adv_mode & ADV_JBU) || (adv_mode & ADV_MULTI))) {
		if(g_verbose > 1)
			cout << "Clearing initial weight images" << endl;
		imgWeights.clear();
	}
	
    if (g_verbose > 0) {
        std::cout << "deghosting (Khan algorithm) " << std::endl;
    }

	int bias = 5; //hat function coefficient to make "wrong" pixels less weighted
					//min value 1
	int rad_neighbors = 1;
#if ENABLE_SCALING
	int jbu_neighbors = 2;
#endif
	vector<vector<float> > init_weights(weights);
	
	if(g_verbose > 0)
		std::cout << "deghosting..." << std::endl;

	//variables used for multi-scaling
	int proportion = 1;
	int nw, nh;
	vector <FImagePtr> smallGrayImages;
	vector <FImagePtr> smallInitWeights;	
	vector<FImagePtr> smallWeights;
	vector<BImagePtr> smallAlphaImages;
							
	//for each iteration
	for(int iter = 0; iter < num_iters; iter++) {
		
		if(g_verbose > 0)
			std::cout << "\n\niteration " << iter << endl;
		
		if(adv_mode & ADV_JBU || adv_mode & ADV_MULTI) {
			proportion = 1 << ((num_iters - iter)/2);
			nw = width / proportion;
			nh = height / proportion;
		}
		
		try {
		#if ENABLE_SCALING
			//small image
			if((proportion > 1)) {
				//diff than last size,  need to resize
								
				if(!smallGrayImages.size() || 
					smallGrayImages[0]->width() != 
					img_bounds[0].width() / proportion) { 
					if(g_verbose > 1)
						std::cout << "resizing to 1/" << proportion << " size" << endl;
					
					//don't use jbu if smaller
					bool smaller = !smallGrayImages.size() ||
									(smallGrayImages[0]->width() >
									img_bounds[0].width() / proportion);
					
					weights.clear();
					source_images.clear();
					init_weights.clear();
					smallWeights.clear();

					if(smaller) { //1st time, need to initialize
						smallGrayImages.resize(img_bounds.size());
						smallInitWeights.resize(img_bounds.size());
						smallAlphaImages.resize(img_bounds.size());
						Fvectors2Images(img_bounds, alpha_images, weights, width,
									&smallWeights);
					}
					else {
						Fvectors2Images(img_bounds, smallAlphaImages, weights, 
							width * (proportion >> 1), &smallWeights);
							// ^-- width of last iter
					}
									
					assert(smallWeights.size() == img_bounds.size());
											
					for(unsigned i = 0; i < smallWeights.size(); i++) {
						int w = img_bounds.at(i).width();
						int h = img_bounds.at(i).height();
						
						FImagePtr smallGray(new FImage(w, h));
						FImagePtr newInitWeight(new FImage(w, h));
						BImagePtr smallAlpha(new BImage(w, h));
						//resize input img
						resizeImageLinearInterpolation(
							srcImageRange(*(grayImages.at(i))),
							destImageRange(*smallGray));
						smallGrayImages.at(i) = smallGray;
						
						//resize init_weights
						resizeImageLinearInterpolation(
							srcImageRange(*(imgWeights.at(i))),
							destImageRange(*newInitWeight));
						smallInitWeights.at(i) = newInitWeight;
						
						//resize alpha images
						if(!(adv_mode & ADV_ALPHA)) {
							resizeImageLinearInterpolation(
								srcImageRange(*(alpha_images.at(i))),
								destImageRange(*smallAlpha));
							smallAlphaImages.at(i) = smallAlpha;
						}
						
						//resize weights
						if(smaller && (adv_mode & ADV_JBU)) {
							smallWeights.at(i) = jbuImage(smallWeights.at(i), 
													smallGray, jbu_neighbors);
						}
						else {
							FImagePtr newWeight(new FImage(w, h));
							resizeImageLinearInterpolation(
								srcImageRange(*(smallWeights.at(i))),
								destImageRange(*newWeight));
							//replace old weights w/ new weights
							smallWeights.at(i) = newWeight;
						}

					} //end for i
					
					//convert everything to vectors
					if(Fimages2Vectors(exrInfo, alpha_images, smallWeights,
						&weights) == -1)
						abort();
					if(Fimages2Vectors(exrInfo, alpha_images, smallGrayImages,
						&source_images) == -1)
						abort();
					if(Fimages2Vectors(exrInfo, alpha_images, smallInitWeights,
						&init_weights) == -1)
						abort();
				
				} // end if != ns
					
				khanIteration(source_images, nh, nw, &weights, init_weights, 
					rad_neighbors, adv_mode);
			} // end if proportion
			else {//if just computing normal sized
				if(smallWeights.size() &&
					smallWeights[0]->width() != img_bounds[0].width()) { //nw == width
					if(g_verbose > 1) {
						std::cout << "resizing to 1/1" << " size" << endl;
					}
					
					//delete cache for smaller layers to save memory
					smallGrayImages.clear();
					smallInitWeights.clear();
					smallAlphaImages.clear();

					weights.clear();
					source_images.clear();
					init_weights.clear();
					smallWeights.clear();
					Fvectors2Images(img_bounds, alpha_images, weights, width,
									&smallWeights);
					
					for(unsigned i = 0; i < smallWeights.size(); i++) {
						int w = img_bounds.at(i).width();
						int h = img_bounds.at(i).height();
						//resize weights
						FImagePtr newWeight(new FImage(w, h));
						resizeImageLinearInterpolation(
							srcImageRange(*(smallWeights.at(i))),
							destImageRange(*newWeight));
						//replace old weights w/ new weights
						smallWeights.at(i) = newWeight;
					}//end for
					
					//convert everything to vectors
					if(Fimages2Vectors(exrInfo, alpha_images, smallWeights,
						&weights) == -1)
						abort();
					smallWeights.clear();
					
					if(Fimages2Vectors(exrInfo, alpha_images, grayImages,
						&source_images) == -1)
						abort();
					grayImages.clear();
					
					if(Fimages2Vectors(exrInfo, alpha_images, imgWeights,
						&init_weights) == -1)
						abort();
					imgWeights.clear();
				}//end if	
		#endif
				khanIteration(source_images, height, width, &weights, 
					init_weights, rad_neighbors, adv_mode);
		#if ENABLE_SCALING
			}
		#endif
		}
		catch(std::exception &e) {
			cerr << "caught exception in khanMain(1): " << e.what() << endl;
			abort();
		}

		vector<FImagePtr> w;
		//save files for debugging purposes
		if(save_mode & SAVE_WEIGHTS) {
			if(g_verbose > 1)
				std::cout << "saving debug weights..." << endl;
			
			Fvectors2Images(img_bounds, alpha_images, weights, width, &w);
			
			char tmpfn[100];
			for(unsigned i = 0; i < w.size(); i++) {
				snprintf(tmpfn, 99, "iter%d_weight_layer%d.tiff", iter, i);
				ImageExportInfo exWeight(tmpfn);
				exportImage(srcImageRange(*(w.at(i))), exWeight.setPixelType("UINT8"));
			}
		}
		if((save_mode & SAVE_RESULTS)) {
			if(g_verbose > 1)
				std::cout << "saving debug result..." << endl;

			char tmpfn[100];
			snprintf(tmpfn, 99, "iter%d_result.exr", iter);
			FRGBImage debug_out;
			BImage debug_mask;
			
			if(!w.size())
				Fvectors2Images(img_bounds, alpha_images, weights, width, &w);

			#if ENABLE_SCALING
			//need to upscale weights 1st			
			if(adv_mode & ADV_JBU) {
				jointBilateralUpsampling(&weights, &grayImages, &w, 
					jbu_neighbors);
			}
			elseif(adv_mode & ADV_MULTI) {
				for(int i = 0; i < num_layers; i++) {
					FImagePtr tmpW(new FImage(width, height));
					resizeImageLinearInterpolation(
						srcImageRange(*(w.at(i))),
						destImageRange(*tmpW));
					tmpWeights.push_back(tmpW);
				}
			}
			#endif
			
			vector<FImagePtr> w;
			Fvectors2Images(img_bounds, alpha_images, weights, width, &w);
			weightedAverageOfImageFiles(exrInfo, width, height, w, &debug_out,
											&debug_mask);
			ImageExportInfo exinfo(tmpfn);
			exportImageAlpha(srcImageRange(debug_out), srcImage(debug_mask), exinfo);
		}
	} //end iteration loop
	
	#if ENABLE_SCALING
	if(weights[0]->width() < grayImages[0]->width()) {
			if(g_verbose > 1)
				cout << "performing final joint bilateral upsampling" << endl;
			jointBilateralUpsampling(&weights, &grayImages, &weights, 
										jbu_neighbors);
	}
	#endif

	if(adv_mode & ADV_BIAS) {
		//bias and renormalize weights
		if(g_verbose > 0) {
			std::cout << "\nadjusting weights" << endl;
		}
		for(int i = 0; i < height; i++) {
			int y_offset = i * width;
			for(int j = 0; j < width; j++) {
				int total_offset = y_offset + j;
				float sum = 0;
				vector<float> *tmp_weights = &(weights.at(total_offset));
				int num_layers = tmp_weights->size();
				
				//cout <<"raar" << endl;
				for(int layer = 0; layer < num_layers; layer++) {
					float tmp = tmp_weights->at(layer);
						tmp = pow(tmp, bias);
					sum += tmp;
					tmp_weights->at(layer) = tmp;
				} 
				
				float avg = 1/(float)num_layers;
				for(int layer = 0; layer < num_layers; layer++) {
					tmp_weights->at(layer) =
						(sum ? tmp_weights->at(layer) / sum : avg);
				} 
			} // end for j
		} // end for i
	}
	else if(adv_mode & ADV_UNAVG) {
		//make largest weight = 1, rest 0
		if(g_verbose > 0)
			std::cout << "\nadjusting weights" << endl;
		
		FImagePtr w;
		
		if(save_mode & SAVE_WEIGHTS)
			w = FImagePtr(new FImage(Size2D(width, height)));
		
		for(int i = 0; i < height; i++) {
			int y_offset = i * width;
			for(int j = 0; j < width; j++) {
				vector<int> max_ind;
				float max = 0; //consider everything w/in 1% of max as max
				int total_offset = y_offset + j;
				vector<float> *tmpWeights = &(weights.at(total_offset));
				int num_layers = tmpWeights->size();
				
				for(int layer = 0; layer < num_layers; layer++) {
					float tmp = tmpWeights->at(layer);
					if(tmp > max + max * 0.01) {
						//make prev max 0
						while(!max_ind.empty()) {
							tmpWeights->at(layer) = 0;
							max_ind.pop_back();
						}
						//make curr 1
						tmpWeights->at(layer) = 1;

						max = tmp;
						max_ind.push_back(layer);
					}
					else if(tmp > max - max * 0.01) {
						max_ind.push_back(layer);
					}
					else { //make curr 0
						tmpWeights->at(layer) = 0;
					}
				}//end for layer
				
				if((save_mode & SAVE_WEIGHTS) && max_ind.size() == 1) {
					*(float *)(w->data() + total_offset) = 
						(max_ind[0] + 1) / (float)num_layers;
				}				
			} //end for j
		} // end for i
		
		if(save_mode & SAVE_WEIGHTS) { //save labels
			if(g_verbose > 1)
				cout << "saving weight labels..." << endl;
			
			ImageExportInfo exWeight("weight_labels.tiff");
			exportImage(srcImageRange(*w), exWeight.setPixelType("UINT8"));
		}
	}

    // apply weights and store result in output image
	vector<FImagePtr> w;
	Fvectors2Images(img_bounds, alpha_images, weights, width, &w);
	weights.clear();
	alpha_images.clear();
    weightedAverageOfImageFiles(exrInfo, width, height, w, &output, &mask);
    return true;
}//end khanMain

/** writes the new weights in the given weights object pointer
 */
 
 void khanIteration(const std::vector<std::vector<float> > &source_images, 
					 const int height, const int width,
					 std::vector<std::vector<float> > *weights, 
					 const std::vector<std::vector<float> > &init_weights, 
					 const int rad_neighbors, const char adv_mode)
{
	try {
		if(g_verbose > 0)
			std::cout << "processing..." << endl;
		
		if(g_verbose > 3)
			cout << "copying prev weights" << endl;
		
		vector<vector<float> > old_weights = *weights;
		
		//for each row
		for(int row = 0; row < height; row++) {
			int row_offset = row * width;
	
			if(g_verbose ==2 || g_verbose == 3) {
				if(row % 100) {
					if(g_verbose == 3 && !(row % 10))
						cout << "." << flush;
				}
				else
					cout << "\nrow " << row << " to " << (row +100) << flush;
			}
			
			//for each column
			for (int col = 0; col < width; col++) {
				int total_offset = col + row_offset;
				
				if(g_verbose > 3)
					cout << "(x, y) = (" << col << ", " << row << ")" << endl;

				//get descriptors
				vector<int> neighbor_offsets;
				khanNeighbors(&neighbor_offsets, col, row, width, height, rad_neighbors);
				
				float total_sum = 0;
				std::vector<float> raw_weights;
				float max_weight = 0;
				float min_weight = FLT_MAX;
				
				int num_layers = old_weights.at(total_offset).size();
				assert((unsigned)num_layers == source_images.at(total_offset).size());
				//for each layer
				for(int layer = 0; layer < num_layers; layer++) {
					if(g_verbose > 3) {
						cout << "layer " << layer << endl;
					}
	
					//initialize sum to 0
					float sum_weight = 0;
					float sum_prev_weight = 0;
					//get center pixel
					float curr_px = source_images.at(total_offset).at(layer);
					
					if(g_verbose >3)
						cout << "\tpixel = " << curr_px << endl;
					//for each neighbor location
					for(unsigned n = 0; n < neighbor_offsets.size(); n++) {
						int n_offset = neighbor_offsets.at(n);
						vector<float> *px_neighbor_w = &(old_weights.at(n_offset));
						const vector<float> *px_neighbor_v = 
							&(source_images.at(n_offset));
						unsigned num_l = px_neighbor_w->size();
						assert(num_l == px_neighbor_v->size());
						float tmp_weight = 0;
						float tmp_prev_weight = 0;
						
						if(g_verbose > 3) {
							cout << "\t\tneighbor " << n << endl
							<< "\t\t\toffset: " << n_offset << endl;
						}
						
						//number of layers at this neighbor location
						for(unsigned l = 0; l < num_l; l++) {
							
							if(g_verbose > 3) {
								cout << "\t\t\tlayer " << l << endl
								<< "\t\t\tvalue = " << px_neighbor_v->at(l) << endl
								<< " \t\t\tweight = " << px_neighbor_w->at(l) 
								<< endl;
							}
							
							//diff against center
							float val = curr_px - px_neighbor_v->at(l);

							//gaussian
							val = std::exp(val * val * -0.5) / sqrt(2 * 3.14159);
							//multiply by prev weight
							val *= px_neighbor_w->at(l);
							
							if(g_verbose > 3) {
								cout << "\t\t\tresult = " << val << endl;
							}
							//add to sum
							tmp_prev_weight += px_neighbor_w->at(l);
							tmp_weight += val;
						} //end l loop
						if(num_l) {
							//make sure each position gets same amount of weight
							//regardless of number of layers
							sum_prev_weight += tmp_prev_weight / num_l;
							sum_weight += tmp_weight / num_l;
						}
					} //end neighbor loop
						
					if(sum_prev_weight == 0) {
						sum_prev_weight = 1;
					}
					
					sum_weight = sum_weight / sum_prev_weight * 
									init_weights.at(total_offset).at(layer);
						
					
					#if ENABLE_SAME
					if(adv_mode &ADV_SAME) {
						//sum of neighbors (on same img) weights
						float sum_neighbor_weights = 0;
						for(int n = 0; n < num_neighbors / num_layers; n++) {
							try {
							sum_neighbor_weights +=
								*(old_weights.at(layer)->data() + neighbor_ptr_vec.at(n));
							}
							catch(std::exception &e) {
								abort();
							}
						}
						sum_weight *= sum_neighbor_weights;
					}
					#endif
					
					if(sum_weight > max_weight)
						max_weight = sum_weight;
					if(sum_weight < min_weight)
						min_weight = sum_weight;
						
					//store in weights
					raw_weights.push_back(sum_weight);
					total_sum += sum_weight;
					if(g_verbose > 3) {
						cout << "\t\traw weight = " << sum_weight << endl;
					}
					
				} //end layer loop
				/*
				cout << "maxweigt: " << max_weight << endl;
				cout << "minweight: " << min_weight << endl;
				*/
				//if diff is too small, just choose one with the heighest weight
				if((adv_mode & ADV_UNAVG2) &&
					(min_weight >= 0.9 * max_weight)) {
					bool tmp = true;
					for(int layer = 0; layer < num_layers; layer++) {
						if(tmp && raw_weights.at(layer) == max_weight) {
							raw_weights.at(layer) = 1;
							tmp = false;
						}
						else
							raw_weights.at(layer) = 0;
					}
					
					weights->at(total_offset) = raw_weights;
				}
				else if(total_sum) {
					//normalize weights
					for(int layer = 0; layer < num_layers;	layer++) {
						weights->at(total_offset).at(layer) =
							raw_weights.at(layer) / total_sum;
					}
				}
				else if(num_layers) {
					float avg = 1 / (float)num_layers;
					for(int layer = 0; layer < num_layers; layer++) {
						weights->at(total_offset).at(layer) = avg;
					}
				}
			} //end column loop
		} //end row loop
		
		cout << "\n";
	}
	catch(std::exception &e) {
		cerr << "caught exception in khanIteration: " << e.what() << endl;
		abort();
	}
}// end khanIteration


inline void khanNeighbors(vector <int> *neighbors, //return value
	int x, int y, int width, int height, int rad_neighbors)
{
	try {
		neighbors->clear();
		
		for(int v = y - rad_neighbors; v < y + rad_neighbors + 1; v++) {
		
			int v_offset = (v < 0 ? 0 : (v >= height ? height-1: v)) * width;
			
			for(int h = x - rad_neighbors;	h < x + rad_neighbors + 1; h++) {
				int h_offset = (h < 0 ? 0 : (h >= width ? width-1 : h));
				neighbors->push_back(h_offset + v_offset);
			}//end h
		}//end v
		
		if(g_verbose > 4) {
			cout << "got " << neighbors->size() << " neighbors from " << width
				<< " x " << height << " image" << endl;
		}
	}
	catch(std::exception &e) {
		cerr << "caught exception in khanNeighbors: " << e.what() << endl;
		abort();
	}
}
