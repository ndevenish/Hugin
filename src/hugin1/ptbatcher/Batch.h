// -*- c-basic-offset: 4 -*-

/** @file Batch.h
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: Batch.h 3322 2008-08-18 1:10:07Z mkuder $
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

#ifndef BATCH_H
#define BATCH_H

#include <string>
#include "ProjectArray.h"
#include <wx/dir.h>
#ifndef __WXMSW__
#include <sys/wait.h>
#endif
#include "RunStitchFrame.h"

#ifndef FRAMEARRAY
#define FRAMEARRAY
WX_DEFINE_ARRAY_PTR(RunStitchFrame*,FrameArray);
#endif

using namespace std;

struct FailedProject
{
    wxString project;
    wxString logfile;
};

class Batch : public wxFrame
{
public:
    // this needs to be syncronized with the ressource file
    enum EndTask
    {
        DO_NOTHING = 0,
        CLOSE_PTBATCHERGUI = 1,
        SHUTDOWN = 2,
        SUSPEND = 3,   // only implemented for Windows
        HIBERNATE = 4  // only implemented for Windows
    };
    bool parallel;
    bool deleteFiles;
    EndTask atEnd;
    bool overwrite;
    bool verbose;
    bool autostitch;
    bool autoremove;
    bool saveLog;

    /** Main constructor */
    Batch(wxFrame* parent);

    /** load settings, especially which programs should be used */
    void LoadSettings(wxConfigBase* config);
    /** Adds an application entry in the batch list */
    void  AddAppToBatch(wxString app);
    /** Adds a project entry in the batch list */
    void  AddProjectToBatch(wxString projectFile, wxString outputFile = _T(""), Project::Target target=Project::STITCHING);
    /** Returns true if there are no more projects pending execution */
    bool  AllDone();
    /** Appends projects from file to batch list */
    void  AppendBatchFile(wxString file);
    /** Stops batch run, failing projects in progress */
    void  CancelBatch();
    /** Cancels project at index in batch, failing it */
    void  CancelProject(int index);
    /** Changes output prefix for project at index */
    void  ChangePrefix(int index, wxString newPrefix);
    /** Clears batch list and returns 0 if succesful */
    int   ClearBatch();
    /** Compares two project at indexes in both lists and returns true if they have identical project ids */
    bool  CompareProjectsInLists(int stitchListIndex, int batchListIndex);
    /** Returns index of first waiting project in batch */
    int   GetFirstAvailable();
    /** Returns index of project with selected id */
    int   GetIndex(int id);
    /** Returns project at index */
    Project* GetProject(int index);
    /** Returns number of projects in batch list */
    int   GetProjectCount();
    /** Returns number of projects in batch list with the input file path */
    int   GetProjectCountByPath(wxString path);
    /** Returns number of projects currently in progress */
    int   GetRunningCount();
    /** Returns current status of project at index */
    Project::Status GetStatus(int index);
    /** return true, if batch is running */
    bool IsRunning();
    /** Returns true if batch execution is currently paused */
    bool  IsPaused();
    /** Returns last saved batch file */
    const wxString GetLastFile()
    {
        return m_lastFile;
    };
    wxDateTime GetLastFileDate()
    {
        return m_lastmod;
    };
    /** Used in console mode. Prints out all projects and their statuses to the console */
    void  ListBatch();
    /** Clears current batch list and loads projects from batch file */
    int  LoadBatchFile(wxString file);
    /** Loads temporary batch file */
    int   LoadTemp();
    /** Returns true if there are no failed projects in batch */
    bool  NoErrors();
    /** Called internally when all running processes have completed and need to be removed from running list */
    void  OnProcessTerminate(wxProcessEvent& event);
    /** Called to start stitch of project with input scriptFile */
    bool  OnStitch(wxString scriptFile, wxString outname, int id);
    /** called to start detecting */
    bool OnDetect(wxString scriptFile, int id);
    /** Pauses and continues batch execution */
    void  PauseBatch();
    /** Removes project with id from batch list */
    void  RemoveProject(int id);
    /** Removes project at index from batch list */
    void  RemoveProjectAtIndex(int selIndex);
    /** Starts batch execution */
    void  RunBatch();
    /** Starts execution of next waiting project in batch */
    void  RunNextInBatch();
    /** Saves batch list to file */
    void  SaveBatchFile(wxString file);
    /** Saves batch list to temporary file */
    void  SaveTemp();
    /** Used internally to set status of selected project */
    void  SetStatus(int index,Project::Status status);
    /** Swaps position in batch of project at index with project at index+1 */
    void  SwapProject(int index);
    /** Set visibility of all running projects
     * @param isVisible If true display the project output, otherwise hide it.
     */
    void ShowOutput(bool isVisible=true);
    /** returns number of failed projects */
    size_t GetFailedProjectsCount()
    {
        return m_failedProjects.size();
    };
    /** returns project file name of failed project with index i */
    wxString GetFailedProjectName(unsigned int i);
    /** returns log file name of failed project with index i */
    wxString GetFailedProjectLog(unsigned int i);

private:
    // environment config objects
    wxConfigBase* m_config;
    //internal list of projects in batch
    ProjectArray  m_projList;
    //list of projects in progress
    FrameArray    m_stitchFrames;
    //last saved ptbt file
    wxString m_lastFile;
    wxDateTime m_lastmod;

    //batch state flags
    bool m_cancelled;
    bool m_paused;
    bool m_running;
    bool m_clearedInProgress;

    //vector, which stores the failed projects and filename of saved logfile
    std::vector<FailedProject> m_failedProjects;

    //external program config
    PTPrograms progs;
    AssistantPrograms progsAss;

    DECLARE_EVENT_TABLE()
};

#if _WINDOWS && defined Hugin_shared
DECLARE_LOCAL_EVENT_TYPE(EVT_BATCH_FAILED,-1)
DECLARE_LOCAL_EVENT_TYPE(EVT_INFORMATION,-1)
DECLARE_LOCAL_EVENT_TYPE(EVT_UPDATE_PARENT, -1)
#else
DECLARE_EVENT_TYPE(EVT_BATCH_FAILED,-1)
DECLARE_EVENT_TYPE(EVT_INFORMATION,-1)
DECLARE_EVENT_TYPE(EVT_UPDATE_PARENT, -1)
#endif

#endif //BATCH_H
