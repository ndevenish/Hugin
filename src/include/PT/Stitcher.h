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

#ifndef _SIMPLESTITCHER_H
#define _SIMPLESTITCHER_H

#include <sstream>
#include <iomanip>
#include <vector>
#include <utility>

#include <vigra/impex.hxx>
#include <vigra/stdimage.hxx>
#include "vigra/rgbvalue.hxx"

#include <vigra_ext/NearestFeatureTransform.h>
#include <vigra_ext/tiffUtils.h>
#include <vigra_ext/blend.h>

#include <PT/ImageTransforms.h>

namespace vigra {

// define missing std image types in vigra.

#define VIGRA_EXT_DEFINE_ITERATORTRAITS(VALUETYPE, ACCESSOR, CONSTACCESSOR) \
    template<> \
    struct IteratorTraits< \
        BasicImageIterator<VALUETYPE, VALUETYPE **> > \
    { \
        typedef BasicImageIterator<VALUETYPE, VALUETYPE **> \
                                                     Iterator; \
        typedef Iterator                             iterator; \
        typedef iterator::iterator_category          iterator_category; \
        typedef iterator::value_type                 value_type; \
        typedef iterator::reference                  reference; \
        typedef iterator::index_reference            index_reference; \
        typedef iterator::pointer                    pointer; \
        typedef iterator::difference_type            difference_type; \
        typedef iterator::row_iterator               row_iterator; \
        typedef iterator::column_iterator            column_iterator; \
        typedef ACCESSOR<VALUETYPE >                 default_accessor; \
        typedef ACCESSOR<VALUETYPE >                 DefaultAccessor; \
    }; \
    template<> \
    struct IteratorTraits< \
        ConstBasicImageIterator<VALUETYPE, VALUETYPE **> > \
    { \
        typedef \
          ConstBasicImageIterator<VALUETYPE, VALUETYPE **> \
                                                     Iterator; \
        typedef Iterator                             iterator; \
        typedef iterator::iterator_category          iterator_category; \
        typedef iterator::value_type                 value_type; \
        typedef iterator::reference                  reference; \
        typedef iterator::index_reference            index_reference; \
        typedef iterator::pointer                    pointer; \
        typedef iterator::difference_type            difference_type; \
        typedef iterator::row_iterator               row_iterator; \
        typedef iterator::column_iterator            column_iterator; \
        typedef CONSTACCESSOR<VALUETYPE >            default_accessor; \
        typedef CONSTACCESSOR<VALUETYPE >            DefaultAccessor; \
    };

VIGRA_EXT_DEFINE_ITERATORTRAITS(unsigned short, StandardValueAccessor,
                                StandardConstValueAccessor)
typedef BasicImage<unsigned short> USImage;

VIGRA_EXT_DEFINE_ITERATORTRAITS(unsigned int, StandardValueAccessor,
                                StandardConstValueAccessor)
typedef BasicImage<unsigned int> UIImage;


VIGRA_EXT_DEFINE_ITERATORTRAITS(RGBValue<short>, RGBAccessor,
                            RGBAccessor)
typedef BasicImage<RGBValue<short> > SRGBImage;

VIGRA_EXT_DEFINE_ITERATORTRAITS(RGBValue<unsigned short>, RGBAccessor,
                            RGBAccessor)
typedef BasicImage<RGBValue<unsigned short> > USRGBImage;

VIGRA_EXT_DEFINE_ITERATORTRAITS(RGBValue<unsigned int>, RGBAccessor,
                            RGBAccessor)
typedef BasicImage<RGBValue<unsigned int> > UIRGBImage;

}

namespace PT{


/** determine blending order (starting with image 0), and continue to
 *  stitch the image with the biggest overlap area with the real image..
 *  do everything on a low res version of the masks
 */
void estimateBlendingOrder(const PT::Panorama & pano, PT::UIntSet images,
			   std::vector<unsigned int> & blendOrder);

/** implements a stitching algorithm */
class Stitcher
{
public:
    /** create a stitcher for the given panorama */
    Stitcher(const PT::Panorama & pano, utils::MultiProgressDisplay & prog)
	: m_pano(pano), m_progress(prog)
    {
    }

    /** Stitch some images into a panorama file */
    virtual void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & images, const std::string & file) = 0;

    //    template<typename ImgIter, typename ImgAccessor>

    //void stitch(const PanoramaOptions & opts, UIntSet & images, vigra::triple<ImgIter, ImgIter, ImgAccessor> output);

protected:
    const PT::Panorama & m_pano;
    utils::MultiProgressDisplay & m_progress;
};
/** remap a set of images, and store the individual remapped files. */
template <typename ImageType, typename AlphaImageType>
class MultiImageRemapper : public Stitcher
{
public:
    MultiImageRemapper(const PT::Panorama & pano,
		       utils::MultiProgressDisplay & progress)
	: Stitcher(pano, progress)
    {
    }

    virtual ~MultiImageRemapper()
    {
    }

    virtual void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & images, const std::string & filename)
    {
	DEBUG_ASSERT(opts.outputFormat == PT::PanoramaOptions::TIFF_multilayer
		     || opts.outputFormat == PT::PanoramaOptions::TIFF_m);

	std::string::size_type idx = filename.rfind('.');
	if (idx != std::string::npos) {
	    m_basename = filename.substr(0, idx);
	} else {
            m_basename = filename;
        }

        // setup the output.
        prepareOutputFile(opts);

	unsigned int nImg = images.size();
	m_progress.pushTask(utils::ProgressTask("Remapping", "", 1.0/(nImg+1)));
        // number of the remapped image in the panorama (for partial remapping,
        // when the images set does not contain all input images.
        unsigned int runningImgNr;
	// remap each image and save
	for (UIntSet::const_iterator it = images.begin();
	     it != images.end(); ++it)
	{
	    // load image
	    const PT::PanoImage & img = m_pano.getImage(*it);
	    vigra::ImageImportInfo info(img.getFilename().c_str());
	    // create a RGB image of appropriate size
	    // FIXME.. use some other mechanism to define what format to use..
	    // have to look at the vigra import/export stuff how this could
	    // handled
	    ImageType srcImg(info.width(), info.height());
	    // import the image just read
	    m_progress.setMessage(std::string("loading image ") + img.getFilename());
	    importImage(info, destImage(srcImg));
	
	    m_progress.setMessage("remapping " + img.getFilename());
	
	    RemappedPanoImage<ImageType, AlphaImageType> remapped;
	    remapped.remapImage(m_pano, opts, *it, m_progress);
	
            saveRemapped(remapped, *it, m_pano.getNrOfImages(), opts);

            runningImgNr++;
        }

        finalizeOutputFile(opts);
    }

    /** prepare the output file (setup file structures etc.) */
    virtual void prepareOutputFile(const PanoramaOptions & opts)
    {
    }

    /** save a remapped image, or layer */
    virtual void saveRemapped(RemappedPanoImage<ImageType, AlphaImageType> & remapped,
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
            filename << m_basename << std::setfill('0') << std::setw(3) << imgNr << ".jpg";

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
            filename << m_basename << setfill('0') << setw(3) << imgNr << ".png";
            vigra::ImageExportInfo exinfo(filename.str().c_str());
            exinfo.setXResolution(150);
            exinfo.setYResolution(150);
            vigra::exportImage(srcImageRange(complete), exinfo);
            break;
        }
        case PanoramaOptions::TIFF_m:
        {
            std::ostringstream filename;
            filename << m_basename << setfill('0') << setw(3) << imgNr << ".tif";
            vigra::TiffImage * tiff = TIFFOpen(filename.str().c_str(), "w");

            vigra_ext::createTiffDirectory(tiff,
                                           m_pano.getImage(imgNr).getFilename(),
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
        std::string filename = m_basename + ".tif";
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
                                       m_pano.getImage(imgNr).getFilename(),
                                       m_basename,
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


template <typename ImageType, typename AlphaImageType>
class WeightedStitcher : public Stitcher
{
public:
    WeightedStitcher(const PT::Panorama & pano,
		     utils::MultiProgressDisplay & progress)
	: Stitcher(pano, progress)
    {
    }

    virtual void stitch(const PT::PanoramaOptions & opts, PT::UIntSet & imgSet, const std::string & filename)
    {
	std::vector<unsigned int> images;
	// calculate stitching order
	estimateBlendingOrder(m_pano, imgSet, images);	

	unsigned int nImg = images.size();

	m_progress.pushTask(utils::ProgressTask("Stitching", "", 1.0/(2*nImg)));	
	// create panorama canvas
	ImageType pano;
	AlphaImageType panoMask;
	
	// the output panorama
	pano.resize(opts.width, opts.getHeight());	
	panoMask.resize(opts.width, opts.getHeight());
	
	// empty ROI
	vigra_ext::ROI<vigra::Diff2D> panoROI;

	// remap each image and save
	for (UIntVector::const_iterator it = images.begin();
	     it != images.end(); ++it)
	{
	    // load image
	    const PT::PanoImage & img = m_pano.getImage(*it);
	    vigra::ImageImportInfo info(img.getFilename().c_str());
	    // create a output image of appropriate size
	    ImageType srcImg(info.width(), info.height());
	    // import the image just read
	    m_progress.setMessage(std::string("loading image ") + img.getFilename());
	    importImage(info, std::make_pair(srcImg.upperLeft(), srcImg.accessor()));
	
	    m_progress.setMessage("remapping " + img.getFilename());
	
	    RemappedPanoImage<ImageType, AlphaImageType> remapped;
	    remapped.remapImage(m_pano, opts, *it, m_progress);

	    m_progress.setMessage("blending " + img.getFilename());
	    // add image to pano and panoalpha, adjusts panoROI as well.
	    blend(remapped, destImageRange(pano), destImage(panoMask), panoROI);
	}
	
	m_progress.setMessage("saving result");
	// save the remapped image
	DEBUG_DEBUG("Saving panorama: " << filename);
	vigra::ImageExportInfo exinfo(filename.c_str());
	exinfo.setXResolution(150);
	exinfo.setYResolution(150);
	vigra::exportImage(srcImageRange(pano), exinfo);
#ifdef DEBUG
	vigra::exportImage(srcImageRange(panoMask), vigra::ImageExportInfo("pano_alpha.tif"));
#endif
	m_progress.popTask();

    }

    /** blend \p img into \p pano, using \p alpha mask and \p panoROI
     *
     *  updates \p pano, \p alpha and \p panoROI
     */
    template <typename PanoIter, typename PanoAccessor,
	      typename AlphaIter, typename AlphaAccessor>
    void blend(RemappedPanoImage<ImageType, AlphaImageType> & img,
	       vigra::triple<PanoIter, PanoIter, PanoAccessor> pano,
	       std::pair<AlphaIter, AlphaAccessor> alpha,
	       vigra_ext::ROI<vigra::Diff2D> & panoROI)
    {
	typedef typename AlphaIter::value_type AlphaValue;
	// intersect the ROI's.
	vigra_ext::ROI<vigra::Diff2D> overlap;
        const vigra_ext::ROI<vigra::Diff2D> & imgROI = img.roi();
	if (panoROI.intersect(img.roi(),overlap)) {
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
                                       (*it).apply(destIter(pano.first),panoROI) );
                    // copy mask
                    vigra::copyImageIf((*it).apply(img.alpha(),imgROI),
                                       (*it).apply(maskIter(img.alpha().first),imgROI),
                                       (*it).apply(alpha,panoROI) );

                }
            } else {
                // copy image with mask.
                vigra::copyImageIf(img.image(),
                                   maskIter(img.alpha().first),
                                   img.roi().apply(destIter(pano.first),panoROI) );
                // copy mask
                vigra::copyImageIf(img.alpha(),
                                   maskIter(img.alpha().first),
                                   img.roi().apply(alpha,panoROI) );
            }

	} else {
	    // image ROI's do not overlap, no blending, just copy
	    // alpha channel is not considered, because the whole target
	    // is free.
	    vigra::copyImage(img.image(),
			     img.roi().apply(destIter(pano.first)));

	    // copy mask
	    vigra::copyImage(img.alpha(),
			     img.roi().apply(alpha));
	}
	img.roi().unite(panoROI, panoROI);
    }

    /** blends two images, they overlap and the iterators point
     *  to excatly the same position.
     */
    template <typename PanoIter, typename PanoAccessor,
	      typename MaskIter, typename MaskAccessor>
    void blendOverlap(vigra::triple<PanoIter, PanoIter, PanoAccessor> image,
		      std::pair<MaskIter, MaskAccessor> imageMask,
		      std::pair<PanoIter, PanoAccessor> pano,
		      std::pair<MaskIter, MaskAccessor> panoMask)
    {
	vigra::Diff2D size = image.second - image.first;

#ifdef DEBUG
	// save the masks
	exportImage(srcIterRange(imageMask.first, imageMask.first + size),
                    vigra::ImageExportInfo("blendImageMask_before.tif"));
	exportImage(srcIterRange(panoMask.first, panoMask.first + size),
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
                                           m_progress);

#ifdef DEBUG
	// save the masks
	exportImage(srcImageRange(blendImageMask), vigra::ImageExportInfo("blendImageMask.tif"));
	exportImage(srcImageRange(blendPanoMask), vigra::ImageExportInfo("blendPanoMask.tif"));
	
#endif
	// copy the image into the panorama
	vigra::copyImageIf(image, vigra::maskImage(blendImageMask), pano);
	// copy mask
	vigra::copyImageIf(vigra::srcImageRange(blendImageMask), vigra::maskImage(blendImageMask), panoMask);
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

} // namespace PTools

#endif // _SIMPLESTITCHER_H
