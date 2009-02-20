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

#include "../PTBatcher/RunStitchFrame.h"
#include "../PTBatcher/Batch.h"
#include "ProjectListBox.h"
#include "DirTraverser.h"
//#include <wx/app.h>

class BatchFrame : public wxFrame, wxThreadHelper
{
public:
	//Main constructor
	BatchFrame(const wxString& title, wxLocale* locale, wxString xrc);
	//Main thread for all file polling - checking for new projects and updating modified ones.
	void *Entry();

	void OnButtonAddCommand(wxCommandEvent &event);
	void OnButtonAddDir(wxCommandEvent &event);
	void OnButtonAddToList(wxCommandEvent &event);
	void OnButtonCancel(wxCommandEvent &event);
	void OnButtonChangePrefix(wxCommandEvent &event);
	void OnButtonClear(wxCommandEvent &event);
	void OnButtonHelp(wxCommandEvent &event);
	void OnButtonMoveDown(wxCommandEvent &event);
	void OnButtonMoveUp(wxCommandEvent &event);
	void OnButtonOpenBatch(wxCommandEvent &event);
	void OnButtonOpenWithHugin(wxCommandEvent &event);
	void OnButtonPause(wxCommandEvent &event);
	void OnButtonRemoveComplete(wxCommandEvent &event);
	void OnButtonRemoveFromList(wxCommandEvent &event);
	void OnButtonReset(wxCommandEvent &event);
	void OnButtonResetAll(wxCommandEvent &event);
	void OnButtonRunBatch(wxCommandEvent &event);
	void OnButtonSaveBatch(wxCommandEvent &event);
	void OnButtonSkip(wxCommandEvent &event);
	
	void OnCheckDelete(wxCommandEvent &event);
	void OnCheckOverwrite(wxCommandEvent &event);
	void OnCheckParallel(wxCommandEvent &event);
	void OnCheckShutdown(wxCommandEvent &event);
	void OnCheckVerbose(wxCommandEvent &event);
	
	//Called on window close to take care of the child thread
	void OnClose(wxCloseEvent &event);
	//Resets all checkboxes based on m_batch object properties
	void PropagateDefaults();
	//Starts batch execution
	void RunBatch();
	//Sets locale and XRC prefix pointers from main app
	void SetLocaleAndXRC(wxLocale* locale, wxString xrc);
	//Swaps the project entry at index in the list with the next (at index+1).
	void SwapProject(int index);
	//PanoramaOptions readOptions(wxString projectFile);

	//wxMutex* projListMutex;
	ProjectListBox *projListBox;

private:
	wxLocale* m_locale;
	wxString m_xrcPrefix;
	Batch* m_batch;
	bool m_cancelled;
	bool m_paused;
	bool m_closeThread; //included to signal the thread to finish execution
	//TO-DO: include a batch or project progress gauge? Test initialization commented out in constructor
	//wxGauge* m_gauge;
	wxHtmlHelpController * m_help;

	void OnProcessTerminate(wxProcessEvent & event);
	void OnSizeChange(wxSizeEvent &event);
	
	DECLARE_EVENT_TABLE()
	//PTPrograms progs;
};

//DECLARE_APP(PTBatcherGUI)

//component IDs
const int BUTTONADD = 1;
const int BUTTONREMOVE = 2;
const int BUTTONRUN = 3;
const int PROJLISTBOX = 4;
const int MENUADD = 5;
const int STATUSBAR = 6;
const int CHECKPARALLEL = 7;
const int CHECKDELETE = 8;
const int BUTTONHUGIN = 9;
const int BUTTONUP = 10;
const int BUTTONDOWN = 11;
const int BUTTONCOMMAND = 12;
const int BUTTONPREFIX = 13;
const int BUTTONCOMPLETE = 14;
const int TOOLRUN = 15;
const int TOOLPAUSE = 16;
const int TOOLSKIP = 17;
const int TOOLCANCEL = 18;
const int TOOLBAR = 19;
const int TOOLADD = 20;
const int TOOLREMOVE = 21;
const int BUTTONRESET = 22;
const int BUTTONRESETALL = 23;
const int BUTTONCLEAR = 24;
const int CHECKSHUTDOWN = 25;
const int CHECKOVERWRITE = 26;
const int TOOLOPEN = 27;
const int TOOLSAVE = 28;
const int TOOLCLEAR = 29;
const int TOOLADDDIR = 30;
const int MENUHELP = 31;
const int CHECKVERBOSE = 32;

#endif //BATCHFRAME_H
