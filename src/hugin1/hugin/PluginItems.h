// -*- c-basic-offset: 4 -*-
/** @file PluginItems.h
 *
 *  @brief Reading python plugins metadata
 *
 *  @author Y. Levy, T. Modes
 *
 */
 
/*
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PLUGINITEMS_H
#define PLUGINITEMS_H

#include <wx/string.h>
#include <wx/filename.h>
#include <list>

/** class for generating plugin menu items */
class PluginItem
{
public:
    /** constructor
      * @param filename filename to python script to be parsed for metadata 
      */
    explicit PluginItem(wxFileName filename);
    /** returns true, if plugin can run on given system and version */
    const bool IsAPIValid() const;
    /** return category name */
    const wxString GetCategory() const;
    /** returns filename */
    const wxFileName GetFilename() const;
    /** return name from metadata */
    const wxString GetName() const;
    /** return description */
    const wxString GetDescription() const;
private:
    void ParseMetadata();
    wxString m_category;
    wxString m_name;
    wxFileName m_filename;
    wxString m_description;
    bool m_validAPI;
};

typedef std::list<PluginItem> PluginItems;
/** compares 2 plugin with category and name */
bool comparePluginItem(PluginItem item1,PluginItem item2);

#endif // PLUGINITEMS_H
