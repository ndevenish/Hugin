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

std::string getProgram(wxConfigBase * config, wxString bindir, wxString file, wxString name) throw (wxString)
{
    std::string pname;
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE

    if (config->Read(name + wxT("/Custom"), 0l)) {
        wxString fn = config->Read(name + wxT("/Exe"),wxT(""));
        if (wxFileName::FileExists(fn)) {
            pname = fn.mb_str(HUGIN_CONV_FILENAME);
            return pname;
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found as specified in preferences, reverting to bundled version"), file.c_str()), _("Error"));
        }
    }

    if(name == wxT("Exiftool")) {
        wxString exiftoolDirPath = MacGetPathToBundledResourceFile(CFSTR("ExifTool"));
        if(exiftoolDirPath != wxT(""))
        {
            pname = (exiftoolDirPath+wxT("/")+file).mb_str(HUGIN_CONV_FILENAME);
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), file.c_str()), _("Error"));
            pname = file.mb_str(HUGIN_CONV_FILENAME);
        }
        return pname;
    }

    CFStringRef filename = MacCreateCFStringWithWxString(file);
    wxString fn = MacGetPathToBundledExecutableFile(filename);
    CFRelease(filename);

    if(fn == wxT(""))
    {
        wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), file.c_str()), _("Error"));
        pname = file.mb_str(HUGIN_CONV_FILENAME);
    } else {
        pname = fn.mb_str(HUGIN_CONV_FILENAME);
    }
    return pname;

#elif defined __WXMSW__
    if (config->Read(name + wxT("/Custom"), 0l)) {
        wxString fn = config->Read(name + wxT("/Exe"),wxT(""));
        if (wxFileName::FileExists(fn)) {
            pname = fn.mb_str(HUGIN_CONV_FILENAME);
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found as specified in preferences, reverting to bundled version"), file.c_str()), _("Error"));
            pname = (bindir + wxT("\\") +  file).mb_str(HUGIN_CONV_FILENAME);
        }
    } else {
        pname = (bindir + wxT("\\") + file).mb_str(HUGIN_CONV_FILENAME);
    }
    return pname;
#else
    // unix, never bundled
    if (config->Read(name + wxT("/Custom"), 0l)) {
        wxString fn = config->Read(name + wxT("/Exe"),wxT(""));
        pname = fn.mb_str(HUGIN_CONV_FILENAME);
        if (pname == "")
            throw wxString::Format(_("Program %s not found in preferences, reverting to default value"), file.c_str());
        return pname;
	// TODO: need to search path, a simple FileExists() doesn't work as expected.
	/*
        if (wxFileName::FileExists(fn)) {
            pname = fn.mb_str(HUGIN_CONV_FILENAME);
            return pname;
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found as specified in preferences, reverting to system path"), file.c_str()), _("Error"));
        }
	*/
    }
    pname = file.mb_str(HUGIN_CONV_FILENAME);
    if (pname == "")
        throw wxString::Format(_("Program %s not found in preferences, reverting to default value"), file.c_str());
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

    try {
        progs.nona = getProgram(config,bindir, wxT("nona"), wxT("nona"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.hdrmerge= getProgram(config,bindir, wxT("hugin_hdrmerge"), wxT("hugin_hdrmerge"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }

    try {
        progs.PTmender = getProgram(config,bindir, wxT("PTmender"), wxT("PTmender"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.PTblender= getProgram(config,bindir, wxT("PTblender"), wxT("PTblender"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.PTmasker= getProgram(config,bindir, wxT("PTmasker"), wxT("PTmasker"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.PTroller= getProgram(config,bindir, wxT("PTroller"), wxT("PTroller"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }

    try {
        progs.enblend = getProgram(config,bindir, wxT("enblend"), wxT("Enblend"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.enblend_opts = config->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.enfuse = getProgram(config,bindir, wxT("enfuse"), wxT("Enfuse"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.enfuse_opts = config->Read(wxT("/Enfuse/Args"), wxT(HUGIN_ENFUSE_ARGS)).mb_str(wxConvLocal);
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }

    try {
        progs.exiftool = getProgram(config,bindir, wxT("exiftool"), wxT("Exiftool"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.exiftool_opts = config->Read(wxT("/Exiftool/CopyArgs"), wxT(HUGIN_EXIFTOOL_COPY_ARGS)).mb_str(wxConvLocal);
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }

    // smartblend (never bundled)
    try {
        progs.smartblend = config->Read(wxT("/Smartblend/SmartblendExe"),wxT("smartblend.exe")).mb_str(HUGIN_CONV_FILENAME);
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.smartblend_opts = config->Read(wxT("/Smartblend/SmartblendArgs"),wxT(HUGIN_SMARTBLEND_ARGS)).mb_str(wxConvLocal);
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }

    return progs;
}

AssistantPrograms getAssistantProgramsConfig(wxString bundledBinDir, wxConfigBase * config)
{
    AssistantPrograms progs;

    wxString bindir;
#ifndef __WXGTK__
    // add trailing directory separator, if needed
    wxFileName bindirFN = wxFileName::DirName(bundledBinDir);
    bindir =  bindirFN.GetPath();
#endif

    try {
        progs.icpfind = getProgram(config,bindir, wxT("icpfind"), wxT("icpfind"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.celeste= getProgram(config,bindir, wxT("celeste_standalone"), wxT("celeste_standalone"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }

    try {
        progs.cpclean = getProgram(config,bindir, wxT("cpclean"), wxT("cpclean"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.autooptimiser= getProgram(config,bindir, wxT("autooptimiser"), wxT("autooptimiser"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.pano_modify= getProgram(config,bindir, wxT("pano_modify"), wxT("pano_modify"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try {
        progs.checkpto= getProgram(config,bindir, wxT("checkpto"), wxT("checkpto"));
    } catch (wxString s) {
        wxMessageBox(s, _("Warning"));
    }
    try
    {
        progs.linefind= getProgram(config,bindir, wxT("linefind"), wxT("linefind"));
    }
    catch (wxString s)
    {
        wxMessageBox(s, _("Warning"));
    }
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

// functions to handle with default project/output filenames
typedef std::map<wxString, wxString> Placeholdersmap;

void FillDefaultPlaceholders(Placeholdersmap & placeholder)
{
    placeholder[wxT("%firstimage")]=_("first image");
    placeholder[wxT("%lastimage")]=_("last image");
    placeholder[wxT("%#images")]=wxT("0");
    placeholder[wxT("%directory")]=_("directory");
    placeholder[wxT("%projection")]=_("Equirectangular");
    placeholder[wxT("%focallength")]=wxT("28");
    wxDateTime datetime=wxDateTime(13,wxDateTime::May,2012,11,35);
    placeholder[wxT("%date")]=datetime.FormatDate();
    placeholder[wxT("%time")]=datetime.FormatTime();
    placeholder[wxT("%maker")]=_("Camera maker");
    placeholder[wxT("%model")]=_("Camera model");
    placeholder[wxT("%lens")]=_("Lens");
};

void FillPlaceholders(Placeholdersmap & placeholder, const HuginBase::Panorama & pano)
{
    const HuginBase::SrcPanoImage & img0=pano.getImage(0);
    wxFileName firstImg(wxString(img0.getFilename().c_str(),HUGIN_CONV_FILENAME));
    placeholder[wxT("%firstimage")]=firstImg.GetName();
    if(firstImg.GetDirCount()>0)
    {
        placeholder[wxT("%directory")]=firstImg.GetDirs().Last();
    };
    placeholder[wxT("%focallength")]=wxString::Format(wxT("%.0f"), img0.getExifFocalLength());
    struct tm exifdatetime;
    if(img0.getExifDateTime(&exifdatetime)==0)
    {
        wxDateTime datetime=wxDateTime(exifdatetime);
        placeholder[wxT("%date")]=datetime.FormatDate();
        placeholder[wxT("%time")]=datetime.FormatTime();
    };
    placeholder[wxT("%maker")]=wxString(img0.getExifMake().c_str(), wxConvLocal);
    placeholder[wxT("%model")]=wxString(img0.getExifModel().c_str(), wxConvLocal);
    placeholder[wxT("%lens")]=wxString(img0.getExifLens().c_str(), wxConvLocal);

    wxFileName lastImg(wxString(pano.getImage(pano.getNrOfImages()-1).getFilename().c_str(),HUGIN_CONV_FILENAME));
    placeholder[wxT("%lastimage")]=lastImg.GetName();
    placeholder[wxT("%#images")]=wxString::Format(wxT("%d"), pano.getNrOfImages());
    PanoramaOptions opts=pano.getOptions();
    pano_projection_features proj;
    if (panoProjectionFeaturesQuery(opts.getProjection(), &proj))
    {
        wxString str2(proj.name, wxConvLocal);
        placeholder[wxT("%projection")]=wxGetTranslation(str2);
    }
    else
    {
        placeholder[wxT("%projection")]=_("unknown projection");
    };
};

wxString getDefaultProjectName(const HuginBase::Panorama & pano,const wxString filenameTemplate)
{
    wxString filename;
    if(filenameTemplate.IsEmpty())
    {
        filename=wxConfigBase::Get()->Read(wxT("ProjectFilename"), wxT(HUGIN_DEFAULT_PROJECT_NAME));
#ifdef __WXMSW__
        filename.Replace(wxT("/"), wxT("\\"), true);
#endif
    }
    else
    {
        filename=filenameTemplate;
    };
    wxString pathPrefix;
    Placeholdersmap placeholder;
    if(pano.getNrOfImages()>0)
    {
        FillPlaceholders(placeholder, pano);
        wxFileName firstImg(wxString(pano.getImage(0).getFilename().c_str(),HUGIN_CONV_FILENAME));
        pathPrefix=firstImg.GetPathWithSep();
    }
    else
    {
        FillDefaultPlaceholders(placeholder);
    };
    // now replace all placeholder
    for(Placeholdersmap::const_iterator it=placeholder.begin(); it!=placeholder.end(); it++)
    {
        filename.Replace(it->first, it->second, true);
    };
    if(filename.empty())
    {
        filename=wxT("pano");
    };
    return pathPrefix+filename;
};

/** gets the default output prefix, based on filename and images in project
  * the setting is read from the preferences */
wxString getDefaultOutputName(const wxString projectname, const HuginBase::Panorama & pano, const wxString filenameTemplate)
{
    wxFileName project;
    if (projectname.IsEmpty())
    {
        project=getDefaultProjectName(pano);
    }
    else
    {
        project=projectname;
    }
    if(project.HasExt())
    {
        project.ClearExt();
    };

    wxString filename;
    if(filenameTemplate.IsEmpty())
    {
        filename=wxConfigBase::Get()->Read(wxT("OutputFilename"), wxT(HUGIN_DEFAULT_PROJECT_NAME));
#ifdef __WXMSW__
        filename.Replace(wxT("/"), wxT("\\"), true);
#endif
    }
    else
    {
        filename=filenameTemplate;
    };
    wxString pathPrefix=project.GetPathWithSep();
    Placeholdersmap placeholder;
    if(pano.getNrOfImages()>0)
    {
        FillPlaceholders(placeholder, pano);
        wxFileName firstImg(wxString(pano.getImage(0).getFilename().c_str(),HUGIN_CONV_FILENAME));
    }
    else
    {
        FillDefaultPlaceholders(placeholder);
    };
    placeholder.insert(std::make_pair(wxT("%projectname"), project.GetName()));
    // now replace all placeholder
    for(Placeholdersmap::const_iterator it=placeholder.begin(); it!=placeholder.end(); it++)
    {
        filename.Replace(it->first, it->second, true);
    };
    if(filename.empty())
    {
        filename=wxT("pano");
    };
    return pathPrefix+filename;
};
