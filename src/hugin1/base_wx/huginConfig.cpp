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

PTPrograms getPTProgramsConfig(wxString huginRoot, wxConfigBase * config)
{
    PTPrograms progs;

    std::string root = (const char *) huginRoot.mb_str();
    std::string enblendroot;
    std::string ptroot;

#if defined __WXMSW__
    enblendroot = root + "/enblend/";
    ptroot = root + "/panotools/";

#elif defined __WXMAC__
    // dangelo: on OSX, the tools are inside the application bundle, but I don't know
    // where.
    root = "";
    enblendroot = "";
    ptroot = "";
#else
    // on unix, custom tools don't make any sense, since on linux, hugin is never
    // bundled. Just just the executables in the path
    root = "";
    enblendroot = "";
    ptroot = "";
#endif

    if (config->Read(wxT("/Panotools/PTmenderExeCustom"),0l)) {
        progs.PTmender = config->Read(wxT("/PanoTools/PTmender"),wxT(HUGIN_PT_MENDER_EXE)).mb_str();
    } else {
        progs.PTmender = ptroot + "PTmender";
    }

    if (config->Read(wxT("/Panotools/PTblenderExeCustom"),0l)) {
        progs.PTblender = config->Read(wxT("/PanoTools/PTblender"),wxT(HUGIN_PT_BLENDER_EXE)).mb_str();
    } else {
        progs.PTblender = ptroot + "PTblender";
    }

    if (config->Read(wxT("/Panotools/PTmaskerExeCustom"),0l)) {
        progs.PTmasker = config->Read(wxT("/PanoTools/PTmasker"),wxT(HUGIN_PT_MASKER_EXE)).mb_str();
    } else {
        progs.PTmasker = ptroot + "PTmasker";
    }

    if (config->Read(wxT("/Panotools/PTrollerExeCustom"),0l)) {
        progs.PTroller = config->Read(wxT("/PanoTools/PTroller"),wxT(HUGIN_PT_ROLLER_EXE)).mb_str();
    } else {
        progs.PTroller = ptroot + "PTroller";
    }


    // enblend
    if (config->Read(wxT("/Enblend/EnblendExeCustom"),0l)) {
        progs.enblend = config->Read(wxT("/Enblend/EnblendExe"),wxT(HUGIN_ENBLEND_EXE)).mb_str();
    } else {
        progs.enblend = enblendroot + "enblend";
    }
    progs.enblend_opts = config->Read(wxT("/Enblend/EnblendArgs"), wxT(HUGIN_ENBLEND_ARGS)).mb_str();

    // smartblend (never bundled)
    progs.smartblend = config->Read(wxT("/Smartblend/SmartblendExe"),wxT(HUGIN_SMARTBLEND_EXE)).mb_str();
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