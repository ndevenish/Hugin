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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>	// needed on mingw
#include <wx/imaglist.h>
#include <wx/spinctrl.h>

#include "common/utils.h"
#include "common/stl_utils.h"
#include "PT/PanoramaMemento.h"
#include "hugin/ImageCache.h"

#include "hugin/PanoToolsInterface.h"

using namespace PT;
using namespace PTools;

// create an empty image, with a pointer to a wxImage
void PTools::setDestImage(Image & image, wxImage & imgData, const PT::PanoramaOptions & opts)
{
    SetImageDefaults(&image);
    image.width = imgData.GetWidth();
    image.height = imgData.GetHeight();
    image.bytesPerLine = image.width*3;
    image.bitsPerPixel = 24;
    image.dataSize = image.height * image.bytesPerLine;
    // Allocate memory for pointer to pointer to image data
    image.data = (unsigned char**)malloc( sizeof(unsigned char*) );
    if(image.data == NULL) {
        DEBUG_FATAL("Out of memory");
    }
    *(image.data) = imgData.GetData();
    switch (opts.projectionFormat) {
    case PanoramaOptions::RECTILINEAR:
        image.format = _rectilinear;
        break;
    case PanoramaOptions::CYLINDRICAL:
        image.format= _panorama;
        break;
    case PanoramaOptions::EQUIRECTANGULAR:
        image.format = _equirectangular;
        break;
    }
    image.hfov = opts.HFOV;
}


void PTools::setFullImage(Image & image, wxImage & imgData, const VariableMap & vars,
                  const PT::Lens::LensProjectionFormat format,
                  bool correctDistortions)
{
    SetImageDefaults(&image);
    image.width = imgData.GetWidth();
    image.height = imgData.GetHeight();
    image.bytesPerLine = image.width*3;
    image.bitsPerPixel = 24;
    image.dataSize = image.height * image.bytesPerLine;

    // Allocate memory for pointer to pointer to image data
    image.data = (unsigned char**)malloc( sizeof(unsigned char*) );
    if(image.data == NULL) {
        DEBUG_FATAL("Out of memory");
    }
    *(image.data) = imgData.GetData();

    image.dataformat = _RGB;
    switch (format) {
    case Lens::RECTILINEAR:
        image.format = _rectilinear;
        break;
    case Lens::PANORAMIC:
        image.format = _panorama;
        break;
    case Lens::CIRCULAR_FISHEYE:
        image.format = _fisheye_circ;
        break;
    case Lens::FULL_FRAME_FISHEYE:
        image.format = _fisheye_ff;
        break;
    case Lens::EQUIRECTANGULAR_LENS:
        image.format = _equirectangular;
        break;
    }
    image.hfov = const_map_get(vars,"v").getValue();
    image.yaw = const_map_get(vars,"y").getValue();
    image.pitch = const_map_get(vars,"p").getValue();
    image.roll = const_map_get(vars,"r").getValue();

    //fill cPrefs struct
    if (correctDistortions) {
        initCPrefs(image.cP,vars);
    }

    // no name
    image.name[0]=0;
    image.yaw = const_map_get(vars,"y").getValue();
    image.yaw = const_map_get(vars,"y").getValue();


    image.selection.top = 0;
    image.selection.left = 0;
    image.selection.right = image.width;
    image.selection.bottom = image.height;
}

void PTools::initCPrefs(cPrefs & p, const VariableMap &vars)
{
    double val;
    p.magic = 20;
    double a = const_map_get(vars,"a").getValue();
    double b = const_map_get(vars,"b").getValue();
    double c = const_map_get(vars,"c").getValue();
    if (a != 0.0 || b != 0.0 || c != 0) {
        p.radial = 1;
        p.radial_params[0][3] = p.radial_params[1][3] = p.radial_params[2][3] = a;
        p.radial_params[0][2] = p.radial_params[1][2] = p.radial_params[2][2] = b;
        p.radial_params[0][1] = p.radial_params[1][1] = p.radial_params[2][1] = c;
        double d = 1.0 - (a+b+c);
        p.radial_params[0][0] = p.radial_params[1][0] = p.radial_params[2][0] = d;
    }

    val = const_map_get(vars,"e").getValue();
    if (val != 0.0) {
        p.vertical = TRUE;
        p.vertical_params[0] = p.vertical_params[1] = p.vertical_params[2] = val;
    } else {
        p.vertical = FALSE;
        p.vertical_params[0] = p.vertical_params[1] = p.vertical_params[2] = 0;
    }

    val = const_map_get(vars,"d").getValue();
    if (val != 0.0) {
        p.horizontal = TRUE;
        p.horizontal_params[0] = p.horizontal_params[1] = p.horizontal_params[2] = val;
    } else {
        p.horizontal = FALSE;
        p.horizontal_params[0] = p.horizontal_params[1] = p.horizontal_params[2] = 0;
    }
    // FIXME add shear parameters
    p.shear = FALSE;
    p.resize = FALSE;
    p.luminance = FALSE;
    p.cutFrame = FALSE;
    p.fourier = FALSE;
}


// ===========================================================================
// ===========================================================================


void PTools::createAdjustPrefs(aPrefs  & p, TrformStr & transf)
{
    SetAdjustDefaults(&p);
    p.interpolator = _nn;
    p.mode = _insert;

    SetImageDefaults(&(p.im));
    SetImageDefaults(&(p.pano));
}


// ===========================================================================
// ===========================================================================

// prepare a Trform struct for the adjust operation, image -> pano
void PTools::createAdjustTrform(TrformStr & trf)
{
    trf.src = (Image *) malloc(sizeof(Image));
    SetImageDefaults(trf.src);
    trf.dest = (Image *) malloc(sizeof(Image));
    SetImageDefaults(trf.dest);
    trf.success = TRUE;
    trf.tool = _adjust;
    trf.mode = _destSupplied | _honor_valid;
    trf.data = 0;
    trf.interpolator = _nn;
    trf.gamma = 1.0;
}


// free the resources associated with a TrformStr.
// createAdjustTrform must have been used to create it.
void PTools::freeTrform(TrformStr & trf)
{
    if (trf.dest) {
        if (trf.dest->data) {
            free(trf.dest->data);
        }
        free(trf.dest);
    }
    if (trf.src) {
        if (trf.src->data) {
            free(trf.src->data);
        }
        free(trf.src);
    }
}

void PTools::setAdjustSrcImg(TrformStr & trf, aPrefs & ap,
                             wxImage & src, const PT::VariableMap & vars,
                             const PT::Lens::LensProjectionFormat format,
                             bool correctDistortions)
{
    DEBUG_ASSERT(trf.src);
    if (trf.src->data) {
        free(trf.src->data);
    }
    setFullImage(*(trf.src), src, vars, format, correctDistortions);
    ap.im = *(trf.src);
}

void PTools::setAdjustDestImg(TrformStr & trf, aPrefs & ap,
                              wxImage & dest,
                              const PT::PanoramaOptions & opts)
{
    DEBUG_ASSERT(trf.dest);
    if (trf.dest->data) {
        free(trf.dest->data);
    }
    setDestImage(*(trf.dest), dest, opts);
    ap.pano = *(trf.dest);
}

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

    setAdjustDestImg(tform, aP, dest, opts);

    for (UIntSet::const_iterator it = imgNrs.begin();
         it != imgNrs.end();
         ++it)
    {
        const PanoImage & pimg = pano.getImage(*it);
        const VariableMap &vars = pano.getImageVariables(*it);
        const Lens &l = pano.getLens(pimg.getLensNr());
        wxImage * src = ImageCache::getInstance().getImage(
            pimg.getFilename());
        setAdjustSrcImg(tform, aP, *src,  vars, l.projectionFormat, false);

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

    TrformStr tform;
    createAdjustTrform(tform);

    aPrefs aP;
    createAdjustPrefs(aP, tform);

    setAdjustDestImg(tform, aP, dest, opts);

    const PanoImage & pimg = pano.getImage(imgNr);
    const VariableMap &vars = pano.getImageVariables(imgNr);
    const Lens &l = pano.getLens(pimg.getLensNr());
    wxImage * src = ImageCache::getInstance().getImage(
        pimg.getFilename());
    setAdjustSrcImg(tform, aP, *src,  vars, l.projectionFormat, correctLensDistortion);

    DEBUG_DEBUG("remapping image " << imgNr << " into panorama" << (correctLensDistortion  ? "with lens distortion correction" : ""));
    // call the main remapping function
    MakePano(&tform,&aP);
    DEBUG_DEBUG("remapping finished");

    freeTrform(tform);
    return tform.success;
}


#if 0



void setSrcImage(TrformStr & trf, wxImage dest);

Transform::Transform(const Panorama & pano, unsigned int srcImgNr,
                     wxImage & sImg)
{
    const PanoImage & img = pano.getImage(srcImgNr);
    VariableMap vars = pano.getImageVariables(srcImgNr);
    // convert d,e they are measured in image coordinates
    // need to adjust them if the source size is different
    double factor = ((double)sImg.GetWidth())/img.getWidth();
    Variable & d = const_map_get(vars,"d");
    d.setValue(d.getValue() * factor);
    Variable & e = const_map_get(vars,"e");
    e.setValue(e.getValue() * factor);
    const PT::Lens & l = pano.getLens(img.getLensNr());
    init(sImg, vars, l.projectionFormat);
}

bool Transform::remap(wxImage & dest, const PanoramaOptions & opts)
        {
            rPrefs r_prefs;
            r_prefs.magic = 30;
            r_prefs.from = trf.src->format;
            switch (opts.projectionFormat) {
            case PanoramaOptions::RECTILINEAR:
                r_prefs.to = _rectilinear;
                break;
            case PanoramaOptions::CYLINDRICAL:
                r_prefs.to = _panorama;
                break;
            case PanoramaOptions::EQUIRECTANGULAR:
                r_prefs.to = _equirectangular;
                break;
            }
            r_prefs.hfov = opts.HFOV;
            r_prefs.vfov = opts.VFOV;
            PImage destImag(dest);
            trf.dest = & destImag.image;
            trf.data = 0;
            //trf.interpolator = opts.interpolator;
            trf.interpolator = _nn;
            trf.gamma = opts.gamma;

            DEBUG_DEBUG("starting remap");
            // start the remapping
            ::remap(&trf, &r_prefs);
            DEBUG_DEBUG("remap finished");
            if (!trf.success) {
                DEBUG_ERROR("remap failed");
                return false;
            }
            return true;
        }


void Transform::init(wxImage & sImg, const VariableMap & vars,
                PT::Lens::LensProjectionFormat srcProj)
{
    srcImg = new PImage(sImg, vars, srcProj);
    trf.src = &(srcImg->image);
    trf.dest = 0;
    trf.success = TRUE;
    trf.tool = _remap;
    trf.mode = 0;
};


#endif
