// -*- c-basic-offset: 4 -*-
/** @file Lens.h
 *
 *  @brief Lens class
 * 
 *  !! from PanoramaMemento.h 1970
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoramaMemento.h 1970 2007-04-18 22:26:56Z dangelo $
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


#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include <math.h>

#ifdef HasPANO13
extern "C" {

#ifdef __INTEL__
#define __INTELMEMO__
#undef __INTEL__
#endif

#include "pano13/panorama.h"

#ifdef __INTELMEMO__
#define __INTEL__
#undef __INTELMEMO__
#endif

// remove stupid #defines from the evil windows.h

#ifdef DIFFERENCE
#undef DIFFERENCE
#endif

#ifdef MIN
#undef MIN
#endif

#ifdef MAX
#undef MAX
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif
}
#endif

#include "PT/PanoImage.h"

#include "vigra_ext/Interpolators.h"

namespace PT {

class Lens {

public:
    /** Lens type
     */
    enum LensProjectionFormat { RECTILINEAR = 0,
                                PANORAMIC = 1,
                                CIRCULAR_FISHEYE = 2,
                                FULL_FRAME_FISHEYE = 3,
                                EQUIRECTANGULAR = 4};


    /** construct a new lens.
     *
     */
    Lens();

//    QDomElement toXML(QDomDocument & doc);
//    void setFromXML(const QDomNode & node);

    /** try to fill Lens data (HFOV) from EXIF header
     *
     *  @return true if focal length was found, lens will contain
     *          the correct data.
     */
//    bool readEXIF(const std::string & filename);

    /** get projection type */
    LensProjectionFormat getProjection() const
    {
        return m_projectionFormat;
    }

    /** set projection type */
    void setProjection(LensProjectionFormat l) {
        m_projectionFormat = l;
    }

    /** get HFOV in degrees */
    double getHFOV() const;

    /** set HFOV in degrees */
    void setHFOV(double d);

    /** get focal length of lens, it is calculated from the HFOV */
    double getFocalLength() const;

    /** set focal length, updates HFOV */
    void setFocalLength(double);

    /** get crop factor, d35mm/dreal */
    double getCropFactor() const;

    /** Set the crop factor.
     *
     *  This will recalculate the sensor size.
     *
     *  @param c is the ratio of the sensor diagonals:
     *                factor = diag35mm / real_diag
     */
    void setCropFactor(double c);

    /** get sensor dimension */
    FDiff2D getSensorSize() const
    {
        return m_sensorSize;
    }

    /** set sensor dimensions. Only square pixels are supported so far.*/
    void setSensorSize(const FDiff2D & size);

    /** return the sensor ratio (width/height)
     */
    double getAspectRatio() const
    {
        return (double)m_imageSize.x / m_imageSize.y;
    }

    /** check if the image associated with this lens is in landscape orientation.
     */
    bool isLandscape() const
    {
        return m_imageSize.x >= m_imageSize.y;
    }

    /** set the exposure value */
    void setEV(double ev);

    /** get the image size, in pixels */
    vigra::Size2D getImageSize() const
    {
        return m_imageSize;
    }

    /** set image size in pixels */
    void setImageSize(const vigra::Size2D & sz)
    {
        m_imageSize = sz;
    }

    /** try to read image information from file */
    bool initFromFile(const std::string & filename, double &cropFactor, double & roll);

//    double isLandscape() const {
//        return sensorRatio >=1;
//    }

    // updates everything, including the lens variables.
    void update(const Lens & l);


//    bool isLandscape;
    // these are the lens specific settings.
    // lens correction parameters
    LensVarMap variables;
    static char *variableNames[];

    bool m_hasExif;
private:

    LensProjectionFormat m_projectionFormat;
    vigra::Size2D m_imageSize;
    FDiff2D m_sensorSize;
};


typedef std::vector<Lens> LensVector;


} // namespace
#endif // _PANORAMAMEMENTO_H
