// -*- c-basic-offset: 4 -*-
/** @file Lens.h
 *
 *  @brief Lens class
 * 
 *  !! from PanoramaMemento.h 1970
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

#ifndef _PANODATA_LENS_H
#define _PANODATA_LENS_H


#include <string>
#include <vector>
#include <map>
#include <hugin_shared.h>

#include <hugin_math/hugin_math.h>
#include <panodata/PanoramaVariable.h>
#include <panodata/SrcPanoImage.h>

namespace HuginBase {

    
class IMPEX Lens {

    public:
        typedef BaseSrcPanoImage::Projection LensProjectionFormat;

        /** construct a new lens.
         *
         */
        Lens();
        
        
    public:
        ///
    //  QDomElement toXML(QDomDocument & doc);
        
        ///
    //  void setFromXML(const QDomNode & node);

        /** try to fill Lens data (HFOV) from EXIF header
         *
         *  @return true if focal length was found, lens will contain
         *          the correct data.
         */
    //  bool readEXIF(const std::string & filename);

        
    public:
        /** get projection type */
        LensProjectionFormat getProjection() const
        { return m_projectionFormat; }

        /** set projection type */
        void setProjection(LensProjectionFormat l)
        { m_projectionFormat = l; }

        /** get HFOV in degrees */
        double getHFOV() const;

        /** set HFOV in degrees */
        void setHFOV(double d);

        /** get focal length of lens, it is calculated from the HFOV */
        double getFocalLength() const;

        /** get crop factor, d35mm/dreal */
        double getCropFactor() const 
        { return m_cropFactor; };

        /** sets the crop factor */
        void setCropFactor(double newCropFactor)
        { m_cropFactor=newCropFactor; };

        /** return the sensor ratio (width/height)
         */
        double getAspectRatio() const;

        /** check if the image associated with this lens is in landscape orientation.
         */
        bool isLandscape() const;
        
        /** set the exposure value */
        void setEV(double ev);

        /** get the image size, in pixels */
        vigra::Size2D getImageSize() const
        { return m_imageSize; }

        /** set image size in pixels */
        void setImageSize(const vigra::Size2D & sz)
        { m_imageSize = sz; }

        // updates everything, including the lens variables.
        void update(const Lens & l);

        
    public: //?
            
    //  bool isLandscape;
       
        // these are the lens specific settings.
        // lens correction parameters
        LensVarMap variables;

#ifndef SWIG
        // dimensionless array not supported by SWIG
        static const char *variableNames[];
#endif
        
        bool m_hasExif;
        
        
    private:
        LensProjectionFormat m_projectionFormat;
        vigra::Size2D m_imageSize;
        double m_cropFactor;
    
};


///
typedef std::vector<Lens> LensVector;


} // namespace
#endif // _H
