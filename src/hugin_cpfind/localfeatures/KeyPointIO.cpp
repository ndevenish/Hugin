/*
 * Copyright (C) 2009 Pablo d'Angelo
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


#include <iostream>
#include <fstream>
#include <string>

#include "KeyPointIO.h"

namespace lfeat
{
// extremly fagile check...
static bool identifySIFTKeypoints(const std::string& filename)
{
    ImageInfo info;
    std::ifstream in(filename.c_str());
    if (!in)
    {
        return false;
    }

    int nKeypoints = 0;
    int dims = 0;
    in >> nKeypoints >> dims;
    return (in && nKeypoints > 0 && dims >= 0);
}

static ImageInfo loadSIFTKeypoints(const std::string& filename, KeyPointVect_t& vec)
{
    ImageInfo info;
    std::ifstream in(filename.c_str());
    if (!in.good())
    {
        return info;
    }

    int nKeypoints = 0;
    int dims = 0;
    in >> nKeypoints >> dims;

    info.dimensions = dims;

    for (int i = 0; i < nKeypoints; i++)
    {
        KeyPointPtr k(new lfeat::KeyPoint(0, 0, 0, 0, 0));
        in >> k->_y >> k->_x >> k->_scale >> k->_ori >> k->_score;
        if (dims > 0)
        {
            k->allocVector(dims);
            for (int j = 0; j < dims; j++)
            {
                in >> k->_vec[j];
            }
        }
        vec.push_back(k);
    }
    // finish reading empty line
    std::getline(in, info.filename);
    // read line with filename
    std::getline(in, info.filename);
    in >> info.width >> info.height;
    //std::cerr << "*** Loaded keypoints for image " << info.filename  << " ("<<info.width<<"x"<<info.height<<")"  << std::std::endl;

    return info;
}

ImageInfo loadKeypoints(const std::string& filename, KeyPointVect_t& vec)
{
    if (identifySIFTKeypoints(filename))
    {
        return loadSIFTKeypoints(filename, vec);
    }
    else
    {
        ImageInfo r;
        return r;
    }
}


void SIFTFormatWriter::writeHeader(const ImageInfo& imageinfo, int nKeypoints, int dims)
{
    _image = imageinfo;
    o << nKeypoints << std::endl;
    o << dims << std::endl;
}


void SIFTFormatWriter::writeKeypoint(double x, double y, double scale, double orientation, double score, int dims, double* vec)
{
    o << y << " " << x << " " << scale << " " << orientation << " " << score;
    for (int i = 0; i < dims; i++)
    {
        o << " " << vec[i];
    }
    o << std::endl;
}

void SIFTFormatWriter::writeFooter()
{
    o << _image.filename << std::endl;
    o << _image.width << " " << _image.height << std::endl;
}


void DescPerfFormatWriter::writeHeader(const ImageInfo& imageinfo, int nKeypoints, int dims)
{
    _image = imageinfo;
    o << dims << std::endl;
    o << nKeypoints << std::endl;
}


void DescPerfFormatWriter::writeKeypoint(double x, double y, double scale, double orientation, double score, int dims, double* vec)
{
    double sc = 2.5 * scale;
    sc *= sc;
    o << x << " " << y << " " << 1.0 / sc << " 0 " << 1.0 / sc;
    for (int i = 0; i < dims; i++)
    {
        o << " " << vec[i];
    }
    o << std::endl;
}

void DescPerfFormatWriter::writeFooter()
{
}


void AutopanoSIFTWriter::writeHeader(const ImageInfo& imageinfo, int nKeypoints, int dims)
{
    o << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    o << "<KeypointXMLList xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n";
    o << "  <XDim>" << imageinfo.width << "</XDim>" << std::endl;
    o << "  <YDim>" << imageinfo.height << "</YDim>" << std::endl;
    o << "  <ImageFile>" << imageinfo.filename << "</ImageFile>" << std::endl;
    o << "  <Arr>" << std::endl;
}

void AutopanoSIFTWriter::writeKeypoint(double x, double y, double scale, double orientation, double score, int dims, double* vec)
{
    o << "    <KeypointN>" << std::endl;
    o << "      <X>" << x << "</X>" << std::endl;
    o << "      <Y>" << y << "</Y>" << std::endl;
    o << "      <Scale>" << scale << "</Scale>" << std::endl;
    o << "      <Orientation>" << orientation << "</Orientation>" << std::endl;
    if (dims > 0)
    {
        o << "      <Dim>" << dims << "</Dim>" << std::endl;
        o << "      <Descriptor>" << std::endl;

        for (int i = 0; i < dims; i++)
        {
            o << "        <int>" << (vec[i] * 256) << "</int>" << std::endl;
        }
        o << "      </Descriptor>" << std::endl;
    }
    o << "    </KeypointN>" << std::endl;
}

void AutopanoSIFTWriter::writeFooter()
{
    o << "  </Arr>" << std::endl;
    o << "</KeypointXMLList>" << std::endl;
}

} // namespace lfeat