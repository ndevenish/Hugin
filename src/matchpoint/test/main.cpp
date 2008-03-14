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
#define USE_OPENCV
//#define USE_QT 1
//#define USE_VIGRA 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>

#include <string>
#include "Image.h"
#include "HessianDetector.h"
#include "ANN/ANN.h"

using namespace std;

int main(int argc, char *argv[])
{
    int kernelArray[]={9,11,15,17,21,27};

    double nrPoints=1000;

    string mystr;
    if(argc!=1) {
        cout << "Usage: ./main path_to_image" << "\n";
    	return 0;
    }

    mystr="graffiti/img1.ppm";
    Image im2(mystr);
    if(im2.open()) {
       // cout << "Image opened!!!" << "\n";
    } else {
        cout<< "Error!!!"<<"\n";
        return 0;
    }
    im2.integrate();

    mystr="graffiti/img6.ppm";
    Image im(mystr);
    if(im.open()) {
       //cout << "Image opened!!!" << "\n";
    } else {
        cout<< "Error!!!"<<"\n";
        return 0;
    }
    im.integrate();

    double homography[3][3]= {
    1.0427236e+00, 1.2359858e-02,-1.6974167e+01
    ,-4.2238744e-03, 1.0353397e+00,-4.5312478e+01
    , 1.2020516e-05, 8.2950327e-06, 1.0000000e+00
    };

    int nrNeighbours = 1;   //number of nearest neighbours

	ANNpoint			queryPt;				// query point
	ANNpoint			closestPt;				// closest point to the query point
	ANNidxArray			nnIdx;					// near neighbor indices
	ANNdistArray		dists;					// near neighbor distances

	ANNpointArray		dataPts;				// data points

    ANNkd_tree*	kdTree;

    for(int i=0;i<6;i++) {  //over all possible kernels

    int pointsInRegion=0;

    //detector for the first image
    HessianDetector hd2(&im2,nrPoints);
    if(!hd2.detect(kernelArray[i])) {
    cout << "Detection of points failed!";
    return 1;
    }
    //hd2.printPoints();

    //get detected points
    vector<vector<int> > points1 = hd2.getPoints();

    HessianDetector hd(&im,nrPoints,HD_BOX_FILTERS);
    if(!hd.detect(kernelArray[i])) {
    cout << "Detection of points failed!";
    return 1;
    }
    //hd.printPoints();

    vector<vector<int> > points2 = hd.getPoints();

    vector<int > point;

    dataPts = annAllocPts(nrPoints, 2);			// allocate data points

    int pointCount=0;

    //fill the points array with the resulting points from the detector
    vector<vector<int> >::iterator iter = points2.begin();
     while( iter != points2.end()) {
        point=*iter;
        ANNpoint pTmp= annAllocPt(2);
        pTmp[0]=point[0];
        pTmp[1]=point[1];
        dataPts[pointCount]= pTmp;
        pointCount++;
        //cout << "("<<pTmp[0]<<","<<pTmp[1]<< ","<< ")\n";
       iter++;
     }

    //create a tree of 2 dimension from the points
    kdTree = new ANNkd_tree(dataPts,nrPoints,2);

    vector<double> result;
    double tmp=0;
    double score=0;

    vector<vector<int> >::iterator iter1 = points1.begin();
     while( iter1 != points1.end()) {
         vector<int > point=*iter1;
         if(point[0]>200 && point[0]<400 && point[1]>200 && point[1]<400) {
         //cout << "("<<point[0]<<","<<point[1]<< ","<< point[2] <<")->\n";
            pointsInRegion++;
            result.clear();

            //multiply the pixel with the homography
            for(int i=0;i<3;i++) {
                tmp=0;
                tmp+=point[0]*homography[i][1];
                tmp+=point[1]*homography[i][0];
                tmp+=homography[i][2];
                result.push_back(tmp);
            }
            //cout << "("<<result[1]/result[2]<< ","<<result[0]/result[2]<<","<< result[2]/result[2]<<")\n";

            queryPt = annAllocPt(2);					// allocate query point
            queryPt[1]=result[0]/result[2];
            queryPt[0]=result[1]/result[2];

            //cout << "("<<queryPt[0]<<","<<queryPt[1]<< ","<< ")\n";

			nnIdx = new ANNidx[nrNeighbours];						// allocate near neigh indices
			dists = new ANNdist[nrNeighbours];						// allocate near neighbor dists

            //search for closest point on the second image
			kdTree->annkSearch(						// search
					queryPt,						// query point
					nrNeighbours,					// number of near neighbors
					nnIdx,							// nearest neighbors (returned)
					dists,							// distance (returned)
					0);							    // error bound

			for (int l = 0; l < nrNeighbours; l++) {			// print summary
				dists[l] = sqrt(dists[l]);			// unsquare distance
                closestPt = dataPts[nnIdx[l]];
                //cout << "\t" << "\t(" << closestPt[0] << ","<< closestPt[1]<< ") " << nnIdx[l] << "\t" << dists[l] << "\n";
                //if pixel is closer than 1.5 we have correspondence
                if(dists[l]<1.5) {
                    score++;
                }
			}
         }
         iter1++;
     }
    double repeatability= score/pointsInRegion;
    cout << repeatability <<" ";
    }
    //}
    return EXIT_SUCCESS;
}
