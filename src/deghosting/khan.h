// -*- c-basic-offset: 4 -*-

/** @file khan.h
 *
 *  @author Jing Jin
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

//#define NDEBUG

#include <boost/shared_ptr.hpp>

#include <vigra/error.hxx>
#include <hugin_utils/utils.h>
#include <vigra_ext/impexalpha.hxx>
#include "jbu.h"

// define the types of images we will use.
// use float for RGB
typedef vigra::FRGBImage ImageType;
// use byte for original grey value in source image
typedef vigra::BImage WeightImageType;

// smart pointers to the images.
typedef boost::shared_ptr<ImageType> ImagePtr;
typedef boost::shared_ptr<vigra::FImage> FImagePtr;
typedef boost::shared_ptr<vigra::BImage> BImagePtr;

// advanced calculation modes
#define ADV_BIAS	1
#define ADV_MULTI	2
#define ADV_SAME	4
#define ADV_SNR		8
#define ADV_JBU		16
#define ADV_UNAVG	32
#define ADV_UNAVG2	64
#define ADV_ALPHA	128
#define ADV_NOLUM	256
#define ADV_ALL		512

// save modes
#define SAVE_WEIGHTS	1
#define SAVE_RESULTS	2
#define SAVE_SOURCES	4
#define SAVE_ALL		255

// custom user input modes
#define UI_IMPORT_INIT_WEIGHTS 1
#define UI_EXPORT_INIT_WEIGHTS 2

/** Mexican hat function, eq. 5 from Khan paper */
float weightMexicanHat(unsigned char x);
/** Functor that returns large values for inputs near 220 */
float favorHighSNR(unsigned char x);

/** compute output image, apply weights to input images.
 *  Tries to save memory by loading the images one by one
 * @param inputFiles	Import information for the images to be merged
 * @param width			The width of the composite image
 * @param height		The height of the composite image
 * @param weights		Weights to apply to each image loaded with inputFiles
 * @param output_image	The composite image
 * @param mask			The alpha channel of the composite image
 */
bool weightedAverageOfImageFiles(const std::vector<vigra::ImageImportInfo> &inputFiles,
									int width, int height,
									const std::vector<FImagePtr> &weights,
									vigra::FRGBImage *output_image,
									vigra::BImage *mask);

/** Main function of the Khan algorithm.
 */
bool khanMain(std::vector<std::string> inputFiles, vigra::FRGBImage & output,
				vigra::BImage & mask, int num_iters, char save_mode,
				const unsigned int adv_mode, char ui_mode);

/** Gets neighbor pixel offsets for a given pixel and image size
 */
inline void khanNeighbors(std::vector<int> *neighbors, int x, int y,
	 				int width, int height, int rad_neighbors);

/** Executes one iteration of the khan algorithm with the given image
  */
 void khanIteration(const std::vector<std::vector<float> > &srcImages,
					 const int height, const int width,
					 std::vector<std::vector<float> > *weights,
					 const std::vector<std::vector<float> > &init_weights,
					 const int rad_neighbors, const unsigned int adv_mode);

/** Save images given the template array and image array returns true if
  * successful, false (and prints error) on error
  */
bool saveImages(std::vector<std::string> prep, std::string app,
				  const std::vector<FImagePtr> &images);

/** Loads images given the template array and an empty image array. returns true if
  * successful, false (and prints error) on error
  */
bool loadImages(std::vector<std::string> prep, std::string app,
				  std::vector<FImagePtr> *images);

/** remaps a vector of FImages or BImages to a vector of vector of floats or
  * chars, respectively, for faster reads/writes
  * @param file_info	Objects containing the images' offset information
  * @param alpha_images	BImages that indicate transaparency of input images,
  *						in the same order as the input images
  * @param input_images	Input images to be remapped
  * @param return_images Places to store remapped input_images
  * @param output_bounds Rectangles that store each image's offset and size
  * @return The width of the composite image
  */
int Fimages2Vectors(const std::vector<vigra::ImageImportInfo> &file_info,
						const std::vector<BImagePtr> &alpha_images,
						const std::vector<FImagePtr> &input_images,
						std::vector<std::vector<float> > *return_image,
						std::vector<vigra::Rect2D> *output_bounds = NULL);
int Bimages2Vectors(const std::vector<vigra::ImageImportInfo> &file_info,
						const std::vector<BImagePtr> &alpha_images,
						const std::vector<BImagePtr> &input_images,
						std::vector<std::vector<char> > *return_image,
						std::vector<vigra::Rect2D> *output_bounds = NULL);

/** reverses effect of F/Bimages2Vectors: remaps a vector of vector of
  * floats or chars back to FImages or BImages, respectively
  * @param bounds		Rectangles that store each image's offset and size
  * @param alpha_images	BImages that indicate transparency of output images
  * @param input_images	Input vector to be remapped
  * @param width		The width of the composite image represented by
  *						input_images
  * @param output_images Images converted from input_images
  */
void Fvectors2Images(const std::vector<vigra::Rect2D> &bounds,
						const std::vector<BImagePtr> &alpha_images,
						const std::vector<std::vector<float> > &input_images,
						const int width, std::vector<FImagePtr> *output_images);
void Bvectors2Images(const std::vector<vigra::Rect2D> &bounds,
						const std::vector<BImagePtr> &alpha_images,
						const std::vector<std::vector<char> > &input_images,
						const int width,
						std::vector<BImagePtr> *output_images);


