// -*- c-basic-offset: 4 -*-

/** @file platform.cpp
 *
 *  @brief various platfrom specific wxWidgets functions
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

#include "platform.h"

#include <hugin_utils/utils.h>

#ifdef __WXMAC__

#include <CoreFoundation/CFBundle.h>

wxString MacGetPathTOBundledResourceFile(CFStringRef filename)
{
    wxString theResult = wxT("");

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_INFO("Mac: Not bundled");
    }
    else
    {
        CFURLRef XRCurl = CFBundleCopyResourceURL(mainbundle, filename, NULL, NULL);
        if(XRCurl == NULL)
        {
            DEBUG_INFO("Mac: Cannot locate the file in bundle.");
        }
        else
        {
            CFStringRef pathInCFString = CFURLCopyFileSystemPath(XRCurl, kCFURLPOSIXPathStyle);
            if(pathInCFString == NULL)
            {
                DEBUG_INFO("Mac: Failed to get URL in CFString");
            }
            else
            {
                CFRetain( pathInCFString );
                theResult = wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                DEBUG_INFO("Mac: returned the resource file's path in the application bundle");
            }
            CFRelease( XRCurl );
        }
    }
    return theResult;
}

wxString MacGetPathTOBundledExecutableFile(CFStringRef filename)
{
    wxString theResult = wxT("");

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_INFO("Mac: Not bundled");
    }
    else
    {
        CFURLRef PTOurl = CFBundleCopyAuxiliaryExecutableURL(mainbundle, filename);
        if(PTOurl == NULL)
        {
            DEBUG_INFO("Mac: Cannot locate the file in the bundle.");
        }
        else
        {
            CFURLRef PTOAbsURL = CFURLCopyAbsoluteURL( PTOurl );
            if(PTOAbsURL == NULL)
            {
                DEBUG_INFO("Mac: Cannot convert the file path to absolute");
            }
            else
            {
                CFStringRef pathInCFString = CFURLCopyFileSystemPath(PTOAbsURL, kCFURLPOSIXPathStyle);
                if(pathInCFString == NULL)
                {
                    DEBUG_INFO("Mac: Failed to get URL in CFString");
                }
                else
                {
                    CFRetain( pathInCFString );
                    theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    DEBUG_INFO("Mac: returned the executable's full path in the application bundle");
                }
                CFRelease( PTOAbsURL );
            }
            CFRelease( PTOurl );
        }
    }
    return theResult;
}
#endif
