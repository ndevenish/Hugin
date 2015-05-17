// -*- c-basic-offset: 4 -*-
/** @file wxcms.h
 *
 *  @author T. Modes
 */

/*  This is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _WXCMS_H
#define _WXCMS_H

#include <hugin_shared.h>
#include <panoinc_WX.h>
#include <lcms2.h>
#include <vigra/imageinfo.hxx>

namespace HuginBase
{
    namespace Color
    {
        /** retrieve monitor profile from system */
        WXIMPEX void GetMonitorProfile(wxString& profileName, cmsHPROFILE& profile);
        /** apply color correction to given image using input iccProfile and monitor profile */
        WXIMPEX void CorrectImage(wxImage& image, const vigra::ImageImportInfo::ICCProfile& iccProfile, const cmsHPROFILE& monitorProfile);
    }
}
#endif // _WXCMS_H
