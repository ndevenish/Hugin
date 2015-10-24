// -*- c-basic-offset: 4 -*-

/** @file ProjectArray.h
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: ProjectArray.h 3322 2008-08-16 5:00:07Z mkuder $
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

#ifndef PROJECTARRAY_H
#define PROJECTARRAY_H

#include <wx/dynarray.h>
#include <wx/string.h>
#include "panodata/PanoramaOptions.h"
#include <wx/log.h>
#include "panodata/Panorama.h"
#include "base_wx/platform.h"

class Project;

WX_DECLARE_OBJARRAY(Project, ProjectArray);		//declare an array of projects - main data structure for the batch processor
WX_DEFINE_ARRAY_INT(int,IntArray);

class Project
{
public:
    enum Status
    {
        FINISHED=0,
        WAITING,
        RUNNING,
        FAILED,
        MISSING,
        PAUSED
    };
    enum Target
    {
        STITCHING=0,
        DETECTING
    };

    //generator for unique ids of projects
    static long idGenerator;
    //unique id of project
    long id;
    //project status
    Status status;
    //project target: stitching or detecting/assistant
    Target target;
    //project input path
    wxString path;
    //project output prefix path and filename
    wxString prefix;
    //last modification date and time of project
    wxDateTime modDate;
    //project options
    HuginBase::PanoramaOptions options;
    //true if project is missing or should be skipped for a different reason when executing batch
    bool skip;
    // true, if project is probably aligned
    bool isAligned;

    //Constructor for project files
    Project(wxString pth,wxString pfx,Project::Target newTarget=STITCHING);
    //Constructor for applications
    explicit Project(wxString command);
    //Returns status of project in string form
    wxString GetStatusText();
    //Reads and returns options from a project file
    HuginBase::PanoramaOptions ReadOptions(wxString projectFile);
    //Resets the project options of project
    void ResetOptions();
};

#endif //PROJECTARRAY_H
