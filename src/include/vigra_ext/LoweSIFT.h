// -*- c-basic-offset: 4 -*-
/** @file SIFT.h
 *
 *  unix only interface to the keypoints program.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _LOWE_SIFT_H
#define _LOWE_SIFT_H

#include <stdlib.h>

#include <vector>
#include <fstream>

#include <vigra/basicimage.hxx>
#include <vigra/stdimage.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/impex.hxx>

#include <common/math.h>

namespace vigra_ext
{
/** A SIFT feature */
struct SIFTFeature
{
    /** create empty feature */
    SIFTFeature(int dlen=128)
        : scale(0), angle(0), descriptor(dlen) {};

    /** read sift feature from stream (lowe's format) */
    SIFTFeature(std::istream & is, int dlen)
        : descriptor(dlen)
    {
        is >> pos.y >> pos.x >> scale >> angle;
        int x;
        for (int i=0; i < dlen; ++i) {
            is >> x;
            descriptor.push_back((unsigned char)x);
        }
    }
    FDiff2D pos;
    double scale;
    double angle;
    std::vector<unsigned char> descriptor;

    /** compare two features, by calculating the squared distance */
    int match(const SIFTFeature & o) const
    {
        // add scale variance here?
        return utils::sqr_dist(descriptor.begin(), descriptor.end(),
                               o.descriptor.begin(),(int) 0);
    }
};


typedef std::vector<SIFTFeature> SIFTFeatureVector;


/** write sift feature to stream (lowe's format) */
inline std::ostream & operator<<(std::ostream & o, const SIFTFeature & f)
{
    o << f.pos.y << " " << f.pos.x << " " << f.scale << " " << f.angle
      << std::endl;
    for (unsigned int i=0; i < f.descriptor.size(); ++i) {
        o << (int) f.descriptor[i] << " ";
    }
    o << std::endl;
    return o;
}

/** Wrapper around the SIFT keypoint detector program by D. Lowe.
 *
 *  While this code is GPL'ed, the keypoint binary must not be used
 *  for commercial purposes.
 */
template <class SrcIterator, class SrcAccessor>
bool loweDetectSIFT(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> input,
                    std::vector<SIFTFeature> & features)
{
    // write temp pgm image
    // write as JPEG image, using compression quality 80
    vigra::exportImage(input,
                       vigra::ImageExportInfo("/tmp/__keypoints_in.pgm"));

    // run external keypoint detector
    int res = system("keypoints </tmp/__keypoints_in.pgm >/tmp/__keypoints_out.txt");
    if (res == -1) {
        DEBUG_ERROR("keypoints program failed");
        return false;
    }
    // add keypoints to vector
    std::ifstream kp("/tmp/__keypoints_out.txt");
    if (kp.bad()) {
        DEBUG_ERROR("can't read keypoint output");
        return false;
    }
    int nKP, dLen;
    kp >> nKP >> dLen;
    DEBUG_DEBUG( nKP << " features detected. descriptor length: " << dLen);
    for (int i=0; i<nKP; i++) {
        features.push_back(SIFTFeature(kp,dLen));
    }
#ifdef DEBUG
    {
        std::ofstream f("keypoints_read.txt");
        for (int i=0; i<nKP; i++) {
            f << features[i];
        }
    }
#endif
    return true;
}




} // namespace

#endif // _LOWE_SIFT_H
