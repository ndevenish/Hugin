/*
 * Copyright (C) 2004-2007 Andrew Mihal
 *
 * This file is part of Enblend.
 *
 * Stripped down and adjust for use in hugin by Pablo d'Angelo
 *
 * Enblend is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Enblend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Enblend; if not, write to the Free Software
 * <http://www.gnu.org/licenses/>.
 */
#ifndef __PYRAMID_H__
#define __PYRAMID_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <functional>
#include <vector>

#include <vigra/convolution.hxx>
#include <vigra/error.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/numerictraits.hxx>
#include <vigra/rgbvalue.hxx>
#include <vigra/sized_int.hxx>
#include <vigra/transformimage.hxx>

//#include "fixmath.h"

namespace enblend {

using std::cout;
using std::vector;
using std::pair;

using vigra::linearRangeMapping;
using vigra::NumericTraits;
using vigra::transformImage;
using vigra::triple;
using vigra::UInt16;
using vigra::UInt16Image;
using vigra::UInt16RGBImage;
using vigra::Diff2D;


#define IMUL6(A) (A * SKIPSMImagePixelType(6))
#define IMUL5(A) (A * SKIPSMImagePixelType(5))
#define IMUL11(A) (A * SKIPSMImagePixelType(11))
#define AMUL6(A) (A * SKIPSMAlphaPixelType(6))

/** Calculate the half-width of a n-level filter.
 *  Assumes that the input function is a left-handed function,
 *  and the last non-zero input is at location 0.
 *  Returns the location of the last non-zero output.
 */
template <typename ImagePixelComponentType>
unsigned int filterHalfWidth(const unsigned int levels) {
    // For levels >= 30, the full width will just barely fit in int32.
    // When this range is added to a bounding box it will certainly
    // overflow the Diff2D.
    vigra_precondition((levels >= 1 && levels <= 29),
            "filterHalfWidth: levels outside of range [1,29]");

    // This is the arithmetic half width.
    int halfWidth = (levels == 1) ? 0 : ((1 << (levels+1)) - 4);

    return halfWidth;
}

/** The Burt & Adelson Reduce operation.
 *  This version is for images with alpha channels.
 *  Gaussian blur, downsampling, and extrapolation in one pass over the input image using SKIPSM-based algorithm.
 *  Uses only integer math, visits each pixel only once.
 *
 *  Reference:
 *  Frederick M. Waltz and John W.V. Miller. An efficient algorithm for Gaussian blur using finite-state machines.
 *  SPIE Conf. on Machine Vision Systems for Inspection and Metrology VII. November 1998.
 *
 *  *************************************************************************************************
 *  1-D explanation of algorithm:
 *  
 *  src image pixels:     A    B    C    D    E    F    G
 *  dst image pixels:     W         X         Y         Z
 *  
 *  Algorithm iterates over src image pixels from left to right.
 *  At even src image pixels, the output of the previous dst image pixel is calculated.
 *  For example, when visiting E, the value of X is written to the dst image.
 *  
 *  State variables before visiting E:
 *  sr0 = C
 *  sr1 = A + 4B
 *  srp = 4D
 *  
 *  State variables after visiting E:
 *  sr0 = E
 *  sr1 = C + 4D
 *  srp = 4D
 *  X = A + 4B + 6C + 4D + E
 *  
 *  Updates when visiting even source pixel:
 *  (all updates occur in parallel)
 *  sr0 <= current
 *  sr1 <= sr0 + srp
 *  dst(-1) <= sr1 + 6*sr0 + srp + current
 *  
 *  Updates when visiting odd source pixel:
 *  srp <= 4*current
 *  
 *  *************************************************************************************************
 *  2-D explanation:
 *
 *  src image pixels:   A  B  C  D  E       dst image pixels:   a     b     c
 *                      F  G  H  I  J
 *                      K  L  M  N  O                           d     e     f
 *                      P  Q  R  S  T
 *                      U  V  W  X  Y                           g     h     i
 *  
 *  Algorithm visits all src image pixels from left to right and top to bottom.
 *  When visiting src pixel Y, the value of e will be written to the dst image.
 *  
 *  State variables before visiting Y:
 *  sr0 = W
 *  sr1 = U + 4V
 *  srp = 4X
 *  sc0[2] = K + 4L + 6M + 4N + O
 *  sc1[2] = (A + 4B + 6C + 4D + E) + 4*(F + 4G + 6H + 4I + J)
 *  scp[2] = 4*(P + 4Q + 6R + 4S + T)
 *  
 *  State variables after visiting Y:
 *  sr0 = Y
 *  sr1 = W + 4X
 *  srp = 4X
 *  sc0[2] = U + 4V + 6W + 4X + Y
 *  sc1[2] = (K + 4L + 6M + 4N + O) + 4*(P + 4Q + 6R + 4S + T)
 *  scp[2] = 4*(P + 4Q + 6R + 4S + T)
 *  e =   1 * (A + 4B + 6C + 4D + E)
 *      + 4 * (F + 4G + 6H + 4I + J)
 *      + 6 * (K + 4L + 6M + 4N + O)
 *      + 4 * (P + 4Q + 6R + 4S + T)
 *      + 1 * (U + 4V + 6W + 4X + Y)
 *  
 *  Updates when visiting (even x, even y) source pixel:
 *  (all updates occur in parallel)
 *  sr0 <= current
 *  sr1 <= sr0 + srp
 *  sc0[x] <= sr1 + 6*sr0 + srp + current
 *  sc1[x] <= sc0[x] + scp[x]
 *  dst(-1,-1) <= sc1[x] + 6*sc0[x] + scp + (new sc0[x])
 *  
 *  Updates when visiting (odd x, even y) source pixel:
 *  srp <= 4*current
 *  
 *  Updates when visiting (even x, odd y) source pixel:
 *  sr0 <= current
 *  sr1 <= sr0 + srp
 *  scp[x] <= 4*(sr1 + 6*sr0 + srp + current)
 *  
 *  Updates when visting (odd x, odd y) source pixel:
 *  srp <= 4*current
 */
template <typename SKIPSMImagePixelType, typename SKIPSMAlphaPixelType,
        typename SrcImageIterator, typename SrcAccessor,
        typename AlphaIterator, typename AlphaAccessor,
        typename DestImageIterator, typename DestAccessor,
        typename DestAlphaIterator, typename DestAlphaAccessor>
inline void reduce(bool wraparound,
        SrcImageIterator src_upperleft,
        SrcImageIterator src_lowerright,
        SrcAccessor sa,
        AlphaIterator alpha_upperleft,
        AlphaAccessor aa,
        DestImageIterator dest_upperleft,
        DestImageIterator dest_lowerright,
        DestAccessor da,
        DestAlphaIterator dest_alpha_upperleft,
        DestAlphaIterator dest_alpha_lowerright,
        DestAlphaAccessor daa) {

    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestAlphaAccessor::value_type DestAlphaPixelType;

    int src_w = src_lowerright.x - src_upperleft.x;
    int src_h = src_lowerright.y - src_upperleft.y;
    int dst_w = dest_lowerright.x - dest_upperleft.x;
    //int dst_h = dest_lowerright.y - dest_upperleft.y;

    vigra_precondition(src_w > 1 && src_h > 1,
            "src image too small in reduce");

    // State variables for source image pixel values
    SKIPSMImagePixelType isr0, isr1, isrp;
    SKIPSMImagePixelType *isc0 = new SKIPSMImagePixelType[dst_w + 1];
    SKIPSMImagePixelType *isc1 = new SKIPSMImagePixelType[dst_w + 1];
    SKIPSMImagePixelType *iscp = new SKIPSMImagePixelType[dst_w + 1];

    // State variables for source mask pixel values
    SKIPSMAlphaPixelType asr0, asr1, asrp;
    SKIPSMAlphaPixelType *asc0 = new SKIPSMAlphaPixelType[dst_w + 1];
    SKIPSMAlphaPixelType *asc1 = new SKIPSMAlphaPixelType[dst_w + 1];
    SKIPSMAlphaPixelType *ascp = new SKIPSMAlphaPixelType[dst_w + 1];

    // Convenient constants
    const SKIPSMImagePixelType SKIPSMImageZero(NumericTraits<SKIPSMImagePixelType>::zero());
    const SKIPSMAlphaPixelType SKIPSMAlphaZero(NumericTraits<SKIPSMAlphaPixelType>::zero());
    const SKIPSMAlphaPixelType SKIPSMAlphaOne(NumericTraits<SKIPSMAlphaPixelType>::one());
    const DestPixelType DestImageZero(NumericTraits<DestPixelType>::zero());
    const DestAlphaPixelType DestAlphaZero(NumericTraits<DestAlphaPixelType>::zero());
    const DestAlphaPixelType DestAlphaMax(NumericTraits<DestAlphaPixelType>::max());

    DestImageIterator dy = dest_upperleft;
    DestImageIterator dx = dy;
    SrcImageIterator sy = src_upperleft;
    SrcImageIterator sx = sy;
    AlphaIterator ay = alpha_upperleft;
    AlphaIterator ax = ay;
    DestAlphaIterator day = dest_alpha_upperleft;
    DestAlphaIterator dax = day;

    bool evenY = true;
    bool evenX = true;
    int srcy = 0;
    int srcx = 0;
    //int dsty = 0;
    int dstx = 0;

    // First row
    {
        if (wraparound) {
            asr0 = aa(ay, Diff2D(src_w-2, 0)) ? SKIPSMAlphaOne : SKIPSMAlphaZero;
            asr1 = SKIPSMAlphaZero;
            asrp = aa(ay, Diff2D(src_w-1, 0)) ? (SKIPSMAlphaOne * 4) : SKIPSMAlphaZero;
            isr0 = aa(ay, Diff2D(src_w-2, 0)) ? SKIPSMImagePixelType(sa(sy, Diff2D(src_w-2, 0))) : SKIPSMImageZero;
            isr1 = SKIPSMImageZero;
            isrp = aa(ay, Diff2D(src_w-1, 0)) ? (SKIPSMImagePixelType(sa(sy, Diff2D(src_w-1, 0))) * 4) : SKIPSMImageZero;
        } else {
            asr0 = SKIPSMAlphaZero;
            asr1 = SKIPSMAlphaZero;
            asrp = SKIPSMAlphaZero;
            isr0 = SKIPSMImageZero;
            isr1 = SKIPSMImageZero;
            isrp = SKIPSMImageZero;
        }

        for (sx = sy, ax = ay, evenX = true, srcx = 0, dstx = 0;  srcx < src_w; ++srcx, ++sx.x, ++ax.x) {
            SKIPSMAlphaPixelType mcurrent(aa(ax) ? SKIPSMAlphaOne : SKIPSMAlphaZero);
            SKIPSMImagePixelType icurrent(aa(ax) ? SKIPSMImagePixelType(sa(sx)) : SKIPSMImageZero);
            if (evenX) {
                asc1[dstx] = SKIPSMAlphaZero;
                asc0[dstx] = asr1 + AMUL6(asr0) + asrp + mcurrent;
                asr1 = asr0 + asrp;
                asr0 = mcurrent;
                isc1[dstx] = SKIPSMImageZero;
                isc0[dstx] = isr1 + IMUL6(isr0) + isrp + icurrent;
                isr1 = isr0 + isrp;
                isr0 = icurrent;
            }
            else {
                asrp = mcurrent * 4;
                isrp = icurrent * 4;
                ++dstx;
            }
            evenX = !evenX;
        }

        // Last entries in first row
        if (!evenX) {
            // previous srcx was even
            ++dstx;
            if (wraparound) {
                asc1[dstx] = SKIPSMAlphaZero;
                asc0[dstx] = asr1 + AMUL6(asr0) + (aa(ay) ? (SKIPSMAlphaOne * 4) : SKIPSMAlphaZero) + (aa(ay, Diff2D(1,0)) ? SKIPSMAlphaOne : SKIPSMAlphaZero);
                isc1[dstx] = SKIPSMImageZero;
                isc0[dstx] = isr1 + IMUL6(isr0) + (aa(ay) ? (SKIPSMImagePixelType(sa(sy)) * 4) : SKIPSMImageZero)
                                  + (aa(ay, Diff2D(1,0)) ? SKIPSMImagePixelType(sa(sy, Diff2D(1,0))) : SKIPSMImageZero);
            } else {
                asc1[dstx] = SKIPSMAlphaZero;
                asc0[dstx] = asr1 + AMUL6(asr0);
                isc1[dstx] = SKIPSMImageZero;
                isc0[dstx] = isr1 + IMUL6(isr0);
            }
        }
        else {
            // previous srcx was odd
            if (wraparound) {
                asc1[dstx] = SKIPSMAlphaZero;
                asc0[dstx] = asr1 + AMUL6(asr0) + asrp + (aa(ay) ? SKIPSMAlphaOne : SKIPSMAlphaZero);
                isc1[dstx] = SKIPSMImageZero;
                isc0[dstx] = isr1 + IMUL6(isr0) + isrp + (aa(ay) ? SKIPSMImagePixelType(sa(sy)) : SKIPSMImageZero);
            } else {
                asc1[dstx] = SKIPSMAlphaZero;
                asc0[dstx] = asr1 + AMUL6(asr0) + asrp;
                isc1[dstx] = SKIPSMImageZero;
                isc0[dstx] = isr1 + IMUL6(isr0) + isrp;
            }
        }
    }
    ++sy.y;
    ++ay.y;

    // Main Rows
    {
        for (evenY = false, srcy = 1; srcy < src_h; ++srcy, ++sy.y, ++ay.y) {

            if (wraparound) {
                asr0 = aa(ay, Diff2D(src_w-2, 0)) ? SKIPSMAlphaOne : SKIPSMAlphaZero;
                asr1 = SKIPSMAlphaZero;
                asrp = aa(ay, Diff2D(src_w-1, 0)) ? (SKIPSMAlphaOne * 4) : SKIPSMAlphaZero;
                isr0 = aa(ay, Diff2D(src_w-2, 0)) ? SKIPSMImagePixelType(sa(sy, Diff2D(src_w-2,0))) : SKIPSMImageZero;
                isr1 = SKIPSMImageZero;
                isrp = aa(ay, Diff2D(src_w-1, 0)) ? (SKIPSMImagePixelType(sa(sy, Diff2D(src_w-1,0))) * 4) : SKIPSMImageZero;
            } else {
                asr0 = SKIPSMAlphaZero;
                asr1 = SKIPSMAlphaZero;
                asrp = SKIPSMAlphaZero;
                isr0 = SKIPSMImageZero;
                isr1 = SKIPSMImageZero;
                isrp = SKIPSMImageZero;
            }

            if (evenY) {
                // Even-numbered row

                // First entry in row
                sx = sy;
                ax = ay;
                if (wraparound) {
                    asr1 = asr0 + asrp;
                    isr1 = isr0 + isrp;
                }
                asr0 = aa(ax) ? SKIPSMAlphaOne : SKIPSMAlphaZero;
                isr0 = aa(ax) ? SKIPSMImagePixelType(sa(sx)) : SKIPSMImageZero;
                // isc*[0] are never used
                ++sx.x;
                ++ax.x;
                dx = dy;
                dax = day;

                // Main entries in row
                for (evenX = false, srcx = 1, dstx = 0; srcx < src_w; ++srcx, ++sx.x, ++ax.x) {
                    SKIPSMAlphaPixelType mcurrent(aa(ax) ? SKIPSMAlphaOne : SKIPSMAlphaZero);
                    SKIPSMImagePixelType icurrent(aa(ax) ? SKIPSMImagePixelType(sa(sx)) : SKIPSMImageZero);
                    if (evenX) {
                        SKIPSMAlphaPixelType ap = asc1[dstx] + AMUL6(asc0[dstx]) + ascp[dstx];
                        asc1[dstx] = asc0[dstx] + ascp[dstx];
                        asc0[dstx] = asr1 + AMUL6(asr0) + asrp + mcurrent;
                        asr1 = asr0 + asrp;
                        asr0 = mcurrent;
                        ap += asc0[dstx];

                        SKIPSMImagePixelType ip = isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx];
                        isc1[dstx] = isc0[dstx] + iscp[dstx];
                        isc0[dstx] = isr1 + IMUL6(isr0) + isrp + icurrent;
                        isr1 = isr0 + isrp;
                        isr0 = icurrent;
                        if (ap) {
                            ip += isc0[dstx];
                            ip /= ap;
                            da.set(DestPixelType(ip), dx);
                            daa.set(DestAlphaMax, dax);
                        } else {
                            da.set(DestImageZero, dx);
                            daa.set(DestAlphaZero, dax);
                        }

                        ++dx.x;
                        ++dax.x;
                    }
                    else {
                        asrp = mcurrent * 4;
                        isrp = icurrent * 4;
                        ++dstx;
                    }
                    evenX = !evenX;
                }

                // Last entries in row
                if (!evenX) {
                    // previous srcx was even
                    ++dstx;

                    SKIPSMAlphaPixelType ap = asc1[dstx] + AMUL6(asc0[dstx]) + ascp[dstx];
                    asc1[dstx] = asc0[dstx] + ascp[dstx];
                    if (wraparound) {
                        asc0[dstx] = asr1 + AMUL6(asr0) + (aa(ay) ? (SKIPSMAlphaOne * 4) : SKIPSMAlphaZero) + (aa(ay, Diff2D(1,0)) ? SKIPSMAlphaOne : SKIPSMAlphaZero);
                    } else {
                        asc0[dstx] = asr1 + AMUL6(asr0);
                    }
                    ap += asc0[dstx];

                    SKIPSMImagePixelType ip = isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx];
                    isc1[dstx] = isc0[dstx] + iscp[dstx];
                    if (wraparound) {
                        isc0[dstx] = isr1 + IMUL6(isr0) + (aa(ay) ? (SKIPSMImagePixelType(sa(sy)) * 4) : SKIPSMImageZero)
                                          + (aa(ay, Diff2D(1,0)) ? SKIPSMImagePixelType(sa(sy, Diff2D(1,0))) : SKIPSMImageZero);
                    } else {
                        isc0[dstx] = isr1 + IMUL6(isr0);
                    }
                    if (ap) {
                        ip += isc0[dstx];
                        //ip /= SKIPSMImagePixelType(ap);
                        ip /= ap;
                        da.set(DestPixelType(ip), dx);
                        daa.set(DestAlphaMax, dax);
                    } else {
                        da.set(DestImageZero, dx);
                        daa.set(DestAlphaZero, dax);
                    }
                }
                else {
                    // Previous srcx was odd
                    SKIPSMAlphaPixelType ap = asc1[dstx] + AMUL6(asc0[dstx]) + ascp[dstx];
                    asc1[dstx] = asc0[dstx] + ascp[dstx];
                    if (wraparound) {
                        asc0[dstx] = asr1 + AMUL6(asr0) + asrp + (aa(ay) ? SKIPSMAlphaOne : SKIPSMAlphaZero);
                    } else {
                        asc0[dstx] = asr1 + AMUL6(asr0) + asrp;
                    }
                    ap += asc0[dstx];

                    SKIPSMImagePixelType ip = isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx];
                    isc1[dstx] = isc0[dstx] + iscp[dstx];
                    if (wraparound) {
                        isc0[dstx] = isr1 + IMUL6(isr0) + isrp + (aa(ay) ? SKIPSMImagePixelType(sa(sy)) : SKIPSMImageZero);
                    } else {
                        isc0[dstx] = isr1 + IMUL6(isr0) + isrp;
                    }
                    if (ap) {
                        ip += isc0[dstx];
                        //ip /= SKIPSMImagePixelType(ap);
                        ip /= ap;
                        da.set(DestPixelType(ip), dx);
                        daa.set(DestAlphaMax, dax);
                    } else {
                        da.set(DestImageZero, dx);
                        daa.set(DestAlphaZero, dax);
                    }
                }

                ++dy.y;
                ++day.y;

            }
            else {
                // First entry in odd-numbered row
                sx = sy;
                ax = ay;
                if (wraparound) {
                    asr1 = asr0 + asrp;
                    isr1 = isr0 + isrp;
                }
                asr0 = aa(ax) ? SKIPSMAlphaOne : SKIPSMAlphaZero;
                isr0 = aa(ax) ? SKIPSMImagePixelType(sa(sx)) : SKIPSMImageZero;
                // isc*[0] are never used
                ++sx.x;
                ++ax.x;

                // Main entries in odd-numbered row
                for (evenX = false, srcx = 1, dstx = 0; srcx < src_w; ++srcx, ++sx.x, ++ax.x) {
                    SKIPSMAlphaPixelType mcurrent(aa(ax) ? SKIPSMAlphaOne : SKIPSMAlphaZero);
                    SKIPSMImagePixelType icurrent(aa(ax) ? SKIPSMImagePixelType(sa(sx)) : SKIPSMImageZero);
                    if (evenX) {
                        ascp[dstx] = (asr1 + AMUL6(asr0) + asrp + mcurrent) * 4;
                        asr1 = asr0 + asrp;
                        asr0 = mcurrent;
                        iscp[dstx] = (isr1 + IMUL6(isr0) + isrp + icurrent) * 4;
                        isr1 = isr0 + isrp;
                        isr0 = icurrent;
                    }
                    else {
                        asrp = mcurrent * 4;
                        isrp = icurrent * 4;
                        ++dstx;
                    }
                    evenX = !evenX;
                }

                // Last entries in row
                if (!evenX) {
                    // previous srcx was even
                    ++dstx;
                    if (wraparound) {
                        ascp[dstx] = (asr1 + AMUL6(asr0) + (aa(ay) ? (SKIPSMAlphaOne * 4) : SKIPSMAlphaZero)
                                           + (aa(ay, Diff2D(1,0)) ? SKIPSMAlphaOne : SKIPSMAlphaZero)
                                     ) * 4;
                        iscp[dstx] = (isr1 + IMUL6(isr0) + (aa(ay) ? (SKIPSMImagePixelType(sa(sy)) * 4) : SKIPSMImageZero)
                                           + (aa(ay, Diff2D(1,0)) ? SKIPSMImagePixelType(sa(sy, Diff2D(1,0))) : SKIPSMImageZero)
                                     ) * 4;
                    } else {
                        ascp[dstx] = (asr1 + AMUL6(asr0)) * 4;
                        iscp[dstx] = (isr1 + IMUL6(isr0)) * 4;
                    }
                }
                else {
                    // previous srcx was odd
                    if (wraparound) {
                        ascp[dstx] = (asr1 + AMUL6(asr0) + asrp + (aa(ay) ? SKIPSMAlphaOne : SKIPSMAlphaZero)) * 4;
                        iscp[dstx] = (isr1 + IMUL6(isr0) + isrp + (aa(ay) ? SKIPSMImagePixelType(sa(sy)) : SKIPSMImageZero)) * 4;
                    } else {
                        ascp[dstx] = (asr1 + AMUL6(asr0) + asrp) * 4;
                        iscp[dstx] = (isr1 + IMUL6(isr0) + isrp) * 4;
                    }
                }
            }
            evenY = !evenY;
        }
    }

    // Last Rows
    {
        if (!evenY) {
            // Last srcy was even
            // odd row will set all iscp[] to zero
            // even row will do:
            //isc0[dstx] = 0;
            //isc1[dstx] = isc0[dstx] + 4*iscp[dstx]
            //out = isc1[dstx] + 6*isc0[dstx] + 4*iscp[dstx] + newisc0[dstx]
            for (dstx = 1, dx = dy, dax = day; dstx < (dst_w + 1); ++dstx, ++dx.x, ++dax.x) {
                SKIPSMAlphaPixelType ap = asc1[dstx] + AMUL6(asc0[dstx]);
                if (ap) {
                    //SKIPSMImagePixelType ip = (isc1[dstx] + IMUL6(isc0[dstx])) / SKIPSMImagePixelType(ap);
                    SKIPSMImagePixelType ip = (isc1[dstx] + IMUL6(isc0[dstx])) / ap;
                    da.set(DestPixelType(ip), dx);
                    daa.set(DestAlphaMax, dax);
                } else {
                    da.set(DestImageZero, dx);
                    daa.set(DestAlphaZero, dax);
                }
            }
        }
        else {
            // Last srcy was odd
            // even row will do:
            // isc0[dstx] = 0;
            // isc1[dstx] = isc0[dstx] + 4*iscp[dstx]
            // out = isc1[dstx] + 6*isc0[dstx] + 4*iscp[dstx] + newisc0[dstx]
            for (dstx = 1, dx = dy, dax = day; dstx < (dst_w + 1); ++dstx, ++dx.x, ++dax.x) {
                SKIPSMAlphaPixelType ap = asc1[dstx] + AMUL6(asc0[dstx]) + ascp[dstx];
                if (ap) {
                    //SKIPSMImagePixelType ip = (isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx]) / SKIPSMImagePixelType(ap);
                    SKIPSMImagePixelType ip = (isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx]) / ap;
                    da.set(DestPixelType(ip), dx);
                    daa.set(DestAlphaMax, dax);
                } else {
                    da.set(DestImageZero, dx);
                    daa.set(DestAlphaZero, dax);
                }
            }
        }
    }

    delete[] isc0;
    delete[] isc1;
    delete[] iscp;

    delete[] asc0;
    delete[] asc1;
    delete[] ascp;

};

// Version using argument object factories.
template <typename SKIPSMImagePixelType, typename SKIPSMAlphaPixelType,
        typename SrcImageIterator, typename SrcAccessor,
        typename AlphaIterator, typename AlphaAccessor,
        typename DestImageIterator, typename DestAccessor,
        typename DestAlphaIterator, typename DestAlphaAccessor>
inline void reduce(bool wraparound,
        triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
        pair<AlphaIterator, AlphaAccessor> mask,
        triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
        triple<DestAlphaIterator, DestAlphaIterator, DestAlphaAccessor> destMask) {
    reduce<SKIPSMImagePixelType, SKIPSMAlphaPixelType>(wraparound,
            src.first, src.second, src.third,
            mask.first, mask.second,
            dest.first, dest.second, dest.third,
            destMask.first, destMask.second, destMask.third);
};

/** The Burt & Adelson Reduce operation.
 *  This version is for images that do not have alpha channels.
 */
template <typename SKIPSMImagePixelType,
        typename SrcImageIterator, typename SrcAccessor,
        typename DestImageIterator, typename DestAccessor>
inline void reduce(bool wraparound,
        SrcImageIterator src_upperleft,
        SrcImageIterator src_lowerright,
        SrcAccessor sa,
        DestImageIterator dest_upperleft,
        DestImageIterator dest_lowerright,
        DestAccessor da) {

    typedef typename DestAccessor::value_type DestPixelType;

    int src_w = src_lowerright.x - src_upperleft.x;
    int src_h = src_lowerright.y - src_upperleft.y;
    int dst_w = dest_lowerright.x - dest_upperleft.x;
    //int dst_h = dest_lowerright.y - dest_upperleft.y;

    vigra_precondition(src_w > 1 && src_h > 1,
            "src image too small in reduce");

    // State variables for source image pixel values
    SKIPSMImagePixelType isr0, isr1, isrp;
    SKIPSMImagePixelType *isc0 = new SKIPSMImagePixelType[dst_w + 1];
    SKIPSMImagePixelType *isc1 = new SKIPSMImagePixelType[dst_w + 1];
    SKIPSMImagePixelType *iscp = new SKIPSMImagePixelType[dst_w + 1];

    // Convenient constants
    const SKIPSMImagePixelType SKIPSMImageZero(NumericTraits<SKIPSMImagePixelType>::zero());

    DestImageIterator dy = dest_upperleft;
    DestImageIterator dx = dy;
    SrcImageIterator sy = src_upperleft;
    SrcImageIterator sx = sy;

    bool evenY = true;
    bool evenX = true;
    int srcy = 0;
    int srcx = 0;
    //int dsty = 0;
    int dstx = 0;

    // First row
    {
        if (wraparound) {
            isr0 = SKIPSMImagePixelType(sa(sy, Diff2D(src_w-2, 0)));
            isr1 = SKIPSMImageZero;
            isrp = SKIPSMImagePixelType(sa(sy, Diff2D(src_w-1, 0))) * 4;
        } else {
            isr0 = SKIPSMImagePixelType(sa(sy));
            isr1 = SKIPSMImageZero;
            isrp = SKIPSMImagePixelType(sa(sy)) * 4;
        }

        // Main pixels in first row
        for (sx = sy, evenX = true, srcx = 0, dstx = 0;  srcx < src_w; ++srcx, ++sx.x) {
            SKIPSMImagePixelType icurrent(SKIPSMImagePixelType(sa(sx)));
            if (evenX) {
                isc0[dstx] = isr1 + IMUL6(isr0) + isrp + icurrent;
                isc1[dstx] = IMUL5(isc0[dstx]);
                isr1 = isr0 + isrp;
                isr0 = icurrent;
            }
            else {
                isrp = icurrent * 4;
                ++dstx;
            }
            evenX = !evenX;
        }

        // Last entries in first row
        if (!evenX) {
            // previous srcx was even
            ++dstx;
            if (wraparound) {
                isc0[dstx] = isr1 + IMUL6(isr0) + (SKIPSMImagePixelType(sa(sy)) * 4)
                                  + SKIPSMImagePixelType(sa(sy, Diff2D(1,0)));
                isc1[dstx] = IMUL5(isc0[dstx]);
            } else {
                isc0[dstx] = isr1 + IMUL11(isr0);
                isc1[dstx] = IMUL5(isc0[dstx]);
            }
        }
        else {
            // previous srcx was odd
            if (wraparound) {
                isc0[dstx] = isr1 + IMUL6(isr0) + isrp + SKIPSMImagePixelType(sa(sy));
                isc1[dstx] = IMUL5(isc0[dstx]);
            } else {
                isc0[dstx] = isr1 + IMUL6(isr0) + isrp + (isrp / 4);
                isc1[dstx] = IMUL5(isc0[dstx]);
            }
        }
    }
    ++sy.y;

    // Main Rows
    {
        for (evenY = false, srcy = 1; srcy < src_h; ++srcy, ++sy.y) {

            if (wraparound) {
                isr0 = SKIPSMImagePixelType(sa(sy, Diff2D(src_w-2,0)));
                isr1 = SKIPSMImageZero;
                isrp = SKIPSMImagePixelType(sa(sy, Diff2D(src_w-1,0))) * 4;
            } else {
                isr0 = SKIPSMImagePixelType(sa(sy));
                isr1 = SKIPSMImageZero;
                isrp = SKIPSMImagePixelType(sa(sy)) * 4;
            }

            if (evenY) {
                // Even-numbered row

                // First entry in row
                sx = sy;
                isr1 = isr0 + isrp;
                isr0 = SKIPSMImagePixelType(sa(sx));
                // isc*[0] are never used
                ++sx.x;
                dx = dy;

                // Main entries in row
                for (evenX = false, srcx = 1, dstx = 0; srcx < src_w; ++srcx, ++sx.x) {
                    SKIPSMImagePixelType icurrent(SKIPSMImagePixelType(sa(sx)));
                    if (evenX) {
                        SKIPSMImagePixelType ip = isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx];
                        isc1[dstx] = isc0[dstx] + iscp[dstx];
                        isc0[dstx] = isr1 + IMUL6(isr0) + isrp + icurrent;
                        isr1 = isr0 + isrp;
                        isr0 = icurrent;
                        ip += isc0[dstx];
                        ip /= 256;
                        da.set(DestPixelType(ip), dx);
                        ++dx.x;
                    }
                    else {
                        isrp = icurrent * 4;
                        ++dstx;
                    }
                    evenX = !evenX;
                }

                // Last entries in row
                if (!evenX) {
                    // previous srcx was even
                    ++dstx;

                    SKIPSMImagePixelType ip = isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx];
                    isc1[dstx] = isc0[dstx] + iscp[dstx];
                    if (wraparound) {
                        isc0[dstx] = isr1 + IMUL6(isr0) + (SKIPSMImagePixelType(sa(sy)) * 4)
                                          + SKIPSMImagePixelType(sa(sy, Diff2D(1,0)));
                    } else {
                        isc0[dstx] = isr1 + IMUL11(isr0);
                    }
                    ip += isc0[dstx];
                    ip /= 256;
                    da.set(DestPixelType(ip), dx);
                }
                else {
                    // Previous srcx was odd
                    SKIPSMImagePixelType ip = isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx];
                    isc1[dstx] = isc0[dstx] + iscp[dstx];
                    if (wraparound) {
                        isc0[dstx] = isr1 + IMUL6(isr0) + isrp + SKIPSMImagePixelType(sa(sy));
                    } else {
                        isc0[dstx] = isr1 + IMUL6(isr0) + isrp + (isrp / 4);
                    }
                    ip += isc0[dstx];
                    ip /= 256;
                    da.set(DestPixelType(ip), dx);
                }

                ++dy.y;
            }
            else {
                // First entry in odd-numbered row
                sx = sy;
                isr1 = isr0 + isrp;
                isr0 = SKIPSMImagePixelType(sa(sx));
                // isc*[0] are never used
                ++sx.x;

                // Main entries in odd-numbered row
                for (evenX = false, srcx = 1, dstx = 0; srcx < src_w; ++srcx, ++sx.x) {
                    SKIPSMImagePixelType icurrent(SKIPSMImagePixelType(sa(sx)));
                    if (evenX) {
                        iscp[dstx] = (isr1 + IMUL6(isr0) + isrp + icurrent) * 4;
                        isr1 = isr0 + isrp;
                        isr0 = icurrent;
                    }
                    else {
                        isrp = icurrent * 4;
                        ++dstx;
                    }
                    evenX = !evenX;
                }
                // Last entries in row
                if (!evenX) {
                    // previous srcx was even
                    ++dstx;
                    if (wraparound) {
                        iscp[dstx] = (isr1 + IMUL6(isr0) + (SKIPSMImagePixelType(sa(sy)) * 4)
                                           + SKIPSMImagePixelType(sa(sy, Diff2D(1,0)))
                                     ) * 4;
                    } else {
                        iscp[dstx] = (isr1 + IMUL11(isr0)) * 4;
                    }
                }
                else {
                    // previous srcx was odd
                    if (wraparound) {
                        iscp[dstx] = (isr1 + IMUL6(isr0) + isrp + SKIPSMImagePixelType(sa(sy))) * 4;
                    } else {
                        iscp[dstx] = (isr1 + IMUL6(isr0) + isrp + (isrp / 4)) * 4;
                    }
                }
            }
            evenY = !evenY;
        }
    }

    // Last Rows
    {
        if (!evenY) {
            // Last srcy was even
            // odd row will set all iscp[] to zero
            // even row will do:
            //isc0[dstx] = 0;
            //isc1[dstx] = isc0[dstx] + 4*iscp[dstx]
            //out = isc1[dstx] + 6*isc0[dstx] + 4*iscp[dstx] + newisc0[dstx]
            for (dstx = 1, dx = dy; dstx < (dst_w + 1); ++dstx, ++dx.x) {
                SKIPSMImagePixelType ip = (isc1[dstx] + IMUL11(isc0[dstx])) / 256;
                da.set(DestPixelType(ip), dx);
            }
        }
        else {
            // Last srcy was odd
            // even row will do:
            // isc0[dstx] = 0;
            // isc1[dstx] = isc0[dstx] + 4*iscp[dstx]
            // out = isc1[dstx] + 6*isc0[dstx] + 4*iscp[dstx] + newisc0[dstx]
            for (dstx = 1, dx = dy; dstx < (dst_w + 1); ++dstx, ++dx.x) {
                SKIPSMImagePixelType ip = (isc1[dstx] + IMUL6(isc0[dstx]) + iscp[dstx] + (iscp[dstx] / 4)) / 256;
                da.set(DestPixelType(ip), dx);
            }
        }
    }

    delete[] isc0;
    delete[] isc1;
    delete[] iscp;

};

// Version using argument object factories.
template <typename SKIPSMImagePixelType,
        typename SrcImageIterator, typename SrcAccessor,
        typename DestImageIterator, typename DestAccessor>
inline void reduce(bool wraparound,
        triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
        triple<DestImageIterator, DestImageIterator, DestAccessor> dest) {
    reduce<SKIPSMImagePixelType>(wraparound,
            src.first, src.second, src.third,
            dest.first, dest.second, dest.third);
};

// SKIPSM update routine used when visiting a pixel in the top two rows
// and the left two rows.
#define SKIPSM_EXPAND(SCALE_OUT00, SCALE_OUT10, SCALE_OUT01, SCALE_OUT11) \
    current = SKIPSMImagePixelType(sa(sx));                         \
    out00 = sc1a[srcx] + IMUL6(sc0a[srcx]);                         \
    out10 = sc1b[srcx] + IMUL6(sc0b[srcx]);                         \
    out01 = sc0a[srcx];                                             \
    out11 = sc0b[srcx];                                             \
    sc1a[srcx] = sc0a[srcx];                                        \
    sc1b[srcx] = sc0b[srcx];                                        \
    sc0a[srcx] = sr1 + IMUL6(sr0) + current;                        \
    sc0b[srcx] = (sr0 + current) * 4;                               \
    sr1 = sr0;                                                      \
    sr0 = current;                                                  \
    out00 += sc0a[srcx];                                            \
    out10 += sc0b[srcx];                                            \
    out01 += sc0a[srcx];                                            \
    out11 += sc0b[srcx];                                            \
    out00 /= SKIPSMImagePixelType(SCALE_OUT00);                     \
    out10 /= SKIPSMImagePixelType(SCALE_OUT10);                     \
    out01 /= SKIPSMImagePixelType(SCALE_OUT01);                     \
    out11 /= SKIPSMImagePixelType(SCALE_OUT11);                     \
    da.set(cf(SKIPSMImagePixelType(da(dx)), out00), dx);            \
    ++dx.x;                                                         \
    da.set(cf(SKIPSMImagePixelType(da(dx)), out10), dx);            \
    ++dx.x;                                                         \
    da.set(cf(SKIPSMImagePixelType(da(dxx)), out01), dxx);          \
    ++dxx.x;                                                        \
    da.set(cf(SKIPSMImagePixelType(da(dxx)), out11), dxx);          \
    ++dxx.x;

// SKIPSM update routine used when visiting a pixel in the main image body.
// Same as above, but with hard-coded scaling factors.
#define SKIPSM_EXPAND_SHIFT                                         \
    current = SKIPSMImagePixelType(sa(sx));                         \
    out00 = sc1a[srcx] + IMUL6(sc0a[srcx]);                         \
    out10 = sc1b[srcx] + IMUL6(sc0b[srcx]);                         \
    out01 = sc0a[srcx];                                             \
    out11 = sc0b[srcx];                                             \
    sc1a[srcx] = sc0a[srcx];                                        \
    sc1b[srcx] = sc0b[srcx];                                        \
    sc0a[srcx] = sr1 + IMUL6(sr0) + current;                        \
    sc0b[srcx] = (sr0 + current) * 4;                               \
    sr1 = sr0;                                                      \
    sr0 = current;                                                  \
    out00 += sc0a[srcx];                                            \
    out10 += sc0b[srcx];                                            \
    out01 += sc0a[srcx];                                            \
    out11 += sc0b[srcx];                                            \
    out00 /= 64;                                                    \
    out10 /= 64;                                                    \
    out01 /= 16;                                                    \
    out11 /= 16;                                                    \
    da.set(cf(SKIPSMImagePixelType(da(dx)), out00), dx);            \
    ++dx.x;                                                         \
    da.set(cf(SKIPSMImagePixelType(da(dx)), out10), dx);            \
    ++dx.x;                                                         \
    da.set(cf(SKIPSMImagePixelType(da(dxx)), out01), dxx);          \
    ++dxx.x;                                                        \
    da.set(cf(SKIPSMImagePixelType(da(dxx)), out11), dxx);          \
    ++dxx.x;

// SKIPSM update routine used for the extra row under the main image body.
#define SKIPSM_EXPAND_ROW_END(SCALE_OUT00, SCALE_OUT10, SCALE_OUT01, SCALE_OUT11) \
    out00 = sc1a[srcx] + IMUL6(sc0a[srcx]);                         \
    out10 = sc1b[srcx] + IMUL6(sc0b[srcx]);                         \
    out00 /= SKIPSMImagePixelType(SCALE_OUT00);                     \
    out10 /= SKIPSMImagePixelType(SCALE_OUT10);                     \
    da.set(cf(da(dx), out00), dx);                                  \
    ++dx.x;                                                         \
    da.set(cf(da(dx), out10), dx);                                  \
    ++dx.x;                                                         \
    if ((dst_h & 1) == 0) {                                         \
        out01 = sc0a[srcx];                                         \
        out11 = sc0b[srcx];                                         \
        out01 /= SKIPSMImagePixelType(SCALE_OUT01);                 \
        out11 /= SKIPSMImagePixelType(SCALE_OUT11);                 \
        da.set(cf(da(dxx), out01), dxx);                            \
        ++dxx.x;                                                    \
        da.set(cf(da(dxx), out11), dxx);                            \
        ++dxx.x;                                                    \
    }

// SKIPSM update routine used for the extra column to the right
// of the main image body.
#define SKIPSM_EXPAND_COLUMN_END(SCALE_OUT00, SCALE_OUT10, SCALE_OUT01, SCALE_OUT11) \
    out00 = sc1a[srcx] + IMUL6(sc0a[srcx]);                         \
    out01 = sc0a[srcx];                                             \
    out10 = sc1b[srcx] + IMUL6(sc0b[srcx]);                         \
    out11 = sc0b[srcx];                                             \
    sc1a[srcx] = sc0a[srcx];                                        \
    sc1b[srcx] = sc0b[srcx];                                        \
    sc0a[srcx] = sr1 + IMUL6(sr0);                                  \
    sc0b[srcx] = sr0 * 4;                                           \
    out00 += sc0a[srcx];                                            \
    out01 += sc0a[srcx];                                            \
    out00 /= SKIPSMImagePixelType(SCALE_OUT00);                     \
    out01 /= SKIPSMImagePixelType(SCALE_OUT01);                     \
    da.set(cf(da(dx), out00), dx);                                  \
    da.set(cf(da(dxx), out01), dxx);                                \
    if ((dst_w & 1) == 0) {                                         \
        ++dx.x;                                                     \
        ++dxx.x;                                                    \
        out10 += sc0b[srcx];                                        \
        out11 += sc0b[srcx];                                        \
        out10 /= SKIPSMImagePixelType(SCALE_OUT10);                 \
        out11 /= SKIPSMImagePixelType(SCALE_OUT11);                 \
        da.set(cf(da(dx), out10), dx);                              \
        da.set(cf(da(dxx), out11), dxx);                            \
    }

// SKIPSM update routine used for the extra column to the right
// of the main image body, with wraparound boundary conditions.
#define SKIPSM_EXPAND_COLUMN_END_WRAPAROUND(SCALE_OUT00, SCALE_OUT10, SCALE_OUT01, SCALE_OUT11) \
    out00 = sc1a[srcx] + IMUL6(sc0a[srcx]);                         \
    out01 = sc0a[srcx];                                             \
    out10 = sc1b[srcx] + IMUL6(sc0b[srcx]);                         \
    out11 = sc0b[srcx];                                             \
    sc1a[srcx] = sc0a[srcx];                                        \
    sc1b[srcx] = sc0b[srcx];                                        \
    sc0a[srcx] = sr1 + IMUL6(sr0) + SKIPSMImagePixelType(sa(sy));   \
    sc0b[srcx] = (sr0 + SKIPSMImagePixelType(sa(sy))) * 4;          \
    out00 += sc0a[srcx];                                            \
    out01 += sc0a[srcx];                                            \
    out00 /= SKIPSMImagePixelType(SCALE_OUT00);                     \
    out01 /= SKIPSMImagePixelType(SCALE_OUT01);                     \
    da.set(cf(da(dx), out00), dx);                                  \
    da.set(cf(da(dxx), out01), dxx);                                \
    if ((dst_w & 1) == 0) {                                         \
        ++dx.x;                                                     \
        ++dxx.x;                                                    \
        out10 += sc0b[srcx];                                        \
        out11 += sc0b[srcx];                                        \
        out10 /= SKIPSMImagePixelType(SCALE_OUT10);                 \
        out11 /= SKIPSMImagePixelType(SCALE_OUT11);                 \
        da.set(cf(da(dx), out10), dx);                              \
        da.set(cf(da(dxx), out11), dxx);                            \
    }

// SKIPSM update routine for the extra column to the right
// of the extra row under the main image body.
#define SKIPSM_EXPAND_ROW_COLUMN_END(SCALE_OUT00, SCALE_OUT10, SCALE_OUT01, SCALE_OUT11) \
    out00 = sc1a[srcx] + IMUL6(sc0a[srcx]);                         \
    out00 /= SKIPSMImagePixelType(SCALE_OUT00);                     \
    da.set(cf(da(dx), out00), dx);                                  \
    if ((dst_w & 1) == 0) {                                         \
        out10 = sc1b[srcx] + IMUL6(sc0b[srcx]);                     \
        out10 /= SKIPSMImagePixelType(SCALE_OUT10);                 \
        ++dx.x;                                                     \
        da.set(cf(da(dx), out10), dx);                              \
    }                                                               \
    if ((dst_h & 1) == 0) {                                         \
        out01 = sc0a[srcx];                                         \
        out01 /= SKIPSMImagePixelType(SCALE_OUT01);                 \
        da.set(cf(da(dxx), out01), dxx);                            \
        if ((dst_w & 1) == 0) {                                     \
            out11 = sc0b[srcx];                                     \
            out11 /= SKIPSMImagePixelType(SCALE_OUT11);             \
            ++dxx.x;                                                \
            da.set(cf(da(dxx), out11), dxx);                        \
        }                                                           \
    }

/** The Burt & Adelson Expand operation.
 *
 *  Upsampling with Gaussian interpolation in one pass over the input image using SKIPSM-based algorithm.
 *  Uses only integer math, visits each pixel only once.
 *
 *  Explanation of algorithm:
 *
 *  src image pixels:   a     b     c      dst image pixels:   A  B  C  D  E
 *                                                             F  G  H  I  J
 *                      d     e     f                          K  L  M  N  O
 *                                                             P  Q  R  S  T
 *                      g     h     i                          U  V  W  X  Y
 *
 *  Algorithm visits all src image pixels from left to right and top to bottom.
 *  At each src pixel, four dst pixels are calculated.
 *  When visiting src pixel i, dst pixels M, N, R and S are written.
 *
 *  State variables before visiting i:
 *  sr0 = h
 *  sr1 = g
 *  sc0a[2] = d + 6e + f
 *  sc0b[2] = 4e + 4f
 *  sc1a[2] = a + 6b + c
 *  sc1b[2] = 4b + 4c
 *
 *  State variables after visiting i:
 *  sr0 = i
 *  sr1 = h
 *  sc0a[2] = g + 6h + i
 *  sc0b[2] = 4h + 4i
 *  sc1a[2] = d + 6e + f
 *  sc1b[2] = 4e + 4f
 *
 *  M =   1 * (a + 6b + c)
 *      + 6 * (d + 6e + f)
 *      + 1 * (g + 6h + i)
 *
 *  N =   1 * (4b + 4c)
 *      + 6 * (4e + 4f)
 *      + 1 * (4h + 4i)
 *
 *  R =   4 * (d + 6e + f)
 *      + 4 * (g + 6h + i)
 *
 *  S =   4 * (4e + 4f)
 *      + 4 * (4h + 4i)
 *
 *  Updates when visiting each src image pixel:
 *  (all assignments occur in parallel)
 *  sr0 <= current
 *  sr1 <= sr0
 *  sc0a[x] <= sr1 + 6*sr0 + current
 *  sc0b[x] <= 4*sr0 + 4*current
 *  sc1a[x] <= sc0a[x]
 *  sc1b[x] <= sc0b[x]
 *  out(-2, -2) <= sc1a[x] + 6*sc0a[x] + (new sc0a[x])
 *  out(-1, -2) <= sc1b[x] + 6*sc0b[x] + (new sc0b[x])
 *  out(-2, -1) <= 4*sc0a[x] + 4*(new sc0a[x])
 *  out(-1, -1) <= 4*sc0b[x] + 4*(new sc0b[x])
 *
 */
template <typename SKIPSMImagePixelType,
        typename SrcImageIterator, typename SrcAccessor,
        typename DestImageIterator, typename DestAccessor,
        typename CombineFunctor>
void expand(bool add, bool wraparound,
        SrcImageIterator src_upperleft,
        SrcImageIterator src_lowerright,
        SrcAccessor sa,
        DestImageIterator dest_upperleft,
        DestImageIterator dest_lowerright,
        DestAccessor da,
        CombineFunctor cf) {

    int src_w = src_lowerright.x - src_upperleft.x;
    int src_h = src_lowerright.y - src_upperleft.y;
    int dst_w = dest_lowerright.x - dest_upperleft.x;
    int dst_h = dest_lowerright.y - dest_upperleft.y;

    // SKIPSM state variables
    SKIPSMImagePixelType current;
    SKIPSMImagePixelType out00, out10, out01, out11;
    SKIPSMImagePixelType sr0, sr1;
    SKIPSMImagePixelType *sc0a = new SKIPSMImagePixelType[src_w + 1];
    SKIPSMImagePixelType *sc0b = new SKIPSMImagePixelType[src_w + 1];
    SKIPSMImagePixelType *sc1a = new SKIPSMImagePixelType[src_w + 1];
    SKIPSMImagePixelType *sc1b = new SKIPSMImagePixelType[src_w + 1];

    // Convenient constants
    const SKIPSMImagePixelType SKIPSMImageZero(NumericTraits<SKIPSMImagePixelType>::zero());

    DestImageIterator dy = dest_upperleft;
    DestImageIterator dyy = dest_upperleft;
    DestImageIterator dx = dy;
    DestImageIterator dxx = dyy;
    SrcImageIterator sy = src_upperleft;
    SrcImageIterator sx = sy;

    int srcy = 0;
    int srcx = 0;
    //int dsty = 0;
    //int dstx = 0;

    // First row
    {
        if (wraparound) {
            sr0 = SKIPSMImagePixelType(sa(sy, Diff2D(src_w-1,0)));
            sr1 = SKIPSMImagePixelType(sa(sy, Diff2D(src_w-2,0)));
        } else {
            sr0 = SKIPSMImageZero;
            sr1 = SKIPSMImageZero;
        }

        for (sx = sy, srcx = 0; srcx < src_w; ++srcx, ++sx.x) {
            current = SKIPSMImagePixelType(sa(sx));
            sc0a[srcx] = sr1 + IMUL6(sr0) + current;
            sc0b[srcx] = (sr0 + current) * 4;
            sc1a[srcx] = SKIPSMImageZero;
            sc1b[srcx] = SKIPSMImageZero;
            sr1 = sr0;
            sr0 = current;
        }

        // extra column at end of first row
        if (wraparound) {
            sc0a[srcx] = sr1 + IMUL6(sr0) + SKIPSMImagePixelType(sa(sy));
            sc0b[srcx] = (sr0 + SKIPSMImagePixelType(sa(sy))) * 4;
        } else {
            sc0a[srcx] = sr1 + IMUL6(sr0);
            sc0b[srcx] = sr0 * 4;
        }
        sc1a[srcx] = SKIPSMImageZero;
        sc1b[srcx] = SKIPSMImageZero;
    }

    // dy  = row 0
    // dyy = row 1
    ++dyy.y;
    // sy = row 1
    srcy = 1;
    ++sy.y;

    // Second row
    if (src_h > 1) {
        // First column
        srcx = 0;
        sx = sy;
        sr0 = SKIPSMImagePixelType(sa(sx));
        if (wraparound) {
            sr1 = SKIPSMImagePixelType(sa(sy, Diff2D(src_w-1,0)));
        } else {
            sr1 = SKIPSMImageZero;
        }
        // sc*[0] are irrelevant

        srcx = 1;
        ++sx.x;
        dx = dy;
        dxx = dyy;

        // Second column
        if (src_w > 1) {
            if (wraparound) {
                SKIPSM_EXPAND(56, 56, 16, 16)
            } else {
                SKIPSM_EXPAND(49, 56, 14, 16)
            }

            // Main columns
            for (srcx = 2, ++sx.x; srcx < src_w; ++srcx, ++sx.x) {
                SKIPSM_EXPAND(56, 56, 16, 16)
            }

            // extra column at end of second row
            if (wraparound) {
                SKIPSM_EXPAND_COLUMN_END_WRAPAROUND(56, 56, 16, 16)
            } else {
                SKIPSM_EXPAND_COLUMN_END(49, 28, 14, 8)
            }
        }
        else {
            // Math works out exactly the same for wraparound and no wraparound when src_w ==1
            SKIPSM_EXPAND_COLUMN_END(42, 28, 12, 8)
        }

    }
    else {
        // No Second Row
        // First Column
        srcx = 0;
        sr0 = SKIPSMImageZero;
        sr1 = SKIPSMImageZero;

        dx = dy;
        dxx = dyy;

        if (src_w > 1) {
            // Second Column
            srcx = 1;
            if (wraparound) {
                SKIPSM_EXPAND_ROW_END(48, 48, 8, 8)
            } else {
                SKIPSM_EXPAND_ROW_END(42, 48, 7, 8)
            }

            // Main columns
            for (srcx = 2; srcx < src_w; ++srcx) {
                SKIPSM_EXPAND_ROW_END(48, 48, 8, 8)
            }

            // extra column at end of row
            if (wraparound) {
                SKIPSM_EXPAND_ROW_COLUMN_END(48, 48, 8, 8)
            } else {
                SKIPSM_EXPAND_ROW_COLUMN_END(42, 24, 7, 4)
            }
        }
        else {
            // No Second Column
            // dst_w, dst_h must be at least 2
            SKIPSM_EXPAND_ROW_COLUMN_END(36, 24, 6, 4)
        }

        delete[] sc0a;
        delete[] sc0b;
        delete[] sc1a;
        delete[] sc1b;

        return;
    }

    // dy = row 2
    // dyy = row 3
    dy.y += 2;
    dyy.y += 2;
    // sy = row 2
    srcy = 2;
    ++sy.y;

    // Main Rows
    for (srcy = 2, sx = sy; srcy < src_h; ++srcy, ++sy.y, dy.y += 2, dyy.y += 2) {
        // First column
        srcx = 0;
        sx = sy;
        sr0 = SKIPSMImagePixelType(sa(sx));
        if (wraparound) {
            sr1 = SKIPSMImagePixelType(sa(sy, Diff2D(src_w-1,0)));
        } else {
            sr1 = SKIPSMImageZero;
        }
        // sc*[0] are irrelvant

        srcx = 1;
        ++sx.x;
        dx = dy;
        dxx = dyy;

        // Second column
        if (src_w > 1) {
            if (wraparound) {
                SKIPSM_EXPAND_SHIFT
            } else {
                SKIPSM_EXPAND(56, 64, 14, 16)
            }

            // Main columns
            for (srcx = 2, ++sx.x; srcx < src_w; ++srcx, ++sx.x) {
                //SKIPSM_EXPAND(64, 64, 16, 16)
                SKIPSM_EXPAND_SHIFT
            }

            // extra column at end of row
            if (wraparound) {
                SKIPSM_EXPAND_COLUMN_END_WRAPAROUND(64, 64, 16, 16)
            } else {
                SKIPSM_EXPAND_COLUMN_END(56, 32, 14, 8)
            }
        }
        else {
            // No second column
            // dst_w must be at least 2
            // Math works out exactly the same for wraparound and no wraparound when src_w ==1
            SKIPSM_EXPAND_COLUMN_END(48, 32, 12, 8)
        }
    }

    // Extra row at end
    {
        srcx = 0;
        sr0 = SKIPSMImageZero;
        sr1 = SKIPSMImageZero;

        dx = dy;
        dxx = dyy;

        if (src_w > 1) {
            // Second Column
            srcx = 1;
            if (wraparound) {
                SKIPSM_EXPAND_ROW_END(56, 56, 8, 8)
            } else {
                SKIPSM_EXPAND_ROW_END(49, 56, 7, 8)
            }

            // Main columns
            for (srcx = 2; srcx < src_w; ++srcx) {
                SKIPSM_EXPAND_ROW_END(56, 56, 8, 8)
            }

            // extra column at end of row
            if (wraparound) {
                SKIPSM_EXPAND_ROW_COLUMN_END(56, 56, 8, 8)
            } else {
                SKIPSM_EXPAND_ROW_COLUMN_END(49, 28, 7, 4)
            }
        }
        else {
            // No Second Column
            // dst_w, dst_h must be at least 2
            SKIPSM_EXPAND_ROW_COLUMN_END(42, 28, 6, 4)
        }
    }

    delete[] sc0a;
    delete[] sc0b;
    delete[] sc1a;
    delete[] sc1b;

};

// Functor that adds two values and de-promotes the result.
// Used when collapsing a laplacian pyramid.
// Explict fromPromote necessary to avoid overflow/underflow problems.
template<typename T1, typename T2, typename T3>
struct FromPromotePlusFunctorWrapper : public std::binary_function<T1, T2, T3> {
    inline T3 operator()(const T1 &a, const T2 &b) const {
        return NumericTraits<T3>::fromPromote(a + b);
    }
};

// Version using argument object factories.
template <typename SKIPSMImagePixelType,
        typename SrcImageIterator, typename SrcAccessor,
        typename DestImageIterator, typename DestAccessor>
inline void expand(bool add, bool wraparound,
        triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
        triple<DestImageIterator, DestImageIterator, DestAccessor> dest) {

    typedef typename DestAccessor::value_type DestPixelType;

    if (add) {
        expand<SKIPSMImagePixelType>(add, wraparound,
                src.first, src.second, src.third,
                dest.first, dest.second, dest.third,
                FromPromotePlusFunctorWrapper<DestPixelType, SKIPSMImagePixelType, DestPixelType>());
    }
    else {
        expand<SKIPSMImagePixelType>(add, wraparound,
                src.first, src.second, src.third,
                dest.first, dest.second, dest.third,
                std::minus<SKIPSMImagePixelType>());
    }

};

#if 0
// doesn't compile

/** Calculate the Gaussian pyramid for the given SrcImage/AlphaImage pair. */
template <typename SrcImageType, typename AlphaImageType, typename PyramidImageType,
          int PyramidIntegerBits, int PyramidFractionBits,
          typename SKIPSMImagePixelType, typename SKIPSMAlphaPixelType>
vector<PyramidImageType*> *gaussianPyramid(unsigned int numLevels,
        bool wraparound,
        typename SrcImageType::const_traverser src_upperleft,
        typename SrcImageType::const_traverser src_lowerright,
        typename SrcImageType::ConstAccessor sa,
        typename AlphaImageType::const_traverser alpha_upperleft,
        typename AlphaImageType::ConstAccessor aa) {

    vector<PyramidImageType*> *gp = new vector<PyramidImageType*>();

    // Size of pyramid level 0
    int w = src_lowerright.x - src_upperleft.x;
    int h = src_lowerright.y - src_upperleft.y;

    // Pyramid level 0
    PyramidImageType *gp0 = new PyramidImageType(w, h);

    // Copy src image into gp0, using fixed-point conversions.
    copyToPyramidImage<SrcImageType, PyramidImageType, PyramidIntegerBits, PyramidFractionBits>(
            src_upperleft, src_lowerright, sa, gp0->upperLeft(), gp0->accessor());

    gp->push_back(gp0);

    if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
        cout << "Generating Gaussian pyramid:  g0";
    }

    // Make remaining levels.
    PyramidImageType *lastGP = gp0;
    AlphaImageType *lastA = NULL;
    for (unsigned int l = 1; l < numLevels; l++) {

        if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
            cout << " g" << l;
            cout.flush();
        }

        // Size of next level
        w = (w + 1) >> 1;
        h = (h + 1) >> 1;

        // Next pyramid level
        PyramidImageType *gpn = new PyramidImageType(w, h);
        AlphaImageType *nextA = new AlphaImageType(w, h);

        if (lastA == NULL) {
            reduce<SKIPSMImagePixelType, SKIPSMAlphaPixelType>(wraparound,
                    srcImageRange(*lastGP), maskIter(alpha_upperleft, aa),
                    destImageRange(*gpn), destImageRange(*nextA));
        } else {
            reduce<SKIPSMImagePixelType, SKIPSMAlphaPixelType>(wraparound,
                    srcImageRange(*lastGP), maskImage(*lastA),
                    destImageRange(*gpn), destImageRange(*nextA));
        }

        gp->push_back(gpn);
        lastGP = gpn;
        delete lastA;
        lastA = nextA;
    }

    delete lastA;

    if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
        cout << endl;
    }

    return gp;

};

// Version using argument object factories.
template <typename SrcImageType, typename AlphaImageType, typename PyramidImageType,
          int PyramidIntegerBits, int PyramidFractionBits,
          typename SKIPSMImagePixelType, typename SKIPSMAlphaPixelType>
inline vector<PyramidImageType*> *gaussianPyramid(unsigned int numLevels,
        bool wraparound,
        triple<typename SrcImageType::const_traverser, typename SrcImageType::const_traverser, typename SrcImageType::ConstAccessor> src,
        pair<typename AlphaImageType::const_traverser, typename AlphaImageType::ConstAccessor> alpha) {
    return gaussianPyramid<SrcImageType, AlphaImageType, PyramidImageType,
            PyramidIntegerBits, PyramidFractionBits,
            SKIPSMImagePixelType, SKIPSMAlphaPixelType>(
            numLevels, wraparound,
            src.first, src.second, src.third,
            alpha.first, alpha.second);
};

/** Calculate the Gaussian pyramid for the given image (without an alpha channel). */
template <typename SrcImageType, typename PyramidImageType,
          int PyramidIntegerBits, int PyramidFractionBits,
          typename SKIPSMImagePixelType>
vector<PyramidImageType*> *gaussianPyramid(unsigned int numLevels,
        bool wraparound,
        typename SrcImageType::const_traverser src_upperleft,
        typename SrcImageType::const_traverser src_lowerright,
        typename SrcImageType::ConstAccessor sa) {

    vector<PyramidImageType*> *gp = new vector<PyramidImageType*>();

    // Size of pyramid level 0
    int w = src_lowerright.x - src_upperleft.x;
    int h = src_lowerright.y - src_upperleft.y;

    // Pyramid level 0
    PyramidImageType *gp0 = new PyramidImageType(w, h);

    // Copy src image into gp0, using fixed-point conversions.
    copyToPyramidImage<SrcImageType, PyramidImageType, PyramidIntegerBits, PyramidFractionBits>(
            src_upperleft, src_lowerright, sa,
            gp0->upperLeft(), gp0->accessor());

    gp->push_back(gp0);

    if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
        cout << "Generating Gaussian pyramid:  g0";
    }

    // Make remaining levels.
    PyramidImageType *lastGP = gp0;
    for (unsigned int l = 1; l < numLevels; l++) {

        if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
            cout << " g" << l;
            cout.flush();
        }

        // Size of next level
        w = (w + 1) >> 1;
        h = (h + 1) >> 1;

        // Next pyramid level
        PyramidImageType *gpn = new PyramidImageType(w, h);

        reduce<SKIPSMImagePixelType>(wraparound, srcImageRange(*lastGP), destImageRange(*gpn));

        gp->push_back(gpn);
        lastGP = gpn;
    }

    if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
        cout << endl;
    }

    return gp;
};

// Version using argument object factories.
template <typename SrcImageType, typename PyramidImageType,
          int PyramidIntegerBits, int PyramidFractionBits,
          typename SKIPSMImagePixelType>
inline vector<PyramidImageType*> *gaussianPyramid(unsigned int numLevels,
        bool wraparound,
        triple<typename SrcImageType::const_traverser, typename SrcImageType::const_traverser, typename SrcImageType::ConstAccessor> src) {
    return gaussianPyramid<SrcImageType, PyramidImageType,
            PyramidIntegerBits, PyramidFractionBits, SKIPSMImagePixelType>(
            numLevels,
            wraparound,
            src.first, src.second, src.third);
};

/** Calculate the Laplacian pyramid of the given SrcImage/AlphaImage pair. */
template <typename SrcImageType, typename AlphaImageType, typename PyramidImageType,
          int PyramidIntegerBits, int PyramidFractionBits,
          typename SKIPSMImagePixelType, typename SKIPSMAlphaPixelType>
vector<PyramidImageType*> *laplacianPyramid(const char* exportName, unsigned int numLevels,
        bool wraparound,
        typename SrcImageType::const_traverser src_upperleft,
        typename SrcImageType::const_traverser src_lowerright,
        typename SrcImageType::ConstAccessor sa,
        typename AlphaImageType::const_traverser alpha_upperleft,
        typename AlphaImageType::ConstAccessor aa) {

    // First create a Gaussian pyramid.
    vector <PyramidImageType*> *gp =
            gaussianPyramid<SrcImageType, AlphaImageType, PyramidImageType,
                    PyramidIntegerBits, PyramidFractionBits,
                    SKIPSMImagePixelType, SKIPSMAlphaPixelType>(
                    numLevels, wraparound,
                    src_upperleft, src_lowerright, sa,
                    alpha_upperleft, aa);

    //exportPyramid(gp, exportName);

    if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
        cout << "Generating Laplacian pyramid:";
        cout.flush();
    }

    // For each level, subtract the expansion of the next level.
    // Stop if there is no next level.
    for (unsigned int l = 0; l < (numLevels-1); l++) {

        if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
            cout << " l" << l;
            cout.flush();
        }

        expand<SKIPSMImagePixelType>(false, wraparound,
                srcImageRange(*((*gp)[l+1])),
                destImageRange(*((*gp)[l])));
    }

    if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
        cout << " l" << (numLevels-1) << endl;
    }

    //exportPyramid(gp, exportName);

    return gp;
};

// Version using argument object factories.
template <typename SrcImageType, typename AlphaImageType, typename PyramidImageType,
          int PyramidIntegerBits, int PyramidFractionBits,
          typename SKIPSMImagePixelType, typename SKIPSMAlphaPixelType>
inline vector<PyramidImageType*> *laplacianPyramid(const char* exportName, unsigned int numLevels,
        bool wraparound,
        triple<typename SrcImageType::const_traverser, typename SrcImageType::const_traverser, typename SrcImageType::ConstAccessor> src,
        pair<typename AlphaImageType::const_traverser, typename AlphaImageType::ConstAccessor> alpha) {
    return laplacianPyramid<SrcImageType, AlphaImageType, PyramidImageType,
            PyramidIntegerBits, PyramidFractionBits,
            SKIPSMImagePixelType, SKIPSMAlphaPixelType>(
            exportName,
            numLevels, wraparound,
            src.first, src.second, src.third,
            alpha.first, alpha.second);
};

/** Collapse the given Laplacian pyramid. */
template <typename SKIPSMImagePixelType, typename PyramidImageType>
void collapsePyramid(bool wraparound, vector<PyramidImageType*> *p) {

    if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
        cout << "Collapsing Laplacian pyramid: "
             << "l" << p->size()-1;
        cout.flush();
    }

    // For each level, add the expansion of the next level.
    // Work backwards from the smallest level to the largest.
    for (int l = (p->size()-2); l >= 0; l--) {

        if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
            cout << " l" << l;
            cout.flush();
        }

        expand<SKIPSMImagePixelType>(true, wraparound,
                srcImageRange(*((*p)[l+1])),
                destImageRange(*((*p)[l])));
    }

    if (Verbose > VERBOSE_PYRAMID_MESSAGES) {
        cout << endl;
    }

};

// Export a scalar pyramid as a set of UINT16 tiff files.
template <typename PyramidImageType>
void exportPyramid(vector<PyramidImageType*> *v, const char *prefix, VigraTrueType) {
    typedef typename PyramidImageType::value_type PyramidValueType;

    //for (unsigned int i = 0; i < (v->size() - 1); i++) {
    //    // Clear all levels except last.
    //    initImage(destImageRange(*((*v)[i])), NumericTraits<PyramidValueType>::zero());
    //}
    //collapsePyramid(false, v);

    for (unsigned int i = 0; i < v->size(); i++) {
        char filenameBuf[512];
        snprintf(filenameBuf, 512, "%s%04u.tif", prefix, i);

        // Rescale the pyramid values to fit in UINT16.
        UInt16Image usPyramid((*v)[i]->width(), (*v)[i]->height());
        transformImage(srcImageRange(*((*v)[i])), destImage(usPyramid),
                linearRangeMapping(NumericTraits<PyramidValueType>::min(),
                                     NumericTraits<PyramidValueType>::max(),
                                     NumericTraits<UInt16>::min(),
                                     NumericTraits<UInt16>::max()));

        ImageExportInfo info(filenameBuf);
        exportImage(srcImageRange(usPyramid), info);
    }
};

// Export a vector pyramid as a set of UINT16 tiff files.
template <typename PyramidImageType>
void exportPyramid(vector<PyramidImageType*> *v, const char *prefix, VigraFalseType) {
    typedef typename PyramidImageType::value_type PyramidVectorType;
    typedef typename PyramidVectorType::value_type PyramidValueType;

    for (unsigned int i = 0; i < (v->size() - 1); i++) {
        // Clear all levels except last.
        initImage(destImageRange(*((*v)[i])), NumericTraits<PyramidValueType>::zero());
    }
    collapsePyramid(false, v);

    for (unsigned int i = 0; i < v->size(); i++) {
        char filenameBuf[512];
        snprintf(filenameBuf, 512, "%s%04u.tif", prefix, i);

        // Rescale the pyramid values to fit in UINT16.
        UInt16RGBImage usPyramid((*v)[i]->width(), (*v)[i]->height());
        transformImage(srcImageRange(*((*v)[i])), destImage(usPyramid),
                linearRangeMapping(PyramidVectorType(NumericTraits<PyramidValueType>::min()),
                                   PyramidVectorType(NumericTraits<PyramidValueType>::max()),
                                   typename UInt16RGBImage::value_type(NumericTraits<UInt16>::min()),
                                   typename UInt16RGBImage::value_type(NumericTraits<UInt16>::max())));

        ImageExportInfo info(filenameBuf);
        exportImage(srcImageRange(usPyramid), info);
    }
};

// Export a pyramid as a set of UINT16 tiff files.
template <typename PyramidImageType>
void exportPyramid(vector<PyramidImageType*> *v, const char *prefix) {
    typedef typename NumericTraits<typename PyramidImageType::value_type>::isScalar pyramid_is_scalar;
    exportPyramid(v, prefix, pyramid_is_scalar());
};
#endif
} // namespace enblend

#endif /* __PYRAMID_H__ */
