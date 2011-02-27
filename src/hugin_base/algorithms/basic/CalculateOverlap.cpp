// -*- c-basic-offset: 4 -*-

/** @file CalculateOverlap.cpp
 *
 *  @brief declaration of class to calculate overlap between different images 
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */

/*  This program is free software; you can redistribute it and/or
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

#include "CalculateOverlap.h"

namespace HuginBase {

using namespace hugin_utils;

CalculateImageOverlap::CalculateImageOverlap(const HuginBase::PanoramaData *pano):m_pano(pano)
{
    m_nrImg=pano->getNrOfImages();
    if(m_nrImg>0)
    {
        m_overlap.resize(m_nrImg);
        PanoramaOptions opts=pano->getOptions();
        m_transform.resize(m_nrImg);
        m_invTransform.resize(m_nrImg);
        for(unsigned int i=0;i<m_nrImg;i++)
        {
            m_overlap[i].resize(m_nrImg,0);
            m_transform[i]=new PTools::Transform;
            m_transform[i]->createTransform(*pano,i,opts);
            m_invTransform[i]=new PTools::Transform;
            m_invTransform[i]->createInvTransform(*pano,i,opts);
        };
        // per default we are testing all images
        fill_set(testImages,0,m_nrImg-1);
    };
};

CalculateImageOverlap::~CalculateImageOverlap()
{
    for(unsigned int i=0;i<m_nrImg;i++)
    {
        delete m_transform[i];
        delete m_invTransform[i];
    };
};

void CalculateImageOverlap::calculate(unsigned int steps)
{
    if(testImages.size()==0)
    {
        return;
    };
    for(UIntSet::const_iterator it=testImages.begin(); it!=testImages.end();it++)
    {
        const SrcPanoImage& img=m_pano->getImage(*it);
        vigra::Rect2D c=vigra::Rect2D(img.getSize());
        if(img.getCropMode()!=SrcPanoImage::NO_CROP)
        {
            c&=img.getCropRect();
        };
        unsigned int frequency=std::min<unsigned int>(steps,std::min<unsigned int>(c.width(),c.height()));
        if(frequency<2)
            frequency=2;
        std::vector<unsigned int> overlapCounter;
        overlapCounter.resize(m_nrImg,0);
        unsigned int pointCounter=0;
        for (unsigned int x=0; x<frequency; x++)
        {
            for (unsigned int y=0; y<frequency; y++)
            {
                // scale (x, y) so it is always within the cropped region of the
                // image.
                double xc = double (x) / double (frequency) * double(c.width()) + c.left();
                double yc = double (y) / double (frequency) * double(c.height()) + c.top();
                vigra::Point2D p(xc,yc);
                //check if inside crop, especially for circular crops
                if(img.isInside(p,true))
                {
                    pointCounter++;
                    //transform to panorama coordinates
                    double xi,yi;
                    if(m_invTransform[*it]->transformImgCoord(xi,yi,xc,yc))
                    {
                        //now, check if point is inside an other image
                        for(unsigned int j=0;j<m_nrImg;j++)
                        {
                            if((*it)==j)
                                continue;
                            double xj,yj;
                            //transform to image coordinates
                            if(m_transform[j]->transformImgCoord(xj,yj,xi,yi))
                            {
                                p.x=xj;
                                p.y=yj;
                                if(m_pano->getImage(j).isInside(p,true))
                                {
                                    overlapCounter[j]++;
                                };
                            };
                        };
                    };
                };
            };
        };
        //now calculate overlap and save
        m_overlap[*it][*it]=1.0;
        if(pointCounter>0)
        {
            for(unsigned int k=0;k<m_nrImg;k++)
            {
                if((*it)==k)
                {
                    continue;
                };
                m_overlap[*it][k]=(double)overlapCounter[k]/(double)pointCounter;
            };
        };
    };
};

double CalculateImageOverlap::getOverlap(unsigned int i, unsigned int j)
{
    if(i==j)
    {
        return 1.0;
    }
    else
    {
        return std::max<double>(m_overlap[i][j],m_overlap[j][i]);
    };
};

UIntSet CalculateImageOverlap::getOverlapForImage(unsigned int i)
{
    UIntSet overlapImgs;
    for(unsigned int j=0;j<m_nrImg;j++)
    {
        if(i!=j)
        {
            if(getOverlap(i,j)>0)
            {
                overlapImgs.insert(j);
            };
        };
    };
    return overlapImgs;
};

void CalculateImageOverlap::limitToImages(UIntSet img)
{
    testImages=img;
};

} // namespace
