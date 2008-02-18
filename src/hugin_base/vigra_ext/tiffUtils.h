// -*- c-basic-offset: 4 -*-
/** @file tiffUtils.h
 *
 *  Some functions to create tiff images with masks
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

#ifndef _TIFFUTILS_H
#define _TIFFUTILS_H

#include <vigra/tiff.hxx>
#include <vigra/imageinfo.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/functorexpression.hxx>

#include <vigra_ext/FunctorAccessor.h>

#include <tiffio.h>

// add this to the vigra_ext namespace
namespace vigra_ext {


//***************************************************************************
//
//  functions to write tiff files with a single alpha channel.
//
//***************************************************************************

/** write a new Tiff directory, for a new layer
 *
 * @param tiff tiff struct
 * @param pagename pagename for tiff layer
 * @param documentname for tiff layer
 * @param page page number
 * @param nImg total number of pages, needed for "image page/nImg" display
 * @param offset layer offset in pixels (must be positive)
 */
inline void createTiffDirectory(vigra::TiffImage * tiff, const std::string & pagename,
                                const std::string & documentname,
                                const std::string comp,
                                uint16 page, uint16 nImg,
                                vigra::Diff2D offset,
                                vigra::Size2D fullSize,
                                const vigra::ImageExportInfo::ICCProfile & icc)
{
    const float dpi = 150;
    // create a new directory for our image
    // hopefully I didn't forget too much stuff..
    // TIFF tag reference at http://www.awaresystems.be/imaging/tiff/tifftags.html

    // set page
    // FIXME: Also only needed for multilayer images
    if (nImg > 1) {
        // create tiff directory for the new layers
        // TIFFOpen already created the first one.
        if (page > 1) {
            TIFFCreateDirectory (tiff);
        }
        TIFFSetField (tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        TIFFSetField (tiff, TIFFTAG_PAGENUMBER, (unsigned short)page, (unsigned short)nImg);
    }
    TIFFSetField (tiff, TIFFTAG_XRESOLUTION, (float) dpi);
    TIFFSetField (tiff, TIFFTAG_YRESOLUTION, (float) dpi);
    // offsets must allways be positive so correct them
    DEBUG_ASSERT(offset.x >= 0);
    DEBUG_ASSERT(offset.y >= 0);
    TIFFSetField (tiff, TIFFTAG_XPOSITION, (float)(offset.x / dpi));
    TIFFSetField (tiff, TIFFTAG_YPOSITION, (float)(offset.y / dpi));

    // TIFFTAG_PIXAR_IMAGEFULLWIDTH and TIFFTAG_PIXAR_IMAGEFULLLENGTH
    // are set when an image has been cropped out of a larger image.  
    // They reflect the size of the original uncropped image.
    // The TIFFTAG_XPOSITION and TIFFTAG_YPOSITION can be used
    // to determine the position of the smaller image in the larger one.
    TIFFSetField(tiff, TIFFTAG_PIXAR_IMAGEFULLWIDTH, fullSize.x);
    TIFFSetField(tiff, TIFFTAG_PIXAR_IMAGEFULLLENGTH, fullSize.y);

    // save input name.
    TIFFSetField (tiff, TIFFTAG_DOCUMENTNAME, documentname.c_str());
    TIFFSetField (tiff, TIFFTAG_PAGENAME, pagename.c_str() );
    //
    TIFFSetField (tiff, TIFFTAG_IMAGEDESCRIPTION, "stitched with hugin");

    // set compression
    unsigned short tiffcomp;
    if ( comp == "JPEG" )
        tiffcomp = COMPRESSION_OJPEG;
    else if ( comp == "LZW" )
        tiffcomp = COMPRESSION_LZW;
    else if ( comp == "DEFLATE" )
        tiffcomp = COMPRESSION_DEFLATE;
    else if ( comp == "PACKBITS" )
        tiffcomp = COMPRESSION_PACKBITS;
    else
        tiffcomp = COMPRESSION_NONE;

    TIFFSetField(tiff, TIFFTAG_COMPRESSION, tiffcomp);

    // Set ICC profile, if available.
    if (icc.size() > 0) {
        TIFFSetField(tiff, TIFFTAG_ICCPROFILE, icc.size(), icc.front());
    }

}


/** internal function to create a scalar tiff image with alpha channel */
template <class ImageIterator, class ImageAccessor,
          class AlphaIterator, class AlphaAccessor>
static void
createScalarATiffImage(ImageIterator upperleft, ImageIterator lowerright,
                       ImageAccessor a,
                       AlphaIterator alphaUpperleft, AlphaAccessor alphaA,
                       vigra::TiffImage * tiff, int sampleformat)
{
    typedef typename ImageAccessor::value_type PixelType;

    int w = lowerright.x - upperleft.x;
    int h = lowerright.y - upperleft.y;

    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, sizeof(PixelType) * 8);
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 2);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, sampleformat);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, 1);

    // for alpha stuff, do not uses premultilied data
    // We do not want to throw away data by premultiplying
    uint16 nextra_samples = 1;
    uint16 extra_samples = EXTRASAMPLE_UNASSALPHA;
    TIFFSetField (tiff, TIFFTAG_EXTRASAMPLES, nextra_samples, &extra_samples);

    int bufsize = TIFFScanlineSize(tiff);
    tdata_t * buf = new tdata_t[bufsize];

    ImageIterator ys(upperleft);
    AlphaIterator ya(alphaUpperleft);

    try
    {
        for(int y=0; y<h; ++y, ++ys.y, ++ya.y)
        {
            PixelType * pg = (PixelType *)buf;
            PixelType * alpha = pg+1;

            ImageIterator xs(ys);
            AlphaIterator xa(ya);

            for(int x=0; x<w; ++x, ++xs.x, pg+=2, alpha+=2, ++xa.x)
            {
                *pg = a(xs);
                *alpha = alphaA(xa);
            }
            TIFFWriteScanline(tiff, buf, y);
        }
    }
    catch(...)
    {
        delete[] buf;
        throw;
    }
    delete[] buf;
}

/** internal function to create a RGB tiff image with alpha channel */
template <class ImageIterator, class ImageAccessor,
          class AlphaIterator, class AlphaAccessor>
void
createRGBATiffImage(ImageIterator upperleft, ImageIterator lowerright,
                    ImageAccessor a,
                    AlphaIterator alphaUpperleft, AlphaAccessor alphaA,
                    vigra::TiffImage * tiff, int sampleformat)
{
    typedef typename ImageAccessor::value_type PType;
    typedef typename PType::value_type PixelType;
//    typedef typename ImageAccessor::value_type PixelType;
    int w = lowerright.x - upperleft.x;
    int h = lowerright.y - upperleft.y;

    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, sizeof(PixelType) * 8);
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, sampleformat);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, 1);
		
    // for alpha stuff, do not uses premultilied data
    // We do not want to throw away data & accuracy by premultiplying
    uint16 nextra_samples = 1;
    uint16 extra_samples = EXTRASAMPLE_UNASSALPHA;
    TIFFSetField (tiff, TIFFTAG_EXTRASAMPLES, nextra_samples, &extra_samples);

    int bufsize = TIFFScanlineSize(tiff);
    tdata_t * buf = new tdata_t[bufsize];

    ImageIterator ys(upperleft);
    AlphaIterator ya(alphaUpperleft);

    try
    {
        for(int y=0; y<h; ++y, ++ys.y, ++ya.y)
        {
            PixelType * pr = (PixelType *)buf;
            PixelType * pg = pr+1;
            PixelType * pb = pg+1;
            PixelType * alpha = pb+1;

            ImageIterator xs(ys);
            AlphaIterator xa(ya);

            for(int x=0; x<w; ++x, ++xs.x, pr+=4, pg+=4, pb+=4, alpha+=4, ++xa.x)
            {
                *pr = a.red(xs);
                *pg = a.green(xs);
                *pb = a.blue(xs);
                *alpha = alphaA(xa);
            }
            TIFFWriteScanline(tiff, buf, y);
        }
    }
    catch(...)
    {
        delete[] buf;
        throw;
    }
    delete[] buf;
}

// try to add stuff the vigra way
// This are constructor classes, to do a kind of compile time switch
// for the different image types.
template <class T>
struct CreateAlphaTiffImage;

// 8 bit images
template <>
struct CreateAlphaTiffImage<vigra::RGBValue<unsigned char> >
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        createRGBATiffImage(iUL, iLR, iA, aUL, aA, tiff, SAMPLEFORMAT_UINT);
    }
};

// 16 bit
template <>
struct CreateAlphaTiffImage<vigra::RGBValue<short> >
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<short>,
	                               AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<short>(128), aA);
        createRGBATiffImage(iUL, iLR, iA, aUL, mA,
                            tiff,  SAMPLEFORMAT_INT);
    }
};

template <>
struct CreateAlphaTiffImage<vigra::RGBValue<unsigned short> >
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<unsigned short>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<unsigned short>(256), aA);
        createRGBATiffImage(iUL, iLR, iA, aUL, mA,
                            tiff,  SAMPLEFORMAT_UINT);
    }
};


// 32 bit
template <>
struct CreateAlphaTiffImage<vigra::RGBValue<int> >
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<int>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<int>(8388608), aA);
        createRGBATiffImage(iUL, iLR, iA, aUL, mA,
                            tiff,  SAMPLEFORMAT_INT);
    }
};

template <>
struct CreateAlphaTiffImage<vigra::RGBValue<unsigned int> >
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<unsigned int>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<unsigned int>(16777216), aA);
        createRGBATiffImage(iUL, iLR, iA, aUL, mA,
                            tiff,  SAMPLEFORMAT_UINT);
    }
};

// float
template <>
struct CreateAlphaTiffImage<vigra::RGBValue<float> >
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<float>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<float>(1.0f/255), aA);
        createRGBATiffImage(iUL, iLR, iA, aUL, mA,
                            tiff,  SAMPLEFORMAT_IEEEFP);
    }
};

// double
template <>
struct CreateAlphaTiffImage<vigra::RGBValue<double> >
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<double>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<double>(1.0f/255),aA);
        createRGBATiffImage(iUL, iLR, iA, aUL, mA,
                            tiff,  SAMPLEFORMAT_IEEEFP);
    }
};

// ================================================================
// for scalar images
// ================================================================

// 8 bit images
template <>
struct CreateAlphaTiffImage<unsigned char>
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        createScalarATiffImage(iUL, iLR, iA, aUL, aA, tiff,  SAMPLEFORMAT_UINT);
    }
};

// 16 bit
template <>
struct CreateAlphaTiffImage<short>
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<short>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<short>(128), aA);
        createScalarATiffImage(iUL, iLR, iA, aUL, mA, tiff,  SAMPLEFORMAT_INT);
    }
};
template <>
struct CreateAlphaTiffImage<unsigned short>
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<unsigned short>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<unsigned short>(256), aA);
        createScalarATiffImage(iUL, iLR, iA, aUL, mA, tiff,  SAMPLEFORMAT_UINT);
    }
};

// 32 bit
template <>
struct CreateAlphaTiffImage<int>
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<int>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<int>(8388608), aA);
        createScalarATiffImage(iUL, iLR, iA, aUL, mA, tiff,  SAMPLEFORMAT_INT);
    }
};

template <>
struct CreateAlphaTiffImage<unsigned int>
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<unsigned int>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<unsigned int>(16777216), aA);
        createScalarATiffImage(iUL, iLR, iA, aUL, mA, tiff,  SAMPLEFORMAT_UINT);
    }
};

// float
template <>
struct CreateAlphaTiffImage<float>
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<float>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<float>(1.0f/255), aA);
        createScalarATiffImage(iUL, iLR, iA, aUL, mA, tiff,  SAMPLEFORMAT_IEEEFP);
    }
};

// double
template <>
struct CreateAlphaTiffImage<double>
{
    template <class ImageIterator, class ImageAccessor,
              class AlphaIterator, class AlphaAccessor>
    static void exec(ImageIterator iUL, ImageIterator iLR,
                     ImageAccessor iA,
                     AlphaIterator aUL,
                     AlphaAccessor aA,
                     vigra::TiffImage * tiff)
    {
        vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<double>, AlphaAccessor>
            mA(vigra::ScalarIntensityTransform<double>(1.0f/255), aA);
        createScalarATiffImage(iUL, iLR, iA, aUL, mA, tiff,  SAMPLEFORMAT_IEEEFP);
    }
};

// ============================================================
// ============================================================

template <class ImageIterator, class ImageAccessor,
          class AlphaIterator, class AlphaAccessor>
inline void
createAlphaTiffImage(ImageIterator upperleft, ImageIterator lowerright,
                     ImageAccessor a,
                     AlphaIterator alphaUpperleft, AlphaAccessor alphaA,
                     vigra::TiffImage * tiff)
{
    // call right constructor class for this image type
    CreateAlphaTiffImage<typename ImageAccessor::value_type>::
        exec(upperleft, lowerright, a,
             alphaUpperleft, alphaA, tiff);
}

/** save an image and an alpha channel to a tiff file.
 *
 *  so far, only gray and RGB images are supported. the alpha
 *  channel is stored with the same type as the color channels.
 *
 *  If the alpha channels uses a different type as the
 *  color channel (for example 8 bit alpha channel, float color channels),
 *  they are converted to a sensible value. (0..1 for float alpha).
 */
template <class ImageIterator, class ImageAccessor,
          class AlphaIterator, class BImageAccessor>

void
createAlphaTiffImage(vigra::triple<ImageIterator, ImageIterator, ImageAccessor> src,
                     vigra::pair<AlphaIterator, BImageAccessor> alpha,
                     vigra::TiffImage * tiff)
{
    createAlphaTiffImage(src.first, src.second, src.third,
                         alpha.first, alpha.second, tiff);
}



//***************************************************************************
//
//  functions to read tiff files with a single alpha channel,
//  multiple layers and offsets.
//
//***************************************************************************


}

#endif // _TIFFUTILS_H
