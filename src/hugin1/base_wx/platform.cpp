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

#if defined __WXMAC__

#include <CoreFoundation/CFBundle.h>

// note this is a "create" function for ownership
CFStringRef MacCreateCFStringWithWxString(const wxString& string)
{
    return CFStringCreateWithCString(NULL,
                                     (const char*)string.mb_str(wxConvUTF8),
                                     kCFStringEncodingUTF8);
    
}

wxString MacGetPathToMainExecutableFileOfBundle(CFStringRef bundlePath)
{
    wxString theResult = wxT("");

    CFURLRef bundleURL = CFURLCreateWithString(NULL,bundlePath,NULL);
    if(bundleURL == NULL)
    {
        DEBUG_ERROR("Mac: CFURL from string failed." );
        return theResult;
    }
    
    CFBundleRef bundle = CFBundleCreate(NULL, bundleURL);
    CFRelease(bundleURL);
    
    if(bundle == NULL)
    {
        DEBUG_ERROR("Mac: CFBundleCreate (" << bundlePath << " ) failed" );
    }
    else
    {
        CFURLRef PTOurl = CFBundleCopyExecutableURL(bundle);
        CFRelease( bundle );
        if(PTOurl == NULL)
        {
            DEBUG_ERROR("Mac: Cannot locate the executable in the bundle.");
        }
        else
        {
            CFURLRef PTOAbsURL = CFURLCopyAbsoluteURL( PTOurl );
            CFRelease( PTOurl );
            if(PTOAbsURL == NULL)
            {
                DEBUG_ERROR("Mac: Cannot convert the file path to absolute");
            }
            else
            {
                CFStringRef pathInCFString = CFURLCopyFileSystemPath(PTOAbsURL, kCFURLPOSIXPathStyle);
                CFRelease( PTOAbsURL );
                if(pathInCFString == NULL)
                {
                    DEBUG_ERROR("Mac: Failed to get URL in CFString");
                }
                else
                {
                    CFRetain( pathInCFString );
                    theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    DEBUG_INFO("Mac: returned the executable's full path in the application bundle");
                }
            }
        }
    }
    return theResult;
}

#if defined MAC_SELF_CONTAINED_BUNDLE

wxString MacGetPathToBundledAppMainExecutableFile(CFStringRef filename)
{
    wxString theResult = wxT("");

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_ERROR("Mac: Not bundled");
    }
    else
    {
        CFURLRef XRCurl = CFBundleCopyResourceURL(mainbundle, filename, NULL, NULL);
        if(XRCurl == NULL)
        {
            DEBUG_ERROR("Mac: Cannot locate the bundle in bundle.");
        }
        else
        {
            CFBundleRef bundledBundle = CFBundleCreate(NULL, XRCurl);
            CFRelease( XRCurl );
            
            if(bundledBundle == NULL)
            {
                DEBUG_ERROR("Mac: Not bundled");
            }
            else
            {
                CFURLRef PTOurl = CFBundleCopyExecutableURL(bundledBundle);
                CFRelease( bundledBundle );
                
                if(PTOurl == NULL)
                {
                    DEBUG_ERROR("Mac: Cannot locate the executable in the bundle.");
                }
                else
                {
                    CFURLRef PTOAbsURL = CFURLCopyAbsoluteURL( PTOurl );
                    CFRelease( PTOurl );
                    if(PTOAbsURL == NULL)
                    {
                        DEBUG_ERROR("Mac: Cannot convert the file path to absolute");
                    }
                    else
                    {
                        CFStringRef pathInCFString = CFURLCopyFileSystemPath(PTOAbsURL, kCFURLPOSIXPathStyle);
                        CFRelease( PTOAbsURL );
                        if(pathInCFString == NULL)
                        {
                            DEBUG_ERROR("Mac: Failed to get URL in CFString");
                        }
                        else
                        {
                            CFRetain( pathInCFString );
                            theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                            DEBUG_INFO("Mac: returned the executable's full path in the application bundle");
                        }
                    }
                }
            }
        }
    }
    return theResult;
}

wxString MacGetPathToBundledResourceFile(CFStringRef filename)
{
    wxString theResult = wxT("");

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_ERROR("Mac: Not bundled");
    }
    else
    {
        CFURLRef XRCurl = CFBundleCopyResourceURL(mainbundle, filename, NULL, NULL);
        if(XRCurl == NULL)
        {
            DEBUG_ERROR("Mac: Cannot locate the file in bundle.");
        }
        else
        {
            CFStringRef pathInCFString = CFURLCopyFileSystemPath(XRCurl, kCFURLPOSIXPathStyle);
            CFRelease( XRCurl );
            if(pathInCFString == NULL)
            {
                DEBUG_ERROR("Mac: Failed to get URL in CFString");
            }
            else
            {
                CFRetain( pathInCFString );
                theResult = wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                DEBUG_INFO("Mac: returned the resource file's path in the application bundle");
            }
        }
    }
    return theResult;
}

wxString MacGetPathToBundledExecutableFile(CFStringRef filename)
{
    wxString theResult = wxT("");

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_ERROR("Mac: Not bundled");
    }
    else
    {
        CFURLRef PTOurl = CFBundleCopyAuxiliaryExecutableURL(mainbundle, filename);
        if(PTOurl == NULL)
        {
            DEBUG_ERROR("Mac: Cannot locate the file in the bundle.");
        }
        else
        {
            CFURLRef PTOAbsURL = CFURLCopyAbsoluteURL( PTOurl );
            if(PTOAbsURL == NULL)
            {
                DEBUG_ERROR("Mac: Cannot convert the file path to absolute");
            }
            else
            {
                CFStringRef pathInCFString = CFURLCopyFileSystemPath(PTOAbsURL, kCFURLPOSIXPathStyle);
                CFRelease( PTOAbsURL );
                if(pathInCFString == NULL)
                {
                    DEBUG_ERROR("Mac: Failed to get URL in CFString");
                }
                else
                {
                    CFRetain( pathInCFString );
                    theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    DEBUG_INFO("Mac: returned the executable's full path in the application bundle");
                }
            }
            CFRelease( PTOurl );
        }
    }
    return theResult;
}


wxString MacGetPathToUserDomainTempDir()
{
    wxString tmpDirPath = wxT("");
    
    FSRef tempDirRef;
    OSErr err = FSFindFolder(kUserDomain, kTemporaryFolderType, kCreateFolder, &tempDirRef);
    if (err == noErr)
    {
        CFURLRef tempDirURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &tempDirRef);
        if (tempDirURL != NULL)
        {
            CFStringRef tmpPath = CFURLCopyFileSystemPath(tempDirURL, kCFURLPOSIXPathStyle);
            CFRetain(tmpPath);
            tmpDirPath = wxMacCFStringHolder(tmpPath).AsString(wxLocale::GetSystemEncoding());
            
            CFRelease(tempDirURL);
        }
    }
    
    return tmpDirPath;
}

#endif // MAC_SELF_CONTAINED_BUNDLE

#endif // __WXMAC__