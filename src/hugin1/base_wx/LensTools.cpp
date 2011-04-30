// -*- c-basic-offset: 4 -*-
/** @file LensTools.cpp
 *
 *  @brief some helper classes for working with lenses
 *
 *  @author T. Modes
 *
 */

/*  This program is free software; you can redistribute it and/or
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

#include "panoinc_WX.h"
#include "panoinc.h"
#include "LensTools.h"
#include <algorithm>

using namespace std;

void FillLensProjectionList(wxControlWithItems* list)
{
    list->Clear();
    list->Append(_("Normal (rectilinear)"),(void*)HuginBase::SrcPanoImage::RECTILINEAR);
    list->Append(_("Panoramic (cylindrical)"),(void*)HuginBase::SrcPanoImage::PANORAMIC);
    list->Append(_("Circular fisheye"),(void*)HuginBase::SrcPanoImage::CIRCULAR_FISHEYE);
    list->Append(_("Full frame fisheye"),(void*)HuginBase::SrcPanoImage::FULL_FRAME_FISHEYE);
    list->Append(_("Equirectangular"),(void*)HuginBase::SrcPanoImage::EQUIRECTANGULAR);
    list->Append(_("Orthographic"),(void*)HuginBase::SrcPanoImage::FISHEYE_ORTHOGRAPHIC);
    list->Append(_("Stereographic"),(void*)HuginBase::SrcPanoImage::FISHEYE_STEREOGRAPHIC);
    list->Append(_("Equisolid"),(void*)HuginBase::SrcPanoImage::FISHEYE_EQUISOLID);
    list->Append(_("Fisheye Thoby"),(void*)HuginBase::SrcPanoImage::FISHEYE_THOBY);
    list->SetSelection(0);
};

void SelectProjection(wxControlWithItems* list,size_t new_projection)
{
    for(unsigned int i=0;i<list->GetCount();i++)
    {
        if((size_t)list->GetClientData(i)==new_projection)
        {
            list->SetSelection(i);
            return;
        };
    };
    list->SetSelection(0);
};

size_t GetSelectedProjection(wxControlWithItems* list)
{
    return (size_t)(list->GetClientData(list->GetSelection()));
};

void SaveLensParameters(const wxString filename, HuginBase::Panorama* pano, unsigned int imgNr)
{
    HuginBase::StandardImageVariableGroups variable_groups(*pano);
    const HuginBase::Lens & lens = variable_groups.getLensForImage(imgNr);
    const HuginBase::VariableMap & vars = pano->getImageVariables(imgNr);
    // get the variable map
    char * p = setlocale(LC_NUMERIC,NULL);
    char * old_locale = strdup(p);
    setlocale(LC_NUMERIC,"C");
    wxFileConfig cfg(wxT("hugin lens file"),wxT(""),filename);
    cfg.Write(wxT("Lens/image_width"), (long) lens.getImageSize().x);
    cfg.Write(wxT("Lens/image_height"), (long) lens.getImageSize().y);
    cfg.Write(wxT("Lens/type"), (long) lens.getProjection());
    cfg.Write(wxT("Lens/hfov"), const_map_get(vars,"v").getValue());
    cfg.Write(wxT("Lens/hfov_link"), const_map_get(lens.variables,"v").isLinked() ? 1:0);
    cfg.Write(wxT("Lens/crop"), lens.getCropFactor());

    // loop to save lens variables
    const char ** varname = HuginBase::Lens::variableNames;
    while (*varname)
    {
        if (string(*varname) == "Eev")
        {
            varname++;
            continue;
        }
        wxString key(wxT("Lens/"));
        key.append(wxString(*varname, wxConvLocal));
        cfg.Write(key, const_map_get(vars,*varname).getValue());
        key.append(wxT("_link"));
        cfg.Write(key, const_map_get(lens.variables,*varname).isLinked() ? 1:0);
        varname++;
    }

    HuginBase::SrcPanoImage image = pano->getSrcImage(imgNr);
    cfg.Write(wxT("Lens/crop/enabled"), image.getCropMode()==HuginBase::SrcPanoImage::NO_CROP ? 0l : 1l);
    cfg.Write(wxT("Lens/crop/autoCenter"), image.getAutoCenterCrop() ? 1l : 0l);
    vigra::Rect2D cropRect=image.getCropRect();
    cfg.Write(wxT("Lens/crop/left"), cropRect.left());
    cfg.Write(wxT("Lens/crop/top"), cropRect.top());
    cfg.Write(wxT("Lens/crop/right"), cropRect.right());
    cfg.Write(wxT("Lens/crop/bottom"), cropRect.bottom());

    if (image.hasEXIFread())
    {
        // write exif data to ini file
        cfg.Write(wxT("EXIF/CameraMake"),  wxString(image.getExifMake().c_str(), wxConvLocal));
        cfg.Write(wxT("EXIF/CameraModel"), wxString(image.getExifModel().c_str(), wxConvLocal));
        cfg.Write(wxT("EXIF/FocalLength"), image.getExifFocalLength());
        cfg.Write(wxT("EXIF/Aperture"), image.getExifAperture());
        cfg.Write(wxT("EXIF/ISO"), image.getExifISO());
        cfg.Write(wxT("EXIF/CropFactor"), image.getExifCropFactor()); 
        cfg.Write(wxT("EXIF/Distance"), image.getExifDistance()); 
    }
    cfg.Flush();

    // reset locale
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);
};
