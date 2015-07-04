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


#ifndef __lfeat_KeyPointIO_h
#define __lfeat_KeyPointIO_h

#include <iostream>
#include <string>

#include "KeyPoint.h"
#include "KeyPointDetector.h"

namespace lfeat
{

struct LFIMPEX ImageInfo
{
    ImageInfo()
        : width(0), height(0), dimensions(0)
    { }

    ImageInfo(const std::string& filename, int width, int height)
        : filename(filename), width(width), height(height), dimensions(0)
    { }

    std::string   filename;
    int           width;
    int           height;
    int           dimensions;
};


// functions to read keypoints
//bool identifySIFTKeypoints( const std::string & filename);
//ImageInfo loadSIFTKeypoints( const std::string & filename, KeyPointInsertor & insertor);

ImageInfo LFIMPEX loadKeypoints( const std::string& filename, KeyPointVect_t& insertor);


/// Base class for a keypoint writer
class LFIMPEX KeypointWriter
{

protected:
    std::ostream& o;

public:

    explicit KeypointWriter(std::ostream& out=std::cout)
        : o ( out )
    {
    }

    virtual void writeHeader ( const ImageInfo& imageinfo, int nKeypoints, int dims ) = 0;

    virtual void writeKeypoint ( double x, double y, double scale, double orientation, double score, int dims, double* vec ) = 0;

    virtual void writeFooter() = 0;
};

class LFIMPEX SIFTFormatWriter : public KeypointWriter
{

    ImageInfo _image;

public:
    explicit SIFTFormatWriter(std::ostream& out=std::cout)
        : KeypointWriter(out)
    {
    }

    void writeHeader (const ImageInfo& imageinfo, int nKeypoints, int dims );

    void writeKeypoint ( double x, double y, double scale, double orientation, double score, int dims, double* vec );

    void writeFooter();
};

class LFIMPEX DescPerfFormatWriter : public KeypointWriter
{

    ImageInfo _image;

public:
    explicit DescPerfFormatWriter(std::ostream& out=std::cout)
        : KeypointWriter(out)
    {
    }

    void writeHeader (const ImageInfo& imageinfo, int nKeypoints, int dims );

    void writeKeypoint ( double x, double y, double scale, double orientation, double score, int dims, double* vec );

    void writeFooter();
};


class LFIMPEX AutopanoSIFTWriter : public KeypointWriter
{

public:
    explicit AutopanoSIFTWriter(std::ostream& out=std::cout)
        : KeypointWriter(out)
    {
    }

    void writeHeader ( const ImageInfo& imageinfo, int nKeypoints, int dims );

    void writeKeypoint ( double x, double y, double scale, double orientation, double score, int dims, double* vec );

    void writeFooter();
};

}

#endif
