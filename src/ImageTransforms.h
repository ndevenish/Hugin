// -*- c-basic-offset: 4 -*-
/** @file ImageTransforms.h
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

#include <qimage.h>
#include <qobject.h>
#include <qapplication.h>

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "utils.h"

namespace IMG {

/// convert a QImage to greyscale
void ImageToGrey(QImage & src, QImage & dst);
void ImageToHue(QImage & src, QImage & dst);

double CalculateMean(const QImage & img, int x1, int y1, int x2, int y2, int stepwidth = 1);


void SetGreyPalette(QImage & src);
void SetPalette(QImage &dst, QRgb * pal);

// palettes:
extern QRgb IMG::CM_JET[];

/*
class Transform
{
public:
    virtual ~Transform() {};
    virtual double process(int x, int y, const QImage & src, int stepwidth=1) = 0;
};
*/

// notifier class, because qt cannot use templates
class TransformerBase : public QObject
{
    Q_OBJECT
signals:
    void progress(int);
};


template<class TRANS>
class Transformer : public TransformerBase
{
public:
    Transformer()
        { };
    virtual ~Transformer();
    QPoint transform(const QImage & src, QImage & dst, TRANS & algo);
    QPoint searchMax(const QImage &src, const QRect & region, TRANS & algo);
    QPoint pyramidSearchMax(const QImage & src, const QRect & rect, QImage & dest, TRANS & algo, int stepwidth=0, uchar cutoff = 0);
    QPoint pyramidSearchMin(const QImage & src, const QRect & rect, QImage & dest, TRANS & algo, int stepwidth=0);

};


class Invert
{
public:
    double process(int x, int y, const QImage & src)
        {
            return ~ src.pixel(x,y);
        }
};


class Convolution
{
public:
    Convolution(double *kernel, int width, int height)
        : kernel(kernel), width(width), height(height)
        {
            xborder = width/2;
            yborder = height/2;
        }
    double process(int Sx, int Sy, const QImage & src, int)
        {
            // ignore border region
            if (Sx < xborder) return 0;
            if (Sx >= src.width() - xborder) return 0;
            if (Sy < yborder) return 0;
            if (Sy >= src.height() - yborder) return 0;

            double sum = 0;
            uchar ** lines = src.jumpTable();
            for(int y=0; y < height; y++) {
                for(int x=0; x < width; x++) {
                    sum = lines[Sy+y-yborder][Sx+x-xborder] * kernel[y*width +x];
                }
            }
            return sum/6 + 127;
        }
private:
    double * kernel;
    int xborder;
    int yborder;
    int width, height;
};

class MeanCorrCoeff
{
public:
    MeanCorrCoeff(const QImage & templ)
        : templ(templ)
        {
            assert(templ.depth() == 8);
            width = templ.width();
            height = templ.height();
            xborder = width/2;
            yborder = height/2;
            tlines = templ.jumpTable();
            templMean = CalculateMean(templ,0,0, width, height);
            DEBUG_DEBUG("template mean: " << templMean);
        };

    inline double process(int Sx, int Sy, const QImage & src, int step=1)
        {
            // ignore border region
            if (Sx < xborder) return 0;
            if (Sx >= src.width() - xborder) return 0;
            if (Sy < yborder) return 0;
            if (Sy >= src.height() - yborder) return 0;

            double numerator = 0;
            double divisor1 = 0;
            double divisor2 = 0;

            double uI=0;
            uchar ** lines = src.jumpTable();
            uI = IMG::CalculateMean(src, Sx- xborder, Sy - yborder, Sx + xborder, Sy + yborder);
            for(int y=0; y < height; y += step) {
                for(int x=0; x < width; x += step) {
                    double I = lines[Sy+y-yborder][Sx+x-xborder] - uI;
                    double T = tlines[y][x] - templMean;
                    // hope we do not make rounding errors here..
                    numerator +=  T*I;
                    divisor1 += I*I;
                    divisor2 += T*T;
                }
            }
            double ret = (numerator/sqrt(divisor1 * divisor2)) * 127 + 127;
            return ret;
        }

private:
    uchar ** tlines;
    int xborder;
    int yborder;
    int width;
    int height;
    QImage templ;
    double templMean;

};

class CorrCoeff
{
public:
    CorrCoeff(const QImage & templ)
        : templ(templ)
        {
            assert(templ.depth() == 8);
            width = templ.width();
            height = templ.height();
            xborder = width/2;
            yborder = height/2;
            tlines = templ.jumpTable();
        };

    inline double process(int Sx, int Sy, const QImage & src, int step=0)
        {

            // ignore border region
            if (Sx < xborder) return 0;
            if (Sx >= src.width() - xborder) return 0;
            if (Sy < yborder) return 0;
            if (Sy >= src.height() - yborder) return 0;

            double numerator = 0;
            double divisor1 = 0;
            double divisor2 = 0;
            uchar ** lines = src.jumpTable();
            for(int y=0; y < height; y += step) {
                for(int x=0; x < width; x += step) {
                    double I = lines[Sy+y-yborder][Sx+x-xborder];
                    double T = tlines[y][x];
                    // hope we do not make rounding errors here..
                    numerator +=  T*I;
                    divisor1 += I*I;
                    divisor2 += T*T;
                }
            }
            return (numerator/sqrt(divisor1 * divisor2)) * 127 + 127;
        }

private:
    uchar ** tlines;
    int xborder;
    int yborder;
    int width;
    int height;
    QImage templ;
};


class CrossCorr
{
public:
    CrossCorr(const QImage & templ, double norm)
        : templ(templ), norm(norm)
        {
            assert(templ.depth() == 8);
            width = templ.width();
            height = templ.height();
            xborder = width/2;
            yborder = height/2;
            tlines = templ.jumpTable();
        };

    inline double process(int Sx, int Sy, const QImage & src, int step = 1)
        {

            // ignore border region
            if (Sx < xborder) return 0;
            if (Sx >= src.width() - xborder) return 0;
            if (Sy < yborder) return 0;
            if (Sy >= src.height() - yborder) return 0;

            double numerator = 0;
            double divisor = 0;
            uchar ** lines = src.jumpTable();
            for(int y=0; y < height; y += step) {
                for(int x=0; x < width; x += step) {
                    uchar I = lines[Sy+y-yborder][Sx+x-xborder];
                    uchar T = tlines[y][x];
                    // hope we do not make rounding errors here..
                    numerator +=  T*I;
                    divisor += I*I;
                }
            }
            return (numerator/sqrt(divisor)) / norm;
        }

private:
    uchar ** tlines;
    int xborder;
    int yborder;
    int width;
    int height;
    QImage templ;
    double norm;
};


class AbsDifference
{
public:
    AbsDifference(const QImage & templ, long norm=0)
      : templ(templ), norm(norm)
        {
            assert(templ.depth() == 8);
            width = templ.width();
            height = templ.height();
            xborder = width/2;
            yborder = height/2;
            tlines = templ.jumpTable();
            if (norm == 0) norm = width * height * 256;
        };

  inline double process(int Sx, int Sy, const QImage & src, int step = 1)
    {
        // ignore border region
        if (Sx < xborder) return width*height * 255;
        if (Sx >= src.width() - xborder) return width*height*255;
        if (Sy < yborder) return width*height * 255;
        if (Sy >= src.height() - yborder) return width*height * 255;

        double diff = 0;
        uchar ** lines = src.jumpTable();
/*
        double uT = 0;
        double uI = 0;
        for(int y=0; y < height; y += step) {
            for(int x=0; x < width; x += step) {
                uT += tlines[y][x];
                uI += lines[Sy+y-yborder][Sx+x-xborder];
            }
        }
        uT = uT / (width * height);
        uI = uI / (width * height);
*/
        for(int y=0; y < height; y += step) {
            for(int x=0; x < width; x += step) {
                diff += fabs((int)tlines[y][x] - (int)lines[Sy+y-yborder][Sx+x-xborder]);
            }
        }
        // need to normalize difference here.
        return diff;
    }

private:
    uchar ** tlines;
    int xborder;
    int yborder;
    int width;
    int height;
    QImage templ;
    long norm;
};

class LensCorr
{
public:
    LensCorr(double a, double b, double c, int x, int y)
        : a(a), b(b), c(c), Mx(x), My(y)
        {
            d = 1-a-b-c;
        }

    // formula: rsrc = ( a * rdest^3 + b * rdest^2 + c * rdest + d ) * rdest
    inline double process(int x, int y, const QImage & src)
        {
            assert(src.depth() == 8);
            double dx = x - Mx;
            double dy = y - My;
            double r_dest = sqrt((dx*dx) + (dy*dy));
            double alpha_dest = atan2(dy,dx);
            double r_src = (a * r_dest * r_dest * r_dest +
                            b * r_dest * r_dest + c * r_dest +d) * r_dest;
            // resample pic here. this is an ugly nearest neighbour interpolation
            y = My + (int)(sin(alpha_dest) * r_src + 0.5);
            x = Mx + (int)(cos(alpha_dest) * r_src + 0.5);
            if (y < 0 || y > src.height()) return 0;
            if (x < 0 || x > src.width()) return 0;
            return (src.scanLine(y)[x]);
        }
private:
    double a,b,c,d;
    int Mx, My;
};



//=============================================================
// template implementation


template<class TRANS>
Transformer<TRANS>::~Transformer()
{

};

template<class TRANS>
QPoint Transformer<TRANS>::transform(const QImage &src, QImage & dst, TRANS & algo)
{
    assert(src.depth() == 8);
    assert(dst.depth() == 8);

  double res;
  double min = DBL_MAX;
  double max = DBL_MIN;
  int xmin,ymin;
  int xmax,ymax;
  int width = dst.width();
  int height = dst.height();
  uchar ** img = dst.jumpTable();
  for (int x=0; x < width; x++) {
    emit (progress(x*100/width));
    qApp->processEvents();
    for (int y=0; y < height; y++) {
      res = algo.process(x,y,src);
      uchar cres;
      if (res <0 ) {
          DEBUG_WARN("Convolution: UNDERFLOW: " << res);
          cres = 0;
      } else if( res > 255) {
          DEBUG_WARN("Convolution: OVERFLOW: " << res);
          cres = 255;
      } else {
          cres = (uchar)res;
      }
      img[y][x] = cres;
      if (res < min) {
        min = res;
        xmin = x;
        ymin = y;
      }
      if (res > max) {
        max = res;
        xmax = x;
        ymax = y;
      }
    }
  }
  DEBUG_INFO("Transform: min: " << min << "(" << xmin << "," << ymin << ")"
             << " max: " << max << "(" << xmax << "," << ymax << ")");
  return QPoint(xmax, ymax);
}


template<class TRANS>
QPoint Transformer<TRANS>::searchMax(const QImage &src, const QRect & region, TRANS & algo)
{
    assert(src.depth() == 8);
    double res;
    double min = DBL_MAX;
    double max = DBL_MIN;
    int xmin,ymin;
    int xmax,ymax;
    int width = region.width();
    for (int x= region.left(); x < region.right(); x++) {
        emit (progress(x*100/width));
        qApp->processEvents();
        for (int y=region.top(); y < region.bottom(); y++) {
            res = algo.process(x,y,src);
            if (res < min) {
                min = res;
                xmin = x;
                ymin = y;
            }
            if (res > max) {
                max = res;
                xmax = x;
                ymax = y;
            }
        }
    }
    DEBUG_INFO("Transform: min: " << min << "(" << xmin << "," << ymin << ")"
             << " max: " << max << "(" << xmax << "," << ymax << ")");
    return QPoint(xmax,ymax);
}


template<class TRANS>
QPoint Transformer<TRANS>::pyramidSearchMax(const QImage & src, const QRect & region, QImage & dest, TRANS & algo, int stepwidth, uchar cutoff)
{
    assert(dest.depth() == 8);
    assert(src.depth() == 8);
    if (stepwidth == 0) {
        // default to a stepwidth of 4
        stepwidth = 4;
    }
    if (cutoff == 0) {
        cutoff = 160;
    }

    double res;
    double min = DBL_MAX;
    double max = DBL_MIN;
    int xmin = -1;
    int ymin = -1;
    int xmax = -1;
    int ymax = -1;
    int width = region.width();

    QString mfile = QString("pyramid") + QString::number(stepwidth) + ".mat";
    std::ofstream out(mfile.ascii(),std::ios_base::out | std::ios_base::trunc);
    out << "# name: C" << stepwidth << std::endl
        << "# type: matrix" << std::endl
        << "# rows: " << region.width() / stepwidth << std::endl
        << "# columns: " << region.height() / stepwidth << std::endl;
    uchar ** img = dest.jumpTable();
//    for (int x= region.left(); x < region.right(); x++) {
    for (int x= region.left(); x < region.right(); x += stepwidth) {
        emit(progress(x*100/width));
        qApp->processEvents();
//        for (int y=region.top(); y < region.bottom(); y++) {
        for (int y=region.top(); y < region.bottom(); y += stepwidth) {
            // skip pixel if a previous run hasn't marked it with a low
            // correlation coeeffient
            if (img[y][x] < cutoff) {
                res = 0;
            } else {
                res = algo.process(x,y,src, stepwidth);
            }
            out << res << " ";
            uchar cres;
            if (res <0 ) {
                DEBUG_WARN("Convolution: UNDERFLOW: " << res);
                cres = 0;
            } else if( res > 255) {
                DEBUG_WARN("Convolution: OVERFLOW: " << res);
                cres = 255;
            } else {
                cres = (uchar)res;
            }
            // fill dest image
            for (int s1 = 0 ; s1 < stepwidth; s1++){
                for (int s2 = 0 ; s2 < stepwidth; s2++){
                    img[y+s1][x+s2] = cres;
                }
            }
//            img[y][x] = cres;
            if (res < min) {
                min = res;
                xmin = x;
                ymin = y;
            }
            if (res > max) {
                max = res;
                xmax = x;
                ymax = y;
            }
        }
        out << std::endl;
    }
    out.close();
    DEBUG_INFO("transform (stepwidth: " << stepwidth << "): min: " << min << "(" << xmin << "," << ymin << ")"
               << " max: " << max << "(" << xmax << "," << ymax << ")");
    QString file = QString("pyramid") + QString::number(stepwidth) + ".png";
    dest.save(file,"PNG");
    // we should pick all matching regions and process them with a smaller
    // step width
    stepwidth = stepwidth / 2;
//    if (stepwidth != 0) {
//        return pyramidSearchMax(src,region,dest, algo,stepwidth);
//    }
    return QPoint(xmax,ymax);
}


template<class TRANS>
QPoint Transformer<TRANS>::pyramidSearchMin(const QImage & src, const QRect & region, QImage & dest, TRANS & algo, int stepwidth)
{
    assert(dest.depth() == 8);
    assert(src.depth() == 8);
    if (stepwidth == 0) {
        // default to a stepwidth of 4
        stepwidth = 4;
    }

    int width = region.width();
    int height = region.height();
    // maximum difference.
    double my_inf = width * height * 256;
    double res;
    double min = my_inf; // not DBL_MAX, because we need some air down there.
    double max = DBL_MIN;
    int xmin = -1;
    int ymin = -1;
    int xmax = -1;
    int ymax = -1;
    uchar cres = 0;

    QString mfile = QString("pyramid") + QString::number(stepwidth) + ".mat";
    std::ofstream out(mfile.ascii(),std::ios_base::out | std::ios_base::trunc);
    out << "# name: C" << stepwidth << std::endl
        << "# type: matrix" << std::endl
        << "# rows: " << region.width() / stepwidth << std::endl
        << "# columns: " << region.height() / stepwidth << std::endl;
    uchar ** img = dest.jumpTable();
//    for (int x= region.left(); x < region.right(); x++) {
    for (int x= region.left(); x < region.right(); x += stepwidth) {
        emit(progress(x*100/width));
        qApp->processEvents();
//        for (int y=region.top(); y < region.bottom(); y++) {
        for (int y=region.top(); y < region.bottom(); y += stepwidth) {
            // absolute difference
            if (img[y][x] == 0) {
                // very high value, do not search on this differene anymore
                res = my_inf;
            } else {
                res = algo.process(x,y,src, stepwidth);
            }
            out << res << " ";
            // consider all points with 50% more than the current minimum
            // not very nice, because we should take it of the the global
            // minimum, wich we do not know yet. we would need to store it
            // but we do not have a double array here right now.
            if (res < min * 1.5) {
                cres = 127;
            } else {
                cres = 0;
            }
            // fill dest image
            for (int s1 = 0 ; s1 < stepwidth; s1++){
                for (int s2 = 0 ; s2 < stepwidth; s2++){
                    img[y+s1][x+s2] = cres;
                }
            }
            if (res < min) {
                out << "<-  ";
                min = res;
                xmin = x;
                ymin = y;
            }
            if (res > max) {
                max = res;
                xmax = x;
                ymax = y;
            }
        }
        out << std::endl;
    }
    out.close();
    DEBUG_INFO("transform (stepwidth: " << stepwidth << "): min: " << min << "(" << xmin << "," << ymin << ")"
               << " max: " << max << "(" << xmax << "," << ymax << ")");
    QString file = QString("pyramid") + QString::number(stepwidth) + ".png";
    dest.save(file,"PNG");
    // we should pick all matching regions and process them with a smaller
    // step width
    stepwidth = stepwidth / 2;
//    if (stepwidth != 0) {
//        return pyramidSearchMax(src,region,dest, algo,stepwidth);
//    }
    return QPoint(xmin,ymin);
}


}
#endif // _IMAGETRANSFORMS_H
