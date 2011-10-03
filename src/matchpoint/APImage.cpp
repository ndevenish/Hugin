/***************************************************************************
 *   Copyright (C) 2007 by Zoran Mesec   *
 *   zoran.mesec@gmail.com   *
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
#include <string>
#include <math.h>
#include "APImage.h"

#ifdef USE_VIGRA
 	using namespace vigra;
#endif

APImage::APImage(string p) {
this->path=p;
}

bool APImage::open() {

#ifdef USE_VIGRA
    try
    {
        // read image given as first argument
        // file type is determined automatically
        ImageImportInfo info(this->path.c_str());
        //cout << this->path.c_str() << "Size:"<< info.width() <<"\n";

        if(info.isGrayscale())
        {
            // create a gray scale image of appropriate size
            vigra::BImage in(info.width(), info.height());
            this->imgBW=new BImage(info.width(), info.height());
            // import the image just read
            importImage(info, destImage(*this->imgBW));
        }
        else
        {

            BRGBImage in(info.width(), info.height());

            //TODO: this does not work. How to make a conversion from RGB to BW image in vigra???
            this->imgBW=new BImage(info.width(), info.height());

            importImage(info, destImage(in));

            // create image iterator that points to upper left corner
            // of source image
            vigra::BRGBImage::Iterator sy = in.upperLeft();

            // create image iterator that points past the lower right corner of
            // source image (similarly to the past-the-end iterator in the STL)
            vigra::BRGBImage::Iterator send = in.lowerRight();

            // create image iterator that points to upper left corner
            // of destination image
            vigra::BImage::Iterator dy = (*this->imgBW).upperLeft();

            // iterate down the first column of the images
            for(; sy.y != send.y; ++sy.y, ++dy.y)
            {
                // create image iterator that points to the first
                // pixel of the current row of the source image
                vigra::BRGBImage::Iterator sx = sy;

                // create image iterator that points to the first
                // pixel of the current row of the destination image
                vigra::BImage::Iterator dx = dy;

                // iterate across current row
                for(; sx.x != send.x; ++sx.x, ++dx.x)
                {
                    RGBValue<int,0u,1u,2u> pixel = *sx;
                    // calculate grayscale value
                    // Y = 0.3*R + 0.59*G + 0.11*B
                    *dx = vigra::round(0.3*pixel.red() + 0.59*pixel.green() + 0.11*pixel.blue());
                }
            }
            //exportAPImage(srcAPImageRange(*this->imgBW), vigra::APImageExportInfo("ttt.jpg"));
        }
        return true;
    }
    catch (...)
    {
        // catch any errors that might have occured and print their reason
        return false;
    }
#endif
}
void APImage::convolute(int* kernel,int dim1, int dim2,double scale) {
    /*CvScalar s;
    int offsetX=floor(dim1/2);
    int offsetY=floor(dim2/2);
    int xStart=0;
    int yStart=0;

    double pixelSum=0;
   // bool outOfRange=false;
    int tmpX=0;
    int tmpY=0;
    double max=0;

    for(int i=0;i<imgBW->height;i++) {
        for(int j=0; j<imgBW->width;j++) {
            //s=cvGet2D(imgBW,i,j);
            outOfRange=true;
            if(j-offsetX<0) {
                outOfRange=true;
            }
            if(i-offsetX<0)

            pixelSum=0;
            xStart=i-offsetX;
            yStart=j-offsetY;
            for(int k=0;k<dim1;k++) {
                tmpX=xStart+k;
                for(int l=0;l<dim2;l++) {
                    tmpY=yStart+l;
                    if((tmpX)<0 ||tmpY<0 || tmpX>=imgBW->height || tmpY>=imgBW->width)  {
                        pixelSum+=kernel[k*dim2+l]*(rand()%256);
                    } else {

                        s=cvGet2D(imgBW,xStart+k,yStart+l);
                        //cout << " "<< s.val[0] << "\n";

                        pixelSum+=kernel[k*dim2+l]*s.val[0];
                    }
                    //cout << "\nPixelSum:"<< pixelSum;
                }
            }
            s.val[0]=pixelSum*0.004;
            cvSet2D(imgBW,i,j,s); // set the (i,j) pixel value
            if(pixelSum>max) max=pixelSum;
            //cout << " "<< pixelSum;
            //this->convolution[i][j]=pixelSum;
            //s=cvGet2D(imgBW,i,j);
//          s.val[0]=rand()%255;
//          cvSet2D(imgBW,i,j,s); // set the (i,j) pixel value
        }
        //out << "\n";
    }*/
}
int APImage::getPixel(int x,int y) {
#ifdef USE_VIGRA
    return (*this->imgBW)(x,y);
#endif
#ifdef USE_OPENCV
    CvScalar s=cvGet2D(imgBW,x,y);
    s.val[0];
#endif
}
int APImage::getWidth() {
	return this->imgBW->width();
}
int APImage::getWidthBW() {
	return this->imgBW->width();
}
int APImage::getHeight() {
	return this->imgBW->height();
}
int APImage::getHeightBW() {
	return this->imgBW->height();
}
APImage* APImage::getCopy() {
	return new APImage("");
}
void APImage::scale(double factor) {
//#ifdef USE_OPENCV
	/*IplAPImage *resized= cvCreateAPImage(cvSize(round(this->getWidthBW()/factor),round(this->getHeightBW()/factor)), imgBW->depth ,imgBW->nChannels);
	cvResize(imgBW,resized);
	this->imgBW=resized;*/
//#endif
}
void APImage::show() {
#ifdef USE_VIGRA
    string add = "00det_";
    add.append(this->getPath());

    cout << "Results:"<< add << endl;
    exportImage(srcImageRange(*this->imgBW), ImageExportInfo(add.c_str()));
#endif
#ifdef USE_OPENCV
    cvNamedWindow( "Image view", 1 );
    cvShowAPImage( "Image view", this->img);
    cvWaitKey(0); // very important, contains event processing loop inside
    cvDestroyWindow( "Image view" );
#endif
        //cvReleaseAPImage( &img );
}
void APImage::drawCircle(int x,int y, int radius) {
    //cout << "Circle:" << x << "," << y << ",radius:"<<radius<<"\n";
#ifdef USE_VIGRA

    (*this->imgBW)(x,y) = 255;
    (*this->imgBW)(x+1,y+1) = 255;
    (*this->imgBW)(x-1,y-1) = 255;
    (*this->imgBW)(x-1,y+1) = 255;
    (*this->imgBW)(x+1,y-1) = 255;

#endif
    //cout << "EOF CIRCLE DRAWING" <<endl;

#ifdef USE_OPENCV
    cvCircle(this->img, cvPoint(x,y), radius, cvScalar(0,255,0), 1);
    cvCircle(this->img, cvPoint(x,y), 0, cvScalar(0,255,0), 1);
#endif

}
void APImage::drawRectangle(int x,int y, int radius) {
    //cvRectangle(this->img,cvPoint(x-radius,y-radius),cvPoint(x+radius,y+radius),cvScalar(0,255,0));
}

void APImage::drawLine(int x1,int y1, int x2,int y2) {
    //cout << "Draw line:("<< x1<< ","<<y1<<")->("<<x2<<","<<y2<<")\n";
#ifdef USE_OPENCV
    cvLine(this->img,cvPoint(x1,y1),cvPoint(x2,y2),cvScalar(0,255,255));
#endif

}

void APImage::smooth() {
        //TODO: create a gaussian mask and call the function convolute
}
/**
 * Calculates the integral image
 */
void APImage::integrate() {

    /*cout << "Height:"<< this->getHeightBW() <<"\n";
    cout << "Width:"<< this->getWidthBW() <<"\n";*/

    /*for(int i=0;i<10;i++) {
        for(int j=0; j<10;j++) {
            cout << this->getPixel(j,i)<<" ";
        }
        cout << "\n";
    }*/

    this->integral.clear();
    this->integral.resize(this->getWidthBW());
    for(int i=0;i<this->getWidthBW();i++) {
        this->integral[i].resize(this->getHeightBW());
        for(int j=0; j<this->getHeightBW();j++) {
            //cout << i << ","<< j<<"\n";
            this->integral[i][j]=this->_getValue4Integral(i,j-1)+this->_getValue4Integral(i-1,j)+this->getPixel(i,j)-this->_getValue4Integral(i-1,j-1);
        }
        //cout << "\n";
    }

     /*for(int i=0;i<10;i++) {
        for(int j=0; j<10;j++) {
            cout << this->getIntegralPixel(j,i)<<" ";
        }
        cout << "\n";
    }

    cout << this->getRegionSum(2,1,2,3) <<"dddddd\n";*/
}
int APImage::_getValue4Integral(int x, int y) {
if(x==-1 || y==-1) return 0;
else return this->integral[x][y];
}
int APImage::getIntegralPixel(int x,int y) {
 return this->integral[x][y];
}
int APImage::getRegionSum(int y1, int x1, int y2, int x2) {
    if(x1<=0) x1=1;
    if(y1<=0) y1=1;
    if(x2<=0) x2=1;
    if(y2<=0) y2=1;
    if(x1>=this->getWidthBW()) x1=this->getWidthBW()-1;
    if(x2>=this->getWidthBW()) x2=this->getWidthBW()-1;
    if(y1>=this->getHeightBW()) y1=this->getHeightBW()-1;
    if(y2>=this->getHeightBW()) y2=this->getHeightBW()-1;
 return this->integral[x2][y2]+this->integral[x1-1][y1-1]-this->integral[x1-1][y2]-this->integral[x2][y1-1];
}
string APImage::getPath() {
	return this->path;
}

/*double APImage::getPointOrientation(int x, int y) {

 return 0.0;
}*/

template <class SrcIterator, class SrcAccessor, class BackInsertable>
void APImage::_cannyEdgelList1(triple<SrcIterator, SrcIterator, SrcAccessor> src,
               BackInsertable & edgels, double scale,vector<int>* point )
{
    _cannyEdgelList(src.first, src.second, src.third, edgels, scale, point);
}

template <class SrcIterator, class SrcAccessor, class BackInsertable>
void APImage::_cannyEdgelList(SrcIterator ul, SrcIterator lr, SrcAccessor src,
                        BackInsertable & edgels, double scale, vector<int>* point)
{
    int w = lr.x - ul.x;
    int h = lr.y - ul.y;

    // calculate image gradients
    typedef typename
        NumericTraits<typename SrcAccessor::value_type>::RealPromote
        TmpType;

    BasicImage<TmpType> tmp(w,h), dx(w,h), dy(w,h);

    Kernel1D<double> smooth, grad;

    smooth.initGaussian(scale);
    grad.initGaussianDerivative(scale, 1);

    separableConvolveX(srcIterRange(ul, lr, src), destImage(tmp), kernel1d(grad));
    separableConvolveY(srcImageRange(tmp), destImage(dx), kernel1d(smooth));

    separableConvolveY(srcIterRange(ul, lr, src), destImage(tmp), kernel1d(grad));
    separableConvolveX(srcImageRange(tmp), destImage(dy), kernel1d(smooth));

    combineTwoImages(srcImageRange(dx), srcImage(dy), destImage(tmp),
                     MagnitudeFunctor<TmpType>());


    // find edgels
    internalCannyFindEdgels(dx, dy, tmp, edgels, point);
}

template <class Image1, class Image2, class BackInsertable>
void APImage::_internalCannyFindEdgels(Image1 const & gx,
                             Image1 const & gy,
                             Image2 const & magnitude,
                             BackInsertable & edgels, vector<int>* p)
{
    typedef typename Image1::value_type PixelType;
    double t = 0.5 / VIGRA_CSTD::sin(M_PI/8.0);

    //last element in edgel list is edgel that holds orientation
    //of interest point

    //orientation assignment
    vector<int > point= *p;
    PixelType gradx = gx(p[0],p[1]);
    PixelType grady = gy(p[0],p[1]);

    double orientation = VIGRA_CSTD::atan2(-grady, gradx) - M_PI * 1.5;
    if(orientation < 0.0)
        orientation += 2.0*M_PI;
    Edgel edgel;
    edgel.orientation=orientation;
    edgels.push_back(edgel);
    //EOF orientation assignment

    for(int y=1; y<gx.height()-1; ++y)
    {
        for(int x=1; x<gx.width()-1; ++x)
        {
            gradx = gx(x,y);
            grady = gy(x,y);
            double mag = magnitude(x, y);

            int dx = (int)VIGRA_CSTD::floor(gradx*t/mag + 0.5);
            int dy = (int)VIGRA_CSTD::floor(grady*t/mag + 0.5);

            int x1 = x - dx,
                x2 = x + dx,
                y1 = y - dy,
                y2 = y + dy;

            PixelType m1 = magnitude(x1, y1);
            PixelType m3 = magnitude(x2, y2);

            if(m1 < mag && m3 <= mag)
            {
                Edgel edgel;

                // local maximum => quadratic interpolation of sub-pixel location
                PixelType del = (m1 - m3) / 2.0 / (m1 + m3 - 2.0*mag);
                edgel.x = x + dx*del;
                edgel.y = y + dy*del;
                edgel.strength = mag;
                orientation = VIGRA_CSTD::atan2(-grady, gradx) - M_PI * 1.5;
                if(orientation < 0.0)
                    orientation += 2.0*M_PI;
                edgel.orientation = orientation;
                edgels.push_back(edgel);
            }
        }
    }
}

void APImage::test() {
            // empty edgel list
            /*std::vector<vigra::Edgel> edgels;

            // find edgels at scale of the interest point
            vigra::cannyEdgelList(srcIterRange( this->imgBW->upperLeft() + vigra::Diff2D(250, 250),
                                                this->imgBW->upperLeft() + vigra::Diff2D(550, 450)),
                                  edgels, 1.2, );
            cout << "Size:" << edgels.size() << endl;
            vector<vigra::Edgel>::iterator iter2 = edgels.begin();
            int a=0;
            while( iter2 != edgels.end()) { //loop over every canny pixel
                vigra::Edgel edgePoint=*iter2;
                //cout << edgePoint.strength << endl;
                if(edgePoint.strength>5) {
                     this->drawCircle(round(edgePoint.x)+250,round(edgePoint.y)+250,0);
                    a++;
                }
                iter2++;
            }
            cout << a << endl;*/
}
