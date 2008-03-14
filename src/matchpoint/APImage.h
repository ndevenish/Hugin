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
#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED
#define USE_VIGRA

#include <string>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>

#ifdef USE_OPENCV
#include "cv.h"
#endif

#ifdef USE_VIGRA
#include "vigra/stdimage.hxx"
#include "vigra/imageinfo.hxx"
#include "vigra/impex.hxx"
#include "vigra/stdimagefunctions.hxx"
#include "edgedetection.hxx"
#include "vigra/utilities.hxx"
#include "vigra/numerictraits.hxx"

#include "vigra/recursiveconvolution.hxx"
#include "vigra/separableconvolution.hxx"
#include "vigra/labelimage.hxx"
#include "vigra/mathutil.hxx"
#include "vigra/pixelneighborhood.hxx"
#include "vigra/linear_solve.hxx"
#endif


using namespace std;
 	using namespace vigra;

class APImage
 {

    public:
    APImage(string p);
    void convolute(int* kernel,int dim1, int dim2,double scale);
    string getPath();
    int getWidth();
    int getWidthBW();
    int getHeight();
    int getHeightBW();
    int getPixel(int x, int y);
    int getIntegralPixel(int x,int y);
    void scale(double factor);
    APImage* getCopy();
    void drawCircle(int x,int y, int radius);
    void drawLine(int x1,int y1, int x2,int y2);
    void drawRectangle(int x,int y, int radius);
    void smooth();
    void integrate();
    int getRegionSum(int x1, int y1, int x2, int y2);

    bool open();
    void show();
    void test();

    template <class SrcIterator, class SrcAccessor, class BackInsertable>
    void _cannyEdgelList1(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> src,
               BackInsertable & edgels, double scale, vector<int>* p);


#ifdef USE_VIGRA
 	vigra::BImage* imgBW;
#endif

/* private slots:
*/

 	private:
 	/**
 	* Holds the convolution of the image.
 	*/
 	vector<vector<int> > convolution;
    /**
 	* Holds the values of the integral image.
 	*/
 	vector<vector<int> > integral;
 	string path;
 	int _getValue4Integral(int x, int y);

    template <class SrcIterator, class SrcAccessor, class BackInsertable>
    inline void _cannyEdgelList(SrcIterator ul, SrcIterator lr, SrcAccessor src,
                            BackInsertable & edgels, double scale, vector<int>* point);

    template <class Image1, class Image2, class BackInsertable>
    void _internalCannyFindEdgels(Image1 const & gx,
                             Image1 const & gy,
                             Image2 const & magnitude,
                             BackInsertable & edgels, vector<int>* p);

 	/*IplImage* img;
 	IplImage* imgBW;*/

 };


#endif // IMAGE_H_INCLUDED
