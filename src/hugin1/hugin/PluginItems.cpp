// -*- c-basic-offset: 4 -*-

/** @file PluginItems.cpp
 *
 *  @brief Reading python plugins metadata
 *
 *  @author Y. Levy, T. Modes
 *
 */

/*
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

#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <hugin_version.h>

//for natural sorting
#include "hugin_utils/alphanum.h"
#include "PluginItems.h"

//for console/debugging output
#include <iostream>

bool comparePluginItem(PluginItem item1,PluginItem item2)
{
    int res=doj::alphanum_comp(
        std::string(item1.GetCategory().mb_str(wxConvLocal)),
        std::string(item2.GetCategory().mb_str(wxConvLocal)));
    if(res<0)
    {
        return true;
    }
    else
    {
        if(res==0)
        {
            return (doj::alphanum_comp(
                std::string(item1.GetName().mb_str(wxConvLocal)),
                std::string(item2.GetName().mb_str(wxConvLocal)))<0);
        }
        else
        {
            return false;
        };
    };
};

bool compareVersion(wxString v1, wxString v2)
{
    return doj::alphanum_comp(std::string(v1.mb_str(wxConvLocal)),std::string(v2.mb_str(wxConvLocal))) < 0;
};

PluginItem::PluginItem(wxFileName filename)
{
    // default category if nothing else found
    m_category = _("Miscellaneous");
    // default plugin name if nothing else found is the file name
    m_name=filename.GetFullName();
    m_filename=filename;
    m_description=wxT("");
    m_validAPI=true;
    ParseMetadata();
};

void PluginItem::ParseMetadata()
{
    // use wxWidgets to read the plugin files in search for meta data
    wxFileInputStream in(m_filename.GetFullPath());
    wxTextInputStream text(in);

    // read the plugin file and search if it contains meta data
    bool foundCategory=false;
    bool foundName=false;
    bool foundAPImin=false;
    bool foundAPImax=false;
    bool foundSYS=false;
    bool foundDescription=false;
#if defined __WXMSW__
    wxString system(wxT("win"));
#elif defined __WXMAC__
    wxString system(wxT("mac"));
#else
    wxString system(wxT("nix"));
#endif
    wxString tagSYS(wxT("@sys"));
    wxString tagAPImin(wxT("@api-min"));
    wxString tagAPImax(wxT("@api-max"));
    wxString tagCategory(wxT("@category"));
    wxString tagName(wxT("@name"));
    wxString tagDescription(wxT("@description"));

    // tell me who you are processing
    std::cout << m_filename.GetFullPath().mb_str(wxConvLocal) << std::endl;


    while(!in.Eof() && !(foundCategory && foundName && foundAPImin && foundAPImax && foundSYS && foundDescription))
    {
        int pos;
        wxString line=text.ReadLine();
        //convert to lower case to make search for tag name case insensitive
        wxString lowerLine=line.Lower();
        pos=lowerLine.Find(tagSYS);
        if(pos!=wxNOT_FOUND)
        {
            foundSYS=true;
            pos=lowerLine.Find(system);
            if(pos==wxNOT_FOUND)
            {
                m_validAPI=false;
                std::cout << "   fails @sys" << std::endl;
            };
            continue;
        };
        pos=lowerLine.Find(tagAPImin);
        if(pos!=wxNOT_FOUND)
        {
            foundAPImin=true;
            wxString APImin = line.Mid(pos+1+tagAPImin.length()).Trim().Trim(false);
            if(compareVersion(wxT(HUGIN_API_VERSION),APImin))
            {
                m_validAPI=false;
                std::cout << "   fails @api-min" << std::endl;
            };
            continue;
        };
        pos=lowerLine.Find(tagAPImax);
        if(pos!=wxNOT_FOUND)
        {
            foundAPImax=true;
            wxString APImax = line.Mid(pos+1+tagAPImax.length()).Trim().Trim(false);
            if(compareVersion(APImax,wxT(HUGIN_API_VERSION)))
            {
                m_validAPI=false;
                std::cout << "   fails @api-max" << std::endl;
            };
            continue;
        };
        pos=lowerLine.Find(tagCategory);
        if(pos!=wxNOT_FOUND)
        {
            m_category = line.Mid(pos+1+tagCategory.length()).Trim().Trim(false);
            foundCategory=true;
            std::cout << "   CAT:" << m_category.mb_str(wxConvLocal) << std::endl;
            continue;
        };
        pos=lowerLine.Find(tagName);
        if(pos!=wxNOT_FOUND)
        {
            m_name = line.Mid(pos+1+tagName.length()).Trim().Trim(false);
            foundName=true;
            std::cout << "   NAM:" << m_name.mb_str(wxConvLocal) << std::endl;
            continue;
        };
        pos=lowerLine.Find(tagDescription);
        if(pos!=wxNOT_FOUND)
        {
            m_description = line.Mid(pos+1+tagDescription.length()).Trim().Trim(false);
            foundDescription=true;
            continue;
        };
    };
};

const bool PluginItem::IsAPIValid() const
{
    return m_validAPI;
};

const wxString PluginItem::GetCategory() const
{
    return m_category;
};

const wxFileName PluginItem::GetFilename() const
{
    return m_filename;
};

const wxString PluginItem::GetName() const
{
    return m_name;
};

const wxString PluginItem::GetDescription() const
{
    return m_description;
};
