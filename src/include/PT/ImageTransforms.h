// -*- c-basic-offset: 4 -*-
/** @file ImageTransforms.h
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

#ifndef _IMAGETRANSFORMS_H
#define _IMAGETRANSFORMS_H

#include <fstream>

#include <vigra/basicimage.hxx>
#include <vigra_ext/ROI.h>
#include <vigra_ext/LayerImage.h>
#include <vigra_ext/Interpolators.h>
#include <vigra/impex.hxx>
#include <vigra/impexalpha.hxx>

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

/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the boundig
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interp Interpolator class (calculates weights for interpolation)
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                         std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                         TRANSFORM & transform,
                         vigra::Diff2D destUL,
                         Interpolator interp,
                         utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;

    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;
    // FIXME: use d & e here.
    vigra::Diff2D srcMiddle = srcSize / 2;

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

    //InterpolatingAccessor(src.third, interp);
    vigra_ext::InterpolatingAccessor<SrcAccessor,
                            typename SrcAccessor::value_type,
                            Interpolator> interpol(src.third, interp);


//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

//    vigra::BilinearInterpolatingAccessor interpol(src.third);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    AlphaImageIterator ydist(alpha.first);
    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        AlphaImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            transform.transformImgCoord(sx,sy,x,y);
            // make sure that the interpolator doesn't
            // access pixels outside.. Should we introduce
            // some sort of border treatment?
            if (sx < interp.size/2 -1
                || sx > srcSize.x-interp.size/2 - 1
                || sy < interp.size/2 - 1
                || sy > srcSize.y-interp.size/2 - 1)
            {
                *xdist = 0;
                // nothing..
            } else {
//                cout << x << "," << y << " -> " << sx << "," << sy << " " << std::endl;

//                nearest neighbour
//                *xd = src.third(src.first, vigra::Diff2D((int)round(sx), (int)round(sy)));
                // use given interpolator function.
                *xd = interpol(src.first, sx, sy);
                *xdist = 255;
            }
        }
        if ((y-ystart)%100 == 0) {
            prog.setProgress(((double)y-ystart)/(yend-ystart));
        }
    }
    prog.popTask();
}

/** transform input images with alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageAlphaIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                               std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                               vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                               std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                               TRANSFORM & transform,
                               vigra::Diff2D destUL,
                               Interpolator interp,
                               utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;

    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;
    // FIXME: use d & e here.
//    vigra::Diff2D srcMiddle = srcSize / 2;

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

    //InterpolatingAccessor(src.third, interp);
    vigra_ext::InterpolatingAccessor<SrcAccessor,
                                     typename SrcAccessor::value_type,
                                     Interpolator> interpol(src.third, interp);

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

//    vigra::BilinearInterpolatingAccessor interpol(src.third);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    AlphaImageIterator ydist(alpha.first);

    typename SrcAccessor::value_type tempval;

    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        AlphaImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            transform.transformImgCoord(sx,sy,x,y);
            // make sure that the interpolator doesn't
            // access pixels outside.. Should we introduce
            // some sort of border treatment?
            if (sx < interp.size/2 -1
                || sx > srcSize.x-interp.size/2 - 1
                || sy < interp.size/2 - 1
                || sy > srcSize.y-interp.size/2 - 1)
            {
                *xdist = 0;
                // nothing..
            } else if (interpol(src.first, srcAlpha, sx, sy, tempval)) {
                *xd = tempval;
                *xdist = 255;
            } else {
                *xdist = 0;
            }
        }
        if ((y-ystart)%100 == 0) {
            prog.setProgress(((double)y-ystart)/(yend-ystart));
        }
    }
    prog.popTask();
}



/** struct to hold a image state for stitching
 *
 */
template <class RemapImage, class AlphaImage>
class RemappedPanoImage : public vigra_ext::LayerImage<RemapImage, AlphaImage>
{
public:

    typedef vigra_ext::LayerImage<RemapImage, AlphaImage> Base;

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
	if (opts.getProjection() == PanoramaOptions::EQUIRECTANGULAR) {

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
	vigra::Diff2D ulInt(static_cast<int>(floor(ulFloat.x)),
                            static_cast<int>(floor(ulFloat.y)));
	vigra::Diff2D lrInt(static_cast<int>(ceil(lrFloat.x)),
                            static_cast<int>(ceil(lrFloat.y)));
        DEBUG_DEBUG("after rounding: " << ulInt << ", " << lrInt << ", size: "
                    << lrInt - ulInt);

	Base::m_ROI.setCorners(ulInt,lrInt);
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
        img.resize(Base::m_ROI.size());
        // calculate the alpha channel,
        int xstart = Base::m_ROI.getUL().x;
        int xend   = Base::m_ROI.getLR().x;
        int ystart = Base::m_ROI.getUL().y;
        int yend   = Base::m_ROI.getLR().y;

        // hack.. doesn't belong here..
        int interpolHalfWidth=0;
        switch (opts.interpolator) {
            case PT::PanoramaOptions::CUBIC:
                interpolHalfWidth = vigra_ext::interp_cubic::size/2;
                break;
            case PT::PanoramaOptions::SPLINE_16:
                interpolHalfWidth = vigra_ext::interp_spline16::size/2;
                break;
            case PT::PanoramaOptions::SPLINE_36:
                interpolHalfWidth = vigra_ext::interp_spline36::size/2;
                break;
            case PT::PanoramaOptions::SPLINE_64:
                interpolHalfWidth = vigra_ext::interp_spline64::size/2;
                break;
            case PT::PanoramaOptions::SINC_256:
                interpolHalfWidth = vigra_ext::interp_sinc<8>::size/2;
                break;
            case PT::PanoramaOptions::BILINEAR:
                interpolHalfWidth = vigra_ext::interp_bilin::size/2;
                break;
            case PT::PanoramaOptions::NEAREST_NEIGHBOUR:
                interpolHalfWidth = vigra_ext::interp_nearest::size/2;
                break;
            case PT::PanoramaOptions::SINC_1024:
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
     *  @BUG doesn't take the interpolator size into accout, so usually the border
     *       will be several pixels smaller.
     */
    void calcAlpha(const PT::Panorama & pano, const PT::PanoramaOptions & opts,
		   unsigned int imgNr)
    {
	setPanoImage(pano, imgNr, opts);
	vigra::Diff2D srcSize(pano.getImage(imgNr).getWidth(),
			      pano.getImage(imgNr).getHeight());
	Base::m_alpha.resize(Base::m_ROI.size());
	// calculate the alpha channel,
	int xstart = Base::m_ROI.getUL().x;
	int xend   = Base::m_ROI.getLR().x;
	int ystart = Base::m_ROI.getUL().y;
	int yend   = Base::m_ROI.getLR().y;

	// hack.. doesn't belong here..
	int interpolHalfWidth=0;
	switch (opts.interpolator) {
	case PT::PanoramaOptions::CUBIC:
	    interpolHalfWidth = vigra_ext::interp_cubic::size/2;
	    break;
	case PT::PanoramaOptions::SPLINE_16:
	    interpolHalfWidth = vigra_ext::interp_spline16::size/2;
	    break;
	case PT::PanoramaOptions::SPLINE_36:
	    interpolHalfWidth = vigra_ext::interp_spline36::size/2;
	    break;
	case PT::PanoramaOptions::SPLINE_64:
	    interpolHalfWidth = vigra_ext::interp_spline64::size/2;
	    break;
	case PT::PanoramaOptions::SINC_256:
	    interpolHalfWidth = vigra_ext::interp_sinc<8>::size/2;
	    break;
	case PT::PanoramaOptions::BILINEAR:
	    interpolHalfWidth = vigra_ext::interp_bilin::size/2;
	    break;
	case PT::PanoramaOptions::NEAREST_NEIGHBOUR:
	    interpolHalfWidth = vigra_ext::interp_nearest::size/2;
	    break;
	case PT::PanoramaOptions::SINC_1024:
	    interpolHalfWidth = vigra_ext::interp_sinc<32>::size/2;
	    break;
	}
	
	PTools::Transform transf;
	
	transf.createTransform(pano, imgNr, opts, srcSize);

	// create dist y iterator
	typename AlphaImage::Iterator yalpha(Base::m_alpha.upperLeft());
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
	setPanoImage(pano, imgNr, opts);
        // load image
	Base::m_image.resize(Base::m_ROI.size());
	Base::m_alpha.resize(Base::m_ROI.size());

//        std::ostringstream msg;
//        msg <<"remapping image "  << imgNr;
//        progress.setMessage(msg.str().c_str());

	PTools::Transform transf;
        vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;
	transf.createTransform(pano, imgNr, opts, srcImgSize);

	transformImage(srcImg,
                       destImageRange(Base::m_image),
                       destImage(Base::m_alpha),
                       Base::m_ROI.getUL(),
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
	setPanoImage(pano, imgNr, opts);
        // load image
	Base::m_image.resize(Base::m_ROI.size());
	Base::m_alpha.resize(Base::m_ROI.size());


	PTools::Transform transf;
        vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;
	transf.createTransform(pano, imgNr, opts, srcImgSize);

	transformImageAlpha(srcImg,
                            alphaImg,
			    destImageRange(Base::m_image),
			    destImage(Base::m_alpha),
			    Base::m_ROI.getUL(),
			    transf,
			    opts.interpolator,
			    progress);
    }

    /** remaps image into output panorama and create alpha channel for this
     *  image.
     *
     *  this functions load the image file from disk and calls the other
     *  remapImage functions.
     *
     *  @param progress progress reporting class
     *
     */
/*
    void remapImage(const PT::Panorama & pano,
		    const PT::PanoramaOptions & opts,
		    unsigned int imgNr,
		    utils::MultiProgressDisplay & progress)
    {

        // load image
        const PT::PanoImage & img = pano.getImage(imgNr);
        vigra::ImageImportInfo info(img.getFilename().c_str());
        // create a RGB image of appropriate size
        // FIXME.. use some other mechanism to define what format to use..
	// have to look at the vigra import/export stuff how this could
	// handled
        // FIXME, check for alpha channel..
        RemapImage srcImg(info.width(), info.height());
        // import the image just read
        importImage(info, destImage(srcImg));


	m_image.resize(m_ROI.size());
	m_alpha.resize(m_ROI.size());

	PTools::Transform transf;
	transf.createTransform(pano, imgNr, opts, srcImg.size());

        remapImage(pano, opts, srcImageRange(srcImg),
                   imgNr, progress);
    }
*/

    const std::vector<FDiff2D> & getOutline()
    {
        return m_outline;
    }


#if 0
    /** get the alpha value of point @p p (in panorama coordinates).
     *
     *  Obselete.. better use getAlpha()/getImage() and maybe ROI::apply()
     * @param p point in output panorama coordinates.
     *
     * @return distance to center of image,
     *         or FLT_MAX if outside remapped image.
     */
    typename AlphaImage::value_type getAlpha(vigra::Diff2D & p)
    {
	vigra::Diff2D lr = ul + image.size() - vigra::Diff2D(1,1);
	if (p.x < ul.x || p.x >= lr.x || p.y < ul.y || p.y >= lr.y) {
	    // not in ROI
	    return NumericTraits<AlphaImage::value_type>::zero();
	}
	return dist.accessor()(dist.upperLeft()+(p-ul));
    }


    typename RemapImage::value_type get(const vigra::Diff2D & p)
    {
//            vigra::Diff2D lr = ul + image.size() - vigra::Diff2D(1,1);
//            if (p.x < ul.x || p.x >= lr.x || p.y < ul.y || p.y >= lr.y) {
//                DEBUG_DEBUG("point not in ROI, p: " << p << " ul: " << ul << " lr: " << lr);
//            }
	return image.accessor()(image.upperLeft()+(p-ul));
    }

#endif

protected:
    /// outline of panorama
    std::vector<FDiff2D> m_outline;
};

/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the boundig
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interp Interpolator class (calculates weights for interpolation)
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class DistImageIterator, class DistAccessor,
          class Interpolator>
void transformImageDist(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                        vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                        vigra::Diff2D destUL,
                        TRANSFORM & transform,
                        vigra::triple<DistImageIterator, DistImageIterator, DistAccessor> centerDist,
                        Interpolator & interp,
                        utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;
    vigra::Diff2D distSize = centerDist.second - centerDist.first;

    bool calcDist=true;
    if (distSize.x == 0 && distSize.y == 0) {
        calcDist=false;
    }

    if (calcDist) {
        DEBUG_ASSERT(distSize == destSize);
    }
    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;
    // FIXME: use d & e here.
    vigra::Diff2D srcMiddle = srcSize / 2;

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

    //InterpolatingAccessor(src.third, interp);
    vigra_ext::InterpolatingAccessor<SrcAccessor,
                          typename SrcAccessor::value_type,
                          Interpolator> interpol(src.third, interp);


//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

//    vigra::BilinearInterpolatingAccessor interpol(src.third);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    DistImageIterator ydist(centerDist.first);
    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        DistImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            transform.transformImgCoord(sx,sy,x,y);
            // make sure that the interpolator doesn't
            // access pixels outside.. Should we introduce
            // some sort of border treatment?
            if (sx < interp.size/2 -1
                || sx > srcSize.x-interp.size/2 - 1
                || sy < interp.size/2 - 1
                || sy > srcSize.y-interp.size/2 - 1)
            {
                if (calcDist) {
                    // save an invalid distance
                    *xdist = FLT_MAX;
                }
                // nothing..
            } else {
//                cout << x << "," << y << " -> " << sx << "," << sy << " " << std::endl;

//                nearest neighbour
//                *xd = src.third(src.first, vigra::Diff2D((int)round(sx), (int)round(sy)));
                // use given interpolator function.
                *xd = interpol(src.first, sx, sy);
                if (calcDist) {
                    double mx = sx - srcMiddle.x;
                    double my = sy - srcMiddle.y;
                    *xdist = sqrt(mx*mx + my*my);
                }
            }
        }
        if ((y-ystart)%100 == 0) {
            prog.setProgress(((double)y-ystart)/(yend-ystart));
        }
    }
    prog.popTask();
}


/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the boundig
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interpol Interpolation algorithm that should be used.
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class AlphaImageIterator, class AlphaAccessor,
          class TRANSFORM>
void transformImage(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                    vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                    std::pair<AlphaImageIterator, AlphaAccessor> alpha,
		    vigra::Diff2D destUL,
                    TRANSFORM & transform,
                    PT::PanoramaOptions::Interpolator interpol,
                    utils::MultiProgressDisplay & progress)
{
    switch (interpol) {
    case PT::PanoramaOptions::CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
	PT::transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_cubic(),
                                 progress);
	break;
    case PT::PanoramaOptions::SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
	PT::transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline16(),
                                 progress);
	break;
    case PT::PanoramaOptions::SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
	PT::transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline36(),
                                 progress);
	break;
    case PT::PanoramaOptions::SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
	PT::transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline64(),
                                 progress);
	break;
    case PT::PanoramaOptions::SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
	PT::transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_sinc<8>(),
                                 progress);
	break;
    case PT::PanoramaOptions::BILINEAR:
	PT::transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_bilin(),
                                 progress);
	break;
    case PT::PanoramaOptions::NEAREST_NEIGHBOUR:
	PT::transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_nearest(),
                                 progress);
	break;
    case PT::PanoramaOptions::SINC_1024:
	PT::transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_sinc<32>(),
                                 progress);
	break;
    }
}

/** Transform image, and respect a possible alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class AlphaImageIterator, class AlphaAccessor,
          class TRANSFORM>
void transformImageAlpha(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                         vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                         std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                         vigra::Diff2D destUL,
                         TRANSFORM & transform,
                         PanoramaOptions::Interpolator interpol,
                         utils::MultiProgressDisplay & progress)
{
    switch (interpol) {
    case PT::PanoramaOptions::CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
	PT::transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_cubic(),
				      progress);
	break;
    case PT::PanoramaOptions::SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
	PT::transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_spline16(),
				      progress);
	break;
    case PT::PanoramaOptions::SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
	PT::transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_spline36(),
				      progress);
	break;
    case PT::PanoramaOptions::SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
	PT::transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_spline64(),
				      progress);
	break;
    case PT::PanoramaOptions::SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
	PT::transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_sinc<8>(),
				      progress);
	break;
    case PT::PanoramaOptions::BILINEAR:
	PT::transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_bilin(),
				      progress);
	break;
    case PT::PanoramaOptions::NEAREST_NEIGHBOUR:
	PT::transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_nearest(),
				      progress);
	break;
    case PT::PanoramaOptions::SINC_1024:
	PT::transformImageAlphaIntern(src,srcAlpha, dest, alpha, transform, destUL,
				      vigra_ext::interp_sinc<32>(),
				      progress);
	break;
    }
}

template <class T, int nr>
void fillVector(T vec[3], T &val, int len)
{
    for (int i=0; i<len; i++) vec[i] = val;
}



/** functor to create a remapped image */
template <typename ImageType, typename AlphaType>
class SingleImageRemapper
{
public:
    /** create a remapped pano image. It stays valid
     *  until operator() is called again, or the
     *  remapper object is destroyed.
     *
     *  the image can be deleted explicitly
     *  with the the release() function
     */
    virtual
    RemappedPanoImage<ImageType, AlphaType> &
    operator()(const Panorama & pano,
               const PanoramaOptions & opts,
               unsigned int imgNr, utils::MultiProgressDisplay & progress) = 0;

    virtual void
    release() = 0;
};

/** functor to create a remapped image */
template <typename ImageType, typename AlphaType>
class FileRemapper : public SingleImageRemapper<ImageType, AlphaType>
{
public:
    FileRemapper()
        : m_remapped(0) { };

    virtual ~FileRemapper()
    {
        release();
    }

    /** create a remapped pano image.
     *
     *  load the file from disk, and remap in memory
     */
    virtual
    RemappedPanoImage<ImageType, AlphaType> &
    operator()(const Panorama & pano, const PanoramaOptions & opts,
             unsigned int imgNr, utils::MultiProgressDisplay & progress)
    {
        // release old image.
	release();

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
        m_remapped->remapImage(pano, opts,
                             vigra::srcImageRange(srcImg),
                             vigra::srcImage(srcAlpha),
                             imgNr, progress);
        return *m_remapped;
    }

    virtual
    void release()
    {
        if (m_remapped) {
            delete m_remapped;
            m_remapped = 0;
        }
    }
protected:
    RemappedPanoImage<ImageType, AlphaType> * m_remapped;
};


}; // namespace

#endif // _IMAGETRANSFORMS_H
