// -*- c-basic-offset: 4 -*-

/** @file preprocessor.cpp
 *
 *  @brief uses angular distance measurements to select
 *         corrosponding points.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>
#include <fstream>
#include <sstream>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>

extern "C" {
#include "klt/klt.h"
}

#include <unistd.h>

#include "panoinc.h"

using namespace vigra;
//using namespace vigra_ext;
using namespace PT;
using namespace std;
using namespace utils;

namespace AngularMatching
{

/** A histogram container class.
 *
 *  a bit like a multiset
 */
template <class TYPE>
struct ValueHistogram : public std::vector<vector<TYPE> >
{

    typedef typename vector<vector<TYPE> >::iterator MYIT;
    typedef typename std::vector<vector<TYPE> > Base;

    ValueHistogram(int nBins, double start, double end)
        : std::vector<vector<TYPE> >(nBins), m_start(start), m_end(end)
    {
        m_width = (end - start)/nBins;
    }

    // remove all entries
    void clearHist()
    {
        for (MYIT it = Base::begin();
             it != Base::m_vector.end(); ++it) {
            it->clear();
        }
    }

    void insertHist(double val, const TYPE & t)
    {
        int idx = getIdx(val);
        Base::operator[](idx).push_back(t);
    }

    vector<double> getCount()
    {
        vector<double> ret(Base::m_vector.size());
        for (typename vector<vector<TYPE> >::iterator it = Base::m_vector.begin();
             it != Base::m_vector.end(); ++it) {
            ret.push_back(it->size());
        }
        return ret;
    }

    const vector<TYPE> & getBinEntries(double val)
    {
        int idx = getIdx(val);
        Base::m_vector[idx].push_back(Base::t);
    }

    double getCenter(int bin)
    {
        return m_start + bin*m_width + m_width/2;
    }

    const vector<TYPE> & getMax(unsigned int & count)
    {
        vector<TYPE> * ret=0;
        count = 0;
        for (typename vector<vector<TYPE> >::iterator it = Base::begin();
             it != Base::end(); ++it)
        {
            if (count < it->size()){
                count = it->size();
                ret = &(*it);
            }
        }
        return *ret;
    }

    void printHist(ostream & o)
    {
        unsigned int count;
        //const vector<TYPE> & maxval = getMax(count);
        getMax(count);
        o << "Histogram with " << Base::size() << " bins, range: " << m_start
          << " .. " << m_end << endl
          << "Center Count=========================================================" << endl;
        for (typename vector<vector<TYPE> >::iterator it = Base::begin();
             it != Base::end(); ++it)
        {
            o << getCenter(it - Base::begin())
              << " ";
            o << it->size() << "  ";
            int fw = roundi(60* ((double)it->size()/count));
            for (int i=0; i < fw; i++) {
                o << "#";
            }
            o << endl;
        }
    }

private:
    // return bin idx for this type
    int getIdx(double val)
    {
        assert(val >= m_start);
        assert(val <= m_end);
        return roundi((val - m_start + m_width/2)/m_width) - 1;
    }

    double m_start;
    double m_end;
    double m_width;
};


// distance between two points
struct Line
{
    Line(int p1, int p2, double dist)
        : p1(p1), p2(p2), dist(dist)
        { }

    /// point numbers
    int p1;
    int p2;
    /// distance
    double dist;
};

struct shorterLine : public binary_function<Line, Line, bool> {
    bool operator()(const Line & x, const Line &  y)
        { return x.dist < y.dist; }
};

// calculate the spherical distance between two points p
double calcSphericalDist(const FDiff2D & p1, const FDiff2D &p2)
{
    // calc latitude from colatitude
    double lat1 = M_PI/2 - p1.y;
    double lat2 = M_PI/2 - p2.y;
    // longitude
    double dlon = p2.x - p1.x;
    double dlat = lat2 - lat1;
    double a = (sin(dlat/2))*(sin(dlat/2)) + cos(lat1) * cos(lat2) * (sin(dlon/2))*(sin(dlon/2));
    return 2.0 * asin(min(1.0,sqrt(a)));
}

/** calculate angle between AB and AC
 *
 *  see http://mathworld.wolfram.com/SphericalTrigonometry.html
 *
 *  for formulas and definitions.
 *  \p a, \p b, \p c are the angular length of the
 *  sides opposite to points \p A, \p B, \p C
 *
 *  This may be ill conditioned if a,b,c are very small.
 *  but should be ok for our cases.
 */
double calcDihedralAngle(double a, double b, double c)
{
    return acos( (cos(a) - cos(b)*cos(c)) / (sin(b)*sin(c)));
}


/** Triangle.
 *
 *  structure with 3 points \p points,
 *  and associated distances \p dist
 *
 *  point distance order: dist12, dist23, dist31
 *
 *  distances are sorted, dist12 <= dist23 <= dist31
 *
 *  normalize() can be used to reorder the point to confirm
 *  to this measure
 *
 */
struct Triangle
{
    /** create a triangle */
    Triangle(int p1, double d12,
             int p2, double d23,
             int p3, double d31)
        {
            points[0] = p1;
            points[1] = p2;
            points[2] = p3;
            dist[0] = d12;
            dist[1] = d23;
            dist[2] = d31;

            normalize();
        }
    // point numbers (indidies)
    int points[3];
    // point distances. order: dist12, dist23, dist31
    double dist[3];

    // safety function
    void Triangle::assertDistances(const vector<FDiff2D> & epoints)
    {
        double d12 = calcSphericalDist(epoints[points[0]], epoints[points[1]]);
        double d23 = calcSphericalDist(epoints[points[1]], epoints[points[2]]);
        double d13 = calcSphericalDist(epoints[points[0]], epoints[points[2]]);
        if (d12 - dist[0] > 1e-9) {
            DEBUG_ERROR("d12 != dist[0]:  " << d12 << " != " << dist[0]);
        }
        if (d23 - dist[1] > 1e-9) {
            DEBUG_ERROR("d23 != dist[1]:  " << d23 << " != " << dist[1]);
        }
        if (d13 - dist[2] > 1e-9) {
            DEBUG_ERROR("d13 != dist[2]:  " << d13 << " != " << dist[2]);
        }
    }

    bool bad(double margin)
        {
//            double avgdist = (dist[0] + dist[1] + dist[2])/3;
            bool bad = false;

            // triangles with same points (or invalid combinations)
            if (dist[0] <= 0.0 || dist[1] <= 0.0 || dist[2] <= 0.0 ) {
                return true;
            }

            // flat triangles are bad
            // a triangle is flat when two lengthes are the same as the third.
            if (dist[0] > dist[1] && dist[0] > dist[2]) {
                if (dist[1] + dist[2] < dist[0] + 5 * margin) bad = true;
            } else if (dist[1] > dist[0] && dist[1] > dist[2]){
                if (dist[0] + dist[2] < dist[1] + 5 * margin) bad = true;
            } else {
                if (dist[0] + dist[1] < dist[2] + 5 * margin) bad = true;
            }
#ifdef DEBUG
            if (bad) {
//                DEBUG_DEBUG("flat triangle");
            }
#endif
            if (bad) return true;

            // symmetrical triangles make it hard to determine
            // keep p1, p2 and p3 appart
            // check for similar lengths
            if (fabs(dist[0] - dist[1]) < margin*5) bad = true;
            if (fabs(dist[0] - dist[2]) < margin*5) bad = true;
            if (fabs(dist[1] - dist[2]) < margin*5) bad = true;
#ifdef DEBUG
            if (bad) {
//                DEBUG_DEBUG("symmetric triangle");
            }
#endif

            return bad;
        }

    /** sort by point number indicies */
    bool operator<(const Triangle & o) const
        {
            if (points[0] < o.points[0])
                return true;
            if (points[1] < o.points[1])
                return true;
            if (points[2] < o.points[2])
                return true;
            return false;
        }

#if 0
    /** triangle comparison operator. triangles are compared by their
     *  circumfence.
     */
    bool operator<(const Triangle & o) const
        {
            return dist[0]+dist[1]+dist[2] < o.dist[0]+dist[1]+dist[2];
        }
#endif

    /** normalize the triangle, enforce distance sort order, and move
     *  point accordingly
     */

    void normalize()
        {
            vector<Line> v;
            // lines
            v.push_back(Line(points[0], points[1], dist[0]));
            v.push_back(Line(points[1], points[2], dist[1]));
            v.push_back(Line(points[2], points[0], dist[2]));

            // sort lines by length
            sort(v.begin(), v.end(), shorterLine());

            dist[0] = v[0].dist;
            dist[1] = v[1].dist;
            dist[2] = v[2].dist;


            // p1 is point in common between lines v[0] and v[2]
            // p2 between lines 0 1
            // p3 between lines 1 2

            if (v[0].p1 == v[2].p1) {
                points[0] = v[0].p1;
                // p2 must the other end
                points[1] = v[0].p2;
                // and p3 is at the other end of v[2]
                points[2] = v[2].p2;
            } else if (v[0].p1 == v[2].p2) {
                points[0] = v[0].p1;
                // p2 must the other end
                points[1] = v[0].p2;
                // and p3 is at the other end of v[2]
                points[2] = v[2].p1;

            } else if (v[0].p2 == v[2].p1) {
                points[0] = v[0].p2;
                // p2 must the other end
                points[1] = v[0].p1;
                // and p3 is at the other end of v[2]
                points[2] = v[2].p2;
            } else if (v[0].p2 == v[2].p2) {
                points[0] = v[0].p2;
                // p2 must the other end
                points[1] = v[0].p1;
                // and p3 is at the other end of v[2]
                points[2] = v[2].p1;
            }
        }
};

ostream & operator<<(ostream &o, const Triangle & t)
{
    o << " tri:[ " << t.points[0] << "," << t.points[1] << "," << t.points[2]
      << " : " << t.dist[0] << " " << t.dist[1]  << " " << t.dist[2] << "]";
    return o;
}


#if 0
// calculate the spherical distance between two points p
double calcEuclideanDist(const FDiff2D & p1, const FDiff2D &p2)
{
    double dlon = p2.x - p1.x;
    // really?
    double dlat = p2.y - p1.y;
    return sqrt(dlon*dlon + dlat*dlat);
}
#endif

void printMatrix(ostream & o, double *mat, int width, int height)
{
    o.precision(12);
    for (int j=0; j < height; j++) {
        for (int i=0; i < width; i++) {
            o << mat[i + j*width] << " ";
        }
        o << endl;
    }
}

// Distance set, with functions to find triangles.
struct DistSet
{
public:
    /** create distance table of a set of spherical \p points
     *
     *  \param margin maximum distance error allowed while
     *                comparing distances
     *
     *  \param minDist minimum distance
     *  \param maxDist maximum distance
     */
    DistSet(vector<FDiff2D> & points, double margin, double minDist,
            double maxDist)
        : m_points(points), m_margin(margin)
        {
            DEBUG_DEBUG("creating DistSet with radial error bound: " << m_margin);
            int nP = points.size();
            // create distance matrix (only half of it
            dist = new double[nP*nP];
            // fill only half of it.
            for (int j=0; j < nP; j++) {
                for (int i=j; i < nP; i++) {
                    // calculate distance
                    double d = calcSphericalDist(points[i], points[j]);
                    // only use distances > 50 * margin
                    if (d < minDist && d > maxDist) {
//                        d = 0;
                    }
                    dist[i+j*nP] = d;
                }
            }
            // copy the lower half, to make searching easier.
            for (int j=1; j < nP; j++) {
                for (int i=0; i <= j; i++) {
                    dist[i+j*nP] = dist[j+i*nP];
                }
            }
        }

    ~DistSet()
        {
            delete dist;
        }

    /** print dist table */
    void printDistTable(ostream & o)
        {
            int nP = m_points.size();
            printMatrix(o,dist,nP,nP);
        }

    /** get distance between two points */
    double getDistance(int pNr, int p2Nr) const
        {
            int nP = m_points.size();
            return dist[pNr + p2Nr * nP];
        }

    /** get points with this distance +- error dist
     *
     *  @todo Could be optimized! O(n^2)
     */
    void getLines(double distance, vector<Line> & v) const
        {
            int nP = m_points.size();
            double upper = distance + m_margin;
            double lower = distance - m_margin;
            // use lookup table here.
            for (int i=0; i < nP; i++) {
                for (int j=i; j < nP; j++) {
                    double d = dist[i+j*nP];
                    if ( d < upper && d > lower ) {
                        v.push_back(Line(i,j,d));
                    }
                }
            }
        }

#if 0
    /** get neighbouring points of point \p point
     *  with this distance +- error dist.
     *
     *  output: Line::p1 = \p point, to allow efficient comparisons
     *          later on.
     */
    getNeighbourLines(int point, int dist, vector<Line> & v) const
        {
            double upper = dist + m_margin;
            double lower = dist - m_margin;
            // use lookup table here.
            double * distp = dist + point * nP;
            for (int i=0; i < nP; i++) {
                double d = distp[i];
                if ( d < upper && d > lower ) {
                    v.push_back(Line(point,i,d));
                }
            }
        }
#endif

    /** get the point that close the triangle with base
     *  \p line, given two distances.
     */
    void closeTriangle(Line & line, const Triangle & tri,
                  vector<Triangle> & v) const
        {
            int nP = m_points.size();

            // margins for distance between p2, p3 in tri
            double sd23upper = tri.dist[1] + m_margin;
            double sd23lower = tri.dist[1] - m_margin;

            // margins for distance between p3, p1 in tri
            double sd31upper = tri.dist[2] + m_margin;
            double sd31lower = tri.dist[2] - m_margin;

            // find candidates for p3, by looking for a suitable
            // line with length of dist1 or dist2, starting at p1
            double * distlinep1 = dist + line.p1 * nP;
            for (int p3=0; p3 < nP; p3++) {
                // distance between p1 and p3
                double d31 = distlinep1[p3];
                if (d31 < sd31upper && d31 > sd31lower) {
                    // found a candidate, len (p1-p3) = tri.dist31
                    double d23 = dist[p3 +  line.p2 * nP];
                    // check if len(p2-p3) = dist23
                    if (d23 > sd23lower && d23 < sd23upper) {
                        // then we have found a valid triangle, with
                        // similar point placement as tri.
                        v.push_back(Triangle(line.p1, line.dist,
                                             line.p2, d23,
                                             p3, d31));
                    }
                }
                // p1 and p2 can be mirrored
                if ( d31 < sd23upper && d31 > sd23lower ) {
                    // found a candidate, len (p1-p3) = tri.dist23
                    double d23 = dist[p3 +  line.p2 * nP];
                    // if len(p2-p3) = dist31 is true
                    if (d23 > sd31lower && d23 < sd31upper) {
                        // then we found a valid triangle, with
                        // mirrored p1 and p2.
                        v.push_back(Triangle(line.p2, line.dist,
                                             line.p1, d31,
                                             p3, d23));
                    }
                }
            }
        }

    /** try to find similar triangles */
    void findSimilarTriangles(const Triangle & tri, vector<Triangle> & output) const
        {
//            DEBUG_DEBUG("starting search");
            // first Line candidates
            vector<Line> u;

            // get Lines with length of triangle base
            getLines(tri.dist[0], u);

            // check which points form a triangle (same points)
            for (vector<Line>::iterator uIt = u.begin();
                 uIt != u.end(); ++uIt)
            {
                // get the points that form valid triangles
                closeTriangle(*uIt, tri, output);
            }
        }

    // return a specific triangle
    Triangle getTriangle(int p1, int p2, int p3)
        {
            return Triangle (p1, getDistance(p1,p2),
                             p2, getDistance(p2,p3),
                             p3, getDistance(p3,p1));
        }

    // return a random triangle, for matching purposes.
    Triangle getRandomTriangle()
        {
            int nP = m_points.size();

            int p1 =(int) (nP * (rand()/(RAND_MAX+1.0)));
            int p2 =(int) (nP * (rand()/(RAND_MAX+1.0)));
            int p3 =(int) (nP * (rand()/(RAND_MAX+1.0)));

            if (p1 == p2 || p2 == p3 || p1 == p3) {
//                DEBUG_DEBUG("recursing, because rand returnd same result:"
//                            << p1 << " " << p2 << " " << p3);
                // hope that rand will provide 3 different numbers eventually
                return getRandomTriangle();
            }

            Triangle ret (p1, getDistance(p1,p2),
                          p2, getDistance(p2,p3),
                          p3, getDistance(p3,p1));
//            DEBUG_DEBUG("created random triangle: " << ret);
            return ret;
        }

    double getMargin() const
        { return m_margin; };
private:
    // copying not allowed
    DistSet(const DistSet & o);
    DistSet & operator=(const DistSet & o);

    // image points
    vector<FDiff2D> m_points;

    double m_margin;

    // distance matrix, dist[p1,p2] returns the distance.
    // useful for searching.
    double *dist;
};

/** try to match the given points by matching triangles.
 *
 *  uses spherical coordinates, on a unit sphere
 *  (see mathworld.wolfram.com)
 *  azimuth angle (longitude) (in xy-plane, from x axis)       (FDiff2D::x)
 *  polar angle (colatitude) (angle from z axis)               (FDiff2D::y)
 *  r     = 1
 */
void matchTriangleFeatures(Panorama & pano,
                           unsigned int img1, const vector<FDiff2D> & points1,
                           unsigned int img2, const vector<FDiff2D> & points2,
                           int nTriang,
                           double degthresh,
                           int voteThreshhold)
{
    DEBUG_TRACE("");
    int nAdded = 0;
    //PT::SpaceTransform T;
    PTools::Transform T;

    // transform image features into spherical coordinate system
    PanoramaOptions opts;
    opts.projectionFormat = PanoramaOptions::EQUIRECTANGULAR;
    opts.HFOV = 360;
    opts.VFOV = 180;
    opts.width = 360;
    T.createInvTransform(pano, img1, opts);

    vector<FDiff2D> equirect_points1;
    vector<FDiff2D>::const_iterator it;

#ifdef DEBUG
    ofstream f("points1.txt");
    ofstream fimg("points1_img.txt");
    f.precision(12);
    fimg.precision(12);
#endif
    for (it = points1.begin(); it != points1.end(); ++it) {
        FDiff2D d;
        T.transform(d,*it);
        d.x = DEG_TO_RAD(d.x);
        // colataitude = 90 - latitude
        d.y = DEG_TO_RAD(90 - d.y);
        equirect_points1.push_back(d);
#ifdef DEBUG
        f << d.x << " " << d.y << endl;
        fimg << it->x << " " << it->y << endl;
#endif
    }


#ifdef DEBUG
    f.close();
    fimg.close();
    ofstream f2("points2.txt");
    ofstream fimg2("points2_img.txt");
    f2.precision(12);
    fimg2.precision(12);
#endif
    // second image
    T.createInvTransform(pano, img2, opts);
    vector<FDiff2D> equirect_points2;

    for (it = points2.begin(); it != points2.end(); ++it) {
        FDiff2D d;
        T.transform(d,*it);
        d.x = DEG_TO_RAD(d.x);
        // colataitude = 90 - latitude
        d.y = DEG_TO_RAD(90 - d.y);
#ifdef DEBUG
        f2 << d.x << " " << d.y << endl;
        fimg2 << it->x << " " << it->y << endl;
#endif
        equirect_points2.push_back(d);
    }
#ifdef DEBUG
    f2.close();
    fimg2.close();
#endif

    // minimum distance ( 1/20 th of HFOV)
    double HFOV = const_map_get(pano.getImageVariables(img1),"v").getValue();
    double minDist = DEG_TO_RAD(HFOV)/20;
    // maximum distance ( 1/2 of HFOV)
    // we assume that our images do not overlap a lot more than 50%
    // even if they do.. doesn't matter too much..
    double maxDist = DEG_TO_RAD(HFOV)/2;

    DistSet dset1(equirect_points1, degthresh, minDist, maxDist);
    DistSet dset2(equirect_points2, degthresh, minDist, maxDist);

#ifdef DEBUG
    {
        ofstream d1("dist1.txt");
        dset1.printDistTable(d1);
        ofstream d2("dist2.txt");
        dset2.printDistTable(d2);
    }
#endif

    // record triangle matches, and the number of their occurance
    typedef map<Triangle, map<Triangle, int> > TriangleMatches;
    TriangleMatches triangle_matches;

    typedef  map<int, map<int, int > > PointMatches;
    PointMatches point_matches;

    // Angle histogram container.
    ValueHistogram<pair<Triangle, Triangle> > angleHist(36, 0, M_PI);

    DEBUG_DEBUG("starting search");

    int nP = points1.size();
    // brute force matching.. match all possible triangles.
    for (int tp2=1; tp2 < nP; tp2++) {
        cerr << ".";
        for (int tp3 = tp2; tp3 != tp2-1; tp3 = ++tp3%nP) {
            int tp1 = tp2-1;

            Triangle tri = dset1.getTriangle(tp1,tp2,tp3);
            if (tri.bad(dset1.getMargin())) {
//            DEBUG_DEBUG("Throwing away bad triangle: " << tri);
//                cerr << "_";
                continue;
            }

/*
    for (int i=0; i< nTriang; i++) {
        // for timiming purpose
        // create random triangle, from img
        Triangle tri = dset1.getRandomTriangle();
*/
            vector<Triangle> similar;
            dset2.findSimilarTriangles(tri, similar);

            // save triangle candidates, could be used for later
            // analysis ( pose estimation )

            for (vector<Triangle>::iterator it = similar.begin();
                 it != similar.end() ; ++it)
            {
                // safety check.. distances must be correct
                tri.assertDistances(equirect_points1);
                it->assertDistances(equirect_points2);

                // triangle votes
                triangle_matches[tri][*it]++;

                // calculate angle between triangles
                // based on law of cosines for spherical triangles
                // cos(d23) = cos(d13) * cos(d12) + sin(d13) * sin(d12) * cos(alpha)
                // use side for calculation of angles
                FDiff2D p1 = equirect_points1[tri.points[0]];
                FDiff2D p2 = equirect_points1[tri.points[2]];
                // hmm, hope this is correct
                FDiff2D p3 = p1 + equirect_points2[tri.points[2]] - equirect_points2[tri.points[0]];
                double c = calcSphericalDist(p1,p2);
                double a = calcSphericalDist(p2,p3);
                double b = calcSphericalDist(p3,p1);
                double alpha = calcDihedralAngle(a,b,c);

                // add to histogram
                angleHist.insertHist(alpha, make_pair(tri,*it));

            }
        }
    }

    // print histogram
    angleHist.printHist(cerr);

    // add triangle matches.
    unsigned int hist_max;
    const vector<pair<Triangle, Triangle> > vec = angleHist.getMax(hist_max);
    vector<pair<Triangle, Triangle> >::const_iterator tit;
    for (tit = vec.begin(); tit != vec.end() ; ++tit) {
        // add votes for these points
        for (int pnr=0; pnr <3; pnr++) {
            point_matches[tit->first.points[pnr]][tit->second.points[pnr]]++;
        }
    }

#ifdef DEBUG
    {
    // save all triangle matches
    ofstream ft("triangle_matches.txt");
    TriangleMatches::iterator tit;
    for (tit = triangle_matches.begin(); tit != triangle_matches.end() ; ++tit) {
        if (tit->second.size() > 0) {
            for(map<Triangle, int>::iterator tit2 = tit->second.begin();
                tit2 != tit->second.end(); ++tit2)
            {
                // tmp + 2 + 3 + 4
                ft << tit->first.points[0] << " "
                   << tit->first.points[1] << " "
                   << tit->first.points[2] << " "
                   << tit2->first.points[0] << " "
                   << tit2->first.points[1] << " "
                   << tit2->first.points[2] << " "
                   << tit2->second << " ";
                ft << endl;
            }
        }
    }
    ft.close();
    }
#endif

#if 0
    // add triangle matches.
    map<Triangle, vector<Triangle> >::iterator tit;
    for (tit = triangle_matches.begin(); tit != triangle_matches.end() ; ++tit) {
        if (tit->second.size() > 0) {
            // consider best triangle match
            int votes = 0;
            Triangle best;
            for(map<Triangle, int>::iterator tit2 = tit->second.begin();
                tit2 != tit->second.end(); ++tit2)
            {
                if (votes < tit2->second);
            }
        }
    }
#endif

    // add point matches
    for (PointMatches::iterator it = point_matches.begin(); it != point_matches.end() ; ++it) {
        if (it->second.size() > 0) {
            // find match with highest vote
            int votes = 0;
            int bestPoint = 0;
            for (map<int,int>::iterator p2it = it->second.begin();
                 p2it != it->second.end() ; ++p2it)
            {
                if (votes < p2it->second) {
                    votes = p2it->second;
                    bestPoint = p2it->first;
                }
            }

            cerr << "match: " << it->first << " -> " << bestPoint
                 << "  count: " << votes << endl;

            // use a better criterium that just the max...
            if (votes >= voteThreshhold) {
                // add points with score higher than 7
                ControlPoint cp(img1,
                                points1[it->first].x,
                                points1[it->first].y,
                                img2,
                                points2[bestPoint].x,
                                points2[bestPoint].y);
                pano.addCtrlPoint(cp);
                nAdded++;
            }
        }
    }

    DEBUG_DEBUG("added " << nAdded << " points");
}

} // namespace

struct ImgVars
{
    ImgVars()
     :v(0), a(0), b(0), c(0), d(0), e(0)
        { }
    double v,a,b,c,d,e;
};

static void usage(const char * name)
{
    cerr << name << ": create a .pto file with control points, from a given image series" << endl
         << endl
         << "  uses an angular distance measure for the computation." << endl
         << "  HFOV and lens correction parameters MUST be correct!" << endl
         << endl
         << "  currently assumes single row panos, where the images have been taken from" << endl
         << "  left to right" << endl
         << endl
         << "Usage: " << name  << " [options] pano.pto image1 image2 image3 ..." << endl
         << endl
         << "       all images must be of the same size!" << endl
         << endl
         << "  [options] can be: " << endl
         << "     -n number   # number of features to track, default: 50" << endl
         << "     -v number   # HFOV of images, in degrees, Used for images that do" << endl
         << "                   not provide this information in the EXIF header" << endl
         << "                   default: 40" << endl
         << "     -c          # closed panorama. tries to match the first and the last image." << endl;
//         << "     -f          # do an additional correlation pass on the detected features, default: no" << endl;
}

void loadAndAddImage(vigra::BImage & img, const std::string & filename, Panorama & pano, const ImgVars & imgvars)
{
    // load image
    vigra::ImageImportInfo info(filename.c_str());
    // FIXME.. check for grayscale / color
    img.resize(info.width(), info.height());
    if(info.isGrayscale())
    {
      // import the image just read
      importImage(info, destImage(img));
    } else {
      // convert to greyscale
      BRGBImage timg(info.width(), info.height());
      importImage(info, destImage(timg));
      vigra::copyImage(timg.upperLeft(),
                       timg.lowerRight(),
                       RGBToGrayAccessor<RGBValue<unsigned char> >(),
                       img.upperLeft(),
                       BImage::Accessor());
    }

    Lens lens;
    map_get(lens.variables,"v").setValue(imgvars.v);
    map_get(lens.variables,"a").setValue(imgvars.a);
    map_get(lens.variables,"b").setValue(imgvars.b);
    map_get(lens.variables,"c").setValue(imgvars.c);
    map_get(lens.variables,"d").setValue(imgvars.d);
    map_get(lens.variables,"e").setValue(imgvars.e);

    lens.isLandscape = (img.width() > img.height());
    if (lens.isLandscape) {
                    lens.setRatio(((double)img.width())/img.height());
                } else {
                    lens.setRatio(((double)img.height())/img.width());
                }

    std::string::size_type idx = filename.rfind('.');
    if (idx == std::string::npos) {
      DEBUG_DEBUG("could not find extension in filename");
    }
    std::string ext = filename.substr( idx+1 );

    if (utils::tolower(ext) == "jpg") {
        // try to read exif data from jpeg files.
        lens.readEXIF(filename);
    }

    int matchingLensNr=-1;
    // FIXME: check if the exif information
    // indicates other camera parameters
    for (unsigned int lnr=0; lnr < pano.getNrOfLenses(); lnr++) {
        const Lens & l = pano.getLens(lnr);

        // use a lens if hfov and ratio are the same
        // should add a check for exif camera information as
        // well.
        if ((l.getRatio() == lens.getRatio()) &&
            (l.isLandscape == lens.isLandscape) &&
            (const_map_get(l.variables,"v").getValue() == const_map_get(lens.variables,"v").getValue()) )
        {
            matchingLensNr= lnr;
        }
    }

    if (matchingLensNr == -1) {
        matchingLensNr = pano.addLens(lens);
    }

    VariableMap vars;
    fillVariableMap(vars);

    DEBUG_ASSERT(matchingLensNr >= 0);
    PanoImage pimg(filename, img.width(), img.height(), (unsigned int) matchingLensNr);
    pano.addImage(pimg, vars);
}

void copyFLToVector(KLT_FeatureList fl, vector<FDiff2D> & features)
{
    for (int i = 0 ; i < fl->nFeatures ; i++)  {
        // remember current feature positions
        FDiff2D p;
        p.x = fl->feature[i]->x;
        p.y = fl->feature[i]->y;
        features.push_back(p);
    }
}


int main(int argc, char *argv[])
{

    // parse arguments
    const char * optstring = "a:b:c:e:f:hn:o:v:s:t:i:j:";
    int c;

    opterr = 0;

    int nFeatures = 100;
    int defaultKLTWindowSize = -1;
    bool closedPanorama = false;
//    bool doFinetune = false;
    double maxangularDev = -1;
    ImgVars imgVars;
    int nrOfTriangles = 100;
    int voteThreshhold = 1;

    // output file
    string outputFile;

    while ((c = getopt (argc, argv, optstring)) != -1)
        switch (c) {
        case 'n':
            nFeatures = atoi(optarg);
            break;
        case 'j':
            voteThreshhold = atoi(optarg);
            break;
        case 'o':
            outputFile = optarg;
            break;
        case 't':
            maxangularDev = atof(optarg);
            break;
        case 'i':
            nrOfTriangles = atoi(optarg);
            break;
        case 'a':
            imgVars.a = atof(optarg);
            break;
        case 'b':
            imgVars.b = atof(optarg);
            break;
        case 'c':
            imgVars.c = atof(optarg);
            break;
        case 'd':
            imgVars.d = atof(optarg);
            break;
        case 'e':
            imgVars.e = atof(optarg);
            break;

//        case 'f':
//            doFinetune = true;
//            break;
        case 's':
            defaultKLTWindowSize = atoi(optarg);
            break;
        case 'v':
            imgVars.v = atof(optarg);
            break;
      case '?':
      case 'h':
          usage(argv[0]);
          return 1;
      default:
          abort ();
      }

    if (argc - optind <2) {
        usage(argv[0]);
        return 1;
    }

    // input images
    unsigned int nImages = argc-optind;
    char **imgNames = argv+optind;

    cerr << "Creating hugin project from " << nImages << " images." << endl
         << " Options: " << endl
         << "  - feature points per overlap: " << nFeatures << endl
         << "  - default HFOV: " << imgVars.v << " (only if images doesn't contain an EXIF header)" << endl;

    // create panorama object
    Panorama pano;
    BImage *firstImg = new BImage();
    BImage *secondImg = new BImage();

    loadAndAddImage(*firstImg, imgNames[0], pano, imgVars);
    // update the defaultHFOV with the hfov from the first image, might
    // contain the EXIF HFOV
    imgVars.v = const_map_get(pano.getImageVariables(0),"v").getValue();

    Diff2D imgsize = firstImg->size();

    // setup KLT tracker
    KLT_TrackingContext tc;
    KLT_FeatureList fl;
    tc = KLTCreateTrackingContext();

    // calculate feature density for perfectly distributed features.
    float featureArea = imgsize.x * imgsize.y / nFeatures;
    if (defaultKLTWindowSize == -1) {
        defaultKLTWindowSize = (int) (sqrt((imgsize.x * imgsize.y)/2500.0));
    }

    // make sure that the features are distributed over the whole image
    // by basing the minimum feature distance on the feature area.
    tc->mindist = (int)(sqrt(featureArea) / 2 + 0.5);
    tc->window_width = defaultKLTWindowSize;
    tc->window_height = defaultKLTWindowSize;
    KLTUpdateTCBorder(tc);

    // create feature lists
    fl = KLTCreateFeatureList(nFeatures);

    KLTPrintTrackingContext(tc);

    // select a starting yaw
    double currentImageYaw = 0;
    // set yaw
    PT::Variable var("y",currentImageYaw);
    pano.updateVariable(0,var);

    unsigned int nPairs = nImages -1;
    if (closedPanorama) {
        nPairs = nImages;
    }

    unsigned char * img1 = firstImg->begin();
    KLTSelectGoodFeatures(tc, img1, firstImg->width(),
                          firstImg->height(), fl);

#ifdef DEBUG
    {
        ostringstream finame;
        finame << "points" << 1 << ".ppm";
        KLTWriteFeatureListToPPM(fl, img1, firstImg->width(), firstImg->height(), finame.str().c_str());
    }
#endif

    // the feature points
    vector<FDiff2D> img1Features;
    vector<FDiff2D> img2Features;
    copyFLToVector(fl, img1Features);

    for (unsigned int pair=0; pair< nPairs; pair++) {
        loadAndAddImage(*secondImg, imgNames[(pair+1)%nImages], pano, imgVars);
        unsigned char * img2 = secondImg->begin();
        KLTSelectGoodFeatures(tc, img2, secondImg->width(),
                              secondImg->height(), fl);
#ifdef DEBUG
        {
            ostringstream finame;
            finame << "points" << pair+2 << ".ppm";
            KLTWriteFeatureListToPPM(fl, img2, secondImg->width(), secondImg->height(), finame.str().c_str());
        }
#endif

        copyFLToVector(fl, img2Features);

        // add features based on triangle matching.
        // allowed distance error
        if (maxangularDev < 0) {
            maxangularDev = DEG_TO_RAD(imgVars.v)/firstImg->width();
        }

        DEBUG_DEBUG("Before angular matching, similarity threshold: " << maxangularDev);
        AngularMatching::matchTriangleFeatures(pano,
                                               pair%nImages, img1Features,
                                               (pair+1)%nImages, img2Features,
                                               nrOfTriangles,
                                               maxangularDev,
                                               voteThreshhold);

        delete firstImg;
        firstImg = secondImg;
        secondImg = new BImage();
        img1Features = img2Features;
        img2Features.clear();
    }

    // try setting sensible options
    PanoramaOptions opts = pano.getOptions();
    FDiff2D fov = pano.calcFOV();
    opts.HFOV = fov.x;
    opts.VFOV = fov.y;
    if (opts.HFOV < 80 && opts.VFOV < 80) {
        opts.projectionFormat = PanoramaOptions::RECTILINEAR;
    } else {
        opts.projectionFormat = PanoramaOptions::EQUIRECTANGULAR;
    }

    opts.optimizeReferenceImage = nImages/2;
    OptimizeVector optvec(nImages);
    // fill optimize vector, just anchor one image.
    for (unsigned int i=0; i<nImages; i++) {
        if (i != opts.optimizeReferenceImage) {
            optvec[i].insert("y");
            optvec[i].insert("p");
            optvec[i].insert("r");
        }
    }
    if (outputFile != "") {
        ofstream of(outputFile.c_str());
        pano.printOptimizerScript(of, optvec, opts);
    } else {
        pano.printOptimizerScript(cout, optvec, opts);
    }
}
