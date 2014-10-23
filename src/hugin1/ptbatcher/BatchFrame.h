// -*- c-basic-offset: 4 -*-

/** @file BatchFrame.h
 *
 *  @brief Batch processor for Hugin with GUI
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: BatchFrame.h 3322 2008-08-16 5:00:07Z mkuder $
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

#ifndef BATCHFRAME_H
#define BATCHFRAME_H

#include "RunStitchFrame.h"
#include "Batch.h"
#include "ProjectListBox.h"
#include "DirTraverser.h"
#ifdef __WXMSW__
#include "wx/msw/helpchm.h"
#endif
#include "BatchTrayIcon.h"
//#include <wx/app.h>
#include "ProgressStatusBar.h"

/** Simple class that forward the drop to the mainframe */
class BatchDropTarget : public wxFileDropTarget
{
public:
    /** File/directory drag and drop handler method
     *
     * When a project file is droped, it is added with default prefix.
     * When a directory is dropped, the directory and all sub-directory are scanned and
     * all found project files are added to the queue.
     */
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
};

class BatchFrame : public wxFrame, public wxThreadHelper
{
public:
    //Main constructor
    BatchFrame(wxLocale* locale, wxString xrc);
    //Main thread for all file polling - checking for new projects and updating modified ones.
    void* Entry();
    // create statusbar with progress control
    wxStatusBar* OnCreateStatusBar(int number, long style, wxWindowID id, const wxString& name);

    void OnUserExit(wxCommandEvent& event);
    void OnButtonAddCommand(wxCommandEvent& event);
    void OnButtonAddDir(wxCommandEvent& event);
    void OnButtonSearchPano(wxCommandEvent& e);
    /** let the user select a project file which should be added to the stitching queue */
    void OnButtonAddToStitchingQueue(wxCommandEvent& event);
    /** let the user select a project file which should be added to the stitching queue */
    void OnButtonAddToAssistantQueue(wxCommandEvent& event);
    void OnButtonCancel(wxCommandEvent& event);
    void OnButtonChangePrefix(wxCommandEvent& event);
    void OnButtonClear(wxCommandEvent& event);
    void OnButtonHelp(wxCommandEvent& event);
    void OnButtonMoveDown(wxCommandEvent& event);
    void OnButtonMoveUp(wxCommandEvent& event);
    void OnButtonOpenBatch(wxCommandEvent& event);
    void OnButtonOpenWithHugin(wxCommandEvent& event);
    void OnButtonPause(wxCommandEvent& event);
    void OnButtonRemoveComplete(wxCommandEvent& event);
    void OnButtonRemoveFromList(wxCommandEvent& event);
    void OnButtonReset(wxCommandEvent& event);
    void OnButtonResetAll(wxCommandEvent& event);
    void OnButtonRunBatch(wxCommandEvent& event);
    void OnButtonSaveBatch(wxCommandEvent& event);
    void OnButtonSkip(wxCommandEvent& event);

    void OnCheckOverwrite(wxCommandEvent& event);
    void OnCheckParallel(wxCommandEvent& event);
    void OnChoiceEnd(wxCommandEvent& event);
    void OnCheckVerbose(wxCommandEvent& event);
    /** event handler called when auto remove checkbox was changed */
    void OnCheckAutoRemove(wxCommandEvent& event);
    /** event handler called when auto stitch checkbox was changed */
    void OnCheckAutoStitch(wxCommandEvent& event);
    /** event handler called when always save log checkbox was changed */
    void OnCheckSaveLog(wxCommandEvent& event);
    /** event handler for update progress controls */
    void OnProgress(wxCommandEvent& event);

    //Called on window close to take care of the child thread
    void OnClose(wxCloseEvent& event);
    /** handle when minimize or restore image */
    void OnMinimize(wxIconizeEvent& e);
    //Resets all checkboxes based on m_batch object properties
    void PropagateDefaults();
    //Sets all checkboxes corresponding the setting in config
    void SetCheckboxes();
    //Starts batch execution
    void RunBatch();
    //Sets locale and XRC prefix pointers from main app
    void SetLocaleAndXRC(wxLocale* locale, wxString xrc);
    //Swaps the project entry at index in the list with the next (at index+1).
    void SwapProject(int index);
    //PanoramaOptions readOptions(wxString projectFile);
    /** return if parallel checkbox is checked */
    bool GetCheckParallel();
    /** return which task should be executed at end*/
    Batch::EndTask GetEndTask();
    /** return if overwrite checkbox is checked */
    bool GetCheckOverwrite();
    /** return if verbose checkbox is checked */
    bool GetCheckVerbose();
    /** return if auto remove checkbox is checked */
    bool GetCheckAutoRemove();
    /** return if auto stitch checkbox is checked */
    bool GetCheckAutoStitch();
    /** return if always save log is checked */
    bool GetCheckSaveLog();
    void RestoreSize();
    void AddToList(wxString aFile, Project::Target target=Project::STITCHING);
    void AddDirToList(wxString aDir);
    void ChangePrefix(int index,wxString newPrefix);
    /** returns true, if batch is running */
    bool IsRunning();
    /** returns true, if batch is paused */
    bool IsPaused();
    /** sets status message, also updates tooltip of taskbar icon
     *  @param status text for status bar
     *  @param showBalloon true if the status text should also shown in balloon */
    void SetStatusInformation(wxString status,bool showBalloon);
    /** update visibility of verbose output window depending on status of verbose checkbox */
    void UpdateBatchVerboseStatus();
    /** returns true, if last session was finished minimized */
    bool IsStartedMinimized()
    {
        return m_startedMinimized;
    };
    /** sets the current verbose status, does not update the checkbox */
    void SetInternalVerbose(bool newVerbose);
    /** update the progress bar in the task bar */
    void UpdateTaskBarProgressBar();

#ifdef __WXMSW__
    /** return help controller for open help */
    wxCHMHelpController& GetHelpController()
    {
        return m_msHtmlHelp;
    }
#endif

    //wxMutex* projListMutex;
    ProjectListBox* projListBox;

private:
    wxLocale* m_locale;
    wxString m_xrcPrefix;
    Batch* m_batch;
    bool m_cancelled;
    bool m_paused;
    bool m_closeThread; //included to signal the thread to finish execution
    wxChoice* m_endChoice;
    //TO-DO: include a batch or project progress gauge? Test initialization commented out in constructor
    //wxGauge* m_gauge;
#ifdef __WXMSW__
    wxCHMHelpController m_msHtmlHelp;
#else
    wxHtmlHelpController* m_help;
#endif
    BatchTaskBarIcon* m_tray;
    ProgressStatusBar* m_progStatusBar;
    bool m_startedMinimized;

    void OnProcessTerminate(wxProcessEvent& event);
    /** called by thread when queue was changed outside of PTBatcherGUI
    */
    void OnReloadBatch(wxCommandEvent& event);
    /** called by thread to update listbox */
    void OnUpdateListBox(wxCommandEvent& event);
    /** called when batch was finished and there are failed projects */
    void OnBatchFailed(wxCommandEvent& event);
    /** called when batch wants to show some progress message */
    void OnBatchInformation(wxCommandEvent& e);
    /** called if the project box needs to be updated, because projects were added or deleted */
    void OnRefillListBox(wxCommandEvent& e);

    DECLARE_EVENT_TABLE()
};

#endif //BATCHFRAME_H
