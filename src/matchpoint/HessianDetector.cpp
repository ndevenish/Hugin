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
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <limits>
#include "HessianDetector.h"

using namespace std;

HessianDetector::HessianDetector(APImage* i, int nrPoints, CONVOLUTION_TYPE type, int octaves) {
    if(octaves>HD_MAX_OCTAVES) return;  // TODO (zoran#1#): Throw error or log status.

	this->image=i;
	this->nrPoints=nrPoints;
	this->nrOctaves=octaves;
	this->convolutionType = type;

    vector<int> point;
    point.resize(2);

    this->orderedList.reserve(sizeof(point)*nrPoints);

    //initialize the vectors
	determinants.resize(this->image->getWidthBW());
	maximas.resize(this->image->getWidthBW());

    const int iMax=std::numeric_limits<int>::max();
	for(int i=0;i<this->image->getWidthBW();i++) {
		maximas[i].resize(this->image->getHeightBW());
		determinants[i].resize(this->image->getHeightBW());
		for(int j=0; j<this->image->getHeightBW();j++) {
			maximas[i][j] =0 ;
			determinants[i][j]=iMax;
		}
	}
}

bool HessianDetector::detect() {
    if(this->convolutionType==HD_BOX_FILTERS) {
     return this->_boxFilterDetect();
    } else if(this->convolutionType==HD_SLIDING_WINDOW) {
     return this->_slidingWDetect();
    }
    return false;
}

bool HessianDetector::_slidingWDetect() {
	int height = this->image->getHeightBW();
	int width = this->image->getWidthBW();

	int kernelX=9;
	int kernelY=9;
	int kernelSize=9;
	double scale=1.2;

	int gxx[][9]={
	1, 7, 29, 68, 90, 68, 29, 7, 1,   //9x9 gauss kernels
    4, 26, 106, 247, 327, 247, 106, 26, 4,
    5, 33, 133, 310, 411, 310, 133, 33, 5,
    -4, -27, -109, -252, -335, -252, -109, -27, -4,
    -11, -81, -329, -765, -1013, -765, -329, -81, -11,
    -4, -27, -109, -252, -335, -252, -109, -27, -4,
    5, 33, 133, 310, 411, 310, 133, 33, 5,
    4, 26, 106, 247, 327, 247, 106, 26, 4,
    1, 7, 29, 68, 90, 68, 29, 7, 1
	};

	int gyy[][9]={
    1, 4, 5, -4, -11, -4, 5, 4, 1,
    7, 26, 33, -27, -81, -27, 33, 26, 7,
    29, 106, 133, -109, -329, -109, 133, 106, 29,
    68, 247, 310, -252, -765, -252, 310, 247, 68,
    90, 327, 411, -335, -1013, -335, 411, 327, 90,
    68, 247, 310, -252, -765, -252, 310, 247, 68,
    29, 106, 133, -109, -329, -109, 133, 106, 29,
    7, 26, 33, -27, -81, -27, 33, 26, 7,
    1, 4, 5, -4, -11, -4, 5, 4, 1
    };

    int gxy[][9]={
    1, 5, 15, 17, 0, -17, -15, -5, -1,
    5, 29, 78, 91, 0, -91, -78, -29, -5,
    15, 78, 214, 248, 0, -248, -214, -78, -15,
    17, 91, 248, 289, 0, -289, -248, -91, -17,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    -17, -91, -248, -289, 0, 289, 248, 91, 17,
    -15, -78, -214, -248, 0, 248, 214, 78, 15,
    -5, -29, -78, -91, 0, 91, 78, 29, 5,
    -1, -5, -15, -17, 0, 17, 15, 5, 1
    };

    int offsetX=floor((double)kernelX/2);
    int offsetY=floor((double)kernelY/2);
    int xStart=0;
    int yStart=0;
    int pixelSumYY;
    int pixelSumXX;
    int pixelSumXY;

    int tmpX=0;
    int tmpY=0;
    int val;
    int mappingX;
    int mappingY;

	int det;


    for(int s=0; s<6;s+=1.0) {   //scale-space loop
    //cout << s << "\n";
    height=image->getHeightBW();
    width=image->getWidthBW();
    /*cout << height << "," << width << " \n";
    cout << " ";*/
    for(int i=0;i<height;i++) {
        for(int j=0; j<width;j++) {
            pixelSumYY=0;
            pixelSumXX=0;
            pixelSumXY=0;

            xStart=i-offsetX;
            yStart=j-offsetY;

            for(int k=0;k<kernelX;k++) {
                tmpX=xStart+k;
                if(tmpX<0) tmpX=height+tmpX;
                if(tmpX>=height) tmpX=tmpX-height;

                for(int l=0;l<kernelY;l++) {
                    tmpY=yStart+l;

                    if(tmpY<0) tmpY=width+tmpY;
                    if(tmpY>=height) tmpY=tmpY-height;

                    val=image->getPixel(tmpX,tmpY);

                    pixelSumYY+=gyy[k][l]*val;
                    pixelSumXX+=gxx[k][l]*val;
                    pixelSumXY+=gxy[k][l]*val;
                }
            }

            //determinant of the Hessian Matrix
			det = pixelSumXX*pixelSumYY-pow((double)pixelSumXY,2.0);

            //map coordinates on the scaled image onto coordinates on the original image
            mappingX=vigra::round(i*pow(scale,s));
            mappingY=vigra::round(j*pow(scale,s));

			if(det>determinants[mappingX][mappingY]) {
                //if(det>max) max=det;
                determinants[mappingX][mappingY]=det;
                maximas[mappingX][mappingY]=kernelSize*pow(scale,s);
			}
        }
       //cout << "\n";
    }
    image->scale(scale);
    }   //scale-space

    int xEnd=0;
    int yEnd=0;

    for(int i=0;i<image->getHeight();i++) {
        if(i>=(image->getHeight()-1)) {
            xStart=i-1;
            xEnd=0;
        } else if(i==0) {
            xStart=image->getHeight()-1;
            xEnd=i+1;
        } else {
            xStart=i-1;
            xEnd=i+1;
        }
        for(int j=0; j<image->getWidth();j++) {
            if(j>=(image->getWidth()-1)) {
                yStart=j-1;
                yEnd=0;
            } else if(j==0) {
                yStart=image->getWidth()-1;
                yEnd=j+1;
            } else {
                yStart=j-1;
                yEnd=j+1;
            }
            /**if any determinant in the area around the pixel is
             *greater than the pixel, suppress the current pixel
             *
             * This can also be done in a loop.
             *
             */

            if(determinants[i][j]>determinants[xStart][yStart] &&
            determinants[i][j]>determinants[xStart][j] &&
            determinants[i][j]>determinants[xStart][yEnd] &&
            determinants[i][j]>determinants[i][yStart] &&
            determinants[i][j]>determinants[i][yEnd] &&
            determinants[i][j]>determinants[xEnd][yStart] &&
            determinants[i][j]>determinants[xEnd][j] &&
            determinants[i][j]>determinants[xEnd][yEnd] ) {
                this->_insertToList(&i,&j);
            }

        }
        //cout << "\n";
    }
    return true;
}

bool HessianDetector::_boxFilterDetect() {

	int height = this->image->getHeightBW();
	int width = this->image->getWidthBW();

    cout << "Height:"<<height << ",width:"<<width<<endl;
    //cout << "Max vector size:"<< orderedList.max_size() << ", capacity:" << orderedList.capacity() << "\n";


    const int iMax=std::numeric_limits<int>::max();

    int offsetX=width*0.05;
    int offsetY=height*0.05;


    int xStart=offsetX;
    int yStart=offsetY;
    int xEnd=width-offsetX;
    int yEnd=height-offsetY;

    for(int j=yStart;j<yEnd;j++) {
        for(int i=xStart; i<xEnd;i++) {
            //_calculateMaxDet(x_coord,y_coord)
            _calculateMaxDet(i,j);  //calculate determinant

        }
    }

 vector<vector<int> >::iterator it;

  it = orderedList.begin();
  double sum=0;
  double count=0;
    //point (x,y) => (j,i)
    for(int j=yStart;j<yEnd;j++) {
        for(int i=xStart; i<xEnd;i++) {
            /**Non-maxima suppression
             *if any determinant in the area around the pixel is
             *greater than the pixel, suppress the current pixel.
             *
             * This can also be done in a loop.
             *
             */

            if(determinants[i][j]>determinants[i-1][j-1] &&
            determinants[i][j]>determinants[i-1][j] &&
            determinants[i][j]>determinants[i-1][j+1] &&
            determinants[i][j]>determinants[i][j-1] &&
            determinants[i][j]>determinants[i][j+1] &&
            determinants[i][j]>determinants[i+1][j-1] &&
            determinants[i][j]>determinants[i+1][j] &&
            determinants[i][j]>determinants[i+1][j+1] &&
            determinants[i][j]>300000) {
            //this->_insertToList(&i,&j);
            //cout << maximas[i][j]<<","<<determinants[i][j]<< ";"<<endl;
            /*cout << i<<","<<j<< "\n";*/
            vector<int> point;
            point.resize(2);

            point[0]=i;
            point[1]=j;

            this->orderedList.push_back(point);

            count++;
            sum+=determinants[i][j];
            //it = orderedList.insert ( it , point);

            }
                //cout << maximas[i][j]<<","<<determinants[i][j]<< " ";
        }
       //cout << "\n";
    }
    cout << "Detected points:"<< this->orderedList.size() <<" ";
    cout << "Average:"<< sum/count <<endl;
    this->_cutPointList(sum/count,10000);
    return true;
}

void HessianDetector::_cutPointList(double threshold, int nrPoints) {
    if(this->orderedList.size()<nrPoints) return;
    vector<vector<int> >::iterator iter1 = this->orderedList.begin();
    double avg=0;
    int count=0;
    while( iter1 != this->orderedList.end()) { //loop over every interest point
        vector<int > interestPoint=*iter1;

        iter1++;
        vector<int > interestPoint2=*iter1;

        //cout << this->determinants[interestPoint[0]][interestPoint[1]] << endl;
        //cout << count << endl;


        iter1--;

        if(this->determinants[interestPoint[0]][interestPoint[1]]<threshold) {
                    this->orderedList.erase(iter1);
        } else {
                avg+=this->determinants[interestPoint[0]][interestPoint[1]];
                iter1++;
        }
        count++;
    }

    cout << "Number of points:"<< this->orderedList.size()<< endl;

    this->_cutPointList(avg/this->orderedList.size(), nrPoints);
}

void HessianDetector::_calculateMaxDet(int x, int y) {
    /**
     * Each octave has 4 kernel sizes.
     */
    int octaves[][4] ={
    9,15,21,27,
    15,27,39,51,
    21,45,69,93
    };
    double interp1, interp2;

    //first octave
    interp1=(this->_convolutePixel(&x,&y,&octaves[0][0])+this->_convolutePixel(&x,&y,&octaves[0][1]))/2;
    interp2=(this->_convolutePixel(&x,&y,&octaves[0][2])+this->_convolutePixel(&x,&y,&octaves[0][3]))/2;
    if(interp1>interp2) {
        this->determinants[x][y]=interp1;
        this->maximas[x][y]=(((octaves[0][0]+octaves[0][1])/2)*HD_INITIAL_SCALE)/HD_INIT_KERNEL_SIZE;
    } else {
        this->determinants[x][y]=interp2;
        this->maximas[x][y]=(((octaves[0][2]+octaves[0][3])/2)*HD_INITIAL_SCALE)/HD_INIT_KERNEL_SIZE; //scale of interpolation
    }

    //second octave
    if(this->nrOctaves>1 && x%2==0 && y%2==0) { //points are sampled
        interp1=(this->_convolutePixel(&x,&y,&octaves[1][0])+this->_convolutePixel(&x,&y,&octaves[1][1]))/2;
        interp2=(this->_convolutePixel(&x,&y,&octaves[1][2])+this->_convolutePixel(&x,&y,&octaves[1][3]))/2;
        if(interp1>this->determinants[x][y] || interp2>this->determinants[x][y] ) {
            if(interp1>interp2) {
                this->determinants[x][y]=interp1;
                this->maximas[x][y]=(((octaves[1][0]+octaves[1][1])/2)*HD_INITIAL_SCALE)/HD_INIT_KERNEL_SIZE;
            } else {
                this->determinants[x][y]=interp2;
                this->maximas[x][y]=(((octaves[1][2]+octaves[1][3])/2)*HD_INITIAL_SCALE)/HD_INIT_KERNEL_SIZE; //scale of interpolation
            }
        }
    }

    //third octave
    if(this->nrOctaves==HD_MAX_OCTAVES && x%4==0 && y%4==0) { //points are sampled
        interp1=(this->_convolutePixel(&x,&y,&octaves[2][0])+this->_convolutePixel(&x,&y,&octaves[2][1]))/2;
        interp2=(this->_convolutePixel(&x,&y,&octaves[2][2])+this->_convolutePixel(&x,&y,&octaves[2][3]))/2;
        if(interp1>this->determinants[x][y] || interp2>this->determinants[x][y] ) {
            if(interp1>interp2) {
                this->determinants[x][y]=interp1;
                this->maximas[x][y]=(((octaves[2][0]+octaves[2][1])/2)*HD_INITIAL_SCALE)/HD_INIT_KERNEL_SIZE;
            } else {
                this->determinants[x][y]=interp2;
                this->maximas[x][y]=(((octaves[2][2]+octaves[2][3])/2)*HD_INITIAL_SCALE)/HD_INIT_KERNEL_SIZE; //scale of interpolation
            }
        }
    }
}

int HessianDetector::_getHessianDeterminant(int* pixelSumXX, int* pixelSumXY, int* pixelSumYY) {
 return *pixelSumXX*(*pixelSumYY)-pow((0.9*(*pixelSumXY)),2);
}

int HessianDetector::_convolutePixel(int* x, int* y, int* kernelSize) {
//for box filters only
    int pixelSumXX=0;
    int pixelSumYY=0;
    int pixelSumXY=0;
    int det;
    switch(*kernelSize) {
        case 9:
            //9x9 kernel
            //getRegionSum(y1,x1,y2,x2)
            pixelSumYY=this->image->getRegionSum(*y-4,*x-2,*y-2,*x+2);
            pixelSumYY+=this->image->getRegionSum(*y+2,*x-2,*y+4,*x+2);
            pixelSumYY+=-2*this->image->getRegionSum(*y-1,*x-2,*y+1,*x+2);

            pixelSumXY=-1*this->image->getRegionSum(*y-3,*x+1,*y-1,*x+3);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-3,*y+3,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-3,*x-3,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+3,*x+3);

            pixelSumXX=this->image->getRegionSum(*y-2,*x-4,*y+2,*x-2);
            pixelSumXX+=this->image->getRegionSum(*y-2,*x+2,*y+2,*x+4);
            pixelSumXX+=-2*this->image->getRegionSum(*y-2,*x-1,*y+2,*x+1);

            //determinant of the Hessian Matrix
			det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY);
        break;
        case 11:
            //11x11 kernel
            pixelSumYY=this->image->getRegionSum(*y-5,*x-3,*y-2,*x+3);
            pixelSumYY+=this->image->getRegionSum(*y+2,*x-3,*y+5,*x+3);
            pixelSumYY+=-2*this->image->getRegionSum(*y-1,*x-3,*y+1,*x+3);

            pixelSumXY=-1*this->image->getRegionSum(*y-4,*x+1,*y-1,*x+4);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-4,*y+4,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-4,*x-4,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+4,*x+4);

            pixelSumXX=this->image->getRegionSum(*y-3,*x-5,*y+3,*x-2);
            pixelSumXX+=this->image->getRegionSum(*y-3,*x+2,*y+3,*x+5);
            pixelSumXX+=-2*this->image->getRegionSum(*y-3,*x-1,*y+3,*x+1);
            //determinant of the Hessian Matrix
			det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/2.232;
            break;
        case 15:
            //15x15 kernel
            pixelSumYY=this->image->getRegionSum(*y-7,*x-4,*y-3,*x+4);
            pixelSumYY+=this->image->getRegionSum(*y+3,*x-4,*y+7,*x+4);
            pixelSumYY+=-2*this->image->getRegionSum(*y-2,*x-4,*y+2,*x+4);

            pixelSumXY=-1*this->image->getRegionSum(*y-5,*x+1,*y-1,*x+5);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-5,*y+5,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-5,*x-5,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+5,*x+5);

            pixelSumXX=this->image->getRegionSum(*y-4,*x-7,*y+4,*x-3);
            pixelSumXX+=this->image->getRegionSum(*y-4,*x+3,*y+4,*x+7);
            pixelSumXX+=-2*this->image->getRegionSum(*y-4,*x-2,*y+4,*x+2);

            det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/7.72;
            break;
        case 17:
            //17x17 kernel
            pixelSumYY=this->image->getRegionSum(*y-8,*x-4,*y-3,*x+4);
            pixelSumYY+=this->image->getRegionSum(*y+3,*x-4,*y+8,*x+4);
            pixelSumYY+=-2*this->image->getRegionSum(*y-2,*x-4,*y+2,*x+4);

            pixelSumXY=-1*this->image->getRegionSum(*y-6,*x+1,*y-1,*x+6);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-6,*y+6,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-6,*x-6,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+6,*x+6);

            pixelSumXX=this->image->getRegionSum(*y-4,*x-8,*y+4,*x-3);
            pixelSumXX+=this->image->getRegionSum(*y-4,*x+3,*y+4,*x+8);
            pixelSumXX+=-2*this->image->getRegionSum(*y-4,*x-2,*y+4,*x+2);
            //determinant of the Hessian Matrix
			det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/12.73;
            break;
        case 21:
            //21x21 kernel
            pixelSumYY=this->image->getRegionSum(*y-10,*x-6,*y-4,*x+6);
            pixelSumYY+=this->image->getRegionSum(*y+4,*x-6,*y+10,*x+6);
            pixelSumYY+=-2*this->image->getRegionSum(*y-3,*x-6,*y+3,*x+6);

            pixelSumXY=-1*this->image->getRegionSum(*y-7,*x+1,*y-1,*x+7);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-7,*y+7,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-7,*x-7,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+7,*x+7);

            pixelSumXX=this->image->getRegionSum(*y-6,*x-10,*y+6,*x-4);
            pixelSumXX+=this->image->getRegionSum(*y-6,*x+4,*y+6,*x+10);
            pixelSumXX+=-2*this->image->getRegionSum(*y-6,*x-3,*y+6,*x+3);

            det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/29.642;
            break;
        case 27:
            //27x27 kernel
            pixelSumYY=this->image->getRegionSum(*y-13,*x-7,*y-5,*x+7);
            pixelSumYY+=this->image->getRegionSum(*y+5,*x-7,*y+13,*x+7);
            pixelSumYY+=-2*this->image->getRegionSum(*y-4,*x-7,*y+4,*x+7);

            pixelSumXY=-1*this->image->getRegionSum(*y-9,*x+1,*y-1,*x+9);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-9,*y+9,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-9,*x-9,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+9,*x+9);

            pixelSumXX=this->image->getRegionSum(*y-7,*x-13,*y+7,*x-5);
            pixelSumXX+=this->image->getRegionSum(*y-7,*x+5,*y+7,*x+13);
            pixelSumXX+=-2*this->image->getRegionSum(*y-7,*x-4,*y+7,*x+4);

            det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/81;
            break;
        case 39:
            //39x39 kernel
            pixelSumYY=this->image->getRegionSum(*y-19,*x-11,*y-7,*x+11);
            pixelSumYY+=this->image->getRegionSum(*y+7,*x-11,*y+19,*x+11);
            pixelSumYY+=-2*this->image->getRegionSum(*y-6,*x-11,*y+6,*x+11);

            pixelSumXY=-1*this->image->getRegionSum(*y-13,*x+1,*y-1,*x+13);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-13,*y+13,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-13,*x-13,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+13,*x+13);

            pixelSumXX=this->image->getRegionSum(*y-11,*x-19,*y+11,*x-7);
            pixelSumXX+=this->image->getRegionSum(*y-11,*x+7,*y+11,*x+19);
            pixelSumXX+=-2*this->image->getRegionSum(*y-11,*x-6,*y+11,*x+6);

            det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/352.61;
            break;
        case 45:
            //45x45 kernel
            pixelSumYY=this->image->getRegionSum(*y-22,*x-12,*y-8,*x+12);
            pixelSumYY+=this->image->getRegionSum(*y+8,*x-12,*y+22,*x+12);
            pixelSumYY+=-2*this->image->getRegionSum(*y-7,*x-12,*y+7,*x+12);

            pixelSumXY=-1*this->image->getRegionSum(*y-15,*x+1,*y-1,*x+15);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-15,*y+15,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-15,*x-15,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+15,*x+15);

            pixelSumXX=this->image->getRegionSum(*y-12,*x-22,*y+12,*x-8);
            pixelSumXX+=this->image->getRegionSum(*y-12,*x+8,*y+12,*x+22);
            pixelSumXX+=-2*this->image->getRegionSum(*y-12,*x-7,*y+12,*x+7);

            det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/625;
            break;
        case 51:
            //51x51 kernel
            pixelSumYY=this->image->getRegionSum(*y-25,*x-14,*y-9,*x+14);
            pixelSumYY+=this->image->getRegionSum(*y+9,*x-14,*y+25,*x+14);
            pixelSumYY+=-2*this->image->getRegionSum(*y-8,*x-14,*y+8,*x+14);

            pixelSumXY=-1*this->image->getRegionSum(*y-17,*x+1,*y-1,*x+17);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-17,*y+17,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-17,*x-17,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+17,*x+17);

            pixelSumXX=this->image->getRegionSum(*y-14,*x-25,*y+14,*x-9);
            pixelSumXX+=this->image->getRegionSum(*y-14,*x+9,*y+14,*x+25);
            pixelSumXX+=-2*this->image->getRegionSum(*y-14,*x-8,*y+14,*x+8);

            det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/1031.123;
            break;
        case 69:
// TODO (zoran#1#):
            //69x69 kernel
            pixelSumYY=this->image->getRegionSum(*y-25,*x-14,*y-9,*x+14);
            pixelSumYY+=this->image->getRegionSum(*y+9,*x-14,*y+25,*x+14);
            pixelSumYY+=-2*this->image->getRegionSum(*y-8,*x-14,*y+8,*x+14);

            pixelSumXY=-1*this->image->getRegionSum(*y-17,*x+1,*y-1,*x+17);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-17,*y+17,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-17,*x-17,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+17,*x+17);

            pixelSumXX=this->image->getRegionSum(*y-14,*x-25,*y+14,*x-9);
            pixelSumXX+=this->image->getRegionSum(*y-14,*x+9,*y+14,*x+25);
            pixelSumXX+=-2*this->image->getRegionSum(*y-14,*x-8,*y+14,*x+8);

            det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/32.1;
            break;
        case 93:
// TODO (zoran#1#):
            //93x93 kernel
            pixelSumYY=this->image->getRegionSum(*y-25,*x-14,*y-9,*x+14);
            pixelSumYY+=this->image->getRegionSum(*y+9,*x-14,*y+25,*x+14);
            pixelSumYY+=-2*this->image->getRegionSum(*y-8,*x-14,*y+8,*x+14);

            pixelSumXY=-1*this->image->getRegionSum(*y-17,*x+1,*y-1,*x+17);
            pixelSumXY+=-1*this->image->getRegionSum(*y+1,*x-17,*y+17,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y-17,*x-17,*y-1,*x-1);
            pixelSumXY+=this->image->getRegionSum(*y+1,*x+1,*y+17,*x+17);

            pixelSumXX=this->image->getRegionSum(*y-14,*x-25,*y+14,*x-9);
            pixelSumXX+=this->image->getRegionSum(*y-14,*x+9,*y+14,*x+25);
            pixelSumXX+=-2*this->image->getRegionSum(*y-14,*x-8,*y+14,*x+8);

            det=_getHessianDeterminant(&pixelSumXX,&pixelSumXY,&pixelSumYY)/32.1;
            break;
    }
    return det;
}

void HessianDetector::_insertToList(int* x, int* y) {
    vector<int> point;
    point.resize(2);
    point[0]=*x;
    point[1]=*y;

    if(this->orderedList.size()>=this->nrPoints) {
        vector<int> tmp=this->orderedList.back();
        if(determinants[*x][*y]<=determinants[tmp[0]][tmp[1]]) return;

        tmp = this->orderedList.front();
        if(determinants[*x][*y]>=determinants[tmp[0]][tmp[1]]) {
                this->orderedList.insert(this->orderedList.begin(),point);
                this->orderedList.pop_back();
                return;
        }
        this->orderedList.pop_back();
    } else {
        if(this->orderedList.size()==0) {
                this->orderedList.push_back(point);
                return;
        }
        vector<int> tmp=this->orderedList.back();
        if(determinants[*x][*y]<=determinants[tmp[0]][tmp[1]]) {
            this->orderedList.push_back(point);
            return;
        }
        tmp = this->orderedList.front();
        if(determinants[*x][*y]>=determinants[tmp[0]][tmp[1]]) {
                orderedList.insert(orderedList.begin(),point);
                return;
        }
    }
     vector<vector<int> >::iterator iter1 = orderedList.begin();
     while( iter1 != orderedList.end()) {
         vector<int > tmp=*iter1;
         if(determinants[tmp[0]][tmp[1]]<=determinants[*x][*y]) {
                orderedList.insert(iter1,point);
                return;
         }
       iter1++;
     }
}
void HessianDetector::printPoints() {
    vector<vector<int> >::iterator iter1 = orderedList.begin();
    //int c=0;
     while( iter1 != orderedList.end()) {
         vector<int > tmp2=*iter1;
            cout << "("<<tmp2[0]<<","<<tmp2[1]<<","<<tmp2[2]<<")\n";
                this->image->drawCircle(tmp2[0],tmp2[1],maximas[tmp2[0]][tmp2[1]]*HD_INIT_KERNEL_SIZE);
                //c++;
                //this->image->drawCircle(tmp2[1],tmp2[0],1);
       iter1++;
     }
      this->image->show();
     //cout << c <<"\n";
}
void HessianDetector::printPoints(std::ostream & o) {
    cout << "Print HD points"<<endl;

    vector<vector<int> >::iterator iter1 = orderedList.begin();
    int n=0;

/*
    while( iter1 != orderedList.end()) {
        n++;
        iter1++;
    }*/
    o << 1 << endl;
    o << this->orderedList.size() << endl;
    iter1 = orderedList.begin();
    //int c=0;
    while( iter1 != orderedList.end()) {
         vector<int > tmp2=*iter1;
         //double r = getMaxima(tmp2[0], tmp2[1])*HD_INIT_KERNEL_SIZE;
         double r = 1;
         o <<tmp2[1]<<" "<<tmp2[0]<<" "<< " " << 1/(r*r) << " " << 0 << " " <<  1/(r*r) << endl;
         iter1++;
     }
    cout << "EOF Print HD points"<<endl;

}

vector<vector<int> >* HessianDetector::getPoints() {
    return &this->orderedList;
}
int HessianDetector::getNrPoints() {
    return this->nrPoints;
}
double HessianDetector::getMaxima(int x, int y) {
    return this->maximas[x][y];
}
double HessianDetector::_getScale(int kernelSize) {
    return kernelSize/HD_INIT_KERNEL_SIZE;
}
void HessianDetector::dump() {
    delete(&determinants);
}
