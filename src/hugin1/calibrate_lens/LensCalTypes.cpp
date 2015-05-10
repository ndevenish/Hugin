// -*- c-basic-offset: 4 -*-

/** @file LensCalTypes.cpp
 *
 *  @brief implementation of helper class for LensCal
 *
 *  @author T. Modes
 *
 */

/* 
 *  This program is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "panoinc_WX.h"
#include "panoinc.h"
#include "LensCalTypes.h"
#include "base_wx/platform.h"

ImageLineList::ImageLineList(wxString newFilename)
{
    SetFilename(newFilename);
    m_edge=new vigra::BImage();
};

ImageLineList::~ImageLineList()
{
    delete m_panoImage;
    if(m_edge)
        delete m_edge;
};

const unsigned int ImageLineList::GetNrOfValidLines()
{
    unsigned int count=0;
    for(unsigned int i=0;i<m_lines.size();i++)
    {
        if(m_lines[i].status==HuginLines::valid_line)
            count++;
    };
    return count;
};

void ImageLineList::SetEdgeImage(vigra::BImage* newEdgeImage)
{
    if(m_edge)
        delete m_edge;
    m_edge=newEdgeImage;
};

vigra::BImage* ImageLineList::GetEdgeImage()
{
    return m_edge;
};

void ImageLineList::SetFilename(wxString newFilename)
{
    m_filename=newFilename;
    std::string filenamestring(newFilename.mb_str(HUGIN_CONV_FILENAME));
    m_panoImage=new HuginBase::SrcPanoImage;
    m_panoImage->setFilename(filenamestring);
    m_panoImage->readEXIF();
    m_panoImage->applyEXIFValues();
    if(m_panoImage->getCropFactor()<=0)
    {
        m_panoImage->readCropfactorFromDB();
    };
    m_panoImage->readProjectionFromDB();
    //reset roll to 0, it could be 90/-90/180 if loaded an image with Exif rotation tag
    m_panoImage->setRoll(0);
    //reset exposure value to prevent exposure correction when calculating corrected preview
    m_panoImage->setExposureValue(0);
};

const wxString ImageLineList::GetFilename()
{
    return m_filename;
};

HuginBase::SrcPanoImage* ImageLineList::GetPanoImage()
{
    return m_panoImage;
};

void ImageLineList::SetLines(HuginLines::Lines lines)
{
    m_lines=lines;
};

const HuginLines::Lines ImageLineList::GetLines()
{
    return m_lines;
};

void ImageLineList::ScaleLines(double scaleFactor)
{
    HuginLines::ScaleLines(m_lines,scaleFactor);
};

