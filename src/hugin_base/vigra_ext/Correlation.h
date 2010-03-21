// -*- c-basic-offset: 4 -*-
/** @file Correlation.h
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

#ifndef VIGRA_EXT_CORRELATION_H
#define VIGRA_EXT_CORRELATION_H

#include <hugin_shared.h>
#include <vigra/stdimage.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/copyimage.hxx>
#include <vigra/resizeimage.hxx>
#include <vigra/transformimage.hxx>

#include <vigra/impex.hxx>

#include "hugin_utils/utils.h"
#include "hugin_math/hugin_math.h"
#include "vigra_ext/Pyramid.h"
#include "vigra_ext/FitPolynom.h"
#include "vigra_ext/utils.h"

// hmm.. not really great.. should be part of vigra_ext
#include "vigra_ext/ImageTransforms.h"

#define VIGRA_EXT_USE_FAST_CORR

//#define DEBUG_WRITE_FILES

namespace vigra_ext{

/** Maximum of correlation, position and value */
struct CorrelationResult
{
    CorrelationResult()
        : maxi(-1), maxpos(0,0), curv(0,0), maxAngle(0)
        { }
    // value at correlation peak.
    double maxi;
    hugin_utils::FDiff2D maxpos;
    // curvature of the correlation peak
    hugin_utils::FDiff2D curv;
    double maxAngle;
};

/** correlate a template with an image.
 *
 *  This tries to be faster than the other version, because
 *  it uses the image data directly.
 *
 *  most code is taken from vigra::convoluteImage.
 *  See its documentation for further information.
 *
 *  Correlation result already contains the maximum position
 *  and its correlation value.
 *  it should be possible to set a threshold here.
 */
template <class SrcImage, class DestImage, class KernelImage>
CorrelationResult correlateImageFast(SrcImage & src,
                                     DestImage & dest,
                                     KernelImage & kernel,
                                     vigra::Diff2D kul, vigra::Diff2D klr,
                                     double threshold = 0.7 )
{
    vigra_precondition(kul.x <= 0 && kul.y <= 0,
                 "convolveImage(): coordinates of "
                 "kernel's upper left must be <= 0.");
    vigra_precondition(klr.x >= 0 && klr.y >= 0,
                 "convolveImage(): coordinates of "
                 "kernel's lower right must be >= 0.");

    // use traits to determine SumType as to prevent possible overflow
    typedef typename
        vigra::NumericTraits<typename SrcImage::value_type>::RealPromote SumType;
    typedef typename
        vigra::NumericTraits<typename KernelImage::value_type>::RealPromote KSumType;
    typedef
        vigra::NumericTraits<typename DestImage::value_type> DestTraits;

    // calculate width and height of the image
    int w = src.width();
    int h = src.height();
    int wk = kernel.width();
    int hk = kernel.height();

//    DEBUG_DEBUG("correlate Image srcSize " << (slr - sul).x << "," << (slr - sul).y
//                << " tmpl size: " << wk << "," << hk);

    vigra_precondition(w >= wk && h >= hk,
                 "convolveImage(): kernel larger than image.");

//    int x,y;
    int ystart = -kul.y;
    int yend   = h-klr.y;
    int xstart = -kul.x;
    int xend   = w-klr.x;

    // mean of kernel
    KSumType kmean=0;
    for(int y=0; y < hk; y++) {
        for(int x=0; x < wk; x++) {
            kmean += kernel(x,y);
        }
    }
    kmean = kmean / (hk*wk);

    CorrelationResult res;

    // create y iterators, they iterate over the rows.
//    DestIterator yd = dul + vigra::Diff2D(xstart, ystart);
//    SrcIterator ys = sul + vigra::Diff2D(xstart, ystart);


//    DEBUG_DEBUG("size: " << w << "," <<  h << " ystart: " << ystart <<", yend: " << yend);
    int unifwarned=0;
    for(int yr=ystart; yr < yend; ++yr)
    {
        // create x iterators, they iterate the coorelation over
        // the columns
//        DestIterator xd(yd);
//        SrcIterator xs(ys);

        for(int xr=xstart; xr < xend; ++xr)
        {
            if (dest(xr,yr) < threshold) {
                continue;
            }
//            int x0, y0, x1, y1;

//            y0 = kul.y;
//            y1 = klr.y;
//            x0 = kul.x;
//            x1 = klr.x;;

            // init the sum
            SumType numerator = 0;
            SumType div1 = 0;
            SumType div2 = 0;
            SumType spixel = 0;
            KSumType kpixel = 0;

            // create inner y iterators
            // access to the source image
//            SrcIterator yys = xs + kul;
            // access to the kernel image
//            KernelIterator yk  = ki + kul;

            // mean of image patch
            KSumType mean=0;
			int ym;
            for(ym=yr+kul.y; ym <= yr+klr.y; ym++) {
                for(int xm=xr+kul.x; xm <= xr+klr.x; xm++) {
                    mean += src(xm,ym);
                }
            }
            mean = mean / (hk*wk);

            // perform correlation (inner loop)
            ym=yr+kul.y;
            int yk;
            for(yk=0; yk < hk; yk++) {
                int xm=xr+kul.x;
                int xk;
                for(xk=0; xk < wk; xk++) {
                    spixel = src(xm,ym) - mean;
                    kpixel = kernel(xk,yk) - kmean;
                    numerator += kpixel * spixel;
                    div1 += kpixel * kpixel;
                    div2 += spixel * spixel;
		    xm++;
                }
		ym++;
            }
	    if (div1*div2 == 0) {
		// This happens when one of the patches is perfectly uniform
		numerator = 0;	// Set correlation to zero since this is uninteresting
		if (!unifwarned) {
		    DEBUG_DEBUG("Uniform patch(es) during correlation computation");
		    unifwarned=1;
		}
	    } else
		numerator = (numerator/sqrt(div1 * div2));
	    
            if (numerator > res.maxi) {
                res.maxi = numerator;
                res.maxpos.x = xr;
                res.maxpos.y = yr;
            }
            dest(xr,yr) = DestTraits::fromRealPromote(numerator);
        }
    }
    return res;
}

/** find the subpixel maxima by fitting
 *  2nd order polynoms to x and y.
 *
 *  this estimates the x and y values
 *  separately. Don't know if this is the
 *  best way, but it works well
 */
template <class Iterator, class Accessor>
CorrelationResult subpixelMaxima(vigra::triple<Iterator, Iterator, Accessor> img,
                                 vigra::Diff2D max)
{
    const int interpWidth = 1;
    CorrelationResult res;
    vigra_precondition(max.x > interpWidth && max.y > interpWidth,
                 "subpixelMaxima(): coordinates of "
                 "maxima must be > interpWidth, interpWidth.");
    vigra::Diff2D sz = img.second - img.first;
    vigra_precondition(sz.x - max.x >= interpWidth && sz.y - max.y >= interpWidth,
                 "subpixelMaxima(): coordinates of "
                 "maxima must interpWidth pixels from the border.");
    typedef typename Accessor::value_type T;

    T x[2*interpWidth+1];
    T zx[2*interpWidth+1];
    T zy[2*interpWidth+1];

#ifdef DEBUG_CORRELATION
    exportImage(img,vigra::ImageExportInfo("test.tif"));
#endif
    
    Accessor acc = img.third;
    Iterator begin=img.first;
    for (int i=-interpWidth; i<=interpWidth; i++) {
        // collect first row
        x[i+interpWidth] = i;
        zx[i+interpWidth] = acc(begin, max + vigra::Diff2D(i,0));
        zy[i+interpWidth] = acc(begin, max + vigra::Diff2D(0,i));
    }

    double a,b,c;
    FitPolynom(x, x + 2*interpWidth+1, zx, a,b,c);
    if (hugin_utils::isnan(a) || hugin_utils::isnan(b) || hugin_utils::isnan(c)) {
	exportImage(img,vigra::ImageExportInfo("test.tif"));
	DEBUG_ERROR("Bad polynomial fit results");
	res.maxpos.x=max.x;
	res.maxpos.y=max.y;
	return res;
    }

    // calculate extrema of x position by setting
    // the 1st derivate to zero
    // 2*c*x + b = 0
    if (c==0)
	res.maxpos.x=0;
    else
	res.maxpos.x = -b/(2*c);

    res.curv.x = 2*c;

    // calculate result at maxima
    double maxx = c*res.maxpos.x*res.maxpos.x + b*res.maxpos.x + a;

    FitPolynom(x, x + 2*interpWidth+1, zy, a,b,c);
    // calculate extrema of y position
    if (c==0)
	res.maxpos.y=0;
    else
	res.maxpos.y = -b/(2*c);

    res.curv.y = 2*c;
    double maxy = c*res.maxpos.y*res.maxpos.y + b*res.maxpos.y + a;

    // use mean of both maxima as new interpolation result
    res.maxi = (maxx + maxy) / 2;
    DEBUG_DEBUG("value at subpixel maxima (" << res.maxpos.x << " , "
                <<res.maxpos.y << "): " << maxx << "," << maxy
                << " mean:" << res.maxi << " coeff: a=" << a
                << "; b=" << b << "; c=" << c);

    // test if the point has moved too much ( more than 1.5 pixel).
    // actually, I should also test the confidence of the fit.
    if (fabs(res.maxpos.x) > 1 || fabs(res.maxpos.y) > 1) {
        DEBUG_NOTICE("subpixel Maxima has moved to much, ignoring");
        res.maxpos.x = max.x;
        res.maxpos.y = max.y;
    } else {
        res.maxpos.x = res.maxpos.x + max.x;
        res.maxpos.y = res.maxpos.y + max.y;
    }
    return res;
}

/// clockwise rotation around a origin point, and a translation afterwards.
/// build to be used by PointFineTune
class RotateTransform
{
public:
    RotateTransform(double phi, hugin_utils::FDiff2D origin, hugin_utils::FDiff2D transl)
        : m_phi(phi), m_origin(origin), m_transl(transl)
        { }

    bool transformImgCoord(double &destx, double &desty, double srcx, double srcy) const
    {
        srcx -= m_origin.x;
        srcy -= m_origin.y;

        destx = srcx * cos(m_phi) + srcy * sin(m_phi);
        desty = srcx * -sin(m_phi) + srcy * cos(m_phi);

        destx += m_transl.x;
        desty += m_transl.y;
        return true;
    }

    double m_phi;
    hugin_utils::FDiff2D m_origin;
    hugin_utils::FDiff2D m_transl;
};

/** fine tune a point with normalized cross correlation
 *
 *  takes a patch of \p templSize by \p templSize from \p templImg
 *  images at \p tmplPos and searches it on the \p searchImg, at
 *  \p searchPos, in a neighbourhood of \p sWidth by \p sWidth.
 *
 *  The result in returned in @p tunedPos
 *
 *  @return correlation value
 */
template <class IMAGET, class IMAGES>
CorrelationResult PointFineTune(const IMAGET & templImg,
                                vigra::Diff2D templPos,
                                int templSize,
                                const IMAGES & searchImg,
                                vigra::Diff2D searchPos,
                                int sWidth)
{
//    DEBUG_TRACE("templPos: " vigra::<< templPos << " searchPos: " vigra::<< searchPos);

    // extract patch from template

    int templWidth = templSize/2;
    vigra::Diff2D tmplUL(templPos.x - templWidth, templPos.y-templWidth);
    // lower right iterators "are past the end"
    vigra::Diff2D tmplLR(templPos.x + templWidth + 1, templPos.y + templWidth + 1);
    // clip corners to ensure the template is inside the image.
    vigra::Diff2D tmplImgSize(templImg.size());
    tmplUL = hugin_utils::simpleClipPoint(tmplUL, vigra::Diff2D(0,0), tmplImgSize);
    tmplLR = hugin_utils::simpleClipPoint(tmplLR, vigra::Diff2D(0,0), tmplImgSize);
    vigra::Diff2D tmplSize = tmplLR - tmplUL;
    DEBUG_DEBUG("template position: " << templPos << "  tmplUL: " << tmplUL
		    << "  templLR:" << tmplLR << "  size:" << tmplSize);

    // extract patch from search region
    // make search region bigger, so that interpolation can always be done
    int swidth = sWidth/2 +(2+templWidth);
//    DEBUG_DEBUG("search window half width/height: " << swidth << "x" << swidth);
//    Diff2D subjPoint(searchPos);
    // clip search window
    if (searchPos.x < 0) searchPos.x = 0;
    if (searchPos.x > (int) searchImg.width()) searchPos.x = searchImg.width()-1;
    if (searchPos.y < 0) searchPos.y = 0;
    if (searchPos.y > (int) searchImg.height()) searchPos.x = searchImg.height()-1;

    vigra::Diff2D searchUL(searchPos.x - swidth, searchPos.y - swidth);
    // point past the end
    vigra::Diff2D searchLR(searchPos.x + swidth+1, searchPos.y + swidth+1);
    // clip search window
    vigra::Diff2D srcImgSize(searchImg.size());
    searchUL = hugin_utils::simpleClipPoint(searchUL, vigra::Diff2D(0,0), srcImgSize);
    searchLR = hugin_utils::simpleClipPoint(searchLR, vigra::Diff2D(0,0), srcImgSize);
//    DEBUG_DEBUG("search borders: " << searchLR.x << "," << searchLR.y);

    vigra::Diff2D searchSize = searchLR - searchUL;
    // create output image

//#ifdef VIGRA_EXT_USE_FAST_CORR
    // source input
    vigra::FImage srcImage(searchLR-searchUL);
    vigra::copyImage(vigra::make_triple(searchImg.upperLeft() + searchUL,
                                        searchImg.upperLeft() + searchLR,
                                        vigra::RGBToGrayAccessor<typename IMAGES::value_type>() ),
                     destImage(srcImage) );

    vigra::FImage templateImage(tmplSize);
    vigra::copyImage(vigra::make_triple(templImg.upperLeft() + tmplUL,
                                        templImg.upperLeft() + tmplLR,
                                        vigra::RGBToGrayAccessor<typename IMAGET::value_type>()),
                     destImage(templateImage));
#ifdef DEBUG_WRITE_FILES
    vigra::ImageExportInfo tmpli("hugin_templ.tif");
    vigra::exportImage(vigra::srcImageRange(templateImage), tmpli);

    vigra::ImageExportInfo srci("hugin_searchregion.tif");
    vigra::exportImage(vigra::srcImageRange(srcImage), srci);
#endif

//#endif

    vigra::FImage dest(searchSize);
    dest.init(-1);
    // we could use the multiresolution version as well.
    // but usually the region is quite small.
    CorrelationResult res;
#ifdef VIGRA_EXT_USE_FAST_CORR
    DEBUG_DEBUG("+++++ starting fast correlation");
    res = correlateImageFast(srcImage,
                             dest,
                             templateImage,
                             tmplUL-templPos, tmplLR-templPos - vigra::Diff2D(1,1),
                             -1);
#else
    DEBUG_DEBUG("+++++ starting normal correlation");
    res = correlateImage(srcImage.upperLeft(),
                         srcImage.lowerRight(),
                         srcImage.accessor(),
                         dest.upperLeft(),
                         dest.accessor(),
                         templateImage.upperLeft() + templPos,
                         templateImage.accessor(),
                         tmplUL, tmplLR, -1);

//     res = correlateImage(searchImg.upperLeft() + searchUL,
//                          searchImg.upperLeft() + searchLR,
//                          searchImg.accessor(),
//                          dest.upperLeft(),
//                          dest.accessor(),
//                          templImg.upperLeft() + templPos,
//                          templImg.accessor(),
//                          tmplUL, tmplLR, -1);
#endif
    DEBUG_DEBUG("normal search finished, max:" << res.maxi
                << " at " << res.maxpos);
    // do a subpixel maxima estimation
    // check if the max is inside the pixel boundaries,
    // and there are enought correlation values for the subpixel
    // estimation, (2 + templWidth)
    if (res.maxpos.x > 2 + templWidth && res.maxpos.x < 2*swidth+1-2-templWidth
        && res.maxpos.y > 2+templWidth && res.maxpos.y < 2*swidth+1-2-templWidth)
    {
        // subpixel estimation
        res = subpixelMaxima(vigra::srcImageRange(dest), res.maxpos.toDiff2D());
        DEBUG_DEBUG("subpixel position: max:" << res.maxi
                    << " at " << res.maxpos);
    } else {
        // not enough values for subpixel estimation.
        DEBUG_DEBUG("subpixel estimation not done, maxima too close to border");
    }

    res.maxpos = res.maxpos + searchUL;
    return res;
}

/** fine tune a point with normalized cross correlation, searches x,y and phi (rotation around z)
 *
 *  takes a patch of \p templSize by \p templSize from \p templImg
 *  images at \p tmplPos and searches it on the \p searchImg, at
 *  \p searchPos, in a neighbourhood of \p sWidth by \p sWidth.
 *
 *  The result in returned in @p tunedPos
 *
 *  @return correlation value
 */
template <class IMAGET, class IMAGES>
CorrelationResult PointFineTuneRotSearch(const IMAGET & templImg,
                                         vigra::Diff2D templPos,
                                         int templSize,
                                         const IMAGES & searchImg,
                                         vigra::Diff2D searchPos,
                                         int sWidth,
                                         double startAngle,
                                         double stopAngle,
                                         int angleSteps)
{
    DEBUG_TRACE("templPos: " << templPos << " searchPos: " << searchPos);

    // extract patch from template

    // make template size user configurable as well?
    int templWidth = templSize/2;
    vigra::Diff2D tmplUL(-templWidth, -templWidth);
    vigra::Diff2D tmplLR(templWidth, templWidth);
    // clip template
    if (tmplUL.x + templPos.x < 0) tmplUL.x = -templPos.x;
    if (tmplUL.y + templPos.y < 0) tmplUL.y = -templPos.y;
    if (tmplLR.x + templPos.x> templImg.width())
        tmplLR.x = templImg.width() - templPos.x;
    if (tmplLR.y + templPos.y > templImg.height())
        tmplLR.y = templImg.height() - templPos.y;
    vigra::Diff2D tmplSize = tmplLR - tmplUL + vigra::Diff2D(1,1);

    // extract patch from search region
    // make search region bigger, so that interpolation can always be done
    int swidth = sWidth/2 +(2+templWidth);
    DEBUG_DEBUG("search window half width/height: " << swidth << "x" << swidth);
//    Diff2D subjPoint(searchPos);
    // clip search window
    if (searchPos.x < 0) searchPos.x = 0;
    if (searchPos.x > (int) searchImg.width()) searchPos.x = searchImg.width()-1;
    if (searchPos.y < 0) searchPos.y = 0;
    if (searchPos.y > (int) searchImg.height()) searchPos.x = searchImg.height()-1;

    vigra::Diff2D searchUL(searchPos.x - swidth, searchPos.y - swidth);
    vigra::Diff2D searchLR(searchPos.x + swidth, searchPos.y + swidth);
    // clip search window
    if (searchUL.x < 0) searchUL.x = 0;
    if (searchUL.x > searchImg.width()) searchUL.x = searchImg.width();
    if (searchUL.y < 0) searchUL.y = 0;
    if (searchUL.y > searchImg.height()) searchUL.y = searchImg.height();
    if (searchLR.x > searchImg.width()) searchLR.x = searchImg.width();
    if (searchLR.x < 0) searchLR.x = 0;
    if (searchLR.y > searchImg.height()) searchLR.y = searchImg.height();
    if (searchLR.y < 0) searchLR.y = 0;
    DEBUG_DEBUG("search borders: " << searchUL << "," << searchLR << " size: " << searchLR -searchUL);


#ifdef DEBUG_WRITE_FILES
    DEBUG_DEBUG("template area: " << templPos + tmplUL << " -> " << templPos + tmplLR);
    vigra::exportImage(vigra::make_triple(templImg.upperLeft() + templPos + tmplUL,
                                          templImg.upperLeft() + templPos + tmplLR + vigra::Diff2D(1,1),
                                          templImg.accessor()),
                       vigra::ImageExportInfo("00_original_template.png"));

    vigra::exportImage(make_triple(searchImg.upperLeft() + searchUL,
                                   searchImg.upperLeft() + searchLR,
                                   searchImg.accessor()),
                       vigra::ImageExportInfo("00_searcharea.png"));
#endif

    // rotated template
    vigra::FImage rotTemplate(tmplSize);
    vigra::FImage alpha(tmplSize);

    // correlation output
    vigra::Diff2D searchSize = searchLR - searchUL;
    vigra::FImage dest(searchSize);
    dest.init(1);
    vigra::FImage bestCorrelation(searchSize);

//#ifdef VIGRA_EXT_USE_FAST_CORR
    // source input
    vigra::FImage srcImage(searchLR-searchUL);
    vigra::copyImage(vigra::make_triple(searchImg.upperLeft() + searchUL,
                                        searchImg.upperLeft() + searchLR,
                                        vigra::RGBToGrayAccessor<typename IMAGES::value_type>() ),
                     destImage(srcImage) );
//#endif

    CorrelationResult bestRes;
    bestRes.maxi = -1;
    double bestAngle = 0;

    AppBase::MultiProgressDisplay dummy;
    // test the image at rotation angles with 30 deg. steps.
    double step = (stopAngle - startAngle)/(angleSteps-1);
    double phi=startAngle;
    vigra_ext::PassThroughFunctor<float> nf;
    for (double i=0; phi <= stopAngle; i++, phi += step) {
        DEBUG_DEBUG("+++++ Rotating image, phi: " << RAD_TO_DEG(phi));
        RotateTransform t(phi, hugin_utils::FDiff2D(templWidth, templWidth), templPos);
        vigra_ext::transformImage(srcImageRange(templImg, vigra::RGBToGrayAccessor<typename IMAGET::value_type>()),
                           destImageRange(rotTemplate),
                           destImage(alpha),
                           vigra::Diff2D(0,0),
                           t,
                           nf,
                           false,
                           vigra_ext::INTERP_CUBIC,
                           dummy);
        DEBUG_DEBUG("----- Image rotated");

        // force a search in at all points.
        dest.init(-1);
        DEBUG_DEBUG("+++++ starting correlation");

        CorrelationResult res;
        // we could use the multiresolution version as well.
        // but usually the region is quite small.
#ifdef VIGRA_EXT_USE_FAST_CORR
        res = correlateImageFast(srcImage,
                                 dest,
                                 rotTemplate,
                                 vigra::Diff2D(-templWidth, -templWidth),
                                 vigra::Diff2D(templWidth, templWidth),
                                 -1);
#else
        res = correlateImage(srcImage.upperLeft(),
                             srcImage.lowerRight(),
                             srcImage.accessor(),
                             dest.upperLeft(),
                             dest.accessor(),
                             rotTemplate.upperLeft() + vigra::Diff2D(templWidth, templWidth),
                             rotTemplate.accessor(),
                             vigra::Diff2D(-templWidth, -templWidth), vigra::Diff2D(templWidth, templWidth),
                             -1);
#endif
        DEBUG_DEBUG("---- correlation finished");

#ifdef DEBUG_WRITE_FILES
        char fname[256];
        vigra::BImage tmpImg(rotTemplate.size());
        vigra::copyImage(srcImageRange(rotTemplate),destImage(tmpImg));
        sprintf(fname, "%3.0f_rotated_template.png", phi/M_PI*180);
        vigra::exportImage(srcImageRange(tmpImg), vigra::ImageExportInfo(fname));

        vigra::transformImage(vigra::srcImageRange(dest), vigra::destImage(dest),
                              vigra::linearRangeMapping(
                                  -1, 1,               // src range
                                  (unsigned char)0, (unsigned char)255) // dest range
            );
        tmpImg.resize(dest.size());
        vigra::copyImage(srcImageRange(dest),destImage(tmpImg));
        sprintf(fname, "%3.0f_corr_result.png", phi/M_PI*180);
        vigra::exportImage(srcImageRange(tmpImg), vigra::ImageExportInfo(fname));
#endif


        DEBUG_DEBUG("normal search finished, max:" << res.maxi
                    << " at " << res.maxpos << " angle:" << phi/M_PI*180 << "");

        if (res.maxi > bestRes.maxi) {
            // remember best correlation.
            bestCorrelation = dest;
            bestRes = res;
            bestAngle = phi;
        }

    }

    DEBUG_DEBUG("rotation search finished, max:" << bestRes.maxi
                << " at " << bestRes.maxpos << " angle:" << bestAngle/M_PI*180 << "");

    // do a subpixel maxima estimation
    // check if the max is inside the pixel boundaries,
    // and there are enought correlation values for the subpixel
    // estimation, (2 + templWidth)
    if (bestRes.maxpos.x > 1 + templWidth && bestRes.maxpos.x < 2*swidth+1-1-templWidth
        && bestRes.maxpos.y > 1+templWidth && bestRes.maxpos.y < 2*swidth+1-1-templWidth)
    {
        // subpixel estimation
        bestRes = subpixelMaxima(vigra::srcImageRange(bestCorrelation), bestRes.maxpos.toDiff2D());
        DEBUG_DEBUG("subpixel position: max:" << bestRes.maxi
                    << " at " << bestRes.maxpos << " under angle: " << bestAngle/M_PI*180);
    } else {
        // not enough values for subpixel estimation.
        DEBUG_ERROR("subpixel estimation not done, maxima to close to border");
    }

    bestRes.maxpos = bestRes.maxpos + searchUL;
    bestRes.maxAngle = bestAngle/M_PI*180;
    return bestRes;
}

/** correlate a template with an image.
 *
 *  most code is taken from vigra::convoluteImage.
 *  See its documentation for further information.
 *
 *  Correlation result already contains the maximum position
 *  and its correlation value.
 *  it should be possible to set a threshold here.
 */
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
          class KernelIterator, class KernelAccessor>
CorrelationResult correlateImage(SrcIterator sul, SrcIterator slr, SrcAccessor as,
                                 DestIterator dul, DestAccessor ad,
                                 KernelIterator ki, KernelAccessor ak,
                                 vigra::Diff2D kul, vigra::Diff2D klr,
                                 double threshold = 0.7 )
{
    CorrelationResult res;
    vigra_precondition(kul.x <= 0 && kul.y <= 0,
                 "convolveImage(): coordinates of "
                 "kernel's upper left must be <= 0.");
    vigra_precondition(klr.x >= 0 && klr.y >= 0,
                 "convolveImage(): coordinates of "
                 "kernel's lower right must be >= 0.");

    // use traits to determine SumType as to prevent possible overflow
    typedef typename
        vigra::NumericTraits<typename SrcAccessor::value_type>::RealPromote SumType;
    typedef typename
        vigra::NumericTraits<typename KernelAccessor::value_type>::RealPromote KSumType;
    typedef
        vigra::NumericTraits<typename DestAccessor::value_type> DestTraits;

    // calculate width and height of the image
    int w = slr.x - sul.x;
    int h = slr.y - sul.y;
    int wk = klr.x - kul.x +1;
    int hk = klr.y - kul.y +1;

//    DEBUG_DEBUG("correlate Image srcSize " << (slr - sul).x << "," << (slr - sul).y
//                << " tmpl size: " << wk << "," << hk);

    vigra_precondition(w >= wk && h >= hk,
                 "convolveImage(): kernel larger than image.");

    int x,y;
    int ystart = -kul.y;
    int yend   = h-klr.y;
    int xstart = -kul.x;
    int xend   = w-klr.x;

    // calculate template mean
    vigra::FindAverage<typename KernelAccessor::value_type> average;
    vigra::inspectImage(ki + kul, ki + klr + vigra::Diff2D(1,1),
                        ak,
                        average);
    KSumType kmean = average();

    // create y iterators, they iterate over the rows.
    DestIterator yd = dul + vigra::Diff2D(xstart, ystart);
    SrcIterator ys = sul + vigra::Diff2D(xstart, ystart);


//    DEBUG_DEBUG("size: " << w << "," <<  h << " ystart: " << ystart <<", yend: " << yend);
    for(y=ystart; y < yend; ++y, ++ys.y, ++yd.y)
    {

        // create x iterators, they iterate the coorelation over
        // the columns
        DestIterator xd(yd);
        SrcIterator xs(ys);

        for(x=xstart; x < xend; ++x, ++xs.x, ++xd.x)
        {
            if (as(xd) < threshold) {
                continue;
            }
//            int x0, y0, x1, y1;

//            y0 = kul.y;
//            y1 = klr.y;
//            x0 = kul.x;
//            x1 = klr.x;;

            // init the sum
            SumType numerator = 0;
            SumType div1 = 0;
            SumType div2 = 0;
            SumType spixel = 0;
            KSumType kpixel = 0;

            // create inner y iterators
            // access to the source image
            SrcIterator yys = xs + kul;
            // access to the kernel image
            KernelIterator yk  = ki + kul;

            vigra::FindAverage<typename SrcAccessor::value_type> average;
            vigra::inspectImage(xs + kul, xs + klr + vigra::Diff2D(1,1), as, average);
            double mean = average();

            int xx, yy;
            for(yy=0; yy<hk; ++yy, ++yys.y, ++yk.y)
            {
                // create inner x iterators
                SrcIterator xxs = yys;
                KernelIterator xk = yk;

                for(xx=0; xx<wk; ++xx, ++xxs.x, ++xk.x)
                {
                    spixel = as(xxs) - mean;
                    kpixel = ak(xk) - kmean;
                    numerator += kpixel * spixel;
                    div1 += kpixel * kpixel;
                    div2 += spixel * spixel;
                }
            }
            numerator = (numerator/sqrt(div1 * div2));
            if (numerator > res.maxi) {
                res.maxi = numerator;
                res.maxpos.x = x;
                res.maxpos.y = y;
            }
            numerator = numerator;
            // store correlation in destination pixel
            ad.set(DestTraits::fromRealPromote(numerator), xd);
        }
    }
    return res;
}



#if 0
/** multi resolution template matching using cross correlatation.
 *
 */
template <class Image>
bool findTemplate(const Image & templ, const std::string & filename,
                  CorrelationResult & res)
{
//    DEBUG_TRACE("");
    // calculate pyramid level based on template size
    // the template should have be at least 1 pixel wide or high
    // (in the lowest resolution).
    int tw = templ.width();
    int th = templ.height();
    int ts = th<tw ? th : tw;

    // calculate the lowest level that will make sense
    int maxLevel = (int)floor(sqrt((double)ts/4.0));
    int levels = maxLevel+1;
    DEBUG_DEBUG("starting search on pyramid level " << maxLevel);

	vigra::BImage *templs = new vigra::BImage[levels];
    templs[0].resize(templ.width(), templ.height());
    vigra::copyImage(srcImageRange(templ), destImage(templs[0]));

    // create other levels
    for(int i=1; i<levels; i++) {
        vigra_ext::reduceToNextLevel(templs[i-1], templs[i]);
    }

    // this image will store the correlation coefficients
    // it will also be used by correlateImage as a mask image.
    // only the correlation for pixels with resImage(x,y) > theshold
    // will be calcuated. this avoids searching in completely uninteresting
    // image parts. (saves a lot of time)

    vigra::FImage * prevMask = new vigra::FImage(2,2,0.9);
    vigra::FImage * currentMask = new vigra::FImage();
    // start matching with highest level (lowest resolution images)
    float threshold=0;
    for (int i=maxLevel; i>=0; i--) {
//    for (int i=0; i>=0; i--) {
        DEBUG_DEBUG("correlation level " << i);
        std::stringstream t;
        t << filename << "_" << i << "_";
        std::string basename = t.str();
//        const vigra::BImage & s = ImageCache::getInstance().getPyramidImage(filename, i);
        DEBUG_DEBUG("subject size: " << s.width() << "," << s.height()
                    << "template size: " << templs[i].width() << "," << templs[i].height());
//        saveImage(templs[i], basename + "template.pnm");
//        saveImage(s, basename + "subject.pnm");
        currentMask->resize(s.width(),s.height());
        // scale up dest/mask Image
        // probably this should also use a threshold and extend the
        // resulting mask to the neighborhood pixels, just to catch unfortunate
        // correlations.
        vigra::resizeImageNoInterpolation(vigra::srcImageRange(*prevMask),vigra::destImageRange(*currentMask));

//        saveScaledImage(*currentMask, -1, 1, basename + "mask.pnm");

        res = correlateImage(s.upperLeft(),
                             s.lowerRight(),
                             vigra::StandardValueAccessor<unsigned char>(),
                             currentMask->upperLeft(),
                             vigra::StandardValueAccessor<float>(),
                             templs[i].upperLeft(),
                             vigra::StandardValueAccessor<unsigned char>(),
                             vigra::Diff2D(0,0),
                             templs[i].size() - vigra::Diff2D(1,1),
                             threshold
            );
//        saveScaledImage(*currentMask, -1, 1, basename + "result.pnm");
        DEBUG_DEBUG("Correlation result at level " << i << ":  max:" << res.maxi << " at: " << res.maxpos);
        // simple adaptive threshold.
        // FIXME, make this configurable? or select it according to some
        // statistical value
        threshold = res.maxi - 0.07;

        vigra::FImage *tmp = prevMask;
        prevMask = currentMask;
        currentMask = tmp;
    }
    delete prevMask;
    delete currentMask;
    return true;
}
#endif


} // namespace

#endif // VIGRA_EXT_CORRELATION_H
