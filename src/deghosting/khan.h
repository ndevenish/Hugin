
/**
 * Header file for Khan's deghosting algorithm
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KHAN_H_
#define KHAN_H_

#include "deghosting.h"
// for AlgTinyVector, NormalizeFunctor and LogarithmFunctor
#include "support.h"
#include "algtinyvector.h"

// needed for RGB2Lab
#include <vigra/imageinfo.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/colorconversions.hxx>

// for resampleImage
#include <vigra/resizeimage.hxx>

// for RGBvalue used in hat function
#include <vigra/rgbvalue.hxx>

// needed for Kh()
#define PI 3.14159265358979323846

// number of pixels to look at in all directions
// ie. 1 for neighbourhood of size 3x3, 2 for 5x5 etc.
#define NEIGHB_DIST 1

#if defined WIN32
    #define snprintf _snprintf
#endif
// define for use atan based kernel function
// leave undefined for gaussian normal distribution function
//#define ATAN_KH

#ifdef DEGHOSTING_CACHE_IMAGES
    #include <cstring>
    #include <vigra/cachedfileimage.hxx>
#endif

using namespace vigra;

namespace deghosting
{
    template <class PixelType>
    class ImageTypes {
        public:
            typedef vigra::FImage ImageType;
            typedef boost::shared_ptr<ImageType> ImagePtr;
            #ifdef DEGHOSTING_CACHE_IMAGES
                typedef CachedFileImage<float> ProcessImageType;
            #else
                typedef BasicImage<float> ProcessImageType;
            #endif
            typedef boost::shared_ptr<ProcessImageType> ProcessImageTypePtr;
    };

    template <class PixelType>
    class ImageTypes<RGBValue<PixelType> > {
        public:
            typedef vigra::FRGBImage ImageType;
            typedef boost::shared_ptr<ImageType> ImagePtr;
            #ifdef DEGHOSTING_CACHE_IMAGES
                typedef CachedFileImage<AlgTinyVector<float, 3> > ProcessImageType;
            #else
                typedef BasicImage<AlgTinyVector<float, 3> > ProcessImageType;
            #endif
            typedef boost::shared_ptr<ProcessImageType> ProcessImageTypePtr;
    };

    template <class PixelType>
    class Khan : public Deghosting, private ImageTypes<PixelType>
    {
        public:
            Khan(std::vector<std::string>& inputFiles, const uint16_t flags, const uint16_t debugFlags, int iterations, double sigma, int verbosity);
            Khan(std::vector<ImageImportInfo>& inputFiles, const uint16_t flags, const uint16_t debugFlags, int iterations, double sigma, int verbosity);
            std::vector<FImagePtr> createWeightMasks();
            ~Khan() {}
        protected:
            typedef typename ImageTypes<PixelType>::ImageType ImageType;
            typedef typename ImageTypes<PixelType>::ProcessImageType ProcessImageType;
            typedef typename ImageTypes<PixelType>::ProcessImageType::traverser ProcessImageTraverser;
            typedef typename ImageTypes<PixelType>::ProcessImageType::PixelType ProcessImagePixelType;
            typedef typename ImageTypes<PixelType>::ProcessImageTypePtr ProcessImageTypePtr;
            typedef typename NumericTraits<PixelType>::isScalar srcIsScalar;
            
            // Kh() things
            // (2*pi)^(1/2)
            double PIPOW;
            // 1/Kh denominator
            double denom;
            // sigma in gauusian density function
            double sigma;
            
            // other necessary stuff
            std::vector<ProcessImageTypePtr> processImages;
            std::vector<FImagePtr> weights;
            
            /** set sigma
             * sets sigma for gaussian weigting function
             */
            void setSigma(double sigma);
            
            /** transform image using EMoR response
             * @param inputFile filename of image to be transformed
             * @param *pInputImg FRGBImage to be transformed
             */
            //void linearizeRGB(std::string, FRGBImage* pInputImg);
            
            /** kernel function
             * Standard probability density function
             */
            inline float Kh(ProcessImagePixelType x);
            
            /** convert image for internal use
             * if input image is RGB then convert it to L*a*b
             * if input image is grayscale then only copy image
             */
            void convertImage(ImageType * in, ProcessImageTypePtr & out, VigraFalseType);
            void convertImage(ImageType * in, ProcessImageTypePtr & out, VigraTrueType);
            
            /** import RGB image
             */
            void importRGBImage(ImageImportInfo & info, ImageType * img, VigraFalseType);
            void importRGBImage(ImageImportInfo & info, ImageType * img, VigraTrueType);
            
            /** function to preprocess input image
             * This function loads image, linearize it using EMoR (FIXME),
             * tranform it using logarithm or gamma if input images are HDR
             */
            void preprocessImage(unsigned int i, FImagePtr &weight, ProcessImageTypePtr &output);
    };
    
    template <class PixelType>
    Khan<PixelType>::Khan(std::vector<std::string>& newInputFiles, const uint16_t newFlags, const uint16_t newDebugFlags,
                int newIterations, double newSigma, int newVerbosity) {
        try {
            Deghosting::loadImages(newInputFiles);
            Deghosting::setFlags(newFlags);
            Deghosting::setDebugFlags(newDebugFlags);
            Deghosting::setIterationNum(newIterations);
            Deghosting::setVerbosity(newVerbosity);
            
            // I don't know why, but sigma for HDR input have to approximately 10 times smaller
            // FIXME: Maybe it would be better to use different sigma for different images in case both HDR and LDR are mixed
            const char * fileType= ImageImportInfo(newInputFiles[0].c_str()).getFileType();
            if ( (!strcmp(fileType,"TIFF") && strcmp(fileType,"UINT8")) || !strcmp(fileType,"EXR") || !strcmp(fileType,"FLOAT")) {
                setSigma(newSigma/10);
            } else {
                setSigma(newSigma);
            }
            
            for (unsigned int i=0; i<5; i++)
                Deghosting::response.push_back(0);
            PIPOW = sigma*std::sqrt(2*PI);
            denom = 1/PIPOW;
        } catch (...) {
            throw;
        }
    }
    
    template <class PixelType>
    Khan<PixelType>::Khan(std::vector<ImageImportInfo>& newInputFiles, const uint16_t newFlags, const uint16_t newDebugFlags,
                int newIterations, double newSigma, int newVerbosity) {
        try {
            Deghosting::loadImages(newInputFiles);
            Deghosting::setFlags(newFlags);
            Deghosting::setDebugFlags(newDebugFlags);
            Deghosting::setIterationNum(newIterations);
            Deghosting::setVerbosity(newVerbosity);
            
            // I don't know why, but sigma for HDR input have to approximately 10 times smaller
            // FIXME: Maybe it would be better to use different sigma for different images in case both HDR and LDR are mixed
            const char * fileType= newInputFiles[0].getFileType();
            if ( (!strcmp(fileType,"TIFF") && strcmp(fileType,"UINT8")) || !strcmp(fileType,"EXR") || !strcmp(fileType,"FLOAT")) {
                setSigma(newSigma/10);
            } else {
                setSigma(newSigma);
            }
            
            for (unsigned int i=0; i<5; i++)
                Deghosting::response.push_back(0);
            PIPOW = sigma*std::sqrt(2*PI);
            denom = 1/PIPOW;
        } catch (...) {
            throw;
        }
    }
    
    template <class PixelType>
    void Khan<PixelType>::setSigma(double newSigma) {
        sigma = newSigma;
    }
    
    template <class PixelType>
    float Khan<PixelType>::Kh(ProcessImagePixelType x) {
        #ifdef ATAN_KH
            // good choice for sigma for this function is around 600
            return std::atan(-(x*x)+sigma)/PI + 0.5;
        #else
            // good choice for sigma for this function is around 30
            return (std::exp(-(x*x)/(2*sigma*sigma)) * denom);
        #endif
    }
    
    /*void Khan::linearizeRGB(std::string inputFile,FRGBImage *pInputImg) {
        HuginBase::SrcPanoImage panoImg(inputFile);
        panoImg.setResponseType(HuginBase::SrcPanoImage::RESPONSE_EMOR);
        panoImg.setEMoRParams(response);
        // response transform functor
        HuginBase::Photometric::InvResponseTransform<RGBValue<float>,
                                                     RGBValue<float> > invResponse(panoImg);
        invResponse.enforceMonotonicity();

        // iterator to the upper left corner
        FRGBImage::traverser imgIterSourceY = pInputImg->upperLeft();
        // iterator to he lower right corner
        FRGBImage::traverser imgIterEnd = pInputImg->lowerRight();
        // iterator to the output
        FRGBImage::traverser imgIterOut = pInputImg->upperLeft();
        // loop through the image
        for (int y=1; imgIterSourceY.y != imgIterEnd.y; ++imgIterSourceY.y, ++imgIterOut.y, ++y) {
            // iterator to the input
            FRGBImage::traverser sx = imgIterSourceY;
            // iterator to the output
            FRGBImage::traverser dx = imgIterOut;
            for (int x=1; sx.x != imgIterEnd.x; ++sx.x, ++dx.x, ++x) {
                // transform image using response
                *dx = vigra_ext::zeroNegative(invResponse(*sx, hugin_utils::FDiff2D(x, y)));
            }
        }
    }*/
    
    // RGB
    template <class PixelType>
    void Khan<PixelType>::convertImage(ImageType * in, ProcessImageTypePtr & out, VigraFalseType) {
        RGB2LabFunctor<float> RGB2Lab;
        transformImage(srcImageRange(*in), destImage(*out), RGB2Lab);
    }
    
    // grayscale
    template <class PixelType>
    void Khan<PixelType>::convertImage(ImageType* in, ProcessImageTypePtr& out, VigraTrueType) {
        copyImage(srcImageRange(*in), destImage(*out));
    }
    
    // load image and convert it to grayscale
    template <class PixelType>
    void Khan<PixelType>::importRGBImage(ImageImportInfo & info, ImageType * img, VigraTrueType) {
        // NOTE: I guess this is not optimal, but it works
        RGBToGrayAccessor<FRGBImage::PixelType> color2gray;
        FRGBImage tmpImg(info.size());
        if (info.numBands() == 4) {
            BImage imgAlpha(info.size());
            importImageAlpha(info, destImage(tmpImg), destImage(imgAlpha));
        } else {
            importImage(info, destImage(tmpImg));
        }
        transformImage(srcImageRange(tmpImg, color2gray), destImage(*img), log(Arg1()+Param(1.0f)));
    }
    
    // only load image
    template <class PixelType>
    void Khan<PixelType>::importRGBImage(ImageImportInfo & info, ImageType * img, VigraFalseType) {
        if (info.numBands() == 4) {
            BImage imgAlpha(info.size());
            importImageAlpha(info, destImage(*img), destImage(imgAlpha));
        } else {
            importImage(info, destImage(*img));
        }
    }
    
    template <class PixelType>
    void Khan<PixelType>::preprocessImage(unsigned int i, FImagePtr &weight, ProcessImageTypePtr &output) {
        ImageImportInfo imgInfo(inputFiles[i]);
        ImageType * pInputImg =  new ImageType(imgInfo.size());
        weight = FImagePtr(new FImage(imgInfo.size()));
        output = ProcessImageTypePtr(new ProcessImageType(imgInfo.size()));
        
        // import image
        // NOTE: Maybe alpha can be of some use but I don't
        // know about any now
        if (imgInfo.isColor()) {
            importRGBImage(imgInfo, pInputImg, srcIsScalar());
        } else {
            importImage(imgInfo, destImage(*pInputImg));
        }
        
        // linearize RGB and convert it to L*a*b image
        //linearizeRGB(inputFiles[i], pInputImg);
        
        // take logarithm or gamma correction if the input images are HDR
        // I'm not sure if it's the right way how to
        // find out if they are HDR
        const char * fileType= imgInfo.getFileType();
        if ( (!strcmp(fileType,"TIFF") && strcmp(fileType,"UINT8")) || !strcmp(fileType,"EXR") || !strcmp(fileType,"FLOAT")) {
            // use gamma 2.2
            if (flags & ADV_GAMMA) {
                // GammaFunctor is only in vigra 1.6 GRRR
                // I have to use BrightnessContrastFunctor
                // TODO: change to the GammaFunctor in the future
                vigra::FindMinMax<float> minmax;
                vigra::inspectImage(srcImageRange(*pInputImg), minmax);
                transformImage(srcImageRange(*pInputImg),destImage(*pInputImg),BrightnessContrastFunctor<PixelType>(0.45,1.0,minmax.min, minmax.max));
            } else {
                // take logarithm
                transformImage(srcImageRange(*pInputImg),destImage(*pInputImg),LogarithmFunctor<PixelType>(1.0));
            }
        }
        
        // generate initial weights
        transformImage(srcImageRange(*pInputImg),destImage(*weight),HatFunctor<PixelType>());
        
        convertImage(pInputImg, output, srcIsScalar());
        
        delete pInputImg;
        pInputImg = 0;
    }
    
    template <class PixelType>
    std::vector<FImagePtr> Khan<PixelType>::createWeightMasks() {
        for (unsigned int i = 0; i < inputFiles.size(); i++) {
            FImagePtr weight;
            ProcessImageTypePtr processImage;
            preprocessImage(i, weight, processImage);
            processImages.push_back(processImage);
            weights.push_back(weight);
            
            // save init weights
            if (debugFlags & SAVE_INITWEIGHTS) {
                char tmpfn[100];
                snprintf(tmpfn, 99, "init_weights_%d.tiff", i);
                ImageExportInfo exWeights(tmpfn);
                exportImage(srcImageRange(*weight), exWeights.setPixelType("UINT8"));
            }
        }
        
        float maxWeight = 0;
        // image size
        const int origWidth = weights[0]->width();
        const int origHeight = weights[0]->height();
        
        // if we doing scaling, we have to backup L*a*b images of original size
        std::vector<ProcessImageTypePtr> backupLab;
        if (flags & ADV_MULTIRES) {
            for (unsigned int i = 0; i < processImages.size(); i++) {
                // backup original size L*a*b
                backupLab.push_back(processImages[i]);
            }
        }
        
        if (verbosity > 0)
            cout << "Running khan algorithm" << endl;
        // and we can run khan algorithm
        // khan iteration
        for (int it = 0; it < iterations; it++) {
            if (verbosity > 0)
                cout << "iteration " << it+1 << endl;
            // copy weights from previous iteration
            if (verbosity > 1)
                cout << "copying weights from previous iteration" << endl;
            
            std::vector<FImagePtr> prevWeights;
            for (unsigned int i = 0; i < weights.size(); i++) {
                // scale weights to the requied size
                if (flags & ADV_MULTIRES) {
                    // it would be better to use resampleImage, but it seems to be present only in VIGRA 1.6
                    // so let's use some of the resizeImageINTERPOLATION() functions
                    
                    // compute width
                    int resized_width = origWidth / ( iterations/(it+1) );
                    //compute height
                    int resized_height = origHeight / ( iterations/(it+1) );
                    // destination images
                    FImage resizedWeight;
                    ProcessImageType resizedLab;
                    // it's not worthy to scale to less than 100px per side
                    if (resized_width > 100 && resized_height > 100) {
                        // create destination image of desired size
                        resizedWeight = FImage(Size2D(resized_width,resized_height));
                        resizedLab = ProcessImageType(Size2D(resized_width,resized_height));
                    } else if (origWidth >= 100 && origHeight >= 100) {
                        // resize it to the smallest value (ie 100px for the shorter side)
                        if (origWidth >= origHeight) {
                            resizedWeight = FImage(Size2D(100*origWidth/origHeight, 100));
                            resizedLab = ProcessImageType(Size2D(100*origWidth/origHeight, 100));
                        } else {
                            resizedWeight = FImage(Size2D(100, 100*origHeight/origWidth));
                            resizedLab = ProcessImageType(Size2D(100, 100*origHeight/origWidth));
                        }
                    } else {
                        // don't scale at all
                        // just copy weights as if no scaling seting was applied
                        goto DONTSCALE;
                    }
                    
                    // No interpolation – only for testing
                    resizeImageNoInterpolation(srcImageRange(*weights[i]), destImageRange(resizedWeight));
                    resizeImageNoInterpolation(srcImageRange(*backupLab[i]), destImageRange(resizedLab));
                    
                    FImagePtr tmp(new FImage(resizedWeight));
                    prevWeights.push_back(tmp);
                    processImages[i] = ProcessImageTypePtr(new ProcessImageType(resizedLab));
                    weights[i] = FImagePtr(new FImage(resizedWeight));
                } else {
                    DONTSCALE:
                    FImagePtr tmp(new FImage(*weights[i]));
                    prevWeights.push_back(tmp);
                }
            }
            
            // loop through all images
            for (unsigned int i = 0; i < processImages.size(); i++) {
                if (verbosity > 1)
                    cout << "processing image " << i+1 << endl;
                
                // vector storing pixel data
                ProcessImagePixelType X;
                // sums for eq. 6
                double wpqssum = 0;
                double wpqsKhsum = 0;
                // image size
                const int width = processImages[i]->width();
                const int height = processImages[i]->height();

                // iterator to the upper left corner
                ProcessImageTraverser sy = processImages[i]->upperLeft();
                // iterator to the lower right corner
                ProcessImageTraverser send = processImages[i]->lowerRight();
                // iterator to the weight image left corner
                FImage::traverser wy = weights[i]->upperLeft();
                // loop through the row
                for (int y=0; sy.y != send.y; ++sy.y, ++wy.y, ++y) {
                    // iterator to the source (L*a*b image)
                    ProcessImageTraverser sx = sy;
                    // iterator to the weight
                    FImage::traverser wx = wy;
                    // loop over the pixels
                    for (int x=0; sx.x != send.x; ++sx.x, ++wx.x, ++x) {
                        if (verbosity > 2)
                            cout << "processing pixel (" << x+1 << "," << y+1 << ")" << endl;
                        // set pixel vector
                        X = *sx;
                        
                        // loop through all layers
                        for (unsigned int j = 0; j < processImages.size(); j++) {
                            if (verbosity > 2)
                                cout << "processing layer " << j << endl;
                            
                            // iterator to the neighbour
                            ProcessImageTraverser neighby = processImages[j]->upperLeft();
                            // iterator to the weight
                            FImage::traverser weighty = prevWeights[j]->upperLeft();
                            // pixel offset
                            int ndy = -NEIGHB_DIST;
                            // move them to the upper bound
                            // find the best upper bound
                            if (y-NEIGHB_DIST < 0) {
                                ndy = -y;
                            }
                            else {
                                neighby.y += y-NEIGHB_DIST;
                                weighty.y += y-NEIGHB_DIST;
                            }
                            
                            // iterate through neighbourhoods y axis
                            int maxDisty = (height - y) > NEIGHB_DIST ? NEIGHB_DIST : (height - y-1);
                            for (; ndy <= maxDisty; ++neighby.y, ++weighty.y, ++ndy) {
                                ProcessImageTraverser neighbx = neighby;
                                FImage::traverser weightx = weighty;
                                // pixel offset
                                int ndx = -NEIGHB_DIST;
                                // move them to the upper bound
                                // find the best upper bound
                                if (x-NEIGHB_DIST < 0) {
                                    ndx = -x;
                                }
                                else {
                                    neighbx.x += x-NEIGHB_DIST;
                                    weightx.x += x-NEIGHB_DIST;
                                }
                                // iterate through neighbourhoods x axis
                                int maxDistx = (width - x) > NEIGHB_DIST ? NEIGHB_DIST : (width - x-1);
                                for (; ndx <= maxDistx; ++neighbx.x, ++weightx.x, ++ndx) {
                                    if (verbosity > 3)
                                        cout << "(" << ndx << "," << ndy << ")";
                                    // now we can construct pixel vector
                                    // should omit the middle pixel, ie use only neighbours
                                    if (ndx != 0 || ndy != 0) {
                                        wpqsKhsum += (*weightx * Kh(X-(*neighbx)));
                                        wpqssum += *weightx;
                                    }
                                    
                                    maxDistx = (width - x) > NEIGHB_DIST ? NEIGHB_DIST : (width - x-1);
                                }
                                if (verbosity > 3)
                                    cout << endl;
                                
                                maxDisty = (height - y) > NEIGHB_DIST ? NEIGHB_DIST : (height - y-1);
                            }
                        }
                        
                        if (verbosity > 2)
                            cout << "computing new weight" << endl;
                        // compute probability and set weight
                        //cout << "P=" << (float) wpqsKhsum/wpqssum << endl;
                        if (flags & ADV_ONLYP)
                            *wx = (float) wpqsKhsum/wpqssum;
                        else
                            *wx *= (float) wpqsKhsum/wpqssum;
                        if (maxWeight < *wx)
                            maxWeight = *wx;
                        wpqsKhsum = wpqssum = 0;
                        
                    }
                }
            }
        }
        
        if (verbosity > 1)
                cout << "normalizing weights" << endl;
        double factor = 255.0f/maxWeight;
        for (unsigned int i=0; i<weights.size(); ++i) {
            transformImage(srcImageRange(*(weights[i])), destImage(*(weights[i])), NormalizeFunctor<float>(factor));
        }
        return weights;
    }
}

#endif /* KHAN_H_ */
