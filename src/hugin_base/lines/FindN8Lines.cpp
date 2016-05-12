// -*- c-basic-offset: 4 -*-
/**  @file FindN8Lines.cpp
 *
 *  @brief implementation of find lines algorithm
 *
 *  @author Thomas K. Sharpless
 *
 *  finds straightish, non-crossing lines in an edge map,
 *  using 8-neighborhood operations.
 */

/***************************************************************************
 *   Copyright (C) 2009 Thomas K Sharpless                                 *
 *   tksharpless@gmail.com                                                 *
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
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <assert.h>

#include "FindN8Lines.h"
#include <vigra/copyimage.hxx>
#include <vigra/pixelneighborhood.hxx>

namespace HuginLines
{
// colors used in lines image
#define N8_bg  255
#define N8_end 1
#define N8_mid 96

vigra::BImage edgeMap2linePts(vigra::BImage & input)
{
    vigra::BImage edge(input.width(), input.height(), N8_bg);
    vigra::BImage line(input.size());
    //copy input image, left border pixel in background color
    vigra::copyImage(input.upperLeft() + vigra::Diff2D(1, 1), input.lowerRight() - vigra::Diff2D(1, 1), input.accessor(), edge.upperLeft(), edge.accessor());

    int width  = input.width(),
        height = input.height();

    vigra::BImage::traverser eul, lul;

    /* first pass erases "elbow" points  from edge image,
       leaving only diagonal connections.  An elbow point
       has one horizontal and one vertical neighbor, and no
       more than 4 neighbors in all.

    */
    eul = edge.upperLeft() + vigra::Diff2D(1, 1);
    for(int y=1; y<height-1; ++y, ++eul.y )
    {
        vigra::BImage::traverser ix = eul;
        for(int x=1; x<width-1; ++x, ++ix.x )
        {
          // center must be an edge point
            if( *ix != N8_bg )
            {
                int n = 0;
                int nh = 0, nv = 0, nu = 0, nd = 0;
                if( ix( 1, 0 ) != N8_bg ) ++nh, ++n;
                if( ix( 1, -1 ) != N8_bg ) ++nu, ++n;
                if( ix( 0, -1 ) != N8_bg ) ++nv, ++n;
                if( ix( -1, -1 ) != N8_bg ) ++nd, ++n;
                if( ix( -1, 0 ) != N8_bg ) ++nh, ++n;
                if( ix( -1, 1 ) != N8_bg ) ++nu, ++n;
                if( ix( 0, 1 ) != N8_bg ) ++nv, ++n;
                if( ix( 1, 1 ) != N8_bg ) ++nd, ++n;

                if( nh == 1 && nv == 1 && n > 1 && n < 5 )
                {
                    *ix = N8_bg;    // clear this point
                }
            }
        }
    }

    /* second pass copies center points of 3x3 regions having line-like
       configurations, to the temp image, marked with the orientation of
       the inferred line.  As a result of pass 1, these points will have
       not more than 2 marked neighbors, in an oblique configuration.
       Orientation codes 1 to 8 correspond to 22.5 degree increments
       from horizontal.  We multiply these by 20 to make the differences
       visible on a debug image.

    */
    eul = (edge.upperLeft() + vigra::Diff2D(1, 1));
    lul = (line.upperLeft() + vigra::Diff2D(1, 1));
    line.init(N8_bg);

    for(int y=1; y<height-1; ++y, ++eul.y, ++lul.y)
    {
        vigra::BImage::traverser ix = eul;
        vigra::BImage::traverser ox = lul;
        for(int x=1; x<width-1; ++x, ++ix.x, ++ox.x)
        {
            if( *ix != N8_bg )
            {
                int n = 0;
                int nh = 0, nv = 0, nu = 0, nd = 0;

                if( ix( 1, 0 ) != N8_bg ) ++nh, ++n;
                if( ix( 1, -1 ) != N8_bg ) ++nu, ++n;
                if( ix( 0, -1 ) != N8_bg ) ++nv, ++n;
                if( ix( -1, -1 ) != N8_bg ) ++nd, ++n;
                if( ix( -1, 0 ) != N8_bg ) ++nh, ++n;
                if( ix( -1, 1 ) != N8_bg ) ++nu, ++n;
                if( ix( 0, 1 ) != N8_bg ) ++nv, ++n;
                if( ix( 1, 1 ) != N8_bg ) ++nd, ++n;

              // mark the output pixel
                int code = 0;
                if( n == 1 )
                {
                    if( nh ) code = 1;
                    else if( nu ) code = 3;
                    else if( nv ) code = 5;
                    else code = 7;
                }
                else
                    if( n == 2 )
                    {
                        if( nh == 2 ) code = 1;
                        else if( nu == 2 ) code = 3;
                        else if( nv == 2 ) code = 5;
                        else if( nd == 2 ) code = 7;
                        else if( nh && nu ) code = 2;
                        else if( nh && nd ) code = 8;
                        else if( nv && nu ) code = 4;
                        else if( nv && nd ) code = 6;
                    }
                assert( code < 9 );
                *ox = code ? code : N8_bg;
            }
        }
    }

    /* 3rd pass erases points in the temp image having 2 neighbors whose
       orientations differ by more than one unit.  This breaks corners.

    */
    lul = (line.upperLeft() + vigra::Diff2D(1, 1));

    for(int y=1; y<height-1; ++y, ++lul.y)
    {
        vigra::BImage::traverser ix = lul;
        for(int x=1; x<width-1; ++x, ++ix.x )
        {
            int c = *ix;
            if( c != N8_bg )
            {
                assert( c > 0 && c < 9 );
                int n = 0, omin = 9, omax = 0;
                vigra::NeighborhoodCirculator<vigra::BImage::traverser, vigra::EightNeighborCode>
                               circulator(ix),
                               end(circulator);
                do {
                    int nc = *circulator;
                    if( nc != N8_bg )
                    {
                        assert( nc > 0 && nc < 9 );
                        if( nc < omin ) omin = nc;
                        if( nc > omax ) omax = nc;
                        ++n;
                    }
                } while(++circulator != end);
                // mark the pixel
                if( n == 2 )
                {
                    int d =  omax - omin;
                    assert( d < 8 );
                    if( d > 4 )
                        d = 7 - d;
                    if ( d > 1 )
                        *ix = N8_bg;
                }
            }
        }
    }

    /* 4th pass changes marks in temp image from orientation to
       end = 1, interior = 2, and erases isolated points.
    */
    lul = (line.upperLeft() + vigra::Diff2D(1, 1));

    for(int y=1; y<height-1; ++y, ++lul.y)
    {
        vigra::BImage::traverser ix = lul;
        for(int x=1; x<width-1; ++x, ++ix.x )
        {
            int c = *ix;
            if( c != N8_bg )
            {
                int n = 0;
                vigra::NeighborhoodCirculator<vigra::BImage::traverser, vigra::EightNeighborCode> circulator(ix);
                vigra::NeighborhoodCirculator<vigra::BImage::traverser, vigra::EightNeighborCode> end(circulator);
                do {
                    int nc = *circulator;
                    if( nc != N8_bg )
                    {
                        ++n;
                    }
                } while(++circulator != end);
                // mark the pixel
                if( n == 0 ) *ix = N8_bg;
                else if( n == 1 ) *ix = 1;
                else *ix = 2;
            }
        }
    }

    /* 5th pass copies line points to edge image, skipping end points that are
       neighbors of end points (this removes 2-point lines).  End points are
       marked N8_end, interior points N8_mid.
    */
    lul = (line.upperLeft() + vigra::Diff2D(1, 1));
    eul = (edge.upperLeft() + vigra::Diff2D(1, 1)); // rewind edge
    edge.init(N8_bg);

    for(int y=1; y<height-1; ++y, ++lul.y, ++eul.y )
    {
        vigra::BImage::traverser ox = eul;
        vigra::BImage::traverser ix = lul;
        for(int x=1; x<width-1; ++x, ++ix.x, ++ox.x )
        {
            int c = *ix;
            if( c != N8_bg )
            {
                int n = 0;
                vigra::NeighborhoodCirculator<vigra::BImage::traverser, vigra::EightNeighborCode> circulator(ix);
                vigra::NeighborhoodCirculator<vigra::BImage::traverser, vigra::EightNeighborCode> end(circulator);
                do {
                    int nc = *circulator;
                    if( nc != N8_bg )
                    {
                        ++n;
                        if( c == 1 && nc == 1 ) c = 0;    //skip
                    }
                } while(++circulator != end);

              // mark the pixel
                if( c )
                {
                    if( n == 1 )
                    {
                        *ox = N8_end;
                    }
                    else
                    {
                        *ox = N8_mid;
                    }
                }
            }
        }
    }
    return edge;
}

// chain-code distance
inline float ccdist( int dx, int dy )
{
    dx = abs(dx); dy = abs(dy);
    return float(std::max(dx,dy)) + float(0.41421 * std::min(dx,dy));
}

// Euclidian distance
inline float eudist( int dx, int dy )
{
    register float x = dx, y = dy;
    return sqrt( x*x + y*y );
}

/* 3-point scalar curvature
   positive clockwise
*/
inline float scurv(vigra::Point2D & l, vigra::Point2D & m, vigra::Point2D & r)
{
    return float(m.x - l.x) * float(r.y - l.y)
         + float(m.y - l.y) * float(l.x - r.x );
}

/* 3-point float vector curvature
   positive inward
*/
inline void vcurv(vigra::Point2D & l, vigra::Point2D & m, vigra::Point2D & r, float & vx, float & vy)
{
    vx = 0.5 * (l.x + r.x) - float( m.x );
    vy = 0.5 * (l.y + r.y) - float( m.y );
}


/* final filter for line segments
  input: a point list, a minimum size, the f.l. in pixels and
    the projection center point.
  output: the point list may contain a segment, of at least
    mimimum size, having uniform curvature <= a limit that
    depends on f.l., position and orientation.
  return value is length of the pruned segment, or
      0 if rejectd on length or extreme initial curvature.
     -1 if rejected on orientation
     -2 if rejected on curvature
  Note if a segment has multiple smooth parts, only the longest
  one will survive.
*/
static int lineFilter(std::vector< vigra::Point2D > & pts,
                       int lmin,
                       double flpix,
                       double xcen, double ycen
                      )
{
    // cos( min angle from radius to chord )
    const double crcmin = 0.20;    // about 11 degrees
    // max ratio of actual to theoretical curvature
    const double cvrmax = 3.0;

    double r90 = 1.57 * flpix;    // 90 degree radius, pixels

    int n = (int)pts.size();
    // reject short segments
    if( n < lmin )
        return 0;    // too short

    // the full chord
    double x0 = pts.at(0).x,
           y0 = pts.at(0).y;
    double dx = pts.at(n-1).x - x0,
           dy = pts.at(n-1).y - y0;
    double chord = sqrt( dx*dx + dy*dy );
    if( chord < 0.6 * n )
        return 0;    // way too bent!
    // the chord midpoint (in centered coords)
    double ccx = x0 + 0.5 * dx - xcen,
           ccy = y0 + 0.5 * dy - ycen;
    // the arc center point in centered coords
    // aka radius vector
    double acx = pts.at(n/2).x - xcen,
           acy = pts.at(n/2).y - ycen;
    double acd = sqrt( acx*acx + acy*acy );
    // the curvature vector (points toward convex side)
    double cvx = acx - ccx,
           cvy = acy - ccy;
    double cvd = sqrt( cvx*cvx + cvy*cvy );
    // cos( angle from radius to chord )
    double cosrc =  fabs( acx * dy - acy * dx ) / (chord * acd);
    if( fabs(cosrc) < crcmin )
        return -1;    // too radial!

    /* curvature limit test */
    // curvature limit from lens model
    double S =  1.2 * chord * cosrc / r90;    // pseudo sine
    double C = r90 * (1 - sqrt( 1 - S * S ));    // corresp chord->arc dist
    // curvature test
    if( cvd / C > cvrmax )
        return -2;

    // for testing...
    return n;
}

/*  There are 2 line selection parameters:
  -- minimum size (in points )
  -- the assumed focal length (in pixels) of the lens
  The maximum allowed overall curvature depends on the f.l. and
  the position and orientation of the segment.  This calculation
  assumes the center of projection is in the center of the image.

  Passing a longer f.l. makes the curvature limits smaller. For
  rectilinear lenses, I suggest using a value 3 to 4 times the
  actual lens f.l.

  Lines whose orientation is radial, or nearly so, are removed
  as well, since they can contribute nothing to lens calibration.

  Note flpix == 0 disables the curvature & orientation tests.

  Besides the global curvature test, there are two local tests
  to ensure that the curvature is nearly uniform along the line.
  The first is a "corner test" that may split a line into smaller
  segments while it is being traced.  The second, part of the
  curvature filter, just prunes the ends, if possible, giving
  a single segment of nearly uniform curvature.
*/

int linePts2lineList(vigra::BImage & img, int minsize, double flpix, Lines& lines)
{
    int nadd = 0, nrejL = 0;

    static vigra::Diff2D offs[8] = {
        vigra::Diff2D(1, 0),
        vigra::Diff2D(1, -1),
        vigra::Diff2D(0, -1),
        vigra::Diff2D(-1, -1),
        vigra::Diff2D(-1, 0),
        vigra::Diff2D(-1, 1),
        vigra::Diff2D(0, 1),
        vigra::Diff2D(1, 1)
    };

    // corner filter parameters
    const int span = 10;
    const float maxacd = 2.0f;

    if(minsize < span) minsize = span; // else bend filter fails

    int width  = img.width(),
        height = img.height();
    double xcen = 0.5 * width,
           ycen = 0.5 * height;


    vigra::BImage::traverser ul(img.upperLeft() + vigra::Diff2D(1, 1));
    for(int y=1; y<height-1; ++y, ++ul.y)
    {
        vigra::BImage::traverser ix = ul;
        for(int x=1; x<width-1; ++x, ++ix.x )
        {
            int cd = *ix;    // point code
            if( cd == N8_end )
            { // new line...
              // trace and erase the line
                vigra::BImage::traverser tr = ix;
                vigra::Point2D pos(x, y);
                std::vector<vigra::Point2D> pts;
                pts.push_back(pos);
                *tr = N8_bg;
                int dir;
                vigra::Diff2D dif;
                do {    // scan the neighborhood
                    for( dir = 0; dir < 8; dir++ )
                    {
                        dif = offs[dir];
                        if( tr[dif] != N8_bg ) break;
                    }
                    assert( dir < 8 );
                    tr += dif;        // move to next point
                    cd = *tr;        // pick up that pixel
                    *tr = N8_bg;    // erase it from image
                    pos += dif;        // update position
                    pts.push_back(pos);    // add same to pointlist
                } while( cd != N8_end );
                // validate the point list
                if( pts.size() < minsize )
                {
                    ++nrejL;
                }
                else
                    if( flpix > 0 )
                    {
                        int ncopy = 0, nskip = 0;
                        /* bend test: compute running chord and arc on a fixed span.
                            Output segments where their difference is small.
                            The parameters are set to pass some hooked lines
                            that will be cleaned up later.
                        */
                        int ip = 0, np = (int)pts.size();

                        std::vector<vigra::Point2D> tmp;
                        float ccd[32]{};    // rolling interpoint chaincode distances
                        int isql = 0, isqr = 0;    // left & rgt indices to same
                        int xl = pts.at( 0 ).x,
                            yl = pts.at( 0 ).y;
                        float dx = pts.at(span-1).x - xl,
                              dy = pts.at(span-1).y - yl;
                        float Chord = sqrt(dx*dx + dy*dy);
                        float Arc = 0;
                        // fill 1st span's d's, initialize Dsq
                        for( isqr = 0; isqr < span-1; isqr++ )
                        {
                            register int x = pts.at( isqr + 1).x,
                                         y = pts.at( isqr + 1 ).y;
                            float d = ccdist( x - xl, y - yl );
                            ccd[isqr] = d;
                            Arc += d;
                            xl = x; yl = y;
                        }
                        // copy 1st span pnts if span is good
                        if( Arc - Chord <= maxacd )
                        {
                            for( int i = 0; i < span; i++ )
                            {
                                tmp.push_back(vigra::Point2D(pts.at(i).x, pts.at(i).y));
                            }
                        }

                        // roll span....
                        for( ip = 1; ip < np - span; ip++ )
                        {
                            int irgt = ip + span - 1;
                            // update span
                            Arc -= ccd[isql];
                            isql = (isql + 1) & 31;
                            isqr = (isqr + 1) & 31;
                            int x = pts.at( irgt ).x,
                                y = pts.at( irgt ).y;
                            float d = ccdist( x - xl, y - yl );
                            ccd[isqr] = d;
                            Arc += d;
                            xl = x; yl = y;
                            dx = x - pts.at(ip).x; dy  = y - pts.at(ip).y;
                            Chord = sqrt( dx*dx + dy*dy );
                            /* if this span is good, copy right pnt
                            else flush tmp to lines if long enough
                            */
                            if( Arc - Chord <= maxacd )
                            {
                                ++ncopy;
                                tmp.push_back(vigra::Point2D(pts.at(irgt).x, pts.at(irgt).y));
                            }
                            else
                            {
                                int q = lineFilter( tmp, minsize, flpix, xcen, ycen );
                                SingleLine line;
                                line.line=tmp;
                                if( q > 0 )
                                {
                                  ++nadd;
                                  line.status=valid_line;
                                }
                                else if( q == 0 ) line.status=bad_length;
                                else if( q == -1 ) line.status=bad_orientation;
                                else if( q == -2 ) line.status=bad_curvature;
                                lines.push_back(line);
                                tmp.clear();
                                ++nskip;
                            }
                        }
                        // if the final span is good, copy & flush.
                        int q = lineFilter( tmp, minsize, flpix, xcen, ycen );
                        SingleLine line;
                        line.line=tmp;
                        if( q > 0 )
                        {
                            ++nadd;
                            line.status=valid_line;
                        }
                        else if( q == 0 ) line.status=bad_length;
                        else if( q == -1 ) line.status=bad_orientation;
                        else if( q == -2 ) line.status=bad_curvature;
                        lines.push_back(line);
                    } // end curvature test
                    else
                    {    // length ok, no curvature test
                        SingleLine line;
                        line.line=pts;
                        line.status=valid_line;
                        lines.push_back(line);
                        ++nadd;
                    }
                }  // end trace line
        }    // end x scan
    }  // end y scan

    return nadd;
}

}; //namespace
