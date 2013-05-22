// -*- c-basic-offset: 4 -*-

/** @file hugin1/base_wx/platform.h
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

#include <hugin_shared.h>
#include "panoinc_WX.h"
#include <config.h>

#ifdef __WXMSW__
#define HUGIN_CONV_FILENAME (*wxConvCurrent)
#else
#define HUGIN_CONV_FILENAME (*wxConvFileName)
#endif

#define HUGIN_WX_FILE_IMG_FILTER _("All Image files|*.jpg;*.JPG;*jpeg;*JPEG;*.tif;*.TIF;*.tiff;*.TIFF;*.png;*.PNG;*.bmp;*.BMP;*.gif;*.GIF;*.pnm;*.PNM;*.sun;*.viff;*.hdr;*.HDR;*.exr;*.EXR|JPEG files (*.jpg,*.jpeg)|*.jpg;*.JPG;*.jpeg;*.JPEG|TIFF files (*.tif,*.tiff)|*.tif;*.TIF;*.tiff;*.TIFF|PNG files (*.png)|*.png;*.PNG|HDR files (*.hdr)|*.hdr;*.HDR|EXR files (*.exr)|*.exr;*.EXR|All files (*)|*")

#if defined __WXMAC__ || defined __WXOSX_COCOA__

#if wxCHECK_VERSION(2,9,0)
  #include "wx/osx/core/cfstring.h"
  #include <wx/osx/private.h>
#else
  #include <CoreFoundation/CFString.h>
  #include <wx/mac/private.h>
#endif

CFStringRef MacCreateCFStringWithWxString(const wxString& string);

wxString MacGetPathToMainExecutableFileOfBundle(CFStringRef bundlePath);
wxString MacGetPathToMainExecutableFileOfRegisteredBundle(CFStringRef BundleIdentifier);

#if defined MAC_SELF_CONTAINED_BUNDLE

wxString MacGetPathToBundledResourceFile(CFStringRef filename);
wxString MacGetPathToBundledFrameworksDirectory();
wxString MacGetPathToBundledExecutableFile(CFStringRef filename);
wxString MacGetPathToBundledAppMainExecutableFile(CFStringRef appname);
wxString MacGetPathToUserDomainTempDir();
wxString MacGetPathToUserAppSupportAutoPanoFolder();

#endif // MAC_SELF_CONTAINED_BUNDLE

#endif //__WXMAC__

/** returns all invalid characters for the filename (mainly characters, which does not work with gnu make) */
WXIMPEX const wxString getInvalidCharacters();
/** returns true, if the given strings contains invalid characters */
WXIMPEX bool containsInvalidCharacters(const wxString stringToTest);
/** shows a dialog about filename with invalid characters, all names in filelist will be show in list */
WXIMPEX void ShowFilenameWarning(wxWindow* parent, const wxArrayString filelist);

#endif
