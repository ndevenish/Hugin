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

#include <vigra_ext/NearestFeatureTransform.h>
#include <vigra_ext/tiffUtils.h>
#include <vigra_ext/blend.h>

#include <PT/ImageTransforms.h>
#include <PT/Panorama.h>


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
                        SingleImageRemapper<ImageType, AlphaType> & remapper) = 0;


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

        // setup the output.
        prepareOutputFile(opts);

	unsigned int nImg = images.size();
	Base::m_progress.pushTask(utils::ProgressTask("Remapping", "", 1.0/(nImg+1)));
        // number of the remapped image in the panorama (for partial remapping,
        // when the images set does not contain all input images.
        unsigned int runningImgNr;
	// remap each image and save
	for (UIntSet::const_iterator it = images.begin();
	     it != images.end(); ++it)
	{
            // get a remapped image.
	    RemappedPanoImage<ImageType, AlphaType> &
            remapped = remapper(Base::m_pano, opts, *it, Base::m_progress);
	
            saveRemapped(remapped, *it, Base::m_pano.getNrOfImages(), opts);
            // free remapped image
            remapper.release();

            runningImgNr++;
        }

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
        vigra::copyImage(remapped.image(),
                         remapped.roi().apply(destImage(complete)));

        vigra::copyImage(remapped.alpha(),
                         remapped.roi().apply(destImage(alpha)));

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
                                       imgNr+1, nImg, remapped.roi().getUL());
        vigra_ext::createAlphaTiffImage(remapped.image(),
                                        vigra::srcIter(remapped.alpha().first),
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
	vigra_ext::ROI<vigra::Diff2D> panoROI;

	// remap each image and blend into main pano image
	for (UIntVector::const_iterator it = images.begin();
	     it != images.end(); ++it)
	{
            // get a remapped image.
	    RemappedPanoImage<ImageType, AlphaType> &
            remapped = remapper(Base::m_pano, opts, *it, Base::m_progress);

	    Base::m_progress.setMessage("blending");
	    // add image to pano and panoalpha, adjusts panoROI as well.
	    blend(remapped, pano, alpha, panoROI);
            // free remapped image
            remapper.release();
	}
    }

    virtual void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & imgSet,
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

    /** blends two images, they overlap and the iterators point
     *  to excatly the same position.
     */
    template <typename PanoIter, typename PanoAccessor,
              typename MaskIter, typename MaskAccessor,
              typename ImageIter, typename ImageAccessor,
	      typename ImageMaskIter, typename ImageMaskAccessor>
    void blendOverlap(vigra::triple<ImageIter, ImageIter, ImageAccessor> image,
		      std::pair<ImageMaskIter, ImageMaskAccessor> imageMask,
		      std::pair<PanoIter, PanoAccessor> pano,
		      std::pair<MaskIter, MaskAccessor> panoMask)
    {
	vigra::Diff2D size = image.second - image.first;

#ifdef DEBUG
	// save the masks
	vigra::exportImage(srcIterRange(imageMask.first, imageMask.first + size),
                    vigra::ImageExportInfo("blendImageMask_before.tif"));
        vigra::exportImage(srcIterRange(panoMask.first, panoMask.first + size),
                    vigra::ImageExportInfo("blendPanoMask_before.tif"));
	
#endif

	// create new blending masks
	vigra::BasicImage<typename MaskIter::value_type> blendPanoMask(size);
	vigra::BasicImage<typename MaskIter::value_type> blendImageMask(size);

	// calculate the stitching masks.
	vigra_ext::nearestFeatureTransform(srcIterRange(panoMask.first, panoMask.first + size),
                                           imageMask,
                                           destImage(blendPanoMask),
                                           destImage(blendImageMask),
                                           Base::m_progress);

#ifdef DEBUG
	// save the masks
	vigra::exportImage(srcImageRange(blendImageMask), vigra::ImageExportInfo("blendImageMask.tif"));
	vigra::exportImage(srcImageRange(blendPanoMask), vigra::ImageExportInfo("blendPanoMask.tif"));
	
#endif
	// copy the image into the panorama
	vigra::copyImageIf(image, vigra::maskImage(blendImageMask), pano);
	// copy mask
	vigra::copyImageIf(vigra::srcImageRange(blendImageMask), vigra::maskImage(blendImageMask), panoMask);
    }


    /** blend \p img into \p pano, using \p alpha mask and \p panoROI
     *
     *  updates \p pano, \p alpha and \p panoROI
     */
    template <typename PanoIter, typename PanoAccessor,
	      typename AlphaIter, typename AlphaAccessor>
    void blend(RemappedPanoImage<ImageType, AlphaType> & img,
	       vigra::triple<PanoIter, PanoIter, PanoAccessor> pano,
	       std::pair<AlphaIter, AlphaAccessor> alpha,
	       vigra_ext::ROI<vigra::Diff2D> & panoROI)
    {
	typedef typename AlphaIter::value_type AlphaValue;
	// intersect the ROI's.
	vigra_ext::ROI<vigra::Diff2D> overlap;
        const vigra_ext::ROI<vigra::Diff2D> & imgROI = img.roi();
	if (panoROI.intersect(imgROI, overlap)) {
	    // image ROI's overlap.. calculate overlap mask
	    vigra::BasicImage<AlphaValue> overlapMask(overlap.size());
	    vigra_ext::OverlapSizeCounter counter;
	    // calculate union of panorama and image mask
	    vigra::inspectTwoImages(overlap.apply(img.alpha(),imgROI),
				    overlap.apply(alpha, panoROI),
				    counter);

	    DEBUG_DEBUG("overlap found: " << overlap << " pixels: " << counter.getSize());
 	    if (counter.getSize() > 0) {
		// images overlap, call real blending routine
		blendOverlap(overlap.apply(img.image(), imgROI),
			     overlap.apply(srcIter(img.alpha().first), imgROI),
			     overlap.apply(srcIter(pano.first)),
			     overlap.apply(alpha));

                // now, copy the non-overlapping parts

                vigra::Diff2D imgUL = imgROI.getUL();
                vigra::Diff2D imgLR = imgROI.getLR();

                std::vector<vigra_ext::ROI<vigra::Diff2D> > borderAreas;

                vigra_ext::ROI<vigra::Diff2D> roi;

                // upper part
                roi.setCorners(imgUL,
                               vigra::Diff2D(imgLR.x, overlap.getUL().y));
                DEBUG_DEBUG("upper area: " << roi);
                if (roi.size().x > 0 && roi.size().y > 0) {
                    borderAreas.push_back(roi);
                }

                // left area
                roi.setCorners(vigra::Diff2D(imgUL.x, overlap.getUL().y),
                               vigra::Diff2D(overlap.getUL().x, overlap.getLR().y));
                DEBUG_DEBUG("left area: " << roi);
                if (roi.size().x > 0 && roi.size().y > 0) {
                    borderAreas.push_back(roi);
                }

                // right area
                roi.setCorners(vigra::Diff2D(overlap.getLR().x, overlap.getUL().y),
                               vigra::Diff2D(imgLR.x, overlap.getLR().y));
                DEBUG_DEBUG("right area: " << roi);
                if (roi.size().x > 0 && roi.size().y > 0) {
                    borderAreas.push_back(roi);
                }

                // bottom area
                roi.setCorners(vigra::Diff2D(imgUL.x, overlap.getLR().y),
                               imgLR);
                DEBUG_DEBUG("bottom area: " << roi);
                if (roi.size().x > 0 && roi.size().y > 0) {
                    borderAreas.push_back(roi);
                }

                for (std::vector<vigra_ext::ROI<vigra::Diff2D> >::iterator it = borderAreas.begin();
                     it != borderAreas.end();
                     ++it)
                {
                    // copy image with mask.
                    vigra::copyImageIf((*it).apply(img.image(), imgROI),
                                       (*it).apply(srcIter(img.alpha().first), imgROI),
                                       (*it).apply(std::make_pair(pano.first, pano.third),panoROI) );
                    // copy mask
                    vigra::copyImageIf((*it).apply(img.alpha(),imgROI),
                                       (*it).apply(maskIter(img.alpha().first),imgROI),
                                       (*it).apply(alpha,panoROI) );

                }
            } else {
                // images do not overlap, but roi's do
                // copy image with mask.
                DEBUG_DEBUG("no overlap, copying upper area. imgroi " << img.roi());
                DEBUG_DEBUG("pano roi: " << panoROI);
                vigra::copyImageIf(img.image(),
                                   maskIter(img.alpha().first),
                                   img.roi().apply(std::make_pair(pano.first, pano.third),panoROI) );
                // copy mask
                vigra::copyImageIf(img.alpha(),
                                   maskIter(img.alpha().first),
                                   img.roi().apply(alpha,panoROI) );
            }

	} else {
	    // image ROI's do not overlap, no blending, just copy

            DEBUG_DEBUG("no overlap, copying upper area. imgroi " << img.roi());
            DEBUG_DEBUG("pano roi: " << panoROI);
            DEBUG_DEBUG("size of panorama: " << pano.second - pano.first);
	    vigra::copyImageIf(img.image(),
                               maskIter(img.alpha().first),
                               img.roi().apply(std::make_pair(pano.first, pano.third)));

	    // copy mask
	    vigra::copyImageIf(img.alpha(),
                               maskIter(img.alpha().first),
                               img.roi().apply(alpha));
	}
	img.roi().unite(panoROI, panoROI);
    }


private:
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
