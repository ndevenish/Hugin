// -*- c-basic-offset: 4 -*-

/** @file ImageTransforms.cpp
 *
 *  @brief implementation of ImageTransforms Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <fstream>

#include <qcolor.h>
#include <float.h>
#include "ImageTransforms.h"


void IMG::ImageToGrey(QImage & src, QImage & dst)
{
    assert(dst.depth() == 8);
    if (src.depth() == 8) {
        dst = src;
        return;
    }
    assert(src.depth() == 32 || src.depth() == 16);
    assert(src.size() == dst.size());
    for (int y=0; y < src.height(); y++) {
        uchar * dstline = dst.scanLine(y);
        for (int x=0; x < src.width(); x++) {
            QRgb rgb = src.pixel(x,y);
            double p = (qRed(rgb) * 0.3 + qGreen(rgb) * 0.59 + qBlue(rgb) * 0.11);
            dstline[x] = (unsigned char) (p<255 ? p : 255);
        }
    }
}


void IMG::ImageToHue(QImage & src, QImage & dst)
{
    assert(dst.depth() == 8);
    if (src.depth() == 8) {
        dst = src;
        return;
    }
    assert(src.depth() == 32 || src.depth() == 16);
    assert(src.size() == dst.size());
    for (int y=0; y < src.height(); y++) {
        uchar * dstline = dst.scanLine(y);
        for (int x=0; x < src.width(); x++) {
            QRgb rgb = src.pixel(x,y);
            double r = qRed(rgb);
            double g = qGreen(rgb);
            double b = qBlue(rgb);
            // complicated formula, from
            // http://vision.doc.ntu.ac.uk/publications/bmvc01/html/node4.html
            double p = acos( (0.5*((r-g) +(r-b)))
                             / sqrt((r-g)*(r-g) + (r-b)*(g-b)) );
            p = p / M_PI * 255;
            dstline[x] = (unsigned char) (p<255 ? p : 255);
        }
    }
}

double IMG::CalculateMean(const QImage & img, int x1, int y1, int x2, int y2, int step)
{
    double mean=0;
    uchar ** lines = img.jumpTable();
    for(int y=y1; y < y2; y += step) {
        for(int x=x1; x < x2; x += step) {
            mean += lines[y][x];
        }
    }
    return mean/((x2-x1)/step * (y2-y1)/step);
}

void IMG::SetGreyPalette(QImage & src)
{
    assert(src.depth() == 8);
    for(int i=0; i < 256; i++) {
        src.setColor(i, qRgb(i,i,i));
    }
}

void IMG::SetPalette(QImage &dst, QRgb * pal)
{
    for(int i=0; i <256 ; i++) {
        dst.setColor(i,pal[i]);
    }
}


// colormaps
//

#define C_QRGB(r,g,b) (0xff << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff)

QRgb IMG::CM_JET[] =
{
    C_QRGB(0,0,131),
    C_QRGB(0,0,135),
    C_QRGB(0,0,139),
    C_QRGB(0,0,143),
    C_QRGB(0,0,147),
    C_QRGB(0,0,151),
    C_QRGB(0,0,155),
    C_QRGB(0,0,159),
    C_QRGB(0,0,163),
    C_QRGB(0,0,167),
    C_QRGB(0,0,171),
    C_QRGB(0,0,175),
    C_QRGB(0,0,179),
    C_QRGB(0,0,183),
    C_QRGB(0,0,187),
    C_QRGB(0,0,191),
    C_QRGB(0,0,195),
    C_QRGB(0,0,199),
    C_QRGB(0,0,203),
    C_QRGB(0,0,207),
    C_QRGB(0,0,211),
    C_QRGB(0,0,215),
    C_QRGB(0,0,219),
    C_QRGB(0,0,223),
    C_QRGB(0,0,227),
    C_QRGB(0,0,231),
    C_QRGB(0,0,235),
    C_QRGB(0,0,239),
    C_QRGB(0,0,243),
    C_QRGB(0,0,247),
    C_QRGB(0,0,251),
    C_QRGB(0,0,255),
    C_QRGB(0,4,255),
    C_QRGB(0,8,255),
    C_QRGB(0,12,255),
    C_QRGB(0,16,255),
    C_QRGB(0,20,255),
    C_QRGB(0,24,255),
    C_QRGB(0,28,255),
    C_QRGB(0,32,255),
    C_QRGB(0,36,255),
    C_QRGB(0,40,255),
    C_QRGB(0,44,255),
    C_QRGB(0,48,255),
    C_QRGB(0,52,255),
    C_QRGB(0,56,255),
    C_QRGB(0,60,255),
    C_QRGB(0,64,255),
    C_QRGB(0,68,255),
    C_QRGB(0,72,255),
    C_QRGB(0,76,255),
    C_QRGB(0,80,255),
    C_QRGB(0,84,255),
    C_QRGB(0,88,255),
    C_QRGB(0,92,255),
    C_QRGB(0,96,255),
    C_QRGB(0,100,255),
    C_QRGB(0,104,255),
    C_QRGB(0,108,255),
    C_QRGB(0,112,255),
    C_QRGB(0,116,255),
    C_QRGB(0,120,255),
    C_QRGB(0,124,255),
    C_QRGB(0,128,255),
    C_QRGB(0,131,255),
    C_QRGB(0,135,255),
    C_QRGB(0,139,255),
    C_QRGB(0,143,255),
    C_QRGB(0,147,255),
    C_QRGB(0,151,255),
    C_QRGB(0,155,255),
    C_QRGB(0,159,255),
    C_QRGB(0,163,255),
    C_QRGB(0,167,255),
    C_QRGB(0,171,255),
    C_QRGB(0,175,255),
    C_QRGB(0,179,255),
    C_QRGB(0,183,255),
    C_QRGB(0,187,255),
    C_QRGB(0,191,255),
    C_QRGB(0,195,255),
    C_QRGB(0,199,255),
    C_QRGB(0,203,255),
    C_QRGB(0,207,255),
    C_QRGB(0,211,255),
    C_QRGB(0,215,255),
    C_QRGB(0,219,255),
    C_QRGB(0,223,255),
    C_QRGB(0,227,255),
    C_QRGB(0,231,255),
    C_QRGB(0,235,255),
    C_QRGB(0,239,255),
    C_QRGB(0,243,255),
    C_QRGB(0,247,255),
    C_QRGB(0,251,255),
    C_QRGB(0,255,255),
    C_QRGB(4,255,255),
    C_QRGB(8,255,251),
    C_QRGB(12,255,247),
    C_QRGB(16,255,243),
    C_QRGB(20,255,239),
    C_QRGB(24,255,235),
    C_QRGB(28,255,231),
    C_QRGB(32,255,227),
    C_QRGB(36,255,223),
    C_QRGB(40,255,219),
    C_QRGB(44,255,215),
    C_QRGB(48,255,211),
    C_QRGB(52,255,207),
    C_QRGB(56,255,203),
    C_QRGB(60,255,199),
    C_QRGB(64,255,195),
    C_QRGB(68,255,191),
    C_QRGB(72,255,187),
    C_QRGB(76,255,183),
    C_QRGB(80,255,179),
    C_QRGB(84,255,175),
    C_QRGB(88,255,171),
    C_QRGB(92,255,167),
    C_QRGB(96,255,163),
    C_QRGB(100,255,159),
    C_QRGB(104,255,155),
    C_QRGB(108,255,151),
    C_QRGB(112,255,147),
    C_QRGB(116,255,143),
    C_QRGB(120,255,139),
    C_QRGB(124,255,135),
    C_QRGB(128,255,131),
    C_QRGB(131,255,128),
    C_QRGB(135,255,124),
    C_QRGB(139,255,120),
    C_QRGB(143,255,116),
    C_QRGB(147,255,112),
    C_QRGB(151,255,108),
    C_QRGB(155,255,104),
    C_QRGB(159,255,100),
    C_QRGB(163,255,96),
    C_QRGB(167,255,92),
    C_QRGB(171,255,88),
    C_QRGB(175,255,84),
    C_QRGB(179,255,80),
    C_QRGB(183,255,76),
    C_QRGB(187,255,72),
    C_QRGB(191,255,68),
    C_QRGB(195,255,64),
    C_QRGB(199,255,60),
    C_QRGB(203,255,56),
    C_QRGB(207,255,52),
    C_QRGB(211,255,48),
    C_QRGB(215,255,44),
    C_QRGB(219,255,40),
    C_QRGB(223,255,36),
    C_QRGB(227,255,32),
    C_QRGB(231,255,28),
    C_QRGB(235,255,24),
    C_QRGB(239,255,20),
    C_QRGB(243,255,16),
    C_QRGB(247,255,12),
    C_QRGB(251,255,8),
    C_QRGB(255,255,4),
    C_QRGB(255,255,0),
    C_QRGB(255,251,0),
    C_QRGB(255,247,0),
    C_QRGB(255,243,0),
    C_QRGB(255,239,0),
    C_QRGB(255,235,0),
    C_QRGB(255,231,0),
    C_QRGB(255,227,0),
    C_QRGB(255,223,0),
    C_QRGB(255,219,0),
    C_QRGB(255,215,0),
    C_QRGB(255,211,0),
    C_QRGB(255,207,0),
    C_QRGB(255,203,0),
    C_QRGB(255,199,0),
    C_QRGB(255,195,0),
    C_QRGB(255,191,0),
    C_QRGB(255,187,0),
    C_QRGB(255,183,0),
    C_QRGB(255,179,0),
    C_QRGB(255,175,0),
    C_QRGB(255,171,0),
    C_QRGB(255,167,0),
    C_QRGB(255,163,0),
    C_QRGB(255,159,0),
    C_QRGB(255,155,0),
    C_QRGB(255,151,0),
    C_QRGB(255,147,0),
    C_QRGB(255,143,0),
    C_QRGB(255,139,0),
    C_QRGB(255,135,0),
    C_QRGB(255,131,0),
    C_QRGB(255,128,0),
    C_QRGB(255,124,0),
    C_QRGB(255,120,0),
    C_QRGB(255,116,0),
    C_QRGB(255,112,0),
    C_QRGB(255,108,0),
    C_QRGB(255,104,0),
    C_QRGB(255,100,0),
    C_QRGB(255,96,0),
    C_QRGB(255,92,0),
    C_QRGB(255,88,0),
    C_QRGB(255,84,0),
    C_QRGB(255,80,0),
    C_QRGB(255,76,0),
    C_QRGB(255,72,0),
    C_QRGB(255,68,0),
    C_QRGB(255,64,0),
    C_QRGB(255,60,0),
    C_QRGB(255,56,0),
    C_QRGB(255,52,0),
    C_QRGB(255,48,0),
    C_QRGB(255,44,0),
    C_QRGB(255,40,0),
    C_QRGB(255,36,0),
    C_QRGB(255,32,0),
    C_QRGB(255,28,0),
    C_QRGB(255,24,0),
    C_QRGB(255,20,0),
    C_QRGB(255,16,0),
    C_QRGB(255,12,0),
    C_QRGB(255,8,0),
    C_QRGB(255,4,0),
    C_QRGB(255,0,0),
    C_QRGB(251,0,0),
    C_QRGB(247,0,0),
    C_QRGB(243,0,0),
    C_QRGB(239,0,0),
    C_QRGB(235,0,0),
    C_QRGB(231,0,0),
    C_QRGB(227,0,0),
    C_QRGB(223,0,0),
    C_QRGB(219,0,0),
    C_QRGB(215,0,0),
    C_QRGB(211,0,0),
    C_QRGB(207,0,0),
    C_QRGB(203,0,0),
    C_QRGB(199,0,0),
    C_QRGB(195,0,0),
    C_QRGB(191,0,0),
    C_QRGB(187,0,0),
    C_QRGB(183,0,0),
    C_QRGB(179,0,0),
    C_QRGB(175,0,0),
    C_QRGB(171,0,0),
    C_QRGB(167,0,0),
    C_QRGB(163,0,0),
    C_QRGB(159,0,0),
    C_QRGB(155,0,0),
    C_QRGB(151,0,0),
    C_QRGB(147,0,0),
    C_QRGB(143,0,0),
    C_QRGB(139,0,0),
    C_QRGB(135,0,0),
    C_QRGB(131,0,0)
};
