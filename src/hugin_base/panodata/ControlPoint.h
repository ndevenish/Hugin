// -*- c-basic-offset: 4 -*-
/** @file PanoramaMemento.h
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

#ifndef _PANORAMAMEMENTO_H
#define _PANORAMAMEMENTO_H

#include <hugin_shared.h>

#include <string>
#include <vector>

#include <vigra_ext/Interpolators.h>


namespace HuginBase {

/// represents a control point
class IMPEX ControlPoint
{
    
public:
    /** minimize x,y or both. higher numbers mean multiple line
     * control points
     */
    enum OptimizeMode {
        X_Y = 0,  ///< evaluate x,y
        X,        ///< evaluate x, points are on a vertical line
        Y         ///< evaluate y, points are on a horizontal line
    };
    
    
public:
    ///
    ControlPoint()
        : image1Nr(0), image2Nr(0),
          x1(0),y1(0),
          x2(0),y2(0),
          error(0), mode(X_Y)
        { };

    ///
//  ControlPoint(Panorama & pano, const QDomNode & node);
    
    ///
    ControlPoint(unsigned int img1, double sX, double sY,
                 unsigned int img2, double dX, double dY,
                 int mode = X_Y)
        : image1Nr(img1), image2Nr(img2),
          x1(sX),y1(sY),
          x2(dX),y2(dY),
          error(0), mode(mode)
        { };

    ///
    bool operator==(const ControlPoint & o) const;
   
    
public:
    ///
    const std::string & getModeName(OptimizeMode mode) const;

    /** returns string which contains all features of a control point
      * used for detecting duplicate control points
      * in the string the image numbers are sorted ascending to cover
      * also mirrored control points
      */
    const std::string getCPString() const;

    /// swap (image1Nr,x1,y1) with (image2Nr,x2,y2)
    void mirror();

    
public:
    ///
//  QDomNode toXML(QDomDocument & doc) const;

    ///
//  void setFromXML(const QDomNode & elem, Panorama & pano);
    
    
public:    
    // TODO: accessors
    
    unsigned int image1Nr;
    unsigned int image2Nr;
    double x1,y1;
    double x2,y2;
    double error;
    int mode;

#ifndef SWIG
    // dimensionless array not supported by SWIG
    static std::string modeNames[];
#endif
};


///
typedef std::vector<ControlPoint> CPVector;
// pair of global control point number and corrosponding control point
typedef std::pair<unsigned int, ControlPoint> CPoint;
typedef std::vector<CPoint> CPointVector;


} // namespace
#endif // _PANORAMAMEMENTO_H
