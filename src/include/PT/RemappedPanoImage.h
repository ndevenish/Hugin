// -*- c-basic-offset: 4 -*-
/** @file RemappedPanoImage.h
 *
 *  Contains functions to transform whole images.
 *  Can use PTools::Transform or PT::SpaceTransform for the calculations
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

#ifndef _REMAPPEDPANOIMAGE_H
#define _REMAPPEDPANOIMAGE_H

#include <fstream>

#include <vigra/basicimage.hxx>
#include <vigra/impex.hxx>
#include <vigra/impexalpha.hxx>

#include <vigra_ext/Interpolators.h>
#include <vigra_ext/ROIImage.h>
#include <PT/Panorama.h>

#include <PT/PanoToolsInterface.h>
#include <common/math.h>


namespace PT
{

/** calculate the outline of the image
 *
 *  @param srcSize Size of source picture ( an be small, to
 *                 if not every point is needed)
 *  @param transf  Transformation from image to pano
 *  @param result  insert border point into result container
 *  @param ul      Upper Left corner of the image roi
 *  @param lr      Lower right corner of the image roi
 */
template <class OutputIterator, class TRANSFORM>
void calcBorderPoints(vigra::Diff2D srcSize,
                      TRANSFORM & transf,
                      OutputIterator result,
                      FDiff2D & ul,
                      FDiff2D & lr)
{
    ul.x = DBL_MAX;
    ul.y = DBL_MAX;
    lr.x = -DBL_MAX;
    lr.y = -DBL_MAX;

    int x = 0;
    int y = 0;

#ifdef DEBUG
    std::ofstream o("border_curve.txt");
#endif

    for (x=0; x<srcSize.x ; x++) {
        double sx,sy;
        transf.transformImgCoord(sx,sy,x,y);
#ifdef DEBUG
        o << x << ", " << y << "\t"
          << sx << ", " << sy << std::endl;
#endif
        if (ul.x > sx) ul.x = sx;
        if (ul.y > sy) ul.y = sy;
        if (lr.x < sx) lr.x = sx;
        if (lr.y < sy) lr.y = sy;
        *result = FDiff2D((float)sx, (float) sy);
    }
    x = srcSize.x-1;
    for (y=0; y<srcSize.y ; y++) {
        double sx,sy;
        transf.transformImgCoord(sx,sy,x,y);
#ifdef DEBUG
        o << x << ", " << y << "\t"
          << sx << ", " << sy << std::endl;
#endif
        if (ul.x > sx) ul.x = sx;
        if (ul.y > sy) ul.y = sy;
        if (lr.x < sx) lr.x = sx;
        if (lr.y < sy) lr.y = sy;
        *result = FDiff2D((float)sx, (float) sy);
    }
    y = srcSize.y-1;
    for (x=srcSize.x-1; x>0 ; --x) {
        double sx,sy;
        transf.transformImgCoord(sx,sy,x,y);
#ifdef DEBUG
        o << x << ", " << y << "\t"
          << sx << ", " << sy << std::endl;
#endif
        if (ul.x > sx) ul.x = sx;
        if (ul.y > sy) ul.y = sy;
        if (lr.x < sx) lr.x = sx;
        if (lr.y < sy) lr.y = sy;
        *result = FDiff2D((float)sx, (float) sy);
    }
    x = 0;
    for (y=srcSize.y-1 ; y > 0 ; --y) {
        double sx,sy;
        transf.transformImgCoord(sx,sy,x,y);
#ifdef DEBUG
        o << x << ", " << y << "\t"
          << sx << ", " << sy << std::endl;
#endif
        if (ul.x > sx) ul.x = sx;
        if (ul.y > sy) ul.y = sy;
        if (lr.x < sx) lr.x = sx;
        if (lr.y < sy) lr.y = sy;
        *result = FDiff2D((float)sx, (float) sy);
    }


    DEBUG_DEBUG("bounding box: upper left: " << ul.x << "," << ul.y
                << "  lower right: " << lr.x << "," << lr.y);
}

/** struct to hold a image state for stitching
 *
 */
template <class RemapImage, class AlphaImage>
class RemappedPanoImage : public vigra_ext::ROIImage<RemapImage, AlphaImage>
{
public:
// typedefs for the children types
    typedef typename RemapImage::value_type      image_value_type;
    typedef typename RemapImage::traverser       image_traverser;
    typedef typename RemapImage::const_traverser const_image_traverser;
    typedef typename RemapImage::Accessor        ImageAccessor;
    typedef typename RemapImage::ConstAccessor   ConstImageAccessor;

    typedef typename AlphaImage::value_type       mask_value_type;
    typedef typename AlphaImage::traverser        mask_traverser;
    typedef typename AlphaImage::const_traverser  const_mask_traverser;
    typedef typename AlphaImage::Accessor         MaskAccessor;
    typedef typename AlphaImage::ConstAccessor    ConstMaskAccessor;

    typedef vigra_ext::ROIImage<RemapImage, AlphaImage> Base;

    /** create a remapped pano image
     *
     *  the actual remapping is done by the remapImage() function.
     */
    RemappedPanoImage()
    {
    }

    /** set a new image or panorama options
     *
     *  calculates bounding box, and outline
     */
    void setPanoImage(const PT::Panorama & pano, unsigned int imgNr, const PT::PanoramaOptions & opts)
    {
        const PT::PanoImage & img = pano.getImage(imgNr);

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
	// outline of this image in final panorama
	FDiff2D ulFloat;
	FDiff2D lrFloat;
	
	m_outline.clear();
	calcBorderPoints(srcSize, invTransf, back_inserter(m_outline),
			 ulFloat, lrFloat);
	if (opts.projectionFormat == PanoramaOptions::EQUIRECTANGULAR) {

	    // check if image overlaps the pole
	    double cw = opts.width / opts.HFOV * 360;
	    double startx = - (cw - opts.width)/2;
	    double stopx = opts.width + (cw-opts.width)/2;

	    // handle image overlaps pole case..
	    if (ulFloat.x <= startx + opts.width * 0.1  && lrFloat.x >= stopx - opts.width * 0.1) {
		// image in northern hemisphere
		if (ulFloat.y < opts.getHeight() / 2 ) {
		    ulFloat.y = 0;
		}
		// image in southern hemisphere
		if (lrFloat.y > opts.getHeight() / 2 ) {
		    lrFloat.y = opts.getHeight();
		}
	    }
	}
	// restrict to panorama size
	ulFloat = simpleClipPoint(ulFloat, FDiff2D(0,0), FDiff2D(opts.width, opts.getHeight()));
	lrFloat = simpleClipPoint(lrFloat, FDiff2D(0,0), FDiff2D(opts.width, opts.getHeight()));

	DEBUG_DEBUG("imgnr: " << imgNr << " ROI: " << ulFloat << ", " << lrFloat << std::endl);

	// create an image with the right size..
	vigra::Point2D ulInt(static_cast<int>(ceil(ulFloat.x)),
                            static_cast<int>(ceil(ulFloat.y)));
	vigra::Point2D lrInt(static_cast<int>(floor(lrFloat.x)),
                            static_cast<int>(floor(lrFloat.y)));
        DEBUG_DEBUG("after rounding: " << ulInt << ", " << lrInt << ", size: "
                    << lrInt - ulInt);

        // restrict to panorama size
        vigra::Rect2D imageRect(ulInt, lrInt + vigra::Point2D(1,1));
        vigra::Rect2D panoRect(0,0, opts.getWidth(), opts.getHeight());
	Base::resize(imageRect & panoRect);
	DEBUG_DEBUG("after resize: " << Base::m_region);
    }


    /** calculate distance map. pixels contain distance from image center
     */
    template<class DistImgType>
    void calcDistMap(const PT::Panorama & pano, const PT::PanoramaOptions & opts,
		          unsigned int imgNr, DistImgType & img)
    {
        setPanoImage(pano, imgNr, opts);
        vigra::Diff2D srcSize(pano.getImage(imgNr).getWidth(),
            pano.getImage(imgNr).getHeight());
        img.resize(Base::boundingBox().size());
        // calculate the alpha channel,
        int xstart = Base::boundingBox().left();
        int xend   = Base::boundingBox().right();
        int ystart = Base::boundingBox().top();
        int yend   = Base::boundingBox().bottom();

        // hack.. doesn't belong here..
        int interpolHalfWidth=0;
        switch (opts.interpolator) {
           case vigra_ext::INTERP_CUBIC:
               interpolHalfWidth = vigra_ext::interp_cubic::size/2;
               break;
           case vigra_ext::INTERP_SPLINE_16:
               interpolHalfWidth = vigra_ext::interp_spline16::size/2;
               break;
           case vigra_ext::INTERP_SPLINE_36:
               interpolHalfWidth = vigra_ext::interp_spline36::size/2;
               break;
           case vigra_ext::INTERP_SPLINE_64:
               interpolHalfWidth = vigra_ext::interp_spline64::size/2;
               break;
           case vigra_ext::INTERP_SINC_256:
               interpolHalfWidth = vigra_ext::interp_sinc<8>::size/2;
               break;
           case vigra_ext::INTERP_BILINEAR:
               interpolHalfWidth = vigra_ext::interp_bilin::size/2;
               break;
           case vigra_ext::INTERP_NEAREST_NEIGHBOUR:
               interpolHalfWidth = vigra_ext::interp_nearest::size/2;
               break;
           case vigra_ext::INTERP_SINC_1024:
               interpolHalfWidth = vigra_ext::interp_sinc<32>::size/2;
               break;
        }



        PTools::Transform transf;
	
    	transf.createTransform(pano, imgNr, opts, srcSize);

        vigra::Diff2D srcMiddle = srcSize / 2;

	    // create dist y iterator
	    typename DistImgType::Iterator yalpha(img.upperLeft());
	    // loop over the image and transform
	    for(int y=ystart; y < yend; ++y, ++yalpha.y)
	    {
	        // create x iterators
            typename DistImgType::Iterator xalpha(yalpha);
            for(int x=xstart; x < xend; ++x, ++xalpha.x)
            {
                double sx,sy;
                transf.transformImgCoord(sx,sy,x,y);
                // make sure that the interpolator doesn't
                // access pixels outside.. Should we introduce
                // some sort of border treatment?
                if (sx < interpolHalfWidth -1
                    || sx > srcSize.x - interpolHalfWidth - 1
                    || sy < interpolHalfWidth - 1
                    || sy > srcSize.y-interpolHalfWidth - 1)
                {
                    *xalpha = 0;
                    // nothing..
                } else {
                    double mx = sx - srcMiddle.x;
                    double my = sy - srcMiddle.y;
                    *xalpha = sqrt(mx*mx + my*my);
                }
            }
        }
    }

    /** calculate only the alpha channel.
     *  works for arbitrary transforms, with holes and so on,
     *  but is very crude and slow (remapps all image pixels...)
     *
     *  better transform all images, and get the alpha channel for free!
     *
     *  this is a hack!

     */
    void calcAlpha(const PT::Panorama & pano, const PT::PanoramaOptions & opts,
		   unsigned int imgNr)
    {
        DEBUG_DEBUG("imgNr: " << imgNr);
	setPanoImage(pano, imgNr, opts);
	vigra::Diff2D srcSize(pano.getImage(imgNr).getWidth(),
			      pano.getImage(imgNr).getHeight());
	Base::m_mask.resize(Base::boundingBox().size());
	// calculate the alpha channel,
	int xstart = Base::boundingBox().left();
	int xend   = Base::boundingBox().right();
	int ystart = Base::boundingBox().top();
	int yend   = Base::boundingBox().bottom();

	// hack.. doesn't belong here..
	int interpolHalfWidth=0;
	switch (opts.interpolator) {
	case vigra_ext::INTERP_CUBIC:
	    interpolHalfWidth = vigra_ext::interp_cubic::size/2;
	    break;
	case vigra_ext::INTERP_SPLINE_16:
	    interpolHalfWidth = vigra_ext::interp_spline16::size/2;
	    break;
	case vigra_ext::INTERP_SPLINE_36:
	    interpolHalfWidth = vigra_ext::interp_spline36::size/2;
	    break;
	case vigra_ext::INTERP_SPLINE_64:
	    interpolHalfWidth = vigra_ext::interp_spline64::size/2;
	    break;
	case vigra_ext::INTERP_SINC_256:
	    interpolHalfWidth = vigra_ext::interp_sinc<8>::size/2;
	    break;
	case vigra_ext::INTERP_BILINEAR:
	    interpolHalfWidth = vigra_ext::interp_bilin::size/2;
	    break;
	case vigra_ext::INTERP_NEAREST_NEIGHBOUR:
	    interpolHalfWidth = vigra_ext::interp_nearest::size/2;
	    break;
	case vigra_ext::INTERP_SINC_1024:
	    interpolHalfWidth = vigra_ext::interp_sinc<32>::size/2;
	    break;
	}
	
	PTools::Transform transf;
	
	transf.createTransform(pano, imgNr, opts, srcSize);

	// create dist y iterator
	typename AlphaImage::Iterator yalpha(Base::m_mask.upperLeft());
	// loop over the image and transform
	for(int y=ystart; y < yend; ++y, ++yalpha.y)
	{
	     // create x iterators
	     typename AlphaImage::Iterator xalpha(yalpha);
	     for(int x=xstart; x < xend; ++x, ++xalpha.x)
	     {
		 double sx,sy;
		 transf.transformImgCoord(sx,sy,x,y);
		 // make sure that the interpolator doesn't
		 // access pixels outside.. Should we introduce
		 // some sort of border treatment?
		 if (sx < interpolHalfWidth -1
		     || sx > srcSize.x - interpolHalfWidth - 1
		     || sy < interpolHalfWidth - 1
		     || sy > srcSize.y-interpolHalfWidth - 1)
		 {
		     *xalpha = 0;
		     // nothing..
		 } else {
		     *xalpha = 255;
		 }
	     }
	 }
    }

    /** remap a image */
    template <class ImgIter, class ImgAccessor>
    void remapImage(const PT::Panorama & pano,
		    const PT::PanoramaOptions & opts,
                    vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
		    unsigned int imgNr,
		    utils::MultiProgressDisplay & progress)
    {
        DEBUG_TRACE("Image: " << imgNr);
        setPanoImage(pano, imgNr, opts);

//        std::ostringstream msg;
//        msg <<"remapping image "  << imgNr;
//        progress.setMessage(msg.str().c_str());

	PTools::Transform transf;
        vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;
	transf.createTransform(pano, imgNr, opts, srcImgSize);

	transformImage(srcImg,
                       destImageRange(Base::m_image),
                       destImage(Base::m_alpha),
                       Base::boundingBox().upperLeft(),
                       transf,
                       opts.interpolator,
                       progress);
    }

    /** remap a image */
    template <class ImgIter, class ImgAccessor,
              class AlphaIter, class AlphaAccessor>
    void remapImage(const PT::Panorama & pano,
                    const PT::PanoramaOptions & opts,
                    vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                    std::pair<AlphaIter, AlphaAccessor> alphaImg,
                    unsigned int imgNr,
                    utils::MultiProgressDisplay & progress)
    {
        DEBUG_TRACE("Image: " << imgNr);
        setPanoImage(pano, imgNr, opts);

        PTools::Transform transf;
        vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;
        transf.createTransform(pano, imgNr, opts, srcImgSize);

        transformImageAlpha(srcImg,
                        alphaImg,
                        destImageRange(Base::m_image),
                        destImage(Base::m_mask),
                        Base::boundingBox().upperLeft(),
                        transf,
                        opts.interpolator,
                        progress);
    }


    const std::vector<FDiff2D> & getOutline()
    {
        return m_outline;
    }

protected:
    /// outline of panorama
    std::vector<FDiff2D> m_outline;
};


/** functor to create a remapped image */
template <typename ImageType, typename AlphaType>
class SingleImageRemapper
{
public:
    /** create a remapped pano image.
     *
     *  The image ownership is transferred to the caller.
     */
    virtual
    RemappedPanoImage<ImageType, AlphaType> *
    getRemapped(const Panorama & pano,
               const PanoramaOptions & opts,
               unsigned int imgNr, utils::MultiProgressDisplay & progress) = 0;

	virtual	void
	release(RemappedPanoImage<ImageType,AlphaType> * d) = 0;

};

/** functor to create a remapped image, loads image from disk */
template <typename ImageType, typename AlphaType>
class FileRemapper : public SingleImageRemapper<ImageType, AlphaType>
{
public:

	FileRemapper()
	{
		m_remapped = 0;
	}

    /** create a remapped pano image.
     *
     *  load the file from disk, and remap in memory
     */
    virtual
    RemappedPanoImage<ImageType, AlphaType> *
    getRemapped(const Panorama & pano, const PanoramaOptions & opts,
             unsigned int imgNr, utils::MultiProgressDisplay & progress)
    {
        DEBUG_TRACE("Image: " << imgNr);
        // load image
        const PT::PanoImage & img = pano.getImage(imgNr);
        vigra::ImageImportInfo info(img.getFilename().c_str());
        // create an image of the right size
        ImageType srcImg(info.width(), info.height());
        AlphaType srcAlpha(info.width(), info.height(), 1);

        // import the image just read
        progress.setMessage(std::string("loading ") + utils::stripPath(img.getFilename()));

        // import with alpha channel
        vigra::importImageAlpha(info, vigra::destImage(srcImg),
                                       vigra::destImage(srcAlpha));

        m_remapped = new RemappedPanoImage<ImageType, AlphaType>;
        DEBUG_TRACE("starting remap of image: " << imgNr);
        m_remapped->remapImage(pano, opts,
                             vigra::srcImageRange(srcImg),
                             vigra::srcImage(srcAlpha),
                             imgNr, progress);
        return m_remapped;
    }

	virtual	void
	release(RemappedPanoImage<ImageType,AlphaType> * d)
	{
		delete d;
	}

protected:
	RemappedPanoImage<ImageType,AlphaType> * m_remapped;
};

}; // namespace

#endif
