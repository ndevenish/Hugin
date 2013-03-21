// -*- c-basic-offset: 4 -*-
/** @file nona/Stitcher.h
 *
 *  Contains various routines used for stitching panoramas.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _NONA_STITCHER_H
#define _NONA_STITCHER_H

// To avoid windows.h namespace pollution later on
#include <vigra/windows.h>

#include <sstream>
#include <iomanip>
#include <vector>
#include <utility>
#include <cctype>
#include <algorithm>

#include <vigra/stdimage.hxx>
#include <vigra/rgbvalue.hxx>
#include <vigra/tiff.hxx>

#include <vigra/impex.hxx>
#include <vigra_ext/impexalpha.hxx>

#include <vigra_ext/blend.h>
#include <vigra_ext/NearestFeatureTransform.h>
#include <vigra_ext/tiffUtils.h>
#include <vigra_ext/ImageTransforms.h>

#include <panodata/PanoramaData.h>
#include <algorithms/nona/ComputeImageROI.h>
#include <nona/RemappedPanoImage.h>
#include <nona/ImageRemapper.h>

// calculate distance image for multi file output
#define STITCHER_CALC_DIST_IMG 0

// somehow these are still
#undef DIFFERENCE
#undef min
#undef max
#undef MIN
#undef MAX

namespace HuginBase {
namespace Nona {


/** determine blending order (starting with image 0), and continue to
 *  stitch the image with the biggest overlap area with the real image..
 *  do everything on a low res version of the masks
 */
void estimateBlendingOrder(const PanoramaData & pano, UIntSet images,
	                   std::vector<unsigned int> & blendOrder);

#if 0
/**
 * remap a single image to a multiple/multilayer tiff files.
 *
 * If tiff already contains tiff files, append to the file
 */
template <typename ImageType, typename AlphaType>
bool remapToTiff(const PanoramaData & pano, const PanoramaOptions &opts,
                 unsigned imgNr, TIFF * tiff
                 AppBase::MultiProgressDisplay & progress)
{
    bool saveRoi = opts.tiff_saveROI;

    const PanoImage & img = pano.getImage(imgNr);
	vigra::Diff2D srcSize;
	srcSize.x = img.getWidth();
	srcSize.y = img.getHeight();

	// create transforms
	//    SpaceTransform t;
	//    SpaceTransform invT;
	PTools::Transform transf;
	PTools::Transform invTransf;
	
	transf.createTransform(pano, imgNr, opts, srcSize);
	invTransf.createInvTransform(pano, imgNr, opts, srcSize);

	// calculate ROI for this image.

    ImageOptions imgOpts = img.getOptions();
    vigra::Rect2D imageRect;
    vigra::BImage miniAlpha;
    double alphaScale;
    bool circCrop = pano.getLens(pano.getImage(imgNr).getLensNr()).getProjection() == Lens::CIRCULAR_FISHEYE;
    estimateImageRect(Size2D(opts.width, opts.getHeight()), srcSize,
                      imgOpts.docrop, imgOpts.cropRect, circCrop,
                      transf,
                      imageRect, miniAlpha, alphaScale);

    /** the scanlines */
    ImageType img(;
    AlphaType alpha;

    // write TIFF header
    if (saveRoi) {
        vigra_ext::createTiffDirectory(tiff,
                                       img.getFilename(),
                                       m_basename,
                                       opts.tiffCompression,
                                       imgNr+1, nImg,
                                       imageRect.upperLeft());
    } else {
        vigra_ext::createTiffDirectory(tiff,
                                       img.getFilename(),
                                       m_basename,
                                       opts.tiffCompression,
                                       imgNr+1, nImg,
                                       vigra::Diff2D(0,0));
}
#endif

/** implements a stitching algorithm */
template <typename ImageType, typename AlphaType>
class Stitcher
{
public:
    /** create a stitcher for the given panorama */
    Stitcher(const PanoramaData & pano, AppBase::MultiProgressDisplay & prog)
	: m_pano(pano), m_progress(prog)
    {
    }

    virtual ~Stitcher() {};

    /** Stitch some images into a panorama file.
     *
     *  The filename can be specified with and without extension
     */
    virtual void stitch(const PanoramaOptions & opts,
                        UIntSet & images, const std::string & file,
                        SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
        m_images=images;
        calcOutputROIS(opts, images);
    };


    virtual UIntSet getUsedImages()
    {
        UIntSet ret;
        assert(m_rois.size() == m_images.size());
        std::vector<vigra::Rect2D>::iterator itr = m_rois.begin();
        for(UIntSet::iterator it = m_images.begin(); it != m_images.end(); ++it) {
            if (! itr->isEmpty()) {
                ret.insert(*it);
            }
        }
        return ret;
    }
    //    template<typename ImgIter, typename ImgAccessor>

    //void stitch(const PanoramaOptions & opts, UIntSet & images, vigra::triple<ImgIter, ImgIter, ImgAccessor> output);

protected:
    // calculate ROI's if not already known
    virtual void calcOutputROIS(const PanoramaOptions & opts, const UIntSet & images)
    {
        m_rois = HuginBase::ComputeImageROI::computeROIS(m_pano, opts, images);
    }

    const PanoramaData & m_pano;
    AppBase::MultiProgressDisplay & m_progress;
    UIntSet m_images;
    std::vector<vigra::Rect2D> m_rois;
};

/** remap a set of images, and store the individual remapped files. */
template <typename ImageType, typename AlphaType>
class MultiImageRemapper : public Stitcher<ImageType, AlphaType>
{
public:

    typedef Stitcher<ImageType, AlphaType> Base;

    MultiImageRemapper(const PanoramaData & pano,
	               AppBase::MultiProgressDisplay & progress)
	: Stitcher<ImageType,AlphaType>(pano, progress)
    {
    }

    virtual ~MultiImageRemapper()
    {
    }

    virtual void stitch(const PanoramaOptions & opts, UIntSet & images,
                        const std::string & basename,
                        SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
        Base::stitch(opts, images, basename, remapper);
        DEBUG_ASSERT(opts.outputFormat == PanoramaOptions::TIFF_multilayer
                     || opts.outputFormat == PanoramaOptions::TIFF_m
                     || opts.outputFormat == PanoramaOptions::HDR_m
                     || opts.outputFormat == PanoramaOptions::EXR_m);

        m_basename = basename;

        // setup the output.
        prepareOutputFile(opts);

        unsigned int nImg = images.size();
        Base::m_progress.pushTask(AppBase::ProgressTask("Remapping", "", 1.0/(nImg)));
        // remap each image and save
        int i=0;
        for (UIntSet::const_iterator it = images.begin();
             it != images.end(); ++it)
        {
            // get a remapped image.
            RemappedPanoImage<ImageType, AlphaType> *
                remapped = remapper.getRemapped(Base::m_pano, opts, *it, 
                                                Base::m_rois[i], Base::m_progress);
            try {
                saveRemapped(*remapped, *it, Base::m_pano.getNrOfImages(), opts);
            } catch (vigra::PreconditionViolation & e) {
                // this can be thrown, if an image
                // is completely out of the pano
                std::cerr << e.what();
            }
            // free remapped image
            remapper.release(remapped);
            i++;
        }
        Base::m_progress.popTask();
        finalizeOutputFile(opts);
    }

    /** prepare the output file (setup file structures etc.) */
    virtual void prepareOutputFile(const PanoramaOptions & opts)
    {
    }

    /** save a remapped image, or layer */
    virtual void saveRemapped(RemappedPanoImage<ImageType, AlphaType> & remapped,
                              unsigned int imgNr, unsigned int nImg,
                              const PanoramaOptions & opts)
    {
        ImageType * final_img = 0;
        AlphaType * alpha_img = 0;
        ImageType complete;
        vigra::BImage alpha;

        if ( remapped.boundingBox().isEmpty() )
            // do not save empty files...
            // TODO: need to tell other parts (enblend etc.) about it too!
            return;

        if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
            // export alpha channel as gray channel (original pixel weights)
            std::ostringstream greyname;
            greyname << m_basename << std::setfill('0') << std::setw(4) << imgNr << "_gray.pgm";
            vigra::ImageExportInfo exinfo1(greyname.str().c_str());
            if (! opts.tiff_saveROI) {
                alpha.resize(opts.getROI().size());
                vigra::Rect2D newOutRect = remapped.boundingBox() & opts.getROI();
                vigra::Rect2D newOutArea(newOutRect);
                newOutRect.moveBy(-opts.getROI().upperLeft());
                vigra::copyImage(vigra_ext::applyRect(newOutArea,
                                 vigra_ext::srcMaskRange(remapped)),
                                 vigra_ext::applyRect(newOutRect,
                                 destImage(alpha)));
                vigra::exportImage(srcImageRange(alpha), exinfo1);
            } else {
                exinfo1.setPosition(remapped.boundingBox().upperLeft());
                exinfo1.setCanvasSize(vigra::Size2D(opts.getWidth(), opts.getHeight()));
                vigra::Rect2D rect=remapped.boundingBox();
                if(rect.right() > opts.getROI().right()) {
                    rect&=opts.getROI();
                    rect.moveBy(-rect.upperLeft());
                    vigra::exportImage(srcImageRange(remapped.m_mask, rect), exinfo1);
                } else {
                    vigra::exportImage(srcImageRange(remapped.m_mask), exinfo1);
                };
            }

            // calculate real alpha for saving with the image
            Base::m_progress.setMessage("Calculating mask");
            remapped.calcAlpha();
        }

        vigra::Rect2D subImage;
        if (! opts.tiff_saveROI) {
            // FIXME: this is stupid. Should not require space for full image...
            // but this would need a lower level interface in vigra impex
            complete.resize(opts.getROI().size());
            alpha.resize(opts.getROI().size());
            vigra::Rect2D newOutRect = remapped.boundingBox() & opts.getROI();
            vigra::Rect2D newOutArea(newOutRect);
            newOutRect.moveBy(-opts.getROI().upperLeft());
            vigra::copyImage(vigra_ext::applyRect(newOutArea,
                                 vigra_ext::srcImageRange(remapped)),
                             vigra_ext::applyRect(newOutRect,
                                 destImage(complete)));

            vigra::copyImage(vigra_ext::applyRect(newOutArea,
                                 vigra_ext::srcMaskRange(remapped)),
                             vigra_ext::applyRect(newOutRect,
                                 destImage(alpha)));
            final_img = & complete;
            alpha_img = & alpha;
        } else {
            final_img = &remapped.m_image;
            alpha_img = &remapped.m_mask;
            if(remapped.boundingBox().right() > opts.getROI().right()) {
                subImage=remapped.boundingBox() & opts.getROI();
                subImage.moveBy(-subImage.upperLeft());
            };
        }

/*
        std::string cext = hugin_utils::getExtension(m_basename);
        std::transform(cext.begin(),cext.end(), cext.begin(), (int(*)(int))std::tolower);
        // remove extension only if it specifies the same file type, otherwise
        // its probably part of the filename.
        if (cext == ext) {
            m_basename = hugin_utils::stripExtension(m_basename);
        }
*/
        std::string ext = opts.getOutputExtension();
        std::ostringstream filename;
        filename << m_basename << std::setfill('0') << std::setw(4) << imgNr << "." + ext;


        Base::m_progress.setMessage(std::string("saving ") +
                hugin_utils::stripPath(filename.str()));

        vigra::ImageExportInfo exinfo(filename.str().c_str());
        exinfo.setXResolution(150);
        exinfo.setYResolution(150);
        exinfo.setICCProfile(remapped.m_ICCProfile);
        if (opts.tiff_saveROI) {
            exinfo.setPosition(remapped.boundingBox().upperLeft());
            exinfo.setCanvasSize(vigra::Size2D(opts.getWidth(), opts.getHeight()));
        } else {
            exinfo.setPosition(opts.getROI().upperLeft());
            exinfo.setCanvasSize(vigra::Size2D(opts.getWidth(), opts.getHeight()));
        }
        if (opts.outputPixelType.size() > 0) {
            exinfo.setPixelType(opts.outputPixelType.c_str());
        }
        if (ext == "tif") {
            exinfo.setCompression(opts.tiffCompression.c_str());
        }

        if(subImage.area()>0) {
            vigra::exportImageAlpha(srcImageRange(*final_img, subImage), srcImage(*alpha_img), exinfo);
        } else {
            vigra::exportImageAlpha(srcImageRange(*final_img), srcImage(*alpha_img), exinfo);
        };

        if (opts.saveCoordImgs) {
            vigra::UInt16Image xImg;
            vigra::UInt16Image yImg;

            Base::m_progress.setMessage("creating coordinate images");

            remapped.calcSrcCoordImgs(xImg, yImg);
            vigra::UInt16Image dist;
            if (! opts.tiff_saveROI) {
                dist.resize(opts.getWidth(), opts.getHeight());
                vigra::copyImage(srcImageRange(xImg),
                                vigra_ext::applyRect(remapped.boundingBox(),
                                                    destImage(dist)));
                std::ostringstream filename2;
                filename2 << m_basename << std::setfill('0') << std::setw(4) << imgNr << "_x.tif";

                vigra::ImageExportInfo exinfo(filename2.str().c_str());
                exinfo.setXResolution(150);
                exinfo.setYResolution(150);
                exinfo.setCompression(opts.tiffCompression.c_str());
                vigra::exportImage(srcImageRange(dist), exinfo);


                vigra::copyImage(srcImageRange(yImg),
                                vigra_ext::applyRect(remapped.boundingBox(),
                                        destImage(dist)));
                std::ostringstream filename3;
                filename3 << m_basename << std::setfill('0') << std::setw(4) << imgNr << "_y.tif";

                vigra::ImageExportInfo exinfo2(filename3.str().c_str());
                exinfo2.setXResolution(150);
                exinfo2.setYResolution(150);
                exinfo2.setCompression(opts.tiffCompression.c_str());
                vigra::exportImage(srcImageRange(dist), exinfo2);
            } else {
                std::ostringstream filename2;
                filename2 << m_basename << std::setfill('0') << std::setw(4) << imgNr << "_x.tif";
                vigra::ImageExportInfo exinfo(filename2.str().c_str());
                exinfo.setXResolution(150);
                exinfo.setYResolution(150);
                exinfo.setCompression(opts.tiffCompression.c_str());
                vigra::exportImage(srcImageRange(xImg), exinfo);
                std::ostringstream filename3;
                filename3 << m_basename << std::setfill('0') << std::setw(4) << imgNr << "_y.tif";
                vigra::ImageExportInfo exinfo2(filename3.str().c_str());
                exinfo2.setXResolution(150);
                exinfo2.setYResolution(150);
                exinfo2.setCompression(opts.tiffCompression.c_str());
                vigra::exportImage(srcImageRange(yImg), exinfo2);
            }
        }
    }

    virtual void finalizeOutputFile(const PanoramaOptions & opts)
    {
    }

protected:
    std::string m_basename;
};



/** stitch multilayer */
template <typename ImageType, typename AlphaImageType>
class TiffMultiLayerRemapper : public MultiImageRemapper<ImageType, AlphaImageType>
{
public:

    typedef MultiImageRemapper<ImageType, AlphaImageType> Base;
    TiffMultiLayerRemapper(const PanoramaData & pano,
                           AppBase::MultiProgressDisplay & progress)
	: MultiImageRemapper<ImageType, AlphaImageType> (pano, progress)
    {
    }

    virtual ~TiffMultiLayerRemapper()
    {
    }

    virtual void prepareOutputFile(const PanoramaOptions & opts)
    {
        std::string filename = Base::m_basename + ".tif";
        DEBUG_DEBUG("Layering image into a multi image tif file " << filename);
        m_tiff = TIFFOpen(filename.c_str(), "w");
        DEBUG_ASSERT(m_tiff && "could not open tiff output file");
    }

    /** save the remapped image in a partial tiff layer */
    virtual void saveRemapped(RemappedPanoImage<ImageType, AlphaImageType> & remapped,
                              unsigned int imgNr, unsigned int nImg,
                              const PanoramaOptions & opts)
    {
        if (remapped.boundingBox().isEmpty())
           return;

        DEBUG_DEBUG("Saving TIFF ROI");
        vigra_ext::createTiffDirectory(m_tiff,
                                       Base::m_pano.getImage(imgNr).getFilename(),
                                       Base::m_basename,
                                       opts.tiffCompression,
                                       imgNr+1, nImg, remapped.boundingBox().upperLeft(),
                                       opts.getROI().size(),
                                       remapped.m_ICCProfile);
        vigra_ext::createAlphaTiffImage(vigra::srcImageRange(remapped.m_image),
                                        vigra::maskImage(remapped.m_mask),
                                        m_tiff);
        TIFFFlush(m_tiff);
    }

    /** close the tiff file */
    virtual void finalizeOutputFile(const PanoramaOptions & opts)
    {
	TIFFClose(m_tiff);
    }


protected:
    vigra::TiffImage * m_tiff;
};


typedef std::vector<std::pair<float, unsigned int> > AlphaVector;

/** functor to calculate the union of two images. also calculates the
 *  size of the union
 */
struct CalcMaskUnion
{
    CalcMaskUnion()
	: count(0)
    { }

    template<typename PIXEL>
    PIXEL operator()(PIXEL const & img1, PIXEL const & img2)
    {
	if (img1 > vigra::NumericTraits<PIXEL>::zero() && img2 > vigra::NumericTraits<PIXEL>::zero()) {
	    count++;
	    return img1;
	} else {
	    return vigra::NumericTraits<PIXEL>::zero();
	}
    }

    int count;
};


template <typename ImageType, typename AlphaType>
class WeightedStitcher : public Stitcher<ImageType, AlphaType>
{
public:

    typedef Stitcher<ImageType, AlphaType> Base;
    WeightedStitcher(const PanoramaData & pano,
                     AppBase::MultiProgressDisplay & progress)
	: Stitcher<ImageType, AlphaType>(pano, progress)
    {
    }

    virtual ~WeightedStitcher() {};

    template<class ImgIter, class ImgAccessor,
             class AlphaIter, class AlphaAccessor>
    void stitch(const PanoramaOptions & opts, UIntSet & imgSet,
                        vigra::triple<ImgIter, ImgIter, ImgAccessor> pano,
                        std::pair<AlphaIter, AlphaAccessor> alpha,
                        SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
        std::vector<unsigned int> images;
        // calculate stitching order
        estimateBlendingOrder(Base::m_pano, imgSet, images);

        unsigned int nImg = images.size();

        Base::m_progress.pushTask(AppBase::ProgressTask("Stitching", "", 1.0/(nImg)));	
        // empty ROI
        vigra::Rect2D panoROI;

        int i=0;
        // remap each image and blend into main pano image
        for (UIntVector::const_iterator it = images.begin();
             it != images.end(); ++it)
        {
            // get a remapped image.
            DEBUG_DEBUG("remapping image: " << *it);
            RemappedPanoImage<ImageType, AlphaType> *
            remapped = remapper.getRemapped(Base::m_pano, opts, *it,
                                            Base::m_rois[i], Base::m_progress);
            if(iccProfile.size()==0)
            {
                iccProfile=remapped->m_ICCProfile;
            };
            Base::m_progress.setMessage("blending");
            // add image to pano and panoalpha, adjusts panoROI as well.
            try {
                vigra_ext::blend(*remapped, pano, alpha, panoROI,
                                 Base::m_progress);
                // update bounding box of the panorama
                panoROI = panoROI | remapped->boundingBox();
            } catch (vigra::PreconditionViolation & e) {
                DEBUG_ERROR("exception during stitching" << e.what());
                // this can be thrown, if an image
                // is completely out of the pano
            }
            // free remapped image
            remapper.release(remapped);
            i++;
        }
    }

    void stitch(const PanoramaOptions & opts, UIntSet & imgSet,
                        const std::string & filename,
                        SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
        Base::stitch(opts, imgSet, filename, remapper);

        std::string basename = filename;

	// create panorama canvas
        ImageType pano(opts.getWidth(), opts.getHeight());
        AlphaType panoMask(opts.getWidth(), opts.getHeight());

        stitch(opts, imgSet, vigra::destImageRange(pano), vigra::destImage(panoMask), remapper);
	
	    std::string ext = opts.getOutputExtension();
        std::string cext = hugin_utils::getExtension(basename);
        std::transform(cext.begin(),cext.end(), cext.begin(), (int(*)(int))std::tolower);
        // remove extension only if it specifies the same file type, otherwise
        // its probably part of the filename.
        if (cext == ext) {
            basename = hugin_utils::stripExtension(basename);
        }
        std::string outputfile = basename + "." + ext;
        
	// save the remapped image
        Base::m_progress.setMessage("saving result: " + hugin_utils::stripPath(outputfile));
        DEBUG_DEBUG("Saving panorama: " << outputfile);
        vigra::ImageExportInfo exinfo(outputfile.c_str());
        exinfo.setXResolution(150);
        exinfo.setYResolution(150);
        exinfo.setICCProfile(iccProfile);
        if (opts.outputPixelType.size() > 0) {
            exinfo.setPixelType(opts.outputPixelType.c_str());
        }

        // set compression quality for jpeg images.
        if (opts.outputFormat == PanoramaOptions::JPEG) {
            char jpgCompr[4];
            snprintf(jpgCompr,4,"%d", opts.quality);
            exinfo.setCompression(jpgCompr);
            vigra::exportImage(srcImageRange(pano), exinfo);
        } else if (opts.outputFormat == PanoramaOptions::TIFF) {
            exinfo.setCompression(opts.tiffCompression.c_str());
            vigra::exportImageAlpha(srcImageRange(pano),
                                           srcImage(panoMask), exinfo);
        } else if (opts.outputFormat == PanoramaOptions::HDR) {
            exinfo.setPixelType("FLOAT");
            vigra::exportImage(srcImageRange(pano), exinfo);
        } else {
            vigra::exportImageAlpha(srcImageRange(pano),
                                           srcImage(panoMask), exinfo);
        }
        /*
#ifdef DEBUG
	vigra::exportImage(srcImageRange(panoMask), vigra::ImageExportInfo("pano_alpha.tif"));
#endif
        */
	Base::m_progress.popTask();

    }

protected:
    vigra::ImageImportInfo::ICCProfile iccProfile;
};


/** Difference reduce functor */
template<class VALUETYPE>
struct ReduceToDifferenceFunctor
{
    typedef VALUETYPE  argument_type;
    typedef VALUETYPE  result_type;
    typedef typename vigra::NumericTraits<argument_type> Traits;
    typedef typename Traits::RealPromote float_type;


    ReduceToDifferenceFunctor()
    {
        reset();
    }

    void reset()
    {
        sum = vigra::NumericTraits<float_type>::zero();
        values.clear();
    }

    template<class T, class M> 
    void operator() (T const &v, M const &a)
    {
        if (a) {
            values.push_back(v);
            sum = sum + v;
        }
    }

    /** return the result */ 
    float_type operator()() const
    {
        typedef typename std::vector<float_type>::const_iterator Iter;
        if (values.size() > 1) {
            float_type mean = sum / values.size();
            float_type error = vigra::NumericTraits<float_type>::zero();
            for (Iter it= values.begin();  it != values.end(); ++it) {
                error +=  vigra::abs(*it-mean);
            }
            return error;
        } else {
            return sum;
        }
    }
    std::vector<float_type> values;
    float_type sum;
};

/** create a panorama using the reduce operation on all overlapping
 *  pixels. WARNING: this version is very memory hungry for large
 *  panoramas (> 3 images ;-)
 */
template <typename ImageType, typename AlphaType>
class ReduceStitcher : public Stitcher<ImageType, AlphaType>
{
    typedef Stitcher<ImageType, AlphaType> Base;
public:
    ReduceStitcher(const PanoramaData & pano,
                   AppBase::MultiProgressDisplay & progress)
    : Stitcher<ImageType, AlphaType>(pano, progress)
    {
    }

    virtual ~ReduceStitcher()
    {
    }

    template <class FUNCTOR>
    void stitch(const PanoramaOptions & opts, UIntSet & imgSet,
                const std::string & filename,
                SingleImageRemapper<ImageType, AlphaType> & remapper,
                FUNCTOR & reduce)
    {
        Base::stitch(opts, imgSet, filename, remapper);

        std::string basename = filename;

    // create panorama canvas
        ImageType pano(opts.getWidth(), opts.getHeight());
        AlphaType panoMask(opts.getWidth(), opts.getHeight());

        stitch(opts, imgSet, vigra::destImageRange(pano), vigra::destImage(panoMask),
               remapper, reduce);

    	std::string ext = opts.getOutputExtension();
        std::string cext = hugin_utils::getExtension(basename);
        std::transform(cext.begin(),cext.end(), cext.begin(), (int(*)(int))std::tolower);
        // remove extension only if it specifies the same file type, otherwise
        // its probably part of the filename.
        if (cext == ext) {
            basename = hugin_utils::stripExtension(basename);
        }
        std::string outputfile = basename + "." + ext;

//        Base::m_progress.setMessage("saving result: " + hugin_utils::stripPath(outputfile));
        DEBUG_DEBUG("Saving panorama: " << outputfile);
        vigra::ImageExportInfo exinfo(outputfile.c_str());
        if (opts.outputPixelType.size() > 0) {
            exinfo.setPixelType(opts.outputPixelType.c_str());
        }
        exinfo.setXResolution(150);
        exinfo.setYResolution(150);
        exinfo.setICCProfile(iccProfile);
        // set compression quality for jpeg images.
        if (opts.outputFormat == PanoramaOptions::JPEG) {
            char jpgCompr[4];
            snprintf(jpgCompr,4,"%d", opts.quality);
            exinfo.setCompression(jpgCompr);
            vigra::exportImage(srcImageRange(pano), exinfo);
        } else if (opts.outputFormat == PanoramaOptions::TIFF) {
            exinfo.setCompression(opts.tiffCompression.c_str());
            vigra::exportImageAlpha(srcImageRange(pano),
                                    srcImage(panoMask), exinfo);
        } else if (opts.outputFormat == PanoramaOptions::HDR) {
            vigra::exportImage(srcImageRange(pano), exinfo);
        } else {
            vigra::exportImageAlpha(srcImageRange(pano),
                                    srcImage(panoMask), exinfo);
        }
        /*
#ifdef DEBUG
        vigra::exportImage(srcImageRange(panoMask), vigra::ImageExportInfo("pano_alpha.tif"));
#endif
        */
    }

    
    template<class ImgIter, class ImgAccessor,
             class AlphaIter, class AlphaAccessor,
             class FUNCTOR>
    void stitch(const PanoramaOptions & opts, UIntSet & imgSet,
                vigra::triple<ImgIter, ImgIter, ImgAccessor> pano,
                std::pair<AlphaIter, AlphaAccessor> alpha,
                SingleImageRemapper<ImageType, AlphaType> & remapper,
                FUNCTOR & reduce)
    {
        typedef typename
            vigra::NumericTraits<typename ImageType::value_type> Traits;
        typedef typename
            Traits::RealPromote RealImgType;
        typedef typename ImageType::value_type ImgType;
        typedef typename AlphaAccessor::value_type MaskType;

        Base::stitch(opts, imgSet, "dummy", remapper);

        // remap all images..
        typedef std::vector<RemappedPanoImage<ImageType, AlphaType> *> RemappedVector;
        unsigned int nImg = imgSet.size();

        Base::m_progress.pushTask(AppBase::ProgressTask("Stitching", "", 1.0/(nImg)));	
        // empty ROI
        //	vigra::Rect2D panoROI;
        // remap all images..
        RemappedVector remapped(nImg);

        int i=0;
        // remap each image and blend into main pano image
        for (UIntSet::const_iterator it = imgSet.begin();
                it != imgSet.end(); ++it)
        {
            // get a copy of the remapped image.
            // not very good, keeps all images in memory,
            // but should be enought for the preview.
            remapped[i] = remapper.getRemapped(Base::m_pano, opts, *it,
                                               Base::m_rois[i], Base::m_progress);
            if(iccProfile.size()==0)
            {
                iccProfile=remapped[i]->m_ICCProfile;
            };
            i++;
        }
        vigra::Diff2D size =  pano.second - pano.first;
        ImgIter output = pano.first;
        // iterate over the whole image...
        // calculate something on the pixels that belong together..
        for (int y=0; y < size.y; y++) {
            for (int x=0; x < size.x; x++) {
                reduce.reset();
                MaskType maskRes=0;
                for (unsigned int i=0; i< nImg; i++) {
                    MaskType a = remapped[i]->getMask(x,y);
                    if (a) {
                        maskRes = vigra_ext::LUTTraits<MaskType>::max();
                        reduce(remapped[i]->operator()(x,y), a);
                    }
                }
                pano.third.set(Traits::fromRealPromote(reduce()), output, vigra::Diff2D(x,y));
                alpha.second.set(maskRes, alpha.first, vigra::Diff2D(x,y));
            }
        }
        Base::m_progress.popTask();

        for (typename RemappedVector::iterator it=remapped.begin();
             it != remapped.end(); ++it)
        {
            remapper.release(*it);
        }
    }

protected:
    vigra::ImageImportInfo::ICCProfile iccProfile;
};

/** A stitcher without seaming, just copies the images over each other
 */
template <typename ImageType, typename AlphaType>
class SimpleStitcher : public Stitcher<ImageType, AlphaType>
{
    typedef Stitcher<ImageType, AlphaType> Base;
public:
    SimpleStitcher(const PanoramaData & pano,
		     AppBase::MultiProgressDisplay & progress)
	: Stitcher<ImageType, AlphaType>(pano, progress)
    {
    }

    virtual ~SimpleStitcher()
    {
    }

    template<class ImgIter, class ImgAccessor,
             class AlphaIter, class AlphaAccessor,
             class BlendFunctor>
    void stitch(const PanoramaOptions & opts, UIntSet & imgSet,
                vigra::triple<ImgIter, ImgIter, ImgAccessor> pano,
                std::pair<AlphaIter, AlphaAccessor> alpha,
                SingleImageRemapper<ImageType, AlphaType> & remapper,
                BlendFunctor & blend)
    {
        Base::m_images=imgSet;
        Base::calcOutputROIS(opts, imgSet);

        unsigned int nImg = imgSet.size();

        Base::m_progress.pushTask(AppBase::ProgressTask("Stitching", "", 1.0/(nImg)));
        // empty ROI
        vigra::Rect2D panoROI;

            unsigned i=0;
        // remap each image and blend into main pano image
        for (UIntSet::reverse_iterator it = imgSet.rbegin();
            it != imgSet.rend(); ++it)
        {
            // get a remapped image.
            RemappedPanoImage<ImageType, AlphaType> *
            remapped = remapper.getRemapped(Base::m_pano, opts, *it,
                                            Base::m_rois[i], Base::m_progress);
            if (iccProfile.size() == 0) {
                // try to extract icc profile.
                iccProfile = remapped->m_ICCProfile;
            }
	    Base::m_progress.setMessage("blending");
	    // add image to pano and panoalpha, adjusts panoROI as well.
            try {
                blend(*remapped, pano, alpha, panoROI);
                // update bounding box of the panorama
                panoROI = panoROI | remapped->boundingBox();
            } catch (vigra::PreconditionViolation & e) {
                // this can be thrown, if an image
                // is completely out of the pano
				std::cerr << e.what();
            }
            // free remapped image
            remapper.release(remapped);
            i++;
	}
	Base::m_progress.popTask();
    }

    template <class BlendFunctor>
    void stitch(const PanoramaOptions & opts, UIntSet & imgSet,
                const std::string & filename,
                SingleImageRemapper<ImageType, AlphaType> & remapper,
                BlendFunctor & blend)
    {
        std::string basename = filename;

	// create panorama canvas
        ImageType pano(opts.getWidth(), opts.getHeight());
        AlphaType panoMask(opts.getWidth(), opts.getHeight());

        stitch(opts, imgSet, vigra::destImageRange(pano), vigra::destImage(panoMask), remapper, blend);
	
	    std::string ext = opts.getOutputExtension();
        std::string cext = hugin_utils::getExtension(basename);
        std::transform(cext.begin(),cext.end(), cext.begin(), (int(*)(int))std::tolower);
        // remove extension only if it specifies the same file type, otherwise
        // its probably part of the filename.
        if (cext == ext) {
            basename = hugin_utils::stripExtension(basename);
        }
        std::string outputfile = basename + "." + ext;
	
        Base::m_progress.setMessage("saving result: " + hugin_utils::stripPath(outputfile));
	DEBUG_DEBUG("Saving panorama: " << outputfile);
	vigra::ImageExportInfo exinfo(outputfile.c_str());
	exinfo.setXResolution(150);
	exinfo.setYResolution(150);
        exinfo.setICCProfile(iccProfile);
        // set compression quality for jpeg images.
        if (opts.outputFormat == PanoramaOptions::JPEG) {
            char jpgCompr[4];
            snprintf(jpgCompr,4,"%d", opts.quality);
            exinfo.setCompression(jpgCompr);
            vigra::exportImage(srcImageRange(pano), exinfo);
	} else if (opts.outputFormat == PanoramaOptions::TIFF) {
	    exinfo.setCompression("DEFLATE");
            vigra::exportImageAlpha(srcImageRange(pano),
                                           srcImage(panoMask), exinfo);
	} else {
            vigra::exportImageAlpha(srcImageRange(pano),
                                           srcImage(panoMask), exinfo);
        }
        /*
#ifdef DEBUG
	vigra::exportImage(srcImageRange(panoMask), vigra::ImageExportInfo("pano_alpha.tif"));
#endif
        */
	Base::m_progress.popTask();

    }
protected:
    vigra::ImageExportInfo::ICCProfile iccProfile;
};

/** blend images, by simply stacking them, without soft blending or
 *  boundary calculation
 */
struct StackingBlender
{

public:
    /** blend \p img into \p pano, using \p alpha mask and \p panoROI
     *
     *  updates \p pano, \p alpha and \p panoROI
     */
    template <typename ImageType, typename AlphaType,
              typename PanoIter, typename PanoAccessor,
              typename AlphaIter, typename AlphaAccessor>
    void operator()(RemappedPanoImage<ImageType, AlphaType> & img,
                    vigra::triple<PanoIter, PanoIter, PanoAccessor> pano,
                    std::pair<AlphaIter, AlphaAccessor> alpha,
                    const vigra::Rect2D & panoROI)
    {
        using namespace vigra_ext;
    
        DEBUG_DEBUG("pano roi: " << panoROI << " img roi: " << img.boundingBox());
	    typedef typename AlphaIter::value_type AlphaValue;

//        DEBUG_DEBUG("no overlap, copying upper area. imgroi " << img.roi());
//        DEBUG_DEBUG("pano roi: " << panoROI.upperLeft() << " -> " << panoROI.lowerRight());
        DEBUG_DEBUG("size of panorama: " << pano.second - pano.first);

		// check if bounding box of image is outside of panorama...
		vigra::Rect2D fullPano(vigra::Size2D(pano.second-pano.first));
        // blend only the intersection (which is inside the pano..)
        vigra::Rect2D overlap = fullPano & img.boundingBox();

        vigra::copyImageIf(applyRect(overlap, vigra_ext::srcImageRange(img)),
                           applyRect(overlap, vigra_ext::srcMask(img)),
                           applyRect(overlap, std::make_pair(pano.first, pano.third)));
        // copy mask
        vigra::copyImageIf(applyRect(overlap, srcMaskRange(img)),
                           applyRect(overlap, srcMask(img)),
                           applyRect(overlap, alpha));
    }
};

/** seam blender. */
struct SeamBlender
{
public:

    /** blend \p img into \p pano, using \p alpha mask and \p panoROI
     *
     *  updates \p pano, \p alpha and \p panoROI
     */
    template <typename ImageType, typename AlphaType,
              typename PanoIter, typename PanoAccessor,
              typename AlphaIter, typename AlphaAccessor>
    void operator()(RemappedPanoImage<ImageType, AlphaType> & img,
                    vigra::triple<PanoIter, PanoIter, PanoAccessor> pano,
                    std::pair<AlphaIter, AlphaAccessor> alpha,
                    const vigra::Rect2D & panoROI)
    {
        DEBUG_DEBUG("pano roi: " << panoROI << " img roi: " << img.boundingBox());
        AppBase::MultiProgressDisplay dummy;
        vigra_ext::blend(img, pano, alpha, panoROI, dummy);
    }
};

/** blend by difference */
struct DifferenceBlender
{
public:

    /** blend \p img into \p pano, using \p alpha mask and \p panoROI
     *
     *  updates \p pano, \p alpha and \p panoROI
     */
    template <typename ImageType, typename AlphaType,
              typename PanoIter, typename PanoAccessor,
              typename AlphaIter, typename AlphaAccessor>
    void operator()(RemappedPanoImage<ImageType, AlphaType> & img,
                    vigra::triple<PanoIter, PanoIter, PanoAccessor> pano,
                    std::pair<AlphaIter, AlphaAccessor> alpha,
                    const vigra::Rect2D & panoROI)
    {
		DEBUG_DEBUG("pano roi: " << panoROI << " img roi: " << img.boundingBox());
		typedef typename AlphaIter::value_type AlphaValue;

		// check if bounding box of image is outside of panorama...
		vigra::Rect2D fullPano(vigra::Size2D(pano.second-pano.first));
        // blend only the intersection (which is inside the pano..)
        vigra::Rect2D overlap = fullPano & img.boundingBox();

        vigra::combineTwoImagesIf(applyRect(overlap, vigra_ext::srcImageRange(img)),
                                  applyRect(overlap, std::make_pair(pano.first, pano.third)),
                                  applyRect(overlap, vigra_ext::srcMask(img)),
                                  applyRect(overlap, std::make_pair(pano.first, pano.third)),
                                  abs(vigra::functor::Arg1()-vigra::functor::Arg2()));

        // copy mask
        vigra::copyImageIf(applyRect(overlap, srcMaskRange(img)),
                           applyRect(overlap, srcMask(img)),
                           applyRect(overlap, alpha));
    }
};

/*
template <typename ImageType, typename AlphaImageType>
class MultiResStitcher : public WeightedSticher<ImageType, AlphaImageType>
{
public:
    MultiResStitcher(const PanoramaData & pano,
		     AppBase::MultiProgressDisplay & progress)
	: WeightedStitcher(pano, progress)
    {
    }

    template <typename PanoIter, typename PanoAccessor,
	      typename MaskIter, typename MaskAccessor>
    virtual void blendOverlap(vigra::triple<PanoIter, PanoIter, PanoAccessor> image,
		      std::pair<MaskIter, MaskAccessor> imageMask,
		      std::pair<PanoIter, PanoAccessor> pano,
		      std::pair<MaskIter, MaskAccessor> panoMask)
    {
	vigra::Diff2D size = image.second - image.first;

	// create new blending masks
	vigra::BasicImage<typename MaskIter::value_type> blendPanoMask(size);
	vigra::BasicImage<typename MaskIter::value_type> blendImageMask(size);

	// calculate the stitching masks.
	vigra_ext::nearestFeatureTransform(srcIterRange(panoMask.first, panoMask.first + size),
                                           imageMask,
                                           destImage(blendPanoMask),
                                           destImage(blendImageMask),
                                           m_progress);

        // calculate the a number of

	// copy the image into the panorama
	vigra::copyImageIf(image, maskImage(blendImageMask), pano);
	// copy mask
	vigra::copyImageIf(srcImageRange(blendImageMask), maskImage(blendImageMask), panoMask);
    }

private:
};
*/


#if 0
/** small convinience function to remap and stitch a HDR image 
 *  Warning, this requires tons of memory, since everything is
 *  hold on disk.
 */
template <typename Remapper, typename OutputImage, typename Mask>
void stitchToHDR(Remapper & remappers, OutputImage & panoImg, Mask & mask, 
                 AppBase::MultiProgressDisplay & progress)
{
    std::vector<RemappedPanoImage<OutputImage, Mask> *> remapped;
    // get all remapped images
    for (UIntSet::const_iterator it = displayedImages.begin();
        it != displayedImages.end(); ++it)
    {
        remapped.push_back(remapper.getRemapped(pano, opts, *it, progress));
    }
    ReduceToHDRFunctor<RGBValue<float> > hdrmerge;
    reduceROIImages(remapped,
                    destImageRange(panoImg), destImage(alpha),
                    hdrmerge);
#endif

template<typename ImageType, typename AlphaType>
static void stitchPanoIntern(const PanoramaData & pano,
                             const PanoramaOptions & opts,
                             AppBase::MultiProgressDisplay & progress,
                             const std::string & basename,
                             UIntSet imgs)
{
    using namespace vigra_ext;
    
    //    typedef
    //        vigra::NumericTraits<typename OutputImageType::Accessor::value_type> DestTraits;

    FileRemapper<ImageType, AlphaType> m;
    // determine stitching output
    switch (opts.outputFormat) {
        case PanoramaOptions::JPEG:
        case PanoramaOptions::PNG:
        case PanoramaOptions::TIFF:
        case PanoramaOptions::HDR:
        case PanoramaOptions::EXR:
        {
            if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
                ReduceToHDRFunctor<typename ImageType::value_type> hdrmerge;
                ReduceStitcher<ImageType, AlphaType> stitcher(pano, progress);
                stitcher.stitch(opts, imgs, basename, m, hdrmerge);
            } else {
                WeightedStitcher<ImageType, AlphaType> stitcher(pano, progress);
                stitcher.stitch(opts, imgs, basename,
                                m);
            }
            break;
        }
        case PanoramaOptions::TIFF_m:
        case PanoramaOptions::HDR_m:
        case PanoramaOptions::EXR_m:
        {
            MultiImageRemapper<ImageType, AlphaType> stitcher(pano, progress);
            stitcher.stitch(opts, imgs, basename,
                            m);
            break;
        }
        case PanoramaOptions::TIFF_multilayer:
        {
            TiffMultiLayerRemapper<ImageType, AlphaType> stitcher(pano, progress);
            stitcher.stitch(opts, imgs, basename,
                            m);
            break;
        }
        case PanoramaOptions::TIFF_mask:
        case PanoramaOptions::TIFF_multilayer_mask:
            DEBUG_ERROR("multi mask stitching not implemented!");
            break;
        default:
            DEBUG_ERROR("output format " << opts.getFormatName(opts.outputFormat) << "not supported");
            break;
    }
}

/** stitch a panorama
 *
 * @todo vignetting correction
 * @todo do not keep complete output image in memory
 *
 */
IMPEX void stitchPanorama(const PanoramaData & pano,
		            const PanoramaOptions & opts,
		            AppBase::MultiProgressDisplay & progress,
		            const std::string & basename,
                    const UIntSet & usedImgs);

// the instantiations of the stitching functions have been divided into two .cpp
// files, because g++ will use too much memory otherwise (> 1.5 GB)

void stitchPanoGray_8_16(const PanoramaData & pano,
                         const PanoramaOptions & opts,
                         AppBase::MultiProgressDisplay & progress,
                         const std::string & basename,
                         const UIntSet & usedImgs,
                         const char * pixelType);

void stitchPanoGray_32_float(const PanoramaData & pano,
                             const PanoramaOptions & opts,
                             AppBase::MultiProgressDisplay & progress,
                             const std::string & basename,
                             const UIntSet & usedImgs,
                             const char * pixelType);


void stitchPanoRGB_8_16(const PanoramaData & pano,
                        const PanoramaOptions & opts,
                        AppBase::MultiProgressDisplay & progress,
                        const std::string & basename,
                        const UIntSet & usedImgs,
                        const char * pixelType);

void stitchPanoRGB_32_float(const PanoramaData & pano,
                            const PanoramaOptions & opts,
                            AppBase::MultiProgressDisplay & progress,
                            const std::string & basename,
                            const UIntSet & usedImgs,
                            const char * pixelType);

} // namespace
} // namespace

#endif // _H
