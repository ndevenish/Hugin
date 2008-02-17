// -*- c-basic-offset: 4 -*-
/** @file FeatureMatcher.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Panorama.h 1947 2007-04-15 20:46:00Z dangelo $
 *
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

#include "FeatureMatcher.h"

#include <panodata/PanoramaData.h>
#include <hugin_math/hugin_math.h>
#include <ANN/ANN.h>


namespace HuginBase {

using namespace std;
using namespace hugin_utils;

// TODO: use upper_bound, check if a partial evaluation makes sense.
float sqdistance(const Keypoint & p1, const Keypoint & p2)
{
    float sum=0;
    std::vector<float>::const_iterator it2 = p2.descriptor.begin();
    for(std::vector<float>::const_iterator it1 = p1.descriptor.begin();
        it1 != p1.descriptor.end(); ++it1, ++it2)
    {
        float d1 = *it1 - *it2;
        float d2 = *(it1+1) - *(it2+1);
        d1 *= d1;
        d2 *= d2;
        sum = sum + d1 + d2;
    }
    return sum;
}


// wrapper around the KDTree, tailored for keypoint matching
class KDTreeKeypointMatcher
{
public:
    KDTreeKeypointMatcher() 
    : m_KDTree(0), m_allPoints(0), m_k(5)
    {
        m_nnIdx = new ANNidx[m_k];
        m_dists = new ANNdist[m_k];
    };

    ~KDTreeKeypointMatcher()
    {
        if (m_KDTree)
            delete m_KDTree;
        if (m_allPoints)
            delete[] m_allPoints;
        delete[] m_nnIdx;
        delete[] m_dists;
    }

    /** create KDTree from the feature descriptors stored in the panorama.
     *
     * @param pano Panorama from which the keypoints are taken.
     * @param images Only use keypoints from the specified images.
     *
     */
    void create(const PanoramaData & pano, const UIntSet & imgs)
    {
//        typedef *ANNcoord ANNPoint;
        int nKeypoints=0;
        int dim=0;
        // create KDTree
        // first, count how many descriptors we have
        for (UIntSet::const_iterator it=imgs.begin(); it != imgs.end(); ++it) {
            const std::vector<Keypoint> & keypoints = pano.getImage(*it).getKeypoints();
            nKeypoints += keypoints.size();
            if (keypoints.size() > 0)
                dim = keypoints[0].descriptor.size();
        }
        assert(dim > 0);

        // prepare a big list of points, and insert all points
        if (m_allPoints) delete[] m_allPoints;
        m_allPoints = new ANNpoint[nKeypoints];
        ANNpointArray pointsPtr = m_allPoints;
        for (UIntSet::iterator it=imgs.begin(); it != imgs.end(); ++it) {
            const std::vector<Keypoint> & keypoints = pano.getImage(*it).getKeypoints();
            int j=0;
            for (vector<Keypoint>::const_iterator itkey=keypoints.begin();
                itkey != keypoints.end(); ++itkey) 
            {
                m_keypoints.push_back(ImageKeypoint(*it, j, *itkey));
                *pointsPtr = &(*(m_keypoints.back().keypoint.descriptor.begin()));
                pointsPtr++;
                j++;
            }
        }

        // options: match distance.
        // create KDTree
        m_KDTree = new ANNkd_tree (m_allPoints,              // the data points
                                   nKeypoints,             // number of points
                                   dim);                   // dimension of space
    }

    // match a single keypoint
    std::vector<ImageKeypoint> match(const Keypoint & key, unsigned imageOfKeypoint)
    {

        std::vector<ImageKeypoint> ret;
        int nKeypoints = m_keypoints.size();
        int searchDepth = std::max(200, hugin_utils::roundi(log(nKeypoints)/log(1000)*130));
        DEBUG_DEBUG("search depth: " << searchDepth);
        annMaxPtsVisit(searchDepth);
        // perform nearest neighbour matching
        ANNcoord * acord = const_cast<float *>(&(*(key.descriptor.begin())));
        m_KDTree->annkPriSearch(acord,
                                m_k,                   // number of near neighbors
                                m_nnIdx,             // nearest neighbors (returned)
                                m_dists,             // distance (returned)
                                0.0);                // error bound

        // check for nearest neighbours, using 0.6 rule
        UIntSet usedImages;
        // maximum distance of the second match, before the first one is accepted.
        double minGoodDist = 1.6*m_dists[0];
        if (m_keypoints[m_nnIdx[0]].imageNr == imageOfKeypoint) {
            // nearest neighbour in same image, no matches
            return ret;
        }
        usedImages.insert(m_keypoints[m_nnIdx[0]].imageNr);
        for (int i=1; i < m_k; i++) {
            // TODO: is 0.6 really the right ratio threshold?
            if (set_contains(usedImages, m_keypoints[m_nnIdx[i]].imageNr) ) {
                // we have already found a point in this image
                // do ratio distance check to first match
                if (m_dists[i] < minGoodDist) {
                    // close descriptor in same image found. No good matches
                    return ret;
                } else {
                    // seems to be a good match, next keypoint in
                    // same image is very far away.
                    // TODO: check consistency of other matches, too?
                    if (set_contains(usedImages, imageOfKeypoint)) {
                        // a self match (both points in the same image)
                        // has been found. This is probably due tu a
                        // repetetive or ambigous image structure. 
                        // Don't return any matches.
                        return ret;
                    }
                    for (int j=0; j < i; j++) {
                        ret.push_back(m_keypoints[m_nnIdx[i]]);
                    }
                    return ret;
                }
            } else {
                // this image has not been seen before, if it is
                // close to the best match, it and all closer neighbours
                // might be good matches.
                // Or all are bad matches.
                if (m_dists[i] < minGoodDist) {
                    // maybe all matches good or all matches bad
                    // leave decision for later
                } else {
                    // this descriptor is very far away, and not in any image seen before
                    // the previous nearest neighbours are probably good
                    // matches.
                    if (set_contains(usedImages, imageOfKeypoint)) {
                        // a self match (both points in the same image)
                        // has been found. This is probably due tu a
                        // repetetive or ambigous image structure. 
                        // Don't return any matches.
                        return ret;
                    }
                    for (int j=0; j < i; j++) {
                        ret.push_back(m_keypoints[m_nnIdx[i]]);
                    }
                    return ret;
                }
            }
            usedImages.insert(m_keypoints[m_nnIdx[i]].imageNr);
        }
        // All matches good or all matches bad. Need to check more
        // nearest neighbours. Be conservative and report no matches
        return ret;
    }

    int getKeypointIdxOfMatch(unsigned matchNr) const
    {
        assert(matchNr < m_k);
        return (m_nnIdx[matchNr]);
    }

private:
    ANNkd_tree * m_KDTree;

    /// vector to hold all descriptors
    vector<ImageKeypoint> m_keypoints;

    /// array with pointers to all keypoint descriptors
    ANNpointArray m_allPoints;

    // query results
    ANNidxArray m_nnIdx;
    ANNdistArray m_dists;

    /// number of nearest neighbours
    unsigned m_k;
};

/** Perform nearest neighbour matching using KDTree.
 *  This returns the raw results from matching, suitable
 *  for later postprocessing (RANSAC, area filtration etc)
 *
 *  All keypoints contained in @p srcImages are matched against
 *  the keypoints of @p destImages.
 */
std::vector<std::vector<ImageKeypoint> >
matchKeypoints(const PanoramaData& pano, UIntSet srcImgs, UIntSet destImgs)
{
    std::vector<std::vector<ImageKeypoint> > allMatches;

    // prepare kd tree with nearest neighbours
    KDTreeKeypointMatcher matcher;
    matcher.create(pano, destImgs);

    // keep track of matched points, to avoid re-matching them.
    // TODO: might be useful to use bidirectional matches
    // a double check for valid matches
    // given that only an approximate KDTree search is used.
    UIntSet matchedPoints;

    CPVector cps;
    for (UIntSet::iterator it=srcImgs.begin(); it != srcImgs.end(); ++it) {
        const std::vector<Keypoint> & keypoints = pano.getImage(*it).getKeypoints();
        int pnr = 0;
        for (vector<Keypoint>::const_iterator itkey=keypoints.begin();
             itkey != keypoints.end(); ++itkey)
        {
//            if (map_contains(pointMatched, *it) ) 
            // do not try to match this point, if it has already been matched.
            if (set_contains(matchedPoints, pnr)) continue;

            vector<ImageKeypoint> matches = matcher.match(*itkey, *it);
            if (matches.size() > 0)
                allMatches.push_back(matches);
            // mark matched points and remove them from further matching.
            for (unsigned j=0; j < matches.size(); j++) {
                // mark controlpoint as matched.
                matchedPoints.insert(matcher.getKeypointIdxOfMatch(j));
                /*
                cps.push_back(ControlPoint(*it, itkey->pos.x, itkey->pos.y,
                                            matches[j].imageNr, matches[j].keypoint.pos.x,
                                            matches[j].keypoint.pos.x));
                */
            }
        }
    }
    return allMatches;
}

} // namespace


