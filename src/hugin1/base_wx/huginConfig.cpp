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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "huginConfig.h"

#include "hugin/config_defaults.h"
#include "platform.h"

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
    placeholder[wxT("%#images")]=wxString::Format(wxT("%lu"), (unsigned long)pano.getNrOfImages());
    HuginBase::PanoramaOptions opts = pano.getOptions();
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
    for(Placeholdersmap::const_iterator it=placeholder.begin(); it!=placeholder.end(); ++it)
    {
        filename.Replace(it->first, it->second, true);
    };
    if(filename.empty())
    {
        filename=wxT("pano");
    };
    // check if template is an absolute path, if so ignore path from first image
    wxFileName fileName(filename);
    if (fileName.IsAbsolute())
    {
        return filename;
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
    for(Placeholdersmap::const_iterator it=placeholder.begin(); it!=placeholder.end(); ++it)
    {
        filename.Replace(it->first, it->second, true);
    };
    if(filename.empty())
    {
        filename=wxT("pano");
    };
    // check if template is an absolute path, if so ignore path from first image
    wxFileName fileName(filename);
    if (fileName.IsAbsolute())
    {
        return filename;
    };
    return pathPrefix+filename;
};
