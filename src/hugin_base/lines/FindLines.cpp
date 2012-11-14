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
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <nona/RemappedPanoImage.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include "algorithms/basic/CalculateCPStatistics.h"

using namespace vigra;
using namespace std;

namespace HuginLines
{
template <class ImageType>
double resize_image(ImageType& in, ImageType& out, int resize_dimension)
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
        out.resize(in.size());
        copyImage(srcImageRange(in),destImage(out));
    };
    return 1.0/sizefactor;
}

vigra::BImage* detectEdges(UInt8RGBImage input,double scale,double threshold,unsigned int resize_dimension, double& size_factor)
{
    // Resize image
    UInt8RGBImage scaled;
    size_factor=resize_image(input, scaled, resize_dimension);
    input.resize(0,0);

    // Convert to greyscale
    BImage grey(scaled.width(), scaled.height());
    copyImage(srcImageRange(scaled, RGBToGrayAccessor<RGBValue<UInt8> >()), destImage(grey));

    // Run Canny edge detector
    BImage* image=new BImage(grey.width(), grey.height(), 255);
    cannyEdgeImage(srcImageRange(grey), destImage(*image), scale, threshold, 0);
    return image;
};

vigra::BImage* detectEdges(BImage input,double scale,double threshold,unsigned int resize_dimension, double& size_factor)
{
    // Resize image
    UInt8Image scaled;
    size_factor=resize_image(input, scaled, resize_dimension);
    input.resize(0,0);

    // Run Canny edge detector
    BImage* image=new BImage(scaled.width(), scaled.height(), 255);
    cannyEdgeImage(srcImageRange(scaled), destImage(*image), scale, threshold, 0);
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
    for(unsigned int i=0; i<lines.size(); i++)
    {
        for(unsigned int j=0; j<lines[i].line.size(); j++)
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

#define MAX_RESIZE_DIM 1600

struct VerticalLine
{
    vigra::Point2D start;
    vigra::Point2D end;
};

typedef std::vector<VerticalLine> VerticalLineVector;

//return footpoint of point p on line between point p1 and p2
vigra::Point2D GetFootpoint(vigra::Point2D p, vigra::Point2D p1, vigra::Point2D p2)
{
    hugin_utils::FDiff2D diff=p2-p1;
    double u=((p.x-p1.x)*(p2.x-p1.x)+(p.y-p1.y)*(p2.y-p1.y))/hugin_utils::sqr(hugin_utils::norm(diff));
    diff*=u;
    return vigra::Point2D(p1.x+diff.x,p1.y+diff.y);
};

//linear fit of given line, returns endpoints of fitted line
VerticalLine FitLine(SingleLine line)
{
    size_t n=line.line.size();
    VerticalLine vl;
    double s_x=0;
    double s_y=0;
    double s_xy=0;
    double s_x2=0;
    for(size_t i=0;i<n;i++)
    {
        s_x+=(double)line.line[i].x/n;
        s_y+=(double)line.line[i].y/n;
        s_xy+=(double)line.line[i].x*line.line[i].y/n;
        s_x2+=(double)line.line[i].x*line.line[i].x/n;
    };
    if(abs(s_x2-s_x*s_x)<0.00001)
    {
        //vertical line needs special treatment
        vl.start.x=s_x;
        vl.start.y=line.line[0].y;
        vl.end.x=s_x;
        vl.end.y=line.line[n-1].y;
    }
    else
    {
        //calculate slope and offset
        double slope=(s_xy-s_x*s_y)/(s_x2-s_x*s_x);
        double offset=s_y-slope*s_x;
        //convert to parametric form
        vigra::Point2D p1(0,offset);
        vigra::Point2D p2(100,100*slope+offset);
        //calculate footpoint of first and last point
        vl.start=GetFootpoint(line.line[0],p1,p2);
        vl.end=GetFootpoint(line.line[n-1],p1,p2);
    };
    return vl;
};

//filter detected lines
//return fitted lines which have only a small deviation from the vertical
VerticalLineVector FilterLines(Lines lines,double roll)
{
    VerticalLineVector vertLines;
    if(lines.size()>0)
    {
        for(Lines::const_iterator it=lines.begin(); it!=lines.end(); it++)
        {
            if((*it).status==valid_line && (*it).line.size()>2)
            {
                VerticalLine vl=FitLine(*it);
                vigra::Diff2D diff=vl.end-vl.start;
                if(diff.magnitude()>20)
                {
                    if(abs((diff.x*cos(DEG_TO_RAD(roll))+diff.y*sin(DEG_TO_RAD(roll)))/diff.magnitude())<0.1)
                    {
                        vertLines.push_back(vl);
                    };
                };
            };
        };
    };
    return vertLines;
};

//function to sort HuginBase::CPVector by error distance
bool SortByError(const HuginBase::ControlPoint& cp1, const HuginBase::ControlPoint& cp2)
{
    return cp1.error<cp2.error;
};

template <class ImageType>
HuginBase::CPVector _getVerticalLines(const HuginBase::Panorama& pano,const unsigned int imgNr,ImageType& image, const unsigned int nrLines)
{
    HuginBase::CPVector verticalLines;
    HuginBase::CPVector detectedLines;
    const HuginBase::SrcPanoImage& srcImage=pano.getImage(imgNr);
    bool needsRemap=srcImage.getProjection()!=HuginBase::SrcPanoImage::RECTILINEAR;
    double roll=(needsRemap?0:srcImage.getRoll());
    double size_factor=1.0;
    HuginBase::SrcPanoImage remappedImage;
    HuginBase::PanoramaOptions opts;
    vigra::BImage* edge;
    if(!needsRemap)
    {
        //rectilinear image can be used as is
        //detect edges
        edge=detectEdges(image,2,4,MAX_RESIZE_DIM,size_factor);
    }
    else
    {
        //remap all other image to equirectangular
        //create temporary SrcPanoImage, set appropriate image variables
        remappedImage=pano.getSrcImage(imgNr);
        remappedImage.setYaw(0);
        remappedImage.setPitch(0);
        remappedImage.setX(0);
        remappedImage.setY(0);
        remappedImage.setZ(0);
        remappedImage.setExposureValue(0);
        remappedImage.setEMoRParams(std::vector<float>(5, 0.0));
        remappedImage.deleteAllMasks();
        remappedImage.setActive(true);
        //create PanoramaOptions for remapping of image
        opts.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
        opts.setWidth(MAX_RESIZE_DIM);
        opts.outputExposureValue=0;
        //calculate output canvas size
        HuginBase::Panorama tempPano;
        tempPano.addImage(remappedImage);
        tempPano.setOptions(opts);

        HuginBase::CalculateFitPanorama fitPano(tempPano);
        fitPano.run();
        opts.setHFOV(fitPano.getResultHorizontalFOV());
        opts.setHeight(hugin_utils::roundi(fitPano.getResultHeight()));
        tempPano.setOptions(opts);

        //finally remap image
        HuginBase::Nona::RemappedPanoImage<ImageType,vigra::BImage>* remapped=new HuginBase::Nona::RemappedPanoImage<ImageType,vigra::BImage>;
        AppBase::MultiProgressDisplay* progress=new AppBase::DummyMultiProgressDisplay();
        remapped->setPanoImage(remappedImage,opts,opts.getROI());
        remapped->remapImage(vigra::srcImageRange(image),vigra_ext::INTERP_CUBIC,*progress);
        ImageType remappedBitmap=remapped->m_image;
        //detect edges
        edge=detectEdges(remappedBitmap,2,4,std::max(remappedBitmap.width(),remappedBitmap.height())+10,size_factor);
        delete remapped;
        delete progress;
    };
    //detect lines
    //we need the focal length
    double focalLength=srcImage.getExifFocalLength();
    if(focalLength==0)
    {
        focalLength=HuginBase::SrcPanoImage::calcFocalLength(
            srcImage.getProjection(),srcImage.getHFOV(),srcImage.getExifCropFactor(),srcImage.getSize());
    };
    Lines foundLines=findLines(*edge,0.05,focalLength,srcImage.getExifCropFactor());
    //filter results
    VerticalLineVector filteredLines=FilterLines(foundLines,roll);
    //create control points
    if(filteredLines.size()>0)
    {
        //we need to transform the coordinates to image coordinates because the detection
        //worked on smaller images or in remapped image
        HuginBase::PTools::Transform transform;
        if(needsRemap)
        {
            transform.createTransform(remappedImage,opts);
        };
        for(size_t i=0; i<filteredLines.size(); i++)
        {
            HuginBase::ControlPoint cp;
            cp.image1Nr=0;
            cp.image2Nr=0;
            cp.mode=HuginBase::ControlPoint::X;
            if(!needsRemap)
            {
                cp.x1=filteredLines[i].start.x*size_factor;
                cp.y1=filteredLines[i].start.y*size_factor;
                cp.x2=filteredLines[i].end.x*size_factor;
                cp.y2=filteredLines[i].end.y*size_factor;
            }
            else
            {
                double xout;
                double yout;
                if(!transform.transformImgCoord(xout,yout,filteredLines[i].start.x,filteredLines[i].start.y))
                {
                    continue;
                };
                cp.x1=xout;
                cp.y1=yout;
                if(!transform.transformImgCoord(xout,yout,filteredLines[i].end.x,filteredLines[i].end.y))
                {
                    continue;
                };
                cp.x2=xout;
                cp.y2=yout;
            };
            if(cp.x1>=0 && cp.x1<srcImage.getWidth() && cp.y1>=0 && cp.y1<srcImage.getHeight() &&
               cp.x2>=0 && cp.x2<srcImage.getWidth() && cp.y2>=0 && cp.y2<srcImage.getHeight())
            {
                detectedLines.push_back(cp);
            };
        };
        //now a final check of the found vertical lines
        //we optimize the pano with a single image and disregard vertical lines with bigger errors
        //we need at least 2 lines
        if(detectedLines.size()>1)
        {
            HuginBase::Panorama tempPano;
            HuginBase::SrcPanoImage tempImage=pano.getSrcImage(imgNr);
            tempImage.setYaw(0);
            tempImage.setPitch(0);
            tempImage.setRoll(0);
            tempImage.setX(0);
            tempImage.setY(0);
            tempImage.setZ(0);
            tempPano.addImage(tempImage);
            for(size_t i=0; i<detectedLines.size(); i++)
            {
                tempPano.addCtrlPoint(detectedLines[i]);
            };
            HuginBase::PanoramaOptions opt2;
            opt2.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
            tempPano.setOptions(opt2);
            HuginBase::OptimizeVector optVec;
            std::set<std::string> imgopt;
            imgopt.insert("p");
            imgopt.insert("r");
            optVec.push_back(imgopt);
            tempPano.setOptimizeVector(optVec);
            HuginBase::PTools::optimize(tempPano);
            //first filter stage
            //we disregard all lines with big error
            //calculate statistic and determine limit
            double minError,maxError,mean,var;
            HuginBase::CalculateCPStatisticsError::calcCtrlPntsErrorStats(tempPano,minError,maxError,mean,var);
            detectedLines=tempPano.getCtrlPoints();
            double limit=mean+sqrt(var);
            maxError=0;
            for(int i=detectedLines.size()-1; i>=0; i--)
            {
                if(detectedLines[i].error>limit)
                {
                    detectedLines.erase(detectedLines.begin()+i);
                }
                else
                {
                    //we need the max error of the remaining lines for the next step
                    maxError=std::max(detectedLines[i].error,maxError);
                };
            };
            if(detectedLines.size()>0 && maxError>0) //security check, should never be false
            {
                //now keep only the best nrLines lines
                //we are using error and line length as figure of merrit
                for(size_t i=0;i<detectedLines.size();i++)
                {
                    double length=sqrt(hugin_utils::sqr(detectedLines[i].x2-detectedLines[i].x1)+hugin_utils::sqr(detectedLines[i].y2-detectedLines[i].y1));
                    //calculate number of merrit
                    detectedLines[i].error=detectedLines[i].error/maxError+(1.0-std::min(length,500.0)/500.0);
                };
                std::sort(detectedLines.begin(),detectedLines.end(),SortByError);
                //only save best nrLines control points
                for(size_t i=0;i<detectedLines.size() && i<nrLines; i++)
                {
                    HuginBase::ControlPoint cp=detectedLines[i];
                    cp.image1Nr=imgNr;
                    cp.image2Nr=imgNr;
                    cp.error=0;
                    verticalLines.push_back(cp);
                };
            };
        }
        else
        {
            //if only one line was detected we do a special check
            //the allow deviation between line and roll angle is checked more narrow than in the first check
            if(detectedLines.size()==1)
            {
                vigra::Diff2D diff((double)detectedLines[0].x2-detectedLines[0].x1,(double)detectedLines[0].y2-detectedLines[0].y1);
                if(abs((diff.x*cos(DEG_TO_RAD(roll))+diff.y*sin(DEG_TO_RAD(roll)))/diff.magnitude())<0.05)
                {
                    HuginBase::ControlPoint cp=detectedLines[0];
                    cp.image1Nr=imgNr;
                    cp.image2Nr=imgNr;
                    cp.error=0;
                    verticalLines.push_back(cp);
                };
            };
        };
    };
    return verticalLines;
};

HuginBase::CPVector GetVerticalLines(const HuginBase::Panorama& pano,const unsigned int imgNr,vigra::UInt8RGBImage& image, const unsigned int nrLines)
{
    return _getVerticalLines(pano, imgNr, image, nrLines);
};

HuginBase::CPVector GetVerticalLines(const HuginBase::Panorama& pano,const unsigned int imgNr,vigra::BImage& image, const unsigned int nrLines)
{
    return _getVerticalLines(pano, imgNr, image, nrLines);
};

}; //namespace