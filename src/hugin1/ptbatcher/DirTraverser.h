// -*- c-basic-offset: 4 -*-

/** @file DirTraverser.h
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: DirTraverser.h 3322 2008-08-16 5:00:07Z mkuder $
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

#include <wx/dir.h>
#include <wx/filename.h>

class DirTraverser : public wxDirTraverser
{
public:
    DirTraverser():wxDirTraverser() { }

    //Called when directory traverser evaluates a file
    virtual wxDirTraverseResult OnFile(const wxString& file)
    {
        wxFileName fileName(file);
        wxString ext = fileName.GetExt();
        //we add all project files to array
        if (ext.CmpNoCase(wxT("pto")) == 0 || ext.CmpNoCase(wxT("ptp")) == 0||
                ext.CmpNoCase(wxT("pts")) == 0|| ext.CmpNoCase(wxT("oto")) == 0)
        {
            projectFiles.Add(file);
        }

        //TO-DO: include image file heuristics to detect potential projects
        return wxDIR_CONTINUE;
    }

    //Called when directory traverser evaluates a directory
    virtual wxDirTraverseResult OnDir(const wxString& WXUNUSED(dir))
    {
        return wxDIR_CONTINUE;
    }

    //Returns an array with all project files found
    wxArrayString GetProjectFiles()
    {
        return projectFiles;
    }
private:
    wxArrayString projectFiles;
};
