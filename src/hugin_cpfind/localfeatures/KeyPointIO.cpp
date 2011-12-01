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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <iostream>
#include <fstream>
#include <string>

#include "KeyPointIO.h"

using namespace lfeat;
using namespace std;


// extremly fagile check...
static bool identifySIFTKeypoints( const std::string& filename)
{
    ImageInfo info;
    ifstream in(filename.c_str());
    if (!in)
    {
        return false;
    }

    int nKeypoints = 0;
    int dims = 0;
    in >> nKeypoints >> dims;
    return (cin && nKeypoints > 0 && dims >= 0);
}

static ImageInfo loadSIFTKeypoints( const std::string& filename, KeyPointVect_t& vec)
{
    ImageInfo info;
    ifstream in(filename.c_str());
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
        KeyPointPtr k(new lfeat::KeyPoint(0,0,0,0,0));
        in >> k->_y >> k->_x >> k->_scale >> k->_ori >> k->_score;
        if (dims > 0)
        {
            k->allocVector(dims);
            for (int j=0; j < dims; j++)
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
    //std::cerr << "*** Loaded keypoints for image " << info.filename  << " ("<<info.width<<"x"<<info.height<<")"  << std::endl;

    return info;
}

ImageInfo lfeat::loadKeypoints( const std::string& filename, KeyPointVect_t& vec)
{
    if (identifySIFTKeypoints(filename))
    {
        return loadSIFTKeypoints( filename, vec );
    }
    else
    {
        ImageInfo r;
        return r;
    }
}


void SIFTFormatWriter::writeHeader ( const ImageInfo& imageinfo, int nKeypoints, int dims )
{
    _image = imageinfo;
    o << nKeypoints << endl;
    o << dims << endl;
}


void SIFTFormatWriter::writeKeypoint ( double x, double y, double scale, double orientation, double score, int dims, double* vec )
{
    o << y << " " << x << " " << scale << " " << orientation << " " << score;
    for ( int i=0; i < dims; i++ )
    {
        o << " " <<  vec[i];
    }
    o << endl;
}

void SIFTFormatWriter::writeFooter()
{
    o << _image.filename << endl;
    o << _image.width << " " << _image.height << endl;
}


void DescPerfFormatWriter::writeHeader ( const ImageInfo& imageinfo, int nKeypoints, int dims )
{
    _image = imageinfo;
    o << dims << endl;
    o << nKeypoints << endl;
}


void DescPerfFormatWriter::writeKeypoint ( double x, double y, double scale, double orientation, double score, int dims, double* vec )
{
    double sc = 2.5 * scale;
    sc*=sc;
    o << x << " " << y << " " << 1.0/sc << " 0 " << 1.0/sc;
    for ( int i=0; i < dims; i++ )
    {
        o << " " <<  vec[i];
    }
    o << endl;
}

void DescPerfFormatWriter::writeFooter()
{
}


void AutopanoSIFTWriter::writeHeader ( const ImageInfo& imageinfo, int nKeypoints, int dims )
{
    o << "<?xml version=\"1.0\" encoding=\"utf-8\"?>"<< endl;
    o << "<KeypointXMLList xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n";
    o << "  <XDim>"<< imageinfo.width <<"</XDim>"<< endl;
    o << "  <YDim>"<< imageinfo.height <<"</YDim>"<< endl;
    o << "  <ImageFile>"<< imageinfo.filename <<"</ImageFile>"<< endl;
    o << "  <Arr>"<< endl;
}

void AutopanoSIFTWriter::writeKeypoint ( double x, double y, double scale, double orientation, double score, int dims, double* vec )
{
    o << "    <KeypointN>"<< endl;
    o << "      <X>"<< x <<"</X>"<<endl;
    o << "      <Y>"<< y <<"</Y>"<<endl;
    o << "      <Scale>"<< scale <<"</Scale>"<<endl;
    o << "      <Orientation>"<< orientation <<"</Orientation>"<<endl;
    if ( dims > 0 )
    {
        o << "      <Dim>"<< dims <<"</Dim>"<<endl;
        o << "      <Descriptor>"<<endl;

        for ( int i=0; i < dims; i++ )
        {
            o << "        <int>"<< ( vec[i]*256 ) << "</int>" << endl;
        }
        o << "      </Descriptor>"<<endl;
    }
    o << "    </KeypointN>"<< endl;
}

void AutopanoSIFTWriter::writeFooter()
{
    o << "  </Arr>"<< endl;
    o << "</KeypointXMLList>"<< endl;
}


