// -*- c-basic-offset: 4 -*-

/** @file hugin1/base_wx/platform.cpp
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

#if defined __WXMAC__ || defined __WXOSX_COCOA__

#include <CoreFoundation/CFBundle.h>
#if wxCHECK_VERSION(2,9,0)
 #include "wx/osx/core/cfstring.h"
#endif
#include <iostream>
#include <stdio.h>
#include "wx/utils.h"

using namespace std;

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

    CFURLRef bundleURL = CFURLCreateWithFileSystemPath(NULL, bundlePath, kCFURLPOSIXPathStyle, TRUE);

    if(bundleURL == NULL)
    {
        DEBUG_INFO("Mac: CFURL from string (" << bundlePath << ") failed." );
        return theResult;
    }
    
    CFBundleRef bundle = CFBundleCreate(NULL, bundleURL);
    CFRelease(bundleURL);
    
    if(bundle == NULL)
    {
        DEBUG_INFO("Mac: CFBundleCreate (" << bundlePath << " ) failed" );
    }
    else
    {
        CFURLRef PTOurl = CFBundleCopyExecutableURL(bundle);
        CFRelease( bundle );
        if(PTOurl == NULL)
        {
            DEBUG_INFO("Mac: Cannot locate the executable in the bundle.");
        }
        else
        {
            CFURLRef PTOAbsURL = CFURLCopyAbsoluteURL( PTOurl );
            CFRelease( PTOurl );
            if(PTOAbsURL == NULL)
            {
                DEBUG_INFO("Mac: Cannot convert the file path to absolute");
            }
            else
            {
                CFStringRef pathInCFString = CFURLCopyFileSystemPath(PTOAbsURL, kCFURLPOSIXPathStyle);
                CFRelease( PTOAbsURL );
                if(pathInCFString == NULL)
                {
                    DEBUG_INFO("Mac: Failed to get URL in CFString");
                }
                else
                {
                    CFRetain( pathInCFString );
                    #if wxCHECK_VERSION(2,9,0)
                      theResult =  wxCFStringRef(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    #else
                      theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    #endif
                    DEBUG_INFO("Mac: the executable's full path in the application bundle: " << theResult.mb_str(wxConvLocal));
                }
            }
        }
    }
    return theResult;
}

wxString MacGetPathToMainExecutableFileOfRegisteredBundle(CFStringRef BundleIdentifier)
{
    wxString theResult = wxT("");
	
	FSRef appRef;
	CFURLRef bundleURL;
	FSRef actuallyLaunched;
	OSStatus err;
	FSRef documentArray[1]; // Don't really need an array if we only have 1 item
	LSLaunchFSRefSpec launchSpec;
	//Boolean  isDir;
	
	err = LSFindApplicationForInfo(kLSUnknownCreator,
								   CFSTR("net.sourceforge.hugin.PTBatcherGUI"),
								   NULL,
								   &appRef,
								   &bundleURL);
	if (err != noErr) {
		// error, can't find PTBatcherGUI
		cout << "PTBatcherGui check failed \n" << endl;
		wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), wxT("open")), _("Error"));
	}
    if(bundleURL == NULL)
    {
        DEBUG_INFO("Mac: CFURL from string (" << bundleURL << ") failed." );
        return theResult;
    }
    
    CFBundleRef bundle = CFBundleCreate(NULL, bundleURL);
    CFRelease(bundleURL);
    
    if(bundle == NULL)
    {
        DEBUG_INFO("Mac: CFBundleCreate (" << bundleURL << " ) failed" );
    }
    else
    {
        CFURLRef PTOurl = CFBundleCopyExecutableURL(bundle);
        CFRelease( bundle );
        if(PTOurl == NULL)
        {
            DEBUG_INFO("Mac: Cannot locate the executable in the bundle.");
        }
        else
        {
            CFURLRef PTOAbsURL = CFURLCopyAbsoluteURL( PTOurl );
            CFRelease( PTOurl );
            if(PTOAbsURL == NULL)
            {
                DEBUG_INFO("Mac: Cannot convert the file path to absolute");
            }
            else
            {
                CFStringRef pathInCFString = CFURLCopyFileSystemPath(PTOAbsURL, kCFURLPOSIXPathStyle);
                CFRelease( PTOAbsURL );
                if(pathInCFString == NULL)
                {
                    DEBUG_INFO("Mac: Failed to get URL in CFString");
                }
                else
                {
                    CFRetain( pathInCFString );
                    #if wxCHECK_VERSION(2,9,0)
                      theResult =  wxCFStringRef(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    #else
                      theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    #endif
                    DEBUG_INFO("Mac: the executable's full path in the application bundle: " << theResult.mb_str(wxConvLocal));
                }
            }
        }
    }
	cout << "PTBatcherGui check returned  value " << theResult << "\n" << endl;
    return theResult;
}

#if defined MAC_SELF_CONTAINED_BUNDLE

wxString MacGetPathToBundledAppMainExecutableFile(CFStringRef appname)
{
    wxString theResult = wxT("");

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_INFO("Mac: Not bundled");
    }
    else
    {
        CFURLRef XRCurl = CFBundleCopyResourceURL(mainbundle, appname, NULL, NULL);
        if(XRCurl == NULL)
        {
            DEBUG_INFO("Mac: Cannot locate the bundle in bundle.");
        }
        else
        {
            CFBundleRef bundledBundle = CFBundleCreate(NULL, XRCurl);
            CFRelease( XRCurl );
            
            if(bundledBundle == NULL)
            {
                DEBUG_INFO("Mac: Not bundled");
            }
            else
            {
                CFURLRef PTOurl = CFBundleCopyExecutableURL(bundledBundle);
                CFRelease( bundledBundle );
                
                if(PTOurl == NULL)
                {
                    DEBUG_INFO("Mac: Cannot locate the executable in the bundle.");
                }
                else
                {
                    CFURLRef PTOAbsURL = CFURLCopyAbsoluteURL( PTOurl );
                    CFRelease( PTOurl );
                    if(PTOAbsURL == NULL)
                    {
                        DEBUG_INFO("Mac: Cannot convert the file path to absolute");
                    }
                    else
                    {
                        CFStringRef pathInCFString = CFURLCopyFileSystemPath(PTOAbsURL, kCFURLPOSIXPathStyle);
                        CFRelease( PTOAbsURL );
                        if(pathInCFString == NULL)
                        {
                            DEBUG_INFO("Mac: Failed to get URL in CFString");
                        }
                        else
                        {
                            CFRetain( pathInCFString );
                            #if wxCHECK_VERSION(2,9,0)
                              theResult =  wxCFStringRef(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                            #else
                              theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                            #endif
                            DEBUG_INFO("Mac: the executable's full path in the application bundle: " << theResult.mb_str(wxConvLocal));
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
            CFRelease( XRCurl );
            if(pathInCFString == NULL)
            {
                DEBUG_INFO("Mac: Failed to get URL in CFString");
            }
            else
            {
                CFRetain( pathInCFString );
                #if wxCHECK_VERSION(2,9,0)
                  theResult = wxCFStringRef(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                #else
                  theResult = wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                #endif
                DEBUG_INFO("Mac: the resource file's path in the application bundle: " << theResult.mb_str(wxConvLocal));
            }
        }
    }
    return theResult;
}

wxString MacGetPathToBundledFrameworksDirectory()
{
    wxString theResult = wxT("");
    
    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_INFO("Mac: Not bundled");
    }
    else
    {
        CFURLRef XRCurl = CFBundleCopyBundleURL(mainbundle);
        if(XRCurl == NULL)
        {
            DEBUG_INFO("Mac: Cannot locate the file in bundle.");
        }
        else
        {
            CFStringRef pathInCFString = CFURLCopyFileSystemPath(XRCurl, kCFURLPOSIXPathStyle);
            CFRelease( XRCurl );
            if(pathInCFString == NULL)
            {
                DEBUG_INFO("Mac: Failed to get URL in CFString");
            }
            else
            {
                CFRetain( pathInCFString );
#if wxCHECK_VERSION(2,9,0)
                theResult = wxCFStringRef(pathInCFString).AsString(wxLocale::GetSystemEncoding());
#else
                theResult = wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
#endif
                DEBUG_INFO("Mac: the Frameworks file's path in the application bundle: " << theResult.mb_str(wxConvLocal));
            }
        }
    }
    return theResult + wxT("/Contents/Frameworks");
}

wxString MacGetPathToBundledExecutableFile(CFStringRef filename)
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
                CFRelease( PTOAbsURL );
                if(pathInCFString == NULL)
                {
                    DEBUG_INFO("Mac: Failed to get URL in CFString");
                }
                else
                {
                    CFRetain( pathInCFString );
                    #if wxCHECK_VERSION(2,9,0)
                      theResult =  wxCFStringRef(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    #else
                      theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    #endif
                    DEBUG_INFO("Mac: executable's full path in the application bundle: " << theResult.mb_str(wxConvLocal));
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
            #if wxCHECK_VERSION(2,9,0)
              tmpDirPath = wxCFStringRef(tmpPath).AsString(wxLocale::GetSystemEncoding());
            #else
              tmpDirPath = wxMacCFStringHolder(tmpPath).AsString(wxLocale::GetSystemEncoding());
            #endif
            
            CFRelease(tempDirURL);
        }
    }
    
    return tmpDirPath;
}

wxString MacGetPathToUserAppSupportAutoPanoFolder()
{
    wxString appSupportAutoPanoFolder = wxT("");

    FSRef appSupportFolder;
    OSErr err = FSFindFolder(kUserDomain,kApplicationSupportFolderType,kDontCreateFolder,&appSupportFolder);
    if( err == noErr)
    {
        CFURLRef appSupportFolderURL = CFURLCreateFromFSRef(kCFAllocatorDefault,&appSupportFolder);
        CFURLRef appSupportHugin = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault,appSupportFolderURL,CFSTR("Hugin"),true);
        CFURLRef autopanoURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault,appSupportHugin,CFSTR("Autopano"),true);
        CFStringRef tmpPath = CFURLCopyFileSystemPath(autopanoURL,  kCFURLPOSIXPathStyle);
        CFRetain(tmpPath);
        #if wxCHECK_VERSION(2,9,0)
          appSupportAutoPanoFolder = wxCFStringRef(tmpPath).AsString(wxLocale::GetSystemEncoding());
        #else
          appSupportAutoPanoFolder = wxMacCFStringHolder(tmpPath).AsString(wxLocale::GetSystemEncoding());
        #endif

        CFRelease(autopanoURL);
    }
    return appSupportAutoPanoFolder;
}


#endif // MAC_SELF_CONTAINED_BUNDLE

#endif // __WXMAC__

const wxString getInvalidCharacters()
{
#if defined __WXMSW__
    // the characters :"*?<>| are not allowed in filenames, these are handled well by the file dialog
    // we need only to check for characters, which does not work with the makefiles
    return wxT("=;%");
#else
    // the characters =;:% does not work with the makefile
    // we are also rejecting the characters <>*?| which are principally allowed in filenames but will probably make problems when used
    // the double quote does not work with the panotools file format, so also reject
    //@BUG tilde ~ and backslash \ are not working with vigraimpex, if this works again these characters can be removed from the list
    return wxT("=;:%*?<>|\"\\~");
#endif
};

bool containsInvalidCharacters(const wxString stringToTest)
{
    if(stringToTest.IsEmpty())
        return false;
    wxString forbiddenChars=getInvalidCharacters();
    for(unsigned int j=0;j<forbiddenChars.size();j++)
    {
        if(stringToTest.Find(forbiddenChars[j])!=wxNOT_FOUND)
            return true;
    };
    return false;
};

void ShowFilenameWarning(wxWindow* parent, const wxArrayString filelist)
{
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, parent, wxT("dlg_warning_filename"));
    XRCCTRL(dlg, "dlg_warning_text", wxStaticText)->SetLabel(wxString::Format(_("The filename(s) contains one of the following invalid characters: %s\nHugin can not work with these filenames. Please rename your file(s) and try again."), getInvalidCharacters().c_str()));
    XRCCTRL(dlg, "dlg_warning_list", wxListBox)->Append(filelist);
    dlg.Fit();
    dlg.CenterOnScreen();
    dlg.ShowModal();
};
