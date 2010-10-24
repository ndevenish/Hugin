/***************************************************************************
 *   Copyright (C) 2008 by Tim Nugent
 *   timnugent@gmail.com
 *
 *   This file is part of hugin.
 *
 *   Hugin is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 * 
 *   Hugin is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with Hugin  If not, see <http://www.gnu.org/licenses/>.
 *
 ***************************************************************************/

#ifndef __CELESTE__
#define __CELESTE__

#include <hugin_shared.h>
#include "svm.h"
#include <string>
#include <vector>
#include <vigra/stdimage.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra_ext/utils.h>
#include <panodata/Panorama.h>

using namespace std;

namespace celeste
{

/** loads the SVM model from file
 *  @param model struct, which stores the SVM informations, if you don't need it, call destroySVMmodel
 *  @param model_file filename of model file to load
 *  @return true, if loading of SVM file was sucessfull
 */
CELESTEIMPEX bool loadSVMmodel(struct svm_model*& model, string& model_file);
/** frees the resource of model
 *  @param model SVM model struct to freeing 
 */
CELESTEIMPEX void destroySVMmodel(struct svm_model*& model);

/** calculates the mask using SVM
 *  @param model struct svm_model which is used
 *  @param input input image
 *  @param radius radius for calculation of mask, smaller value results in higher resolution but longer calculation time
 *  @param threshold thresold value, lower value will reject more values
 *  @param resize_dimension the image is resized for calculation that the bigger dimension is small than resize_dimension
 *  @param adaptThreshold if true then the threshold is changed, if the all values are higher than threshold
 *  @return vigra::BImage, which contains the masks
 */
CELESTEIMPEX vigra::BImage getCelesteMask(struct svm_model* model, vigra::UInt16RGBImage& input, int radius, float threshold, int resize_dimension,bool adaptThreshold=false,bool verbose=true);

CELESTEIMPEX HuginBase::UIntSet getCelesteControlPoints(struct svm_model* model, vigra::UInt16RGBImage& input, HuginBase::CPointVector cps, int radius, float threshold, int resize_dimension,bool verbose=true);

/** converts the given image to UInt16RGBImage
 *  only this image is correctly processed by celeste
 *  @param src input image
 *  @param origType pixel type of input image
 *  @param dest converted image
 */
template <class SrcIMG>
void convertToUInt16(SrcIMG & src, const std::string & origType, vigra::UInt16RGBImage & dest)
{
    dest.resize(src.size());
    long newMax=vigra_ext::getMaxValForPixelType("UINT16");
    // float needs to be from min ... max.
    if (origType == "FLOAT" || origType == "DOUBLE")
    {
        /** @TODO this convert routine scale the input values range into the full scale of UInt16
         *  this is not fully correct
         */
        vigra::RGBToGrayAccessor<vigra::RGBValue<float> > ga;
        vigra::FindMinMax<float> minmax;   // init functor
        vigra::inspectImage(srcImageRange(src, ga),
                            minmax);
        double minVal = minmax.min;
        double maxVal = minmax.max;
        vigra_ext::applyMapping(srcImageRange(src), destImage(dest), minVal, maxVal, 0);
    }
    else
    {
        vigra::transformImage(srcImageRange(src), destImage(dest),
            vigra::functor::Arg1()*vigra::functor::Param( newMax/ vigra_ext::getMaxValForPixelType(origType)));
    };
}


}
#endif

