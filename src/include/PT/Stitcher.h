// -*- c-basic-offset: 4 -*-
/** @file Stitcher.h
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

#ifndef _STITCHER_H
#define _STITCHER_H


#include <sstream>
#include <iomanip>
#include <vector>
#include <utility>

#include <vigra/stdimage.hxx>
#include <vigra/rgbvalue.hxx>
#include <vigra/tiff.hxx>

#include <vigra/impex.hxx>
#include <vigra/impexalpha.hxx>

#include <vigra_ext/blend.h>
#include <vigra_ext/NearestFeatureTransform.h>
#include <vigra_ext/tiffUtils.h>
#include <vigra_ext/ImageTransforms.h>

#include <PT/Panorama.h>
#include <PT/RemappedPanoImage.h>

// calculate distance image for multi file output
#define STITCHER_CALC_DIST_IMG 0

namespace PT{


/** determine blending order (starting with image 0), and continue to
 *  stitch the image with the biggest overlap area with the real image..
 *  do everything on a low res version of the masks
 */
void estimateBlendingOrder(const PT::Panorama & pano, PT::UIntSet images,
	                   std::vector<unsigned int> & blendOrder);

/** implements a stitching algorithm */
template <typename ImageType, typename AlphaType>
class Stitcher
{
public:
    /** create a stitcher for the given panorama */
    Stitcher(const PT::Panorama & pano, utils::MultiProgressDisplay & prog)
	: m_pano(pano), m_progress(prog)
    {
    }

    /** Stitch some images into a panorama file.
     *
     *  The filename can be specified with and without extension
     */
    virtual void stitch(const PT::PanoramaOptions & opts,
                        PT::UIntSet & images, const std::string & file,
                        SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
	DEBUG_FATAL("Not implemented");
    };


    //    template<typename ImgIter, typename ImgAccessor>

    //void stitch(const PanoramaOptions & opts, UIntSet & images, vigra::triple<ImgIter, ImgIter, ImgAccessor> output);

protected:

    const PT::Panorama & m_pano;
    utils::MultiProgressDisplay & m_progress;
};

/** remap a set of images, and store the individual remapped files. */
template <typename ImageType, typename AlphaType>
class MultiImageRemapper : public Stitcher<ImageType, AlphaType>
{
public:

    typedef Stitcher<ImageType, AlphaType> Base;

    MultiImageRemapper(const PT::Panorama & pano,
	               utils::MultiProgressDisplay & progress)
	: Stitcher<ImageType,AlphaType>(pano, progress)
    {
    }

    virtual ~MultiImageRemapper()
    {
    }

    virtual void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & images,
                        const std::string & basename,
                        SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
	DEBUG_ASSERT(opts.outputFormat == PT::PanoramaOptions::TIFF_multilayer
		     || opts.outputFormat == PT::PanoramaOptions::TIFF_m);

        m_basename = utils::stripExtension(basename);
        DEBUG_DEBUG("created basename: " << basename << " -> " << m_basename);

        // setup the output.
        prepareOutputFile(opts);

	unsigned int nImg = images.size();
	Base::m_progress.pushTask(utils::ProgressTask("Remapping", "", 1.0/(nImg+1)));
	// remap each image and save
	for (UIntSet::const_iterator it = images.begin();
	     it != images.end(); ++it)
	{
        // get a remapped image.
        RemappedPanoImage<ImageType, AlphaType> *
            remapped = remapper.getRemapped(Base::m_pano, opts, *it, Base::m_progress);
        try {
            saveRemapped(*remapped, *it, Base::m_pano.getNrOfImages(), opts);
        } catch (vigra::PreconditionViolation & e) {
            // this can be thrown, if an image
            // is completely out of the pano
            std::cerr << e.what();
        }
        // free remapped image
        remapper.release(remapped);

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
                              const PT::PanoramaOptions & opts)
    {
        // save image in full size
        ImageType complete(opts.width, opts.getHeight());
        vigra::BImage alpha(opts.width, opts.getHeight());
        vigra::copyImage(vigra_ext::applyRect(remapped.boundingBox(),
                                              vigra_ext::srcImageRange(remapped)),
                         vigra_ext::applyRect(remapped.boundingBox(),
                                              destImage(complete)));

        vigra::copyImage(vigra_ext::applyRect(remapped.boundingBox(),
                                              vigra_ext::srcMaskRange(remapped)),
                         vigra_ext::applyRect(remapped.boundingBox(),
                                              destImage(alpha)));

        switch (opts.outputFormat) {
        case PanoramaOptions::JPEG:
        {
            std::ostringstream filename;
            filename << m_basename << std::setfill('0') << std::setw(4) << imgNr << ".jpg";

            vigra::ImageExportInfo exinfo(filename.str().c_str());
            exinfo.setXResolution(150);
            exinfo.setYResolution(150);
            // set compression options here.
            char jpgCompr[4];
            snprintf(jpgCompr,4,"%d", opts.quality);
            exinfo.setCompression(jpgCompr);
            vigra::exportImage(srcImageRange(complete), exinfo);
            break;
        }
        case PanoramaOptions::PNG:
        {
            std::ostringstream filename;
            filename << m_basename << std::setfill('0') << std::setw(4) << imgNr << ".png";
            vigra::ImageExportInfo exinfo(filename.str().c_str());
            exinfo.setXResolution(150);
            exinfo.setYResolution(150);
            vigra::exportImageAlpha(srcImageRange(complete), srcImage(alpha),
                                           exinfo);
            break;
        }
        case PanoramaOptions::TIFF_m:
        {
            std::ostringstream filename;
            filename << m_basename << std::setfill('0') << std::setw(4) << imgNr << ".tif";
            vigra::TiffImage * tiff = TIFFOpen(filename.str().c_str(), "w");

            vigra_ext::createTiffDirectory(tiff,
                                           Base::m_pano.getImage(imgNr).getFilename(),
                                           m_basename,
                                           imgNr+1, nImg,
                                           vigra::Diff2D(0,0));
            vigra_ext::createAlphaTiffImage(srcImageRange(complete),
                                            srcImage(alpha),
                                            tiff);
            TIFFFlush(tiff);
            TIFFClose(tiff);

#if STITCHER_CALC_DIST_IMG
            vigra::USImage distImg;
            remapped.calcDistMap(Base::m_pano, opts, imgNr, distImg);

            vigra::USImage distFull(opts.width, opts.getHeight());
            vigra::copyImage(srcImageRange(distImg),
                             vigra_ext::applyRect(remapped.boundingBox(),
                                                  destImage(distFull)));


            std::ostringstream filename2;
            filename2 << m_basename << "_dist_" << std::setfill('0') << std::setw(4) << imgNr << ".tif";

            vigra::ImageExportInfo exinfo(filename2.str().c_str());
            exinfo.setXResolution(150);
            exinfo.setYResolution(150);
            vigra::exportImage(srcImageRange(distFull), exinfo);
#endif

            break;
        }
        default:
            DEBUG_FATAL("invalid output format specified:" << opts.outputFormat);
            break;
        }
    }

    virtual void finalizeOutputFile(const PT::PanoramaOptions & opts)
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
    TiffMultiLayerRemapper(const PT::Panorama & pano,
                           utils::MultiProgressDisplay & progress)
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
                              const PT::PanoramaOptions & opts)
    {
        DEBUG_DEBUG("Saving TIFF ROI");
        vigra_ext::createTiffDirectory(m_tiff,
                                       Base::m_pano.getImage(imgNr).getFilename(),
                                       Base::m_basename,
                                       imgNr+1, nImg, remapped.boundingBox().upperLeft());
        vigra_ext::createAlphaTiffImage(vigra::srcImageRange(remapped.m_image),
                                        vigra::maskImage(remapped.m_mask),
                                        m_tiff);
        TIFFFlush(m_tiff);
    }

    /** close the tiff file */
    virtual void finalizeOutputFile(const PT::PanoramaOptions & opts)
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
    WeightedStitcher(const PT::Panorama & pano,
		     utils::MultiProgressDisplay & progress)
	: Stitcher<ImageType, AlphaType>(pano, progress)
    {
    }

    template<class ImgIter, class ImgAccessor,
             class AlphaIter, class AlphaAccessor>
    void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & imgSet,
                        vigra::triple<ImgIter, ImgIter, ImgAccessor> pano,
                        std::pair<AlphaIter, AlphaAccessor> alpha,
                        SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
	std::vector<unsigned int> images;
	// calculate stitching order
	estimateBlendingOrder(Base::m_pano, imgSet, images);

	unsigned int nImg = images.size();

	Base::m_progress.pushTask(utils::ProgressTask("Stitching", "", 1.0/(nImg)));	
	// empty ROI
	vigra::Rect2D panoROI;

	// remap each image and blend into main pano image
	for (UIntVector::const_iterator it = images.begin();
	     it != images.end(); ++it)
	{
            // get a remapped image.
            DEBUG_DEBUG("remapping image: " << *it);
	    RemappedPanoImage<ImageType, AlphaType> *
            remapped = remapper.getRemapped(Base::m_pano, opts, *it, Base::m_progress);

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
	}
    }

    void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & imgSet,
                        const std::string & filename,
                        SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
        std::string basename = utils::stripExtension(filename);

	// create panorama canvas
	ImageType pano(opts.width, opts.getHeight());
	AlphaType panoMask(opts.width, opts.getHeight());

        stitch(opts, imgSet, vigra::destImageRange(pano), vigra::destImage(panoMask), remapper);
	
        std::string outputfile;
	// save the remapped image
        switch (opts.outputFormat) {
        case PanoramaOptions::JPEG:
            outputfile = basename + ".jpg";
            break;
        case PanoramaOptions::PNG:
            outputfile = basename + ".png";
            break;
        case PanoramaOptions::TIFF:
            outputfile = basename + ".tif";
            break;
        default:
            DEBUG_ERROR("unsupported output format: " << opts.outputFormat);
        }
	Base::m_progress.setMessage("saving result: " + utils::stripPath(outputfile));
	DEBUG_DEBUG("Saving panorama: " << outputfile);
	vigra::ImageExportInfo exinfo(outputfile.c_str());
	exinfo.setXResolution(150);
	exinfo.setYResolution(150);
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
#ifdef DEBUG
	vigra::exportImage(srcImageRange(panoMask), vigra::ImageExportInfo("pano_alpha.tif"));
#endif
	Base::m_progress.popTask();

    }

private:
};


/** A stitcher without seaming, just copies the images over each other
 */
template <typename ImageType, typename AlphaType>
class MultiBlendingStitcher : public Stitcher<ImageType, AlphaType>
{
    typedef Stitcher<ImageType, AlphaType> Base;
public:
    MultiBlendingStitcher(const PT::Panorama & pano,
                          utils::MultiProgressDisplay & progress)
	: Stitcher<ImageType, AlphaType>(pano, progress)
    {
    }

    virtual ~MultiBlendingStitcher()
    {
    }

    template<class ImgIter, class ImgAccessor,
             class AlphaIter, class AlphaAccessor>
    void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & imgSet,
                vigra::triple<ImgIter, ImgIter, ImgAccessor> pano,
                std::pair<AlphaIter, AlphaAccessor> alpha,
                SingleImageRemapper<ImageType, AlphaType> & remapper)
    {
        typedef typename
            vigra::NumericTraits<typename ImageType::value_type> Traits;
        typedef typename
            Traits::RealPromote RealImgType;
        typedef typename ImageType::value_type ImgType;

        // remap all images..
        typedef std::vector<RemappedPanoImage<ImageType, AlphaType> *> RemappedVector;
        unsigned int nImg = imgSet.size();

	Base::m_progress.pushTask(utils::ProgressTask("Stitching", "", 1.0/(nImg)));	
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
            remapped[i] = remapper.getRemapped(Base::m_pano, opts, *it, Base::m_progress);
            i++;
	}
	vigra::Diff2D size =  pano.second - pano.first;
	ImgIter output = pano.first;
        // iterate over the whole image...
        // calculate something on the pixels that belong together..
        for (int y=0; y < size.y; y++) {
            for (int x=0; x < size.x; x++) {
                RealImgType mean;
                int n=0;
				unsigned int nFirst=nImg;
                // calculate the minimum and maximum
                for (unsigned int i=0; i< nImg; i++) {
                    typename AlphaType::value_type a;
                    a =remapped[i]->getMask(x,y);
                    if (a) {
                        if (i < nFirst) {
                            nFirst = i;
                        }
                        RealImgType c = remapped[i]->operator()(x,y);
                        mean = mean + c;
                        n++;
                    }
                }
                if (n > 1) {
                    mean = mean / n;
                    RealImgType error = 0;
                    for (unsigned int i=0; i < nImg; i++) {
                        if (remapped[i]->getMask(x,y)) {
                            RealImgType c = remapped[i]->operator()(x,y);
                            error +=  vigra::abs(c-mean);
                        }
                    }
//					*output = Traits::fromRealPromote(error);
                    pano.third.set(Traits::fromRealPromote(error),
                                   output, vigra::Diff2D(x,y));
                } else if (n==1){
                    pano.third.set(remapped[nFirst]->operator()(x,y),output,vigra::Diff2D(x,y));
                }
            }
        }
	Base::m_progress.popTask();
	
	for (typename RemappedVector::iterator it=remapped.begin();
	     it != remapped.end(); ++it)
        {
	    remapper.release(*it);
	}
    }
};

/** A stitcher without seaming, just copies the images over each other
 */
template <typename ImageType, typename AlphaType>
class SimpleStitcher : public Stitcher<ImageType, AlphaType>
{
    typedef Stitcher<ImageType, AlphaType> Base;
public:
    SimpleStitcher(const PT::Panorama & pano,
		     utils::MultiProgressDisplay & progress)
	: Stitcher<ImageType, AlphaType>(pano, progress)
    {
    }

    virtual ~SimpleStitcher()
    {
    }

    template<class ImgIter, class ImgAccessor,
             class AlphaIter, class AlphaAccessor,
             class BlendFunctor>
    void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & imgSet,
                vigra::triple<ImgIter, ImgIter, ImgAccessor> pano,
                std::pair<AlphaIter, AlphaAccessor> alpha,
                SingleImageRemapper<ImageType, AlphaType> & remapper,
                BlendFunctor & blend)
    {
	unsigned int nImg = imgSet.size();

	Base::m_progress.pushTask(utils::ProgressTask("Stitching", "", 1.0/(nImg)));	
	// empty ROI
	vigra::Rect2D panoROI;

	// remap each image and blend into main pano image
	for (UIntSet::const_iterator it = imgSet.begin();
	     it != imgSet.end(); ++it)
	{
            // get a remapped image.
	    RemappedPanoImage<ImageType, AlphaType> *
            remapped = remapper.getRemapped(Base::m_pano, opts, *it, Base::m_progress);

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
	}
	Base::m_progress.popTask();
    }

    template <class BlendFunctor>
    void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & imgSet,
                const std::string & filename,
                SingleImageRemapper<ImageType, AlphaType> & remapper,
                BlendFunctor & blend)
    {
        std::string basename = utils::stripExtension(filename);

	// create panorama canvas
	ImageType pano(opts.width, opts.getHeight());
	AlphaType panoMask(opts.width, opts.getHeight());

        stitch(opts, imgSet, vigra::destImageRange(pano), vigra::destImage(panoMask), remapper, blend);
	
        std::string outputfile;
	// save the remapped image
        switch (opts.outputFormat) {
        case PanoramaOptions::JPEG:
            outputfile = basename + ".jpg";
            break;
        case PanoramaOptions::PNG:
            outputfile = basename + ".png";
            break;
        case PanoramaOptions::TIFF:
            outputfile = basename + ".tif";
            break;
        default:
            DEBUG_ERROR("unsupported output format: " << opts.outputFormat);
        }
	Base::m_progress.setMessage("saving result: " + utils::stripPath(outputfile));
	DEBUG_DEBUG("Saving panorama: " << outputfile);
	vigra::ImageExportInfo exinfo(outputfile.c_str());
	exinfo.setXResolution(150);
	exinfo.setYResolution(150);
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
#ifdef DEBUG
	vigra::exportImage(srcImageRange(panoMask), vigra::ImageExportInfo("pano_alpha.tif"));
#endif
	Base::m_progress.popTask();

    }
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
        utils::MultiProgressDisplay dummy;
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
    MultiResStitcher(const PT::Panorama & pano,
		     utils::MultiProgressDisplay & progress)
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


/** stitch a panorama
 *
 * @todo seam calculation?
 * @todo move seam calculation into a separate class/function?
 * @todo vignetting correction
 * @todo do not keep complete output image in memory
 * @todo proper handling for 16 bit images etc.
 *
 */
void stitchPanorama(const PT::Panorama & pano,
		    const PT::PanoramaOptions & opts,
		    utils::MultiProgressDisplay & progress,
		    const std::string & basename);


} // namespace PT

#endif // _STITCHER_H
