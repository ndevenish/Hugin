// -*- c-basic-offset: 4 -*-
/** @file PanoToolsInterface.h
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

#ifndef _PANOTOOLSINTERFACE_H
#define _PANOTOOLSINTERFACE_H

#include <PT/Panorama.h>
#include <PT/PanoramaMemento.h>
#include <PT/PanoToolsInterface.h>

class wxImage;

namespace PTools {

/** Stitch a Panorama into an output image
 *
 *  stitchs a set of images (@p imgNrs) of Panorama @p pano into the
 *  output image @p dest
 *
 *  it uses a nearest neighbour transform and doesn't do any color
 *  or distortion correction
 *
 *  @bug only the last remapped image is in the buffer,
 *       since panotools overwrites the previous images..
 *       Have to fix this somehow.
 */
bool stitchImage(wxImage & dest, const PT::Panorama & pano,
                 PT::UIntSet imgNrs, const PT::PanoramaOptions & opts);


/** remaps a single image into its final projection */
bool mapImage(wxImage & dest, const PT::Panorama & pano,
              unsigned imgNr, const PT::PanoramaOptions & opts,
              bool correctLensDistortion = true);


} // namespace

#endif // _PANOTOOLSINTERFACE_H
