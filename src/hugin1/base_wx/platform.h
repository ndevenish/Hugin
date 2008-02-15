// -*- c-basic-offset: 4 -*-

/** @file platform.cpp
 *
 *  @brief implementation of huginApp Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: huginApp.cpp 2510 2007-10-28 22:24:11Z dangelo $
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

#ifndef HUGIN1_BASE_PLATFORM_H
#define HUGIN1_BASE_PLATFORM_H

#include "panoinc_WX.h"
#include <config.h>

#define HUGIN_WX_FILE_IMG_FILTER _("All Image files|*.jpg;*.JPG;*jpeg;*JPEG;*.tif;*.TIF;*.tiff;*.TIFF;*.png;*.PNG;*.bmp;*.BMP;*.gif;*.GIF;*.pnm;*.PNM;*.sun;*.viff;*.hdr|JPEG files (*.jpg,*.jpeg)|*.jpg;*.JPG;*.jpeg;*.JPEG|All files (*)|*")

#if defined __WXMAC__ 

#include <CoreFoundation/CFString.h>
#include <wx/mac/private.h>

CFStringRef MacCreateCFStringWithWxString(const wxString& string);

wxString MacGetPathToMainExecutableFileOfBundle(CFStringRef bundlePath);

#if defined MAC_SELF_CONTAINED_BUNDLE

wxString MacGetPathToBundledResourceFile(CFStringRef filename);
wxString MacGetPathToBundledExecutableFile(CFStringRef filename);
wxString MacGetPathToBundledAppMainExecutableFile(CFStringRef appname);
wxString MacGetPathToUserDomainTempDir();

#endif // MAC_SELF_CONTAINED_BUNDLE

#endif //__WXMAC__

#endif
