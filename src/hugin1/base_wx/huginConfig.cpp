// -*- c-basic-offset: 4 -*-

/** @file huginConfig.cpp
 *
 *  @brief functions for interaction with the hugin configuration file
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

#include "huginConfig.h"
#include "hugin_version.h"

#include "hugin/config_defaults.h"
#include "platform.h"


using namespace PT;

std::string getProgram(wxConfigBase * config, wxString bindir, wxString file, wxString name)
{
    std::string pname;
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    
    if (config->Read(name + wxT("/Custom"), 0l)) {
        wxString fn = config->Read(name + wxT("/Exe"),wxT(""));
        if (wxFileName::FileExists(fn)) {
            pname = fn.mb_str();
            return pname;
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found as specified in preferences, reverting to bundled version"), file.c_str()), _("Error"));
        }
    }

    if(name == wxT("Exiftool")) {
        wxString exiftoolDirPath = MacGetPathToBundledResourceFile(CFSTR("ExifTool"));
        if(exiftoolDirPath != wxT(""))
        {
            pname = (exiftoolDirPath+wxT("/")+file).mb_str();
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), file.c_str()), _("Error"));
            pname = file.mb_str();
        }
        return pname;
    }
        
    CFStringRef filename = MacCreateCFStringWithWxString(file);
    wxString fn = MacGetPathToBundledExecutableFile(filename);
    CFRelease(filename);
    
    if(fn == wxT(""))
    {
        wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), file.c_str()), _("Error"));
        pname = file.mb_str();
    } else {
        pname = fn.mb_str();
    }
    return pname;

#elif defined __WXMSW__
    if (config->Read(name + wxT("/Custom"), 0l)) {
        wxString fn = config->Read(name + wxT("/Exe"),wxT(""));
        if (wxFileName::FileExists(fn)) {
            pname = fn.mb_str();
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found as specified in preferences, reverting to bundled version"), file.c_str()), _("Error"));
            pname = (bindir + wxT("\\") +  file).mb_str();
        }
    } else {
        pname = (bindir + wxT("\\") + file).mb_str();
    }
    return pname;
#else
    // unix, never bundled
    if (config->Read(name + wxT("/Custom"), 0l)) {
        wxString fn = config->Read(name + wxT("/Exe"),wxT(""));
        pname = fn.mb_str();
        return pname;
	// TODO: need to search path, a simple FileExists() doesn't work as expected.
	/*
        if (wxFileName::FileExists(fn)) {
            pname = fn.mb_str();
            return pname;
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found as specified in preferences, reverting to system path"), file.c_str()), _("Error"));
        }
	*/
    }
    pname = file.mb_str();
    return pname;
#endif
}


PTPrograms getPTProgramsConfig(wxString bundledBinDir, wxConfigBase * config)
{
    PTPrograms progs;

    wxString bindir;
#ifndef __WXGTK__
    // add trailing directory separator, if needed
    wxFileName bindirFN = wxFileName::DirName(bundledBinDir);
    bindir =  bindirFN.GetPath();
#endif

    progs.nona = getProgram(config,bindir, wxT("nona"), wxT("nona"));
    progs.hdrmerge= getProgram(config,bindir, wxT("hugin_hdrmerge"), wxT("hugin_hdrmerge"));

    progs.PTmender = getProgram(config,bindir, wxT("PTmender"), wxT("PTmender"));
    progs.PTblender= getProgram(config,bindir, wxT("PTblender"), wxT("PTblender"));
    progs.PTmasker= getProgram(config,bindir, wxT("PTmasker"), wxT("PTmasker"));
    progs.PTroller= getProgram(config,bindir, wxT("PTroller"), wxT("PTroller"));

    progs.enblend = getProgram(config,bindir, wxT("enblend"), wxT("Enblend"));
    progs.enblend_opts = config->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str();
    progs.enfuse = getProgram(config,bindir, wxT("enfuse"), wxT("Enfuse"));
    progs.enfuse_opts = config->Read(wxT("/Enfuse/Args"), wxT(HUGIN_ENFUSE_ARGS)).mb_str();

    progs.exiftool = getProgram(config,bindir, wxT("exiftool"), wxT("Exiftool"));
    progs.exiftool_opts = config->Read(wxT("/Exiftool/CopyArgs"), wxT(HUGIN_EXIFTOOL_COPY_ARGS)).mb_str();

    // smartblend (never bundled)
    progs.smartblend = config->Read(wxT("/Smartblend/SmartblendExe"),wxT("smartblend.exe")).mb_str();
    progs.smartblend_opts = config->Read(wxT("/Smartblend/SmartblendArgs"),wxT(HUGIN_SMARTBLEND_ARGS)).mb_str();

    return progs;
}

wxString getExePath(wxString argv0)
{
    wxString huginPath;
#ifdef __WXMSW__
    // special code to find location of hugin.exe under windows
    #if wxUSE_UNICODE
        WCHAR tpath[MAX_PATH];
    #else //ANSI
        char tpath[MAX_PATH];
    #endif
    tpath[0] = 0;
    GetModuleFileName(0,tpath,sizeof(tpath)-1);

    #ifdef wxUSE_UNICODE
        wxString path(tpath);
    #else
        wxString path(tpath, wxConvLocal);
    #endif
    wxFileName::SplitPath( path, &huginPath, NULL, NULL );
#else
    wxFileName::SplitPath( argv0, &huginPath, NULL, NULL );
#endif
    return huginPath;
}

void updateHuginConfig(wxConfigBase * config)
{
    // TODO: read hugin config version, based on SVN_BUILD
    long revision = config->Read(wxT("HuginRevision"), 0l);
    if (revision <= 2797) {
        config->DeleteEntry(wxT("/Exiftool/CopyArgs"));
        config->DeleteEntry(wxT("/Assistant/panoDownsizeFactor"));
    }
    // set new config
    config->Write(wxT("HuginRevision"), HUGIN_WC_REVISION);
}
