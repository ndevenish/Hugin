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
//#include <tiffio.h>

// add this to the vigra namespace
namespace vigra {

// try to add stuff the vigra way
#if 0

template <class RGBImageIterator, class RGBAccessor,
          class BImageIterator, class BAccessor>
inline void
createRGBATiffImage(RGBImageIterator upperleft, RGBImageIterator lowerright,
                    RGBAccessor a,
                    BImageIterator alphaUpperleft, BAccessor alphaA,
                    TiffImage * tiff)
{
    // call right constructor class for this image type
    CreateRGBATiffImage<typename RGBAccessor::value_type>::
        exec(upperleft, lowerright, a, tiff);
}

template <class RGBImageIterator, class RGBAccessor,
          class BImageIterator, class BImageAccessor>

inline void
createRGBATiffImage(triple<RGBImageIterator, RGBImageIterator, RGBAccessor> src,
                    vigra::pair<BImageIterator, BImageAccessor> alpha,
                    TiffImage * tiff)
{
    createRGBTiffImage(src.first, src.second, src.third,
                       alpha.first, alpha.second, tiff);
}

/** constructor class for unsigned char alpha images */
template <>
struct CreateRGBATiffImage<RGBValue<unsigned char> >
{
    template <class ImageIterator, class Accessor,
              class BImageIterator, class BAccessor>
    static void
    exec(ImageIterator upperleft, ImageIterator lowerright, Accessor a,
         BImageIterator aUL, BAccessor aA,
         TiffImage * tiff)
    {
        createRGBATiffImage(upperleft, lowerright, a, aUL, aA, tiff);
    }
};

#endif



template <class RGBImageIterator, class RGBAccessor,
          class BImageIterator, class BAccessor>
void
createBRGBATiffImage(RGBImageIterator upperleft, RGBImageIterator lowerright,
                     RGBAccessor a,
                     BImageIterator alphaUpperleft, BAccessor alphaA,
                     TiffImage * tiff)
{
    int w = lowerright.x - upperleft.x;
    int h = lowerright.y - upperleft.y;

    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    // for alpha stuff, do not uses premultilied data
    // We do not want to throw away data & accuracy by premultiplying
    uint16 nextra_samples = 1;
    uint16 extra_samples = EXTRASAMPLE_UNASSALPHA;
    TIFFSetField (tiff, TIFFTAG_EXTRASAMPLES, nextra_samples, &extra_samples);

    int bufsize = TIFFScanlineSize(tiff);
    tdata_t * buf = new tdata_t[bufsize];

    RGBImageIterator ys(upperleft);
    BImageIterator ya(alphaUpperleft);

    try
    {
        for(int y=0; y<h; ++y, ++ys.y, ++ya.y)
        {
            uint8 * pr = (uint8 *)buf;
            uint8 * pg = pr+1;
            uint8 * pb = pg+1;
            uint8 * alpha = pb+1;

            RGBImageIterator xs(ys);
            BImageIterator xa(ya);

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

}

#endif // _TIFFUTILS_H
