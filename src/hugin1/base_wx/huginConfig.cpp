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
#include "hugin/config_defaults.h"

using namespace PT;

std::string getProgram(wxConfigBase * config, wxString bindir, wxString file, wxString name)
{
    std::string pname;
    if (config->Read(name + wxT("/Custom"), 0l)) {
        wxString fn = config->Read(name + wxT("/Exe"),wxT(""));
        if (wxFileName::FileExists(fn)) {
            pname = fn.mb_str();
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found, reverting to bundled version"), file.c_str()), _("Error"));
            pname = (bindir + file).mb_str();
        }
    } else {
        pname = file.mb_str();
    }
    return pname;
}


PTPrograms getPTProgramsConfig(wxString bundledBinDir, wxConfigBase * config)
{
    PTPrograms progs;


    wxString bindir;
#ifndef __WXGTK__
    // add trailing directory separator, if needed
    wxFileName bindirFN = wxFileName::DirName(bundledBinDir);
    bindir =  bindirFN.GetPath();
    wxMessageBox(bindir, wxT("bindir"));
#endif
    // on unix, custom tools don't make any sense, since on unix, hugin is never
    // bundled. Just just the executables in the path

    progs.PTmender = getProgram(config,bindir, wxT("PTmender"), wxT("PTmender"));
    progs.PTblender= getProgram(config,bindir, wxT("PTblender"), wxT("PTblender"));
    progs.PTmasker= getProgram(config,bindir, wxT("PTmasker"), wxT("PTmasker"));
    progs.PTroller= getProgram(config,bindir, wxT("PTroller"), wxT("PTroller"));

    progs.enblend = getProgram(config,bindir, wxT("enblend"), wxT("Enblend"));
    progs.enblend_opts = config->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str();
    progs.enfuse = getProgram(config,bindir, wxT("enfuse"), wxT("Enfuse"));
    progs.enfuse_opts = config->Read(wxT("/Enfuse/Args"), wxT(HUGIN_ENFUSE_ARGS)).mb_str();

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