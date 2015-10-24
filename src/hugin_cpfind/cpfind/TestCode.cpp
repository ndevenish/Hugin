/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* <http://www.gnu.org/licenses/>.
*/

#include "ImageImport.h"
#include "TestCode.h"
#include <localfeatures/RansacFiltering.h>

// bresenham

static int gen127()
{
    return (int)((double)rand()*127/(double)RAND_MAX);
}


void drawLine(vigra::DRGBImage& img, int x0, int y0, int x1, int y1, vigra::RGBValue<int>& color)
{
    bool steep = (abs(y1 - y0) > abs(x1 - x0));
    if (steep)
    {
        std::swap(x0,y0);
        std::swap(x1,y1);
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax + 1) / 2;
    int ystep;
    int y = y0;

    if (y0 < y1)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for(int x=x0; x<=x1; ++x)
    {
        if (steep)
        {
            img(y,x) = color;
        }
        else
        {
            img(x,y) = color;
        }
        error += deltay;
        if (error >=0)
        {
            y += ystep;
            error -= deltax;
        }
    }
}

void TestCode::drawRansacMatches(std::string& i1, std::string& i2,
                                 lfeat::PointMatchVector_t& iOK,
                                 lfeat::PointMatchVector_t& iNOK,
                                 lfeat::Ransac& iRansac, bool iHalf)
{
    double aDoubleFactor = 1.0;
    if (iHalf)
    {
        aDoubleFactor = 2.0;
    }


    std::cout << "writing file outcomp.png ..." << std::endl;

    // write a side by side image with match pairs and
    vigra::ImageImportInfo info1(i1.c_str());
    vigra::ImageImportInfo info2(i2.c_str());

    vigra::DRGBImage out1(info1.width() * 2, info1.height());

    if ((info1.width() != info2.width()) || (info1.height() != info2.height()))
    {
        std::cout << "images of different size, skip write of test img" << std::endl;
        return;
    }

    if(info1.isGrayscale())
    {
        vigra::DImage aImageGrey(info1.width(), info1.height());
        importImage(info1, destImage(aImageGrey));

        // copy left img
        vigra::copyImage(aImageGrey.upperLeft(),
                         aImageGrey.lowerRight(),
                         vigra::DImage::Accessor(),
                         out1.upperLeft(),
                         vigra::DImage::Accessor());

    }
    else
    {
        vigra::DRGBImage aImageRGB(info1.width(), info1.height());
        if(info1.numExtraBands() == 1)
        {
            vigra::BImage aAlpha(info1.size());
            //importImageAlpha(info1, destImage(aImageRGB), destImage(aAlpha));
        }
        else if (info1.numExtraBands() == 0)
        {
            vigra::importImage(info1, destImage(aImageRGB));
        }

        // copy left img
        vigra::copyImage(aImageRGB.upperLeft(),
                         aImageRGB.lowerRight(),
                         vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
                         out1.upperLeft(),
                         vigra::DImage::Accessor());
    }

    if(info2.isGrayscale())
    {
        vigra::DImage aImageGrey(info2.width(), info2.height());
        importImage(info2, destImage(aImageGrey));

        // copy left img
        vigra::copyImage(aImageGrey.upperLeft(),
                         aImageGrey.lowerRight(),
                         vigra::DImage::Accessor(),
                         out1.upperLeft() + vigra::Diff2D(info1.width(), 0),
                         vigra::DImage::Accessor());

    }
    else
    {
        vigra::DRGBImage aImageRGB(info2.width(), info2.height());
        if(info2.numExtraBands() == 1)
        {
            vigra::BImage aAlpha(info2.size());
            //importImageAlpha(info2, destImage(aImageRGB), destImage(aAlpha));
        }
        else if (info2.numExtraBands() == 0)
        {
            vigra::importImage(info2, destImage(aImageRGB));
        }

        // copy left img
        vigra::copyImage(aImageRGB.upperLeft(),
                         aImageRGB.lowerRight(),
                         vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
                         out1.upperLeft() + vigra::Diff2D(info1.width(), 0),
                         vigra::DImage::Accessor());
    }

    for (size_t i = 0; i < iOK.size(); ++i)
    {
        lfeat::PointMatchPtr& aV = iOK[i];
        vigra::RGBValue<int> color(gen127(), 255 , gen127());
        drawLine(out1,  aDoubleFactor * aV->_img1_x,
                 aDoubleFactor * aV->_img1_y,
                 aDoubleFactor *  aV->_img2_x + info1.width(),
                 aDoubleFactor * aV->_img2_y, color);
        //cout << "----------------------" << endl;
        //cout << "x= " << aV->_img2_x + info1.width() << " y= " << aV->_img2_y << endl;
        double x1p, y1p;
        iRansac.transform(aV->_img1_x, aV->_img1_y, x1p, y1p);
        //cout << "xp= " << x1p << " yp= " << y1p << endl;

        if (x1p <0)
        {
            x1p = 0;
        }
        if (y1p <0)
        {
            y1p = 0;
        }


        if (x1p > info1.width())
        {
            x1p=info1.width()-1;
        }
        if (y1p > info1.height())
        {
            y1p=info1.height()-1;
        }

        vigra::RGBValue<int> color2(0, 255 , 255);
        drawLine(out1,  aDoubleFactor * aV->_img2_x + info1.width(),
                 aDoubleFactor * aV->_img2_y,
                 aDoubleFactor * x1p         + info1.width(),
                 aDoubleFactor * y1p, color2);

    }

    for(size_t i=0; i<iNOK.size(); ++i)
    {
        lfeat::PointMatchPtr& aV = iNOK[i];
        vigra::RGBValue<int> color(255, gen127() , gen127());
        drawLine(out1,  aDoubleFactor * aV->_img1_x,
                 aDoubleFactor * aV->_img1_y,
                 aDoubleFactor * aV->_img2_x + info1.width(),
                 aDoubleFactor * aV->_img2_y, color);
        //cout << "----------------------" << endl;
        //cout << "x= " << aV->_img2_x + info1.width() << " y= " << aV->_img2_y << endl;
        double x1p, y1p;
        iRansac.transform(aV->_img1_x, aV->_img1_y, x1p, y1p);
        //cout << "xp= " << x1p << " yp= " << y1p << endl;

        if (x1p <0)
        {
            x1p = 0;
        }
        if (y1p <0)
        {
            y1p = 0;
        }

        if (x1p > info1.width())
        {
            x1p=info1.width()-1;
        }
        if (y1p > info1.height())
        {
            y1p=info1.height()-1;
        }

        vigra::RGBValue<int> color2(0, 255 , 255);
        drawLine(out1,  aDoubleFactor * aV->_img2_x + info1.width(),
                 aDoubleFactor * aV->_img2_y,
                 aDoubleFactor * x1p         + info1.width(),
                 aDoubleFactor * y1p, color2);

    }

    exportImage(srcImageRange(out1), vigra::ImageExportInfo("outcomp.png"));



}


