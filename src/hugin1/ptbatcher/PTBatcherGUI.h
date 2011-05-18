// -*- c-basic-offset: 4 -*-

/** @file PTBatcherGUI.h
 *
 *  @brief Batch processor for Hugin with GUI
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: PTBatcherGUI.h 3448 2008-09-23 3:42:07 mkuder $
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

#include "RunStitchFrame.h"
#include "Batch.h"
#include "BatchFrame.h"
//#include "FilePoller.h"
//#include "ProjectArray.h"

#include <wx/dir.h>
#include <wx/wfstream.h>
#include <wx/filefn.h>
#include <wx/snglinst.h>
#include <wx/ipc.h>

#include <hugin_config.h>
#include <wx/cmdline.h>

#ifndef FRAMEARRAY
#define FRAMEARRAY
	WX_DEFINE_ARRAY_PTR(RunStitchFrame*,FrameArray);
#endif

#ifndef PTBATCHERGUI_H
#define PTBATCHERGUI_H
// **********************************************************************

/** class for communication between different PTBatcherGUI instances
 *
 * this class is used to transfer the commandline parameters of the second instance of PTBatcherGUI
 * to the first and only running instance of PTBatcherGUI
*/
class BatchIPCConnection : public wxConnection
{
public:
    /** request handler for transfer */
#if wxCHECK_VERSION(2,9,0)
    virtual const void *OnRequest(const wxString& topic, const wxString& item, size_t *size = NULL, wxIPCFormat format = wxIPC_TEXT);
#else
    virtual wxChar *OnRequest(const wxString& topic, const wxString& item, int *size = NULL, wxIPCFormat format = wxIPC_TEXT);
#endif
};

/** server which implements the communication between different PTBatcherGUI instances (see BatchIPCConnection) */
class BatchIPCServer : public wxServer
{
public:
	/**accept connection handler (establish the connection) */
	virtual wxConnectionBase *OnAcceptConnection (const wxString& topic);
};

/** topic name for BatchIPCConnection and BatchIPCServer */
const wxString IPC_START(wxT("BatchStart"));

/** The application class for hugin_stitch_project
 *
 *  it contains the main frame.
 */
class PTBatcherGUI : public wxApp
{
public:
    /** pseudo constructor. with the ability to fail gracefully.
     */
	virtual bool OnInit();
	virtual int OnExit();

	//Handles some input keys for the frame
	void OnItemActivated(wxListEvent &event);
	void OnKeyDown(wxKeyEvent &event);
	
	//Main batch list
	ProjectArray projList;
	//List of projects in progress (their RunStitchFrames)
	FrameArray stitchFrames;
	BatchFrame* GetFrame() {return m_frame;};

#ifdef __WXMAC__
/** the wx calls this method when the app gets "Open file" AppleEvent */
    void MacOpenFile(const wxString &fileName);
#endif

private:
	BatchFrame *m_frame;
    wxLocale m_locale;
	wxString m_xrcPrefix;
	wxSingleInstanceChecker *m_checker;
	BatchIPCServer *m_server;

#ifdef __WXMAC__
    wxString m_macFileNameToOpenOnStart;
#endif

	DECLARE_EVENT_TABLE()
};

DECLARE_APP(PTBatcherGUI)

#endif //PTBATCHERGUI_H
