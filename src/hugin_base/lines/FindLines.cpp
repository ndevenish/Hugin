// -*- c-basic-offset: 4 -*-
/**  @file FindLines.cpp
 *
 *  @brief functions for finding lines
 *
 */

/***************************************************************************
 *   Copyright (C) 2009 by Tim Nugent                                      *
 *   timnugent@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "vigra/edgedetection.hxx"
#include "FindLines.h"
#include "FindN8Lines.h"

using namespace vigra;
using namespace std;

namespace HuginLines
{
double resize_image(UInt8RGBImage& in, UInt8RGBImage& out, int resize_dimension)
{
    // Re-size to max dimension
    double sizefactor=1.0;
    if (in.width() > resize_dimension || in.height() > resize_dimension)
    {
        int nw;
        int nh;
        if (in.width() >= in.height())
        {
            sizefactor = (double)resize_dimension/in.width();
            // calculate new image size
            nw = resize_dimension;
            nh = static_cast<int>(0.5 + (sizefactor*in.height()));
        }
        else
        {
            sizefactor = (double)resize_dimension/in.height();
               // calculate new image size
            nw = static_cast<int>(0.5 + (sizefactor*in.width()));
            nh = resize_dimension;
        }

        // create an image of appropriate size
        out.resize(nw, nh);
        // resize the image, using a bi-cubic spline algorithm
        resizeImageNoInterpolation(srcImageRange(in),destImageRange(out));
    }
    else
    {
        copyImage(srcImageRange(in),destImage(out));
    };
    return 1.0/sizefactor;
}

vigra::BImage* detectEdges(UInt8RGBImage input,double scale,double threshold,unsigned int resize_dimension, double &size_factor)
{
    // Resize image
    UInt8RGBImage scaled;
    size_factor=resize_image(input, scaled, resize_dimension);
    input.resize(0,0);

    // Convert to greyscale
    BImage grey(scaled.width(), scaled.height());
    copyImage(srcImageRange(scaled, RGBToGrayAccessor<RGBValue<UInt16> >()), destImage(grey));

    // Run Canny edge detector
    BImage* image=new BImage(grey.width(), grey.height(), 255);
    cannyEdgeImage(srcImageRange(grey), destImage(*image), scale, threshold, 0);
    return image;
};

double calculate_focal_length_pixels(double focal_length,double cropFactor,double width, double height)
{
    double pixels_per_mm = 0;
    if (cropFactor > 1)
    {
        pixels_per_mm= (cropFactor/24.0)* ((width>height)?height:width);
    }
    else
    {
        pixels_per_mm= (24.0/cropFactor)* ((width>height)?height:width);
    }
    return focal_length*pixels_per_mm;
}


Lines findLines(vigra::BImage& edge, double length_threshold, double focal_length,double crop_factor)
{
    unsigned int longest_dimension=(edge.width() > edge.height()) ? edge.width() : edge.height();
    double min_line_length_squared=(length_threshold*longest_dimension)*(length_threshold*longest_dimension);

    int lmin = int(sqrt(min_line_length_squared));
    double flpix=calculate_focal_length_pixels(focal_length,crop_factor,edge.width(),edge.height());

    BImage lineImage = edgeMap2linePts(edge);
    Lines lines;
    int nlines = linePts2lineList( lineImage, lmin, flpix, lines );

    return lines;
};

void ScaleLines(Lines& lines,const double scale)
{
    for(unsigned int i=0;i<lines.size();i++)
    {
        for(unsigned int j=0;j<lines[i].line.size();j++)
        {
            lines[i].line[j]*=scale;
        };
    };
};

HuginBase::CPVector GetControlPoints(const SingleLine line,const unsigned int imgNr, const unsigned int lineNr,const unsigned int numberOfCtrlPoints)
{
    HuginBase::CPVector cpv;
    double interval = (line.line.size()-1)/(1.0*numberOfCtrlPoints);
    for(unsigned int k = 0; k < numberOfCtrlPoints; k++)
    {
        int start = (int)(k * interval);
        int stop =  (int)((k+1) * interval);
        HuginBase::ControlPoint cp(imgNr,line.line[start].x, line.line[start].y,
                        imgNr,line.line[stop].x, line.line[stop].y,lineNr);
        cpv.push_back(cp);
    };
    return cpv;
};

}; //namespace