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
//#define USE_OPENCV
//#define USE_QT
#define USE_VIGRA

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <ctime>
#include <algorithm>

#include <string>
#include "APImage.h"
#include "HessianDetector.h"
#include "Descriptor.h"
#include <limits>

using namespace std;



double getDistance(vector<double>* d1,vector<double>* d2) {
double dist=0;
for(int i=0; i<DESCRIPTOR_SIZE;i++) {
    dist+=pow((*d1)[i]-(*d2)[i],2);
}
return dist;
}

int main(int argc, char *argv[])
{

    clock_t start,finish;
    double time;

    int nrPoints=250;
    if(argc!=3) {
        cout << "Usage: ./main img1 img2 " << "\n";
    	return 0;
    }
    //APImage im1("trees/img1.ppm");
    APImage im1(argv[1]);
    if(!im1.open()) {
        cout<< "Error! APImage can not be opened"<<"\n";
        return 0;
    }

    APImage im2(argv[2]);
    if(!im2.open()) {
        cout<< "Error! APImage can not be opened"<<"\n";
        return 0;
    }

    //need to integrate the image before the detection process(only if using box filter approximation)
    im1.integrate();
    im2.integrate();

    start = clock();

    HessianDetector hd1(&im1,nrPoints, HD_BOX_FILTERS,1);
    if(!hd1.detect()) {
    cout << "Detection of points failed!";
    return 1;
    }
    //print points and display image with the detected points(debugging only)
    //hd1.printPoints(cout);

    finish = clock();

    time = (double(finish)-double(start))/CLOCKS_PER_SEC;
    cout << "Measured time:"<<time<<"\n";


    vector<vector<int> >* interestPoints1=hd1.getPoints();

    Descriptor d1(&im1,&hd1);
    d1.setPoints(interestPoints1);
    //d.orientate();
    d1.createDescriptors();
    //d1.generateAutopanoXML("test3.xml");
    /*d1.printDescriptors(cout);
    return 0;*/

    HessianDetector hd2(&im2,nrPoints, HD_BOX_FILTERS,1);
    if(!hd2.detect()) {
    cout << "Detection of points failed!";
    return 1;
    }
    //print points and display image with the detected points(debugging only)
    //hd2.printPoints();


    vector<vector<int> >* interestPoints2=hd2.getPoints();

    Descriptor d2(&im2,&hd2);
    d2.setPoints(interestPoints2);
    //d.orientate();
    d2.createDescriptors();
    /*d2.generateAutopanoXML("test4.xml");
    return 0;*/

    vector<vector<double> >* descriptors1=d1.getDescriptors();
    vector<vector<double> >::iterator iter1 = (*descriptors1).begin();

    vector<vector<double> >* descriptors2=d2.getDescriptors();

    int pointCount=0;
    int max;
    int maxId;  //position int the vector of the nearest neighbour

     while( iter1 != (*descriptors1).end()) {
        vector<double> current1 = *iter1;

        vector<vector<double> >::iterator iter2 =(*descriptors2).begin();

        max=1;
        maxId=0;

        vector<double> current2 = *iter2;

        double distance=getDistance(&current1,&current2);
        iter2++;
        double diff=distance;   //nearest neighbour
        double diff2=std::numeric_limits<double>::max();    //second nearest neighbour

        while( iter2 != (*descriptors2).end()) {
            current2 = *iter2;
            distance=getDistance(&current1,&current2);

            if(distance<diff) {
                diff2=diff;
                diff=distance;
                maxId=max;
            } else if(distance<diff2) {
                //set the new second nearest neighbour
                diff2=distance;
            }
            max++;
            iter2++;
        }
        if(diff<(0.5*diff2)) {
            im1.drawCircle((*interestPoints1)[pointCount][1],(*interestPoints1)[pointCount][0],10);
            im2.drawCircle((*interestPoints2)[maxId][1],(*interestPoints2)[maxId][0],10);
        }

        iter1++;
        pointCount++;
     }
     im1.show();
     im2.show();
    return EXIT_SUCCESS;
}
