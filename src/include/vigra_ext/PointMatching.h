// -*- c-basic-offset: 4 -*-
/** @file PointMatching.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This file contains feature point matching functionality.
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

#ifndef _POINTMATCHING_H
#define _POINTMATCHING_H

#include <math.h>

#include <limits.h>
#include <vector>

// BAD: sgi stl extension
//#include <ext/algorithm>

#include "common/utils.h"
#include "common/stl_utils.h"
#include "common/graph.h"

#include <vigra/resizeimage.hxx>
#include "vigra_ext/LoweSIFT.h"

#include "PT/PanoramaMemento.h"


namespace vigra_ext
{

typedef std::vector<SIFTFeatureVector> SIFTFeatureTable;

typedef vigra::triple<int, int, int> SIFTMatch;
typedef std::vector<SIFTMatch> SIFTMatchVector;
typedef std::vector<SIFTMatchVector> SIFTMatchTable;
// for complete match results, uses the following, compact format:
/// [img1][img2-img1-1][nFeature] = match, \forall img1 < img2
typedef std::vector<SIFTMatchTable> SIFTMatchMatrix;

struct SIFTMatchDistanceLower : public std::binary_function<SIFTMatch, SIFTMatch, bool>
{
    bool operator()(const SIFTMatch & a,
                    const SIFTMatch & b)
        { return a.first < b.first; }
};

/** match a set of images, using a set of features.
 *
 *  This is the simple feature matcher from lowe's example,
 *  finds the two best matches, returns the features, if
 *  the match distance between these two is at least 0.6 times
 *  bestmatchVal apart.
 *
 *
 *  @param feat1 features of Image 1
 *  @param feat2 features of Image 2
 *  @return feature pairs
 *
 */
template<class Feature>
bool MatchImageFeatures(const std::vector<Feature> & feat1,
                        const std::vector<Feature> & feat2,
                        std::vector<vigra::triple<int, Feature, Feature> > & result)
{
    DEBUG_DEBUG("Matching features nFeatures: " << feat1.size() << ", " << feat2.size());
    for (typename std::vector<Feature>::const_iterator it1 = feat1.begin();
         it1 != feat1.end(); ++it1)
    {
        int dist1=INT_MAX, dist2=INT_MAX;
        const Feature * best = 0;
        for (typename std::vector<Feature>::const_iterator it2 = feat2.begin();
             it2 != feat2.end(); ++it2)
        {
            int d = it1->match(*it2);
            if (d < dist1) {
                dist2 = dist1;
                dist1 = d;
                best = &(*it2);
            } else if (d < dist2) {
                dist2 = d;
            }
        }
        // add to list
        if (10*10* dist1 < 6*6*dist2 && best != 0) {
            result.push_back(make_triple(dist1, *it1, *best));
        }
    }

    // if less than 10 % of all features where tracked, return false...
    if  (result.size() < feat1.size() / 15) {
        DEBUG_DEBUG("Only " << result.size() << " features found");
        return false;
    }
    return true;
}

/** match a set of images, using a set of features.
 *
 *  This is the simple feature matcher from lowe's example,
 *  finds the two best matches, returns the features, if
 *  the match distance between these two is at least 0.6 times
 *  bestmatchVal apart.
 *
 *  The feature matching routine is part of the feature class
 *  and should return a squared difference measure.
 *
 *  @param feat1 features of Image 1
 *  @param feat2 features of Image 2
 *  @param result vector with all matching features:
 *                triple<sqrdiff, feat1idx, feat2idx>
 *
 */
struct MatchFeatures
{
    bool operator()(const std::vector<SIFTFeature> & feat1,
                    const std::vector<SIFTFeature> & feat2,
                    SIFTMatchVector & result)
    {

        DEBUG_DEBUG("Matching features nFeatures: " << feat1.size() << ", " << feat2.size());
        for (SIFTFeatureVector::const_iterator it1 = feat1.begin();
             it1 != feat1.end(); ++it1)
        {
            int dist1=INT_MAX, dist2=INT_MAX;
            int best = 0;
            for (SIFTFeatureVector::const_iterator it2 = feat2.begin();
                 it2 != feat2.end(); ++it2)
            {
                int d = it1->match(*it2);
                if (d < dist1) {
                    dist2 = dist1;
                    dist1 = d;
                    best = it2 - feat2.begin();
                } else if (d < dist2) {
                    dist2 = d;
                }
            }
            // add to list
            if (10*10* dist1 < 6*6*dist2 && best != 0) {
                result.push_back(vigra::make_triple(dist1, it1 - feat1.begin(), best));
            }
        }
        // if less than 7.5 % of all features where tracked, return false...
        if  (result.size() < feat1.size() / 15) {
            DEBUG_DEBUG("Only " << result.size() << " features found");
            return false;
        } else {
            DEBUG_DEBUG(result.size() << " matching features found");
        }
        return true;
    }
};

/** random sampled matching of features.
 *
 *  This picks a number of random features from feat1
 *  and tries to match them agains feat2.
 */
template<class Matcher>
struct RandomSampledMatcher
{
    RandomSampledMatcher(Matcher & matcher, int percent=10 )
    : m_percent(percent), match(matcher)
        { }

    bool operator()(const std::vector<SIFTFeature> & feat1,
                    const std::vector<SIFTFeature> & feat2,
                    SIFTMatchVector & result)
        {
            std::vector<SIFTFeature> randomfeat(feat1.size()*m_percent/100);
            __gnu_cxx::random_sample(feat1.begin(), feat1.end(),
                               randomfeat.begin(), randomfeat.end());
            return match(randomfeat, feat2, result);
        }
    int m_percent;
    Matcher & match;
};

/**
 *  does a simple O(n^2*m^2) (n=images, m=nFeatures/image) search for
 *  connected images, returns matching features, and a graph
 *  of the connected images.
 */
template<class MatchingFunctor>
void exhaustiveSiftMatching(const SIFTFeatureTable & ftable,
                                       MatchingFunctor & match,
                                       SIFTMatchMatrix & matches,
                                       utils::AdjListGraph & connected,
                                       utils::MultiProgressDisplay & pdisp)
{
    int nImg = ftable.size();
    connected.clear();
    connected.resize(nImg);
    matches.clear();
    matches.resize(nImg);
    SIFTFeatureTable::const_iterator it1;
    pdisp.pushTask(utils::ProgressTask("exhaustive Matching","",1.0/ftable.size()));
    for (it1 = ftable.begin(); it1 != ftable.end(); ++it1) {
        int first = it1 - ftable.begin();
        matches[first].clear();
        matches[first].resize(nImg - first-1);
        SIFTFeatureTable::const_iterator it2;
        pdisp.increase();
//        pdisp.progressMessage("exhaustive SIFT matching",
//                              100 * ((nImg * (nImg -1) / 2) - ((nImg - first) * ((nImg - first) -1) / 2))/(nImg * (nImg -1) / 2));
        for (it2 = it1+1; it2 != ftable.end(); ++it2) {
            int second = it2 - ftable.begin();
            DEBUG_DEBUG("matching " << first << ", " << second);
            // resulting features
            if (match(ftable[first], ftable[second], matches[first][second-first-1])) {
                // remember connected
                DEBUG_DEBUG("Connection " << first << " and " << second)
                connected[first].push_back(second);
                connected[second].push_back(first);
            }
        }
    }
}


/** run sift feature detector on a bunch of images */
void extractSIFT(const std::vector<std::string> & imgfiles,
                 double scale,
                 SIFTFeatureTable &ftable,
                 utils::MultiProgressDisplay & pdisp,
                 const std::string & keypoints);

/** extract two sift feature sets */
void extractSIFT2(const std::vector<std::string> & imgfiles,
                  double scale1,
                  SIFTFeatureTable & ftable1,
                  double scale2,
                  SIFTFeatureTable & ftable2,
                  utils::MultiProgressDisplay & pdisp,
                  const std::string & keypoints);


/** Perform SIFT feature matching on the visited links, remember
 *  good matches
 */
struct SIFTMatchingVisitor
{
    SIFTMatchingVisitor(std::map<int,int> &nodeImgMapping,
//                        MatchingFunctor & match,
                        const SIFTFeatureTable & ft, double scaling,
                        int nFeatures,
                        utils::MultiProgressDisplay & pdisp)
        :  m_NImap(nodeImgMapping), m_ft(ft), m_scale(scaling),
           m_nFeatures(nFeatures), m_pdisp(pdisp)
        {
        }

    void operator()(int vert1, int vert2)
        {
            SIFTMatchVector matches;
            std::ostringstream os;
            if (m_match(m_ft[vert1], m_ft[vert2], matches)) {
                os << "Found " << matches.size() << " points between images " << vert1 << " and " << vert2;
                m_pdisp.setMessage(os.str());
//                m_pdisp.progressMessage(os.str());

                // sort matches by distance..
                std::sort(matches.begin(), matches.end(),SIFTMatchDistanceLower());
                // copy once more...
                for (SIFTMatchVector::iterator mit = matches.begin();
                     mit != matches.end() && mit - matches.begin() < m_nFeatures;
                     ++mit)
                {
                    m_CtrlPnts.push_back(PT::ControlPoint(m_NImap[vert1],
                                                      m_ft[vert1][mit->second].pos.x * m_scale,
                                                      m_ft[vert1][mit->second].pos.y * m_scale,
                                                      m_NImap[vert2],
                                                      m_ft[vert2][mit->third].pos.x * m_scale,
                                                      m_ft[vert2][mit->third].pos.y * m_scale)
                        );
                }
            } else {
                os << "No matches between images " << vert1 << " and " << vert2;
                m_pdisp.setMessage(os.str());
                //m_pdisp.progressMessage(os.str());
            }
        }

    const PT::CPVector & getCtrlPoints()
        { return m_CtrlPnts; }

    std::map<int,int> m_NImap;
    const SIFTFeatureTable & m_ft;
    PT::CPVector m_CtrlPnts;
    double m_scale;
    int m_nFeatures;
    MatchFeatures m_match;
    utils::MultiProgressDisplay &m_pdisp;
};


} // namespace

#endif // _POINTMATCHING_H
