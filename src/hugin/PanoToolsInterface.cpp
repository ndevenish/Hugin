// -*- c-basic-offset: 4 -*-

/** @file PanoToolsInterface.cpp
 *
 *  @brief implementation of PanoToolsInterface Class
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

#include "panoinc.h"
#include "panoinc_WX.h"
#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>	// needed on mingw
#include <wx/imaglist.h>
#include <wx/spinctrl.h>

#include "hugin/ImageCache.h"
#include "hugin/PanoToolsInterface.h"

using namespace PT;
/*
using namespace PTools;

// ==================================================================
// high level, user functions
// ==================================================================

bool PTools::stitchImage(wxImage & dest, const Panorama & pano,
                         UIntSet imgNrs, const PanoramaOptions & opts)
{

    TrformStr tform;
    createAdjustTrform(tform);

    aPrefs aP;
    createAdjustPrefs(aP, tform);

    setAdjustDestImg(tform, aP, dest.GetWidth(), dest.GetHeight(), dest.GetData(), opts);

    for (UIntSet::const_iterator it = imgNrs.begin();
         it != imgNrs.end();
         ++it)
    {
        const PanoImage & pimg = pano.getImage(*it);
        const VariableMap &vars = pano.getImageVariables(*it);
        const Lens &l = pano.getLens(pimg.getLensNr());
        wxImage * src = ImageCache::getInstance().getImage(
            pimg.getFilename());
        setAdjustSrcImg(tform, aP, src->GetWidth(), src->GetHeight(), src->GetData(),  vars, l.projectionFormat, true);

        DEBUG_DEBUG("inserting image " << *it << " into panorama");
        // call the main remapping function
        MakePano(&tform,&aP);
        DEBUG_DEBUG("inserting finished");
    }

    freeTrform(tform);

    return tform.success;
}

bool PTools::mapImage(wxImage & dest, const Panorama & pano,
                      unsigned imgNr, const PanoramaOptions & opts,
                      bool correctLensDistortion)
{
    // not working properly
    TrformStr tform;
    createAdjustTrform(tform);

    aPrefs aP;
    createAdjustPrefs(aP, tform);

    setAdjustDestImg(tform, aP, dest.GetWidth(), dest.GetHeight(),
                     dest.GetData(), opts);

    const PanoImage & pimg = pano.getImage(imgNr);
    const VariableMap &vars = pano.getImageVariables(imgNr);
    const Lens &l = pano.getLens(pimg.getLensNr());
    wxImage * src = ImageCache::getInstance().getImage(
        pimg.getFilename());
    setAdjustSrcImg(tform, aP, src->GetWidth(), src->GetHeight(), src->GetData(),
                    vars, l.projectionFormat, correctLensDistortion);

    DEBUG_DEBUG("remapping image " << imgNr << " into panorama" << (correctLensDistortion  ? "with lens distortion correction" : ""));
    // call the main remapping function
    MakePano(&tform,&aP);
    DEBUG_DEBUG("remapping finished");

    freeTrform(tform);
    return tform.success;
}

*/