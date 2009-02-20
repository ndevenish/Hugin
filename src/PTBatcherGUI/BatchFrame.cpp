// -*- c-basic-offset: 4 -*-

/** @file BatchFrame.cpp
 *
 *  @brief Batch processor for Hugin with GUI
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: BatchFrame.cpp 3322 2008-08-18 1:10:07Z mkuder $
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

#include "BatchFrame.h"
#include <wx/stdpaths.h>
//#include "PTBatcherGUI.h"

BEGIN_EVENT_TABLE(BatchFrame, wxFrame)
 EVT_TOOL(TOOLRUN,BatchFrame::OnButtonRunBatch)
 EVT_TOOL(TOOLPAUSE,BatchFrame::OnButtonPause)
 EVT_TOOL(TOOLSKIP,BatchFrame::OnButtonSkip)
 EVT_TOOL(TOOLCANCEL,BatchFrame::OnButtonCancel)
 EVT_TOOL(TOOLADD,BatchFrame::OnButtonAddToList)
 EVT_TOOL(TOOLREMOVE,BatchFrame::OnButtonRemoveFromList)
 EVT_TOOL(TOOLOPEN,BatchFrame::OnButtonOpenBatch)
 EVT_TOOL(TOOLSAVE,BatchFrame::OnButtonSaveBatch)
 EVT_TOOL(TOOLCLEAR,BatchFrame::OnButtonClear)
 EVT_TOOL(TOOLADDDIR,BatchFrame::OnButtonAddDir)
 EVT_BUTTON(BUTTONADD, BatchFrame::OnButtonAddToList)
 EVT_BUTTON(BUTTONCOMMAND, BatchFrame::OnButtonAddCommand)
 EVT_BUTTON(BUTTONREMOVE, BatchFrame::OnButtonRemoveFromList)
 EVT_BUTTON(BUTTONCOMPLETE, BatchFrame::OnButtonRemoveComplete)
 EVT_BUTTON(BUTTONCLEAR, BatchFrame::OnButtonClear)
 EVT_BUTTON(BUTTONPREFIX, BatchFrame::OnButtonChangePrefix)
 EVT_BUTTON(BUTTONRUN, BatchFrame::OnButtonRunBatch)
 EVT_BUTTON(BUTTONRESET, BatchFrame::OnButtonReset)
 EVT_BUTTON(BUTTONRESETALL, BatchFrame::OnButtonResetAll)
 EVT_BUTTON(BUTTONHUGIN, BatchFrame::OnButtonOpenWithHugin)
 EVT_BUTTON(BUTTONUP, BatchFrame::OnButtonMoveUp)
 EVT_BUTTON(BUTTONDOWN, BatchFrame::OnButtonMoveDown)
 EVT_MENU(MENUADD, BatchFrame::OnButtonAddToList)
 EVT_MENU(MENUHELP, BatchFrame::OnButtonHelp)
 EVT_CHECKBOX(CHECKPARALLEL, BatchFrame::OnCheckParallel)
 EVT_CHECKBOX(CHECKDELETE, BatchFrame::OnCheckDelete)
 EVT_CHECKBOX(CHECKSHUTDOWN, BatchFrame::OnCheckShutdown)
 EVT_CHECKBOX(CHECKOVERWRITE, BatchFrame::OnCheckOverwrite)
 EVT_CHECKBOX(CHECKVERBOSE, BatchFrame::OnCheckVerbose)
 EVT_SIZE(BatchFrame::OnSizeChange)
 EVT_END_PROCESS(-1, BatchFrame::OnProcessTerminate)
 EVT_CLOSE(BatchFrame::OnClose)
END_EVENT_TABLE()

BatchFrame::BatchFrame(const wxString &title, wxLocale* locale, wxString xrc) : wxFrame(NULL, wxID_ANY, title)
{	
	this->SetLocaleAndXRC(locale,xrc);
	m_cancelled = false;
	m_closeThread = false;
	//m_paused = false;
	m_batch = new Batch(this,wxTheApp->argv[0],true);
	m_batch->gui = true;
	m_batch->LoadTemp();
	m_help=0;

	//get saved size
	int width = wxConfigBase::Get()->Read(wxT("/BatchFrame/Width"), -1);
	int height = wxConfigBase::Get()->Read(wxT("/BatchFrame/Height"), -1);
	int max = wxConfigBase::Get()->Read(wxT("/BatchFrame/Max"), -1);;
	if((width != -1) && (height != -1))
		this->SetSize(width,height);
	else
		this->SetSize(600,400);

	if(max)
		this->Maximize();
	//this->GetSizer()->RecalcSizes();
//		this->GetSizer()->SetItemMinSize(this,500,200);

	wxMenu *fileMenu = new wxMenu();
	fileMenu->Append(MENUADD,_("Add project..."),
			_("Adds a project to the batch list."));
	wxMenu *fileMenu1 = new wxMenu();
	fileMenu1->Append(MENUHELP,_("Batch Stitcher help"),
			_("Open Batch Stitcher help."));
	wxMenuBar *menuBar = new wxMenuBar();
	menuBar->Append(fileMenu,_("File"));
	menuBar->Append(fileMenu1,_("Help"));
	SetMenuBar(menuBar);

	CreateStatusBar(1,0,STATUSBAR);
	SetStatusText(_("Not doing much..."));
	/*wxImage image(_T("filenew.bmp"),wxBITMAP_TYPE_ANY);
	wxBitmap icon(image);
	wxToolBar *toolbar = new wxToolBar(this, -1);
	toolbar->AddTool(wxID_ANY, _T("some label"), icon, icon);*/

	//create resizable control window
	wxFlexGridSizer* topSizer = new wxFlexGridSizer(2,1,0,0);
	wxToolBar *toolbar = new wxToolBar(this, TOOLBAR,wxDefaultPosition,wxDefaultSize,wxBORDER_RAISED|wxTB_HORIZONTAL);
	toolbar->SetToolBitmapSize(wxSize(32,32));

	//wxImage::InitStandardHandlers();
	//wxImage image(_T("filenew.bmp"),wxBITMAP_TYPE_ANY);
	//InitAllImageHandlers();
	wxImage::AddHandler(new wxPNGHandler());
	//we get the directory of the program, so we can access icon files by absolute path 
	//wxFileName dir(wxPathOnly(wxString::Format(_T("%s"),wxTheApp->argv[0])));	
	//wxMessageBox(dir.GetPath());
	wxBitmap icon1(this->m_xrcPrefix+_T("data/media-playback-start.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon2(this->m_xrcPrefix+_T("data/media-playback-pause.png"),wxBITMAP_TYPE_PNG);
	//wxBitmap icon3(_T("../share/hugin/xrc/data/media-playback-stop.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon3(this->m_xrcPrefix+_T("data/media-skip-forward.png"),wxBITMAP_TYPE_PNG);
	//wxBitmap icon8(_T("E:/svn/huginbase/hugin-build/INSTALL/FILES/bin/emblem-symbolic-link.png"),wxBITMAP_TYPE_PNG);
	//wxBitmap icon9(_T("E:/svn/huginbase/hugin-build/INSTALL/FILES/bin/emblem-unreadable.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon7(this->m_xrcPrefix+_T("data/process-stop22.png"),wxBITMAP_TYPE_PNG);
	//wxBitmap icon4(_T("E:/svn/huginbase/hugin-build/INSTALL/FILES/bin/image-missing.png"),wxBITMAP_TYPE_PNG);
	//wxBitmap icon5(_T("E:/svn/huginbase/hugin-build/INSTALL/FILES/bin/go-jump.png"),wxBITMAP_TYPE_PNG);
	//wxBitmap icon6(_T("E:/svn/huginbase/hugin-build/INSTALL/FILES/bin/process-stop.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon10(this->m_xrcPrefix+_T("data/list-add.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon11(this->m_xrcPrefix+_T("data/list-remove.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon12(this->m_xrcPrefix+_T("data/document-new.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon13(this->m_xrcPrefix+_T("data/document-open.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon14(this->m_xrcPrefix+_T("data/media-floppy.png"),wxBITMAP_TYPE_PNG);
	wxBitmap icon15(this->m_xrcPrefix+_T("data/folder-saved-search.png"),wxBITMAP_TYPE_PNG);
	toolbar->AddTool(TOOLCLEAR, _("clear"), icon12, wxNullBitmap, wxITEM_NORMAL, _("Clear batch"), _("Clears the batch list"));
	toolbar->AddTool(TOOLOPEN, _("open"), icon13, wxNullBitmap, wxITEM_NORMAL, _("Open batch"), _("Opens a batch file with a list of projects"));
	toolbar->AddTool(TOOLSAVE, _("save"), icon14, wxNullBitmap, wxITEM_NORMAL, _("Save batch"), _("Saves a batch file with a list of projects"));
	toolbar->AddSeparator();
	toolbar->AddSeparator();
	toolbar->AddTool(TOOLRUN, _("start"), icon1, wxNullBitmap, wxITEM_NORMAL, _("Start batch"), _("Starts batch execution"));
	toolbar->AddTool(TOOLSKIP, _("skip"), icon3, wxNullBitmap, wxITEM_NORMAL, _("Skip project(s)"), _("Skips execution of currently running project(s)"));
	toolbar->AddTool(TOOLPAUSE, _("pause"), icon2, wxNullBitmap, wxITEM_CHECK, _("Pause batch"), _("Pauses batch execution"));
	toolbar->AddTool(TOOLCANCEL, _("cancel"), icon7, wxNullBitmap, wxITEM_NORMAL, _("Cancel batch"), _("Cancels batch execution"));
	toolbar->AddSeparator();
	toolbar->AddSeparator();
	toolbar->AddTool(TOOLADD, _("add"), icon10, wxNullBitmap, wxITEM_NORMAL, _("Add project(s)"), _("Appends project file(s) to the batch list"));
	//toolbar->AddSeparator();
	toolbar->AddTool(TOOLREMOVE, _("remove"), icon11, wxNullBitmap, wxITEM_NORMAL, _("Remove project"), _("Removes selected project from the batch list"));
	toolbar->AddTool(TOOLADDDIR, _("adddir"), icon15, wxNullBitmap, wxITEM_NORMAL, _("Search directory"), _("Appends all project files from a directory and subdirectories"));
	
	
	/*toolbar->AddTool(wxID_ANY, _T("cancel"), icon4, wxNullBitmap, wxITEM_NORMAL, _T("Cancel batch"), _T("Cancels batch execution"));
	toolbar->AddTool(wxID_ANY, _T("cancel"), icon5, wxNullBitmap, wxITEM_NORMAL, _T("Cancel batch"), _T("Cancels batch execution"));
	toolbar->AddTool(wxID_ANY, _T("cancel"), icon6, wxNullBitmap, wxITEM_NORMAL, _T("Cancel batch"), _T("Cancels batch execution"));*/
	toolbar->Realize();
	//topSizer->AddGrowableRow(0,0);

	wxFlexGridSizer* firstSizer = new wxFlexGridSizer(1,2,0,0);
	firstSizer->AddGrowableRow(0,1);
	firstSizer->AddGrowableCol(0,1);

	topSizer->Add(toolbar,0,wxEXPAND);
	topSizer->Add(firstSizer,1,wxEXPAND);
	topSizer->AddGrowableRow(1,1);
	topSizer->AddGrowableCol(0,1);
	//wxBoxSizer *secondSizer = new wxBoxSizer(wxVERTICAL);
	
	projListBox = new ProjectListBox(this,PROJLISTBOX,wxPoint(0,0),wxSize(100,100),wxLC_REPORT | wxLC_SINGLE_SEL);
	
	firstSizer->Add(projListBox,1,wxEXPAND);
	wxPanel* panel = new wxPanel(this,wxID_ANY,wxPoint(width-140,0),wxSize(140,height));
	firstSizer->Add(panel,0,wxEXPAND);//,wxEXPAND | wxALL);
	//panel->SetMaxSize(wxSize(120,100));

	//we put all controls on the panel

	wxFlexGridSizer* panelSizer = new wxFlexGridSizer(13,1,0,0);
	/*for(int i=0; i<13; i++)
		panelSizer->AddGrowableRow(i,0);*/
	panelSizer->AddGrowableCol(0,0);

	/*removed because we have an icon
	wxButton* button = new wxButton(panel,BUTTONADD,_T("Add project(s)"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);*/
	wxButton* button = new wxButton(panel,BUTTONCOMMAND,_("Add application"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);
	/*removed because we have an icon
	button = new wxButton(panel,BUTTONREMOVE,_T("Remove project"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);*/
	button = new wxButton(panel,BUTTONCOMPLETE,_("Remove complete"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);
	/*removed because we have an icon
	button = new wxButton(panel,BUTTONCLEAR,_T("Clear Batch"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);*/
	button = new wxButton(panel,BUTTONPREFIX,_("Change prefix"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);
	/*button = new wxButton(panel,BUTTONRUN,_T("Run batch"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);*/
	button = new wxButton(panel,BUTTONRESET,_("Reset project"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);
	button = new wxButton(panel,BUTTONRESETALL,_("Reset all"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);
	button = new wxButton(panel,BUTTONHUGIN,_("Edit with Hugin"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);
	button = new wxButton(panel,BUTTONUP,_("Move project up"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);
	button = new wxButton(panel,BUTTONDOWN,_("Move project down"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(button,0,wxEXPAND);
	panelSizer->AddSpacer(10);
	wxCheckBox* check = new wxCheckBox(panel,CHECKPARALLEL,_("Parallel execution"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(check,0,wxEXPAND);
	check = new wxCheckBox(panel,CHECKDELETE,_("Delete *.pto files"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(check,0,wxEXPAND);
	check = new wxCheckBox(panel,CHECKOVERWRITE,_("Overwrite always"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(check,0,wxEXPAND);
	check = new wxCheckBox(panel,CHECKSHUTDOWN,_("Shutdown when done"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(check,0,wxEXPAND);
	check = new wxCheckBox(panel,CHECKVERBOSE,_("Verbose output"),wxDefaultPosition,wxDefaultSize);
	panelSizer->Add(check,0,wxEXPAND);
	
	panel->SetSizer(panelSizer);

	/*wxPanel* panel2 = new wxPanel(this,wxID_ANY,wxDefaultPosition,wxSize(120,100));
	button = new wxButton(panel2,wxID_ANY,_T("Run batch"),wxPoint(0,60),wxSize(120,20));
	secondSizer->Add(panel,0,wxEXPAND | wxALL);
	secondSizer->Add(panel2,1,wxEXPAND);
	firstSizer->Add(secondSizer,1);*/
	SetSizer(topSizer);
	//firstSizer->SetSizeHints( this );
	//firstSizer->Fit(this);
	Layout();

	//projListMutex = new wxMutex();
	this->wxThreadHelper::Create();
	//wxMessageBox( _T("B"),_T("B"),wxOK | wxICON_INFORMATION );
	this->GetThread()->Run();
	//TO-DO: include a batch or project progress gauge?
	/*m_gauge = new wxGauge(this,wxID_ANY,100,wxPoint(100,100),wxSize(100,10));
	m_gauge->SetValue(50);
	m_gauge->Hide();*/
	projListBox->Fill(m_batch);
}

void *BatchFrame::Entry()
{
	
	//we define the working dir to search in and the file name syntax of the spool files
	//wxMessageBox( _T("new file received1"),_T("new file received1"),wxOK | wxICON_INFORMATION );
	wxDir* workingDir = new wxDir(wxStandardPaths::Get().GetUserConfigDir());
	wxString fileSent = _T(".ptbs*");
	wxString pending;
	/*wxString fileTemp = _T(".ptbt*");
	wxString temp = _T("");
	//we check for existing temporary files
	if(workingDir->GetFirst(&temp,fileTemp,wxDIR_FILES))
	{
		//we find the last existing tempfile (there should be at most two, but we check for multiple just in case)
		while(workingDir->GetNext(&pending))
		{
			wxFileName tempFile(temp);
			wxFileName pendingFile(pending);
			wxDateTime* create1 = new wxDateTime();
			wxDateTime* create2 = new wxDateTime();
			if(tempFile.FileExists() && pendingFile.FileExists())
			{
				tempFile.GetTimes(NULL,NULL,create1);
				pendingFile.GetTimes(NULL,NULL,create2);
				if(create2->IsLaterThan(*create1))
				{
					wxRemoveFile(temp);
					temp=wxString(pending);
				}
			}
			else
			{
				wxMessageBox( _T("Error reading temporary file"),_T("Error!"),wxOK | wxICON_INFORMATION );
			}
		}
	}

	//we load the data from the temp file
	wxGetApp().AppendBatchFile(workingDir->GetName()+wxFileName::GetPathSeparator()+temp);*/

	bool change = false;
	int projectCount = m_batch->GetProjectCount();
	//we constantly poll the working dir for new files and wait a bit on each loop
	while(!m_closeThread)
	{
		//wxMessageBox( _T("test"),_T("Error!"),wxOK | wxICON_INFORMATION );
		pending = workingDir->FindFirst(workingDir->GetName(),fileSent,wxDIR_FILES);
		//wxMessageBox( _T("test1"),_T("Error!"),wxOK | wxICON_INFORMATION );
		if(!pending.IsEmpty())
		{
			wxString projectPending = _T("");
			//wxMessageBox( _T("new file received"),spoolFile,wxOK | wxICON_INFORMATION );
			wxFileInputStream* spoolFile = new wxFileInputStream(pending);
			wxChar input = spoolFile->GetC();
			while(spoolFile->LastRead()!=0)
			{
				projectPending = projectPending + input;
				input = spoolFile->GetC();
			}
			m_batch->AddProjectToBatch(projectPending);
			projListBox->AppendProject(m_batch->GetProject(m_batch->GetProjectCount()-1));
			change = true;
			spoolFile->~wxFileInputStream();
			if(!wxRemoveFile(pending))
				wxMessageBox( _("Error: Could not remove temporary file"),_("Error!"),wxOK | wxICON_INFORMATION );

		}
		//wxMessageBox( _T("test2"),_T("Error!"),wxOK | wxICON_INFORMATION );
		wxFileName* tempFile;
		//wxMessageBox( _T("test3"),_T("Error!"),wxOK | wxICON_INFORMATION );
		//check all projects in list for changes
		//wxString message = wxString();
		//message = message << wxGetApp().m_projList.GetCount();
		//wxMessageBox( message,_T("Error!"),wxOK | wxICON_INFORMATION );
		for(int i = 0; i< m_batch->GetProjectCount(); i++)
		{
			if(m_batch->GetProject(i)->id >= 0)
			{
				tempFile = new wxFileName(m_batch->GetProject(i)->path);
				if(tempFile->FileExists())
				{
					//wxMessageBox(tempFile->GetFullPath(),_T("Error!"),wxOK | wxICON_INFORMATION );
					//wxDateTime* access = new wxDateTime();
					wxDateTime* modify = new wxDateTime();
					//wxDateTime* create = new wxDateTime();
					tempFile->GetTimes(NULL,modify,NULL);
					if(m_batch->GetProject(i)->skip)
					{
						change = true;
						m_batch->GetProject(i)->skip = false;
						m_batch->SetStatus(i,Project::WAITING);
						projListBox->ReloadProject(projListBox->GetIndex(m_batch->GetProject(i)->id),m_batch->GetProject(i));
					}
					else if(!modify->IsEqualTo(m_batch->GetProject(i)->modDate))
					{
						change = true;
						m_batch->GetProject(i)->modDate = *modify;
						m_batch->GetProject(i)->ResetOptions();
						m_batch->SetStatus(i,Project::WAITING);
						projListBox->ReloadProject(projListBox->GetIndex(m_batch->GetProject(i)->id),m_batch->GetProject(i));
					}
				}
				else
				{
					if(m_batch->GetStatus(i) != Project::MISSING)
					{
						change = true;
						m_batch->GetProject(i)->skip = true;
						m_batch->SetStatus(i,Project::MISSING);
						projListBox->SetMissing(projListBox->GetIndex(m_batch->GetProject(i)->id));
					}
				}
			}
			if(projListBox->UpdateStatus(i,m_batch->GetProject(i)))
				change = true;
		}
		if(m_batch->GetProjectCount()!=projectCount)
		{
			projectCount = m_batch->GetProjectCount();
			change = true;
		}
		//if(tempFile!=NULL)
		//	free(tempFile);
		if(change)
		{
			change = false;
			m_batch->SaveTemp();
		}
		
		GetThread()->Sleep(1000);
		//wxFile file;
		//file.Create(workingDir->GetName()+wxFileName::GetPathSeparator()+_T("krneki.txt"));
		//file.Close();		
	}
	//wxMessageBox(_T("Ending thread..."));
	return 0;
}

void BatchFrame::OnButtonAddCommand(wxCommandEvent &event)
{
	wxTextEntryDialog dlg(this,_("Please enter the command-line application to execute:"),_("Enter application"));
	wxTheApp->SetEvtHandlerEnabled(false);
	if(dlg.ShowModal() == wxID_OK)
	{
		wxString line = dlg.GetValue();
		m_batch->AddAppToBatch(line);
		//SetStatusText(_T("Added application"));
		projListBox->AppendProject(m_batch->GetProject(m_batch->GetProjectCount()-1));
	}
	wxTheApp->SetEvtHandlerEnabled(true);
	/*Batch* batch = new Batch(this,wxTheApp->argv[0]);
	batch->gui = true;
	batch->LoadTemp();
	batch->RunBatch();*/
}

void BatchFrame::OnButtonAddDir(wxCommandEvent &event)
{
	wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/actualPath"),wxT(""));
	wxDirDialog dlg(this,
                     _("Specify a directory to search for projects in"),
                     defaultdir, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	//dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/BatchFrame/actualPath"),wxT("")));
	if (dlg.ShowModal() == wxID_OK) 
	{
		wxConfig::Get()->Write(wxT("/BatchFrame/actualPath"), dlg.GetPath());  // remember for later
		//we traverse all subdirectories of chosen path
		DirTraverser traverser;
		wxDir dir(dlg.GetPath());
		dir.Traverse(traverser);
		wxArrayString projects = traverser.GetProjectFiles();
		for(unsigned int i=0; i<projects.GetCount(); i++)
			m_batch->AddProjectToBatch(projects.Item(i));
		SetStatusText(_("Added projects from dir ")+dlg.GetPath());
    }
	else { // bail
			//wxLogError( _("No project files specified"));
	} 
}
void BatchFrame::OnButtonAddToList(wxCommandEvent &event)
{
	wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/actualPath"),wxT(""));
	wxFileDialog dlg(0,
                     _("Specify project source file(s)"),
                     defaultdir, wxT(""),
                     _("Project files (*.pto,*.ptp,*.pts,*.oto)|*.pto;*.ptp;*.pts;*.oto;|All files (*)|*"),
                     wxFD_OPEN | wxFD_MULTIPLE, wxDefaultPosition);

	//dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/BatchFrame/actualPath"),wxT("")));

	if (dlg.ShowModal() == wxID_OK) 
	{
		wxConfig::Get()->Write(wxT("/BatchFrame/actualPath"), dlg.GetDirectory());  // remember for later
		wxArrayString paths;
		dlg.GetPaths(paths);
		for(unsigned int i=0; i<paths.GetCount(); i++)
		{
			wxFileName name(paths.Item(i));
			m_batch->AddProjectToBatch(paths.Item(i),name.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + name.GetName());
			SetStatusText(_("Added project ")+paths.Item(i));
			projListBox->AppendProject(m_batch->GetProject(m_batch->GetProjectCount()-1));
		}
    }
	else { // bail
			//wxLogError( _("No project files specified"));
	} 
}

void BatchFrame::OnButtonCancel(wxCommandEvent &event)
{
	((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
	m_cancelled = true;
	m_batch->CancelBatch();
}
void BatchFrame::OnButtonChangePrefix(wxCommandEvent &event)
{
	int selIndex = projListBox->GetSelectedIndex();
	if(selIndex != -1)
	{
		wxFileDialog dlg(0,_("Specify output prefix for project ")+projListBox->GetSelectedProject(),
                     projListBox->GetSelectedProject(),
                     wxT(""), wxT(""),
                     wxSAVE, wxDefaultPosition);
		if (dlg.ShowModal() == wxID_OK)
		{
			wxString outname(dlg.GetPath());
			m_batch->ChangePrefix(selIndex,outname);
			projListBox->ChangePrefix(selIndex,outname);
			//SetStatusText(_T("Changed prefix for "+projListBox->GetSelectedProject()));
			//m_batch->SaveTemp();
		}
	}
	else
	{
		SetStatusText(_("Please select a project"));
	}
}

void BatchFrame::OnButtonClear(wxCommandEvent &event)
{
	int returnCode = m_batch->ClearBatch();
	if(returnCode == 0)
		projListBox->DeleteAllItems();
	else if(returnCode == 2)
	{
		m_cancelled = true;
		projListBox->DeleteAllItems();
		if(((wxToolBar*)FindWindow(TOOLBAR))->GetToolState(TOOLPAUSE))
		{
			((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
		}
	}
}
void BatchFrame::OnButtonHelp(wxCommandEvent &event)
{
	DEBUG_TRACE("");
    if (m_help == 0)
    {
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
        // On Mac, xrc/data/help_LOCALE should be in the bundle as LOCALE.lproj/help
        // which we can rely on the operating sytem to pick the right locale's.
        wxString strFile = MacGetPathToBundledResourceFile(CFSTR("help"));
        if(strFile!=wxT(""))
        {
            strFile += wxT("/hugin.hhp");
        } else {
            wxLogError(wxString::Format(wxT("Could not find help directory in the bundle"), strFile.c_str()));
            return;
        }
#else
        // find base filename
		wxString helpFile = wxT("help_") + m_locale->GetCanonicalName() + wxT("/hugin.hhp");
        DEBUG_INFO("help file candidate: " << helpFile.mb_str(wxConvLocal));
        //if the language is not default, load custom About file (if exists)
		wxString strFile = m_xrcPrefix + wxT("data/") + helpFile;
        if(wxFile::Exists(strFile))
        {
            DEBUG_TRACE("Using About: " << strFile.mb_str(wxConvLocal));
        } else {
            // try default
            strFile = m_xrcPrefix + wxT("data/help_en_EN/hugin.hhp");
        }
#endif

        if(!wxFile::Exists(strFile))
        {
            wxLogError(wxString::Format(wxT("Could not open help file: %s"), strFile.c_str()));
            return;
        }
        DEBUG_INFO(_("help file: ") << strFile.mb_str(wxConvLocal));
        m_help = new wxHtmlHelpController();
        m_help->AddBook(strFile);
    }
    m_help->Display(wxT("Hugin_Batch_Processor.html"));
	//DisplayHelp(wxT("Hugin_Batch_Processor.html"));
}
void BatchFrame::OnButtonMoveDown(wxCommandEvent &event)
{
	SwapProject(projListBox->GetSelectedIndex());
	m_batch->SaveTemp();
}

void BatchFrame::OnButtonMoveUp(wxCommandEvent &event)
{
	SwapProject(projListBox->GetSelectedIndex()-1);
	m_batch->SaveTemp();
}
void BatchFrame::OnButtonOpenBatch(wxCommandEvent &event)
{	wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/batchPath"),wxT(""));
	wxFileDialog dlg(0,
                     _("Specify batch file to open"),
                     defaultdir, wxT(""),
                     _("Batch files (*.ptb)|*.ptb;|All files (*)|*"),
                     wxOPEN, wxDefaultPosition);
	if (dlg.ShowModal() == wxID_OK) 
	{
		wxConfig::Get()->Write(wxT("/BatchFrame/batchPath"), dlg.GetDirectory());  // remember for later
		int clearCode = m_batch->LoadBatchFile(dlg.GetPath());
		if(clearCode!=1)	//1 is error code for not clearing batch
		{
			/*if(clearCode==2) //we just cancelled the batch, so we need to try loading again
				m_batch->LoadBatchFile(dlg.GetPath());*/
			projListBox->DeleteAllItems();
			projListBox->Fill(m_batch);
		}
    }
}
void BatchFrame::OnButtonOpenWithHugin(wxCommandEvent &event)
{
#ifdef __WINDOWS__
	wxString huginPath = wxConfigBase::Get()->Read(wxT("/startDir"), wxGetCwd())+wxFileName::GetPathSeparator();
#else
	wxString huginPath = _T("");	//we call hugin directly without path on linux
#endif
	if(projListBox->GetSelectedIndex()!=-1)	
		if(projListBox->GetText(projListBox->GetSelectedIndex(),0).Cmp(_T(""))!=0)
			wxExecute(huginPath+_T("hugin \"" + projListBox->GetSelectedProject()+_T("\" -notips")));
		else
			SetStatusText(_("Cannot open app in Hugin."));
	else
	{
		//ask user if he/she wants to load an empty project
		wxMessageDialog message(this,_("No project selected. Open Hugin without project?"), _("Question"),
                  wxYES | wxNO | wxICON_INFORMATION );
		if(message.ShowModal() == wxID_YES)
			wxExecute(huginPath+_T("hugin"));
	}
}
void BatchFrame::OnButtonPause(wxCommandEvent &event)
{
	//((wxToolBar*)frame->FindWindow(TOOLBAR))->;
	if(m_batch->GetRunningCount()>0)
	{
		//wxMessageBox(_T("Pause"));
		//if(((wxToolBar*)FindWindow(TOOLBAR))->GetToolState(TOOLPAUSE))
		//{
			if(!m_batch->IsPaused())
			{
				m_batch->PauseBatch();
				((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,true);
				SetStatusText(_("Batch paused"));
			}
			//m_paused=true;
		//}
		//else
		//{
			else//if(m_batch->paused)
			{
				m_batch->PauseBatch();
				((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
				SetStatusText(_("Continuing batch..."));
			}
			//m_paused=false;
		//}
	}
	else //if no projects are running, we deactivate the button
	{
		((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
	}
}
void BatchFrame::OnButtonRemoveComplete(wxCommandEvent &event)
{
	bool removeErrors=false;
	if(!m_batch->NoErrors())
	{
		wxMessageDialog message(this,_("There are failed projects in the list.\nRemove them also?"), _("Question"),
										wxYES | wxNO | wxICON_INFORMATION );
		if(message.ShowModal()==wxID_YES)
			removeErrors=true;
	}
	for(int i=projListBox->GetItemCount()-1; i>=0; i--)
	{
		if(m_batch->GetStatus(i)==Project::FINISHED || 
		(removeErrors && m_batch->GetStatus(i)==Project::FAILED))
		{
			projListBox->DeleteItem(i);
			m_batch->RemoveProjectAtIndex(i);
		}


	}
	

	//m_batch->SaveTemp();
}
void BatchFrame::OnButtonRemoveFromList(wxCommandEvent &event)
{
	int selIndex = projListBox->GetSelectedIndex();
	if(selIndex != -1)
	{
		if(m_batch->GetStatus(selIndex)==Project::RUNNING || m_batch->GetStatus(selIndex)==Project::PAUSED)
		{
			wxMessageDialog message(this, _("Cannot remove project in progress.\nDo you want to cancel it?"), _("In progress"), wxYES | wxCANCEL | wxICON_INFORMATION);
			if(message.ShowModal()==wxID_YES)
			{
				OnButtonSkip(event);
			}
		}
		else
		{
			m_batch->RemoveProjectAtIndex(selIndex);
			SetStatusText(_("Removed project "+projListBox->GetSelectedProject()));
			projListBox->DeleteItem(selIndex);
			//m_batch->SaveTemp();
		}
	}
	else{
		SetStatusText(_("Please select a project to remove"));
	}
}


void BatchFrame::OnButtonReset(wxCommandEvent &event)
{
	int selIndex = projListBox->GetSelectedIndex();
	if(selIndex != -1)
	{
		if(m_batch->GetStatus(selIndex)==Project::RUNNING || m_batch->GetStatus(selIndex)==Project::PAUSED)
		{
			wxMessageDialog message(this, _("Cannot reset project in progress.\nDo you want to cancel it?"), _("In progress"), wxYES | wxCANCEL | wxICON_INFORMATION);
			if(message.ShowModal()==wxID_YES)
			{
				OnButtonSkip(event);
			}
		}
		else
		{
			m_batch->SetStatus(selIndex,Project::WAITING);
			SetStatusText(_("Reset project "+projListBox->GetSelectedProject()));
		}
	}
	else{
		SetStatusText(_("Please select a project to reset"));
	}
}
void BatchFrame::OnButtonResetAll(wxCommandEvent &event)
{
	if(m_batch->GetRunningCount()!=0)
	{
		wxMessageDialog message(this, _("Cannot reset projects in progress.\nDo you want to cancel the batch?"), _("In progress"), wxYES | wxCANCEL | wxICON_INFORMATION);
		if(message.ShowModal()==wxID_YES)
			OnButtonCancel(event);
	}
	else
	{
		for(int i=projListBox->GetItemCount()-1; i>=0; i--)
			m_batch->SetStatus(i,Project::WAITING);
	}
}
void BatchFrame::OnButtonRunBatch(wxCommandEvent &event)
{
	if(m_batch->IsPaused())
	{
		//m_batch->paused = false;
		OnButtonPause(event);
	}
	else
		RunBatch();
}

void BatchFrame::OnButtonSaveBatch(wxCommandEvent &event)
{
	wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/batchPath"),wxT(""));
	wxFileDialog dlg(0,
                     _("Specify batch file to save"),
                     defaultdir, wxT(""),
                     _("Batch file (*.ptb)|*.ptb;|All files (*)|*"),
                     wxSAVE, wxDefaultPosition);
	if (dlg.ShowModal() == wxID_OK) 
	{
		wxConfig::Get()->Write(wxT("/BatchFrame/batchPath"), dlg.GetDirectory());  // remember for later
		m_batch->SaveBatchFile(dlg.GetPath());
    }
}

void BatchFrame::OnButtonSkip(wxCommandEvent &event)
{
	int selIndex = projListBox->GetSelectedIndex();
	if(selIndex != -1)
	{
		if(m_batch->GetStatus(selIndex)==Project::RUNNING 
		||m_batch->GetStatus(selIndex)==Project::PAUSED)
		{
			if(m_batch->GetStatus(selIndex)==Project::PAUSED)
			{
				if(m_batch->GetRunningCount()==1)
					((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
				for(int i=0; i<m_batch->GetRunningCount(); i++)
				{
					if(m_batch->GetStatus(selIndex)==Project::PAUSED
					&& m_batch->CompareProjectsInLists(i,selIndex))
					{
						m_batch->CancelProject(i);
					}
				}
				//OnButtonCancel(event);
				//((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
				/*m_cancelled = true;
				//we make sure project is still running
				if(m_batch->GetStatus(selIndex)==Project::PAUSED 
				|| m_batch->GetStatus(selIndex)==Project::RUNNING)	
					m_batch->CancelProject(0);*/
			}
			else
			{
				//we go through all running projects to find the one with the same id as chosen one
				for(int i=0; i<m_batch->GetRunningCount(); i++)
				{
					if(m_batch->GetStatus(selIndex)==Project::RUNNING 
					&& m_batch->CompareProjectsInLists(i,selIndex))
						m_batch->CancelProject(i);
				}
			}
		}
		else
		{
			m_batch->SetStatus(selIndex,Project::FAILED);
		}
	}
/*			//we make sure project is still running
		if(wxGetApp().m_projList.Item(selIndex).status==Project::RUNNING)	
			wxGetApp().stitchFrames.Item(0)->OnQuit(event);
		}
		for(unsigned int i=0; i<wxGetApp().stitchFrames.GetCount(); i++)
			wxGetApp().stitchFrames.Item(i)->OnQuit(event);
		if(((wxToolBar*)FindWindow(TOOLBAR))->GetToolState(TOOLPAUSE))
		{
			((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
		}*/
}





void BatchFrame::OnCheckDelete(wxCommandEvent &event)
{
	if(((wxCheckBox*)FindWindow(CHECKDELETE))->GetValue())
	{
		m_batch->deleteFiles = true;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/DeleteCheck"), 1);
	}
	else
	{
		m_batch->deleteFiles = false;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/DeleteCheck"), 0);
	}
}


void BatchFrame::OnCheckOverwrite(wxCommandEvent &event)
{
	if(((wxCheckBox*)FindWindow(CHECKOVERWRITE))->GetValue())
	{
		m_batch->overwrite = true;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/OverwriteCheck"), 1);
	}
	else
	{
		m_batch->overwrite = false;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/OverwriteCheck"), 0);
	}
}
void BatchFrame::OnCheckParallel(wxCommandEvent &event)
{
	if(((wxCheckBox*)FindWindow(CHECKPARALLEL))->GetValue())
	{
		m_batch->parallel = true;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/ParallelCheck"), 1);
	}
	else
	{
		m_batch->parallel = false;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/ParallelCheck"), 0);
	}
}

void BatchFrame::OnCheckShutdown(wxCommandEvent &event)
{
	if(((wxCheckBox*)FindWindow(CHECKSHUTDOWN))->GetValue())
	{
		m_batch->shutdown = true;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/ShutdownCheck"), 1);
	}
	else
	{
		m_batch->shutdown = false;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/ShutdownCheck"), 0);
	}
}

void BatchFrame::OnCheckVerbose(wxCommandEvent &event)
{
	if(((wxCheckBox*)FindWindow(CHECKVERBOSE))->GetValue())
	{
		m_batch->verbose = true;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/VerboseCheck"), 1);
	}
	else
	{
		m_batch->verbose = false;
		wxConfigBase::Get()->Write(wxT("/BatchFrame/VerboseCheck"), 0);
	}
}







void BatchFrame::OnClose(wxCloseEvent &event)
{
	//wxMessageBox(_T("Closing..."));
	m_closeThread = true;
	this->GetThread()->Wait();
	//wxMessageBox(_T("Closing frame..."));
	event.Skip();
}
void BatchFrame::PropagateDefaults()
{
	if(((wxCheckBox*)FindWindow(CHECKPARALLEL))->GetValue())
		m_batch->parallel = true;
	else
		m_batch->parallel = false;
	if(((wxCheckBox*)FindWindow(CHECKDELETE))->GetValue())
		m_batch->deleteFiles = true;
	else
		m_batch->deleteFiles = false;
	if(((wxCheckBox*)FindWindow(CHECKSHUTDOWN))->GetValue())
		m_batch->shutdown = true;
	else
		m_batch->shutdown = false;
	if(((wxCheckBox*)FindWindow(CHECKOVERWRITE))->GetValue())
		m_batch->overwrite = true;
	else
		m_batch->overwrite = false;
	if(((wxCheckBox*)FindWindow(CHECKVERBOSE))->GetValue())
		m_batch->verbose = true;
	else
		m_batch->verbose = false;
}
void BatchFrame::RunBatch()
{
	m_batch->RunBatch();
}

void BatchFrame::SetLocaleAndXRC(wxLocale* locale, wxString xrc)
{
	m_locale = locale;
	m_xrcPrefix = xrc;
}

void BatchFrame::SwapProject(int index)
{
	if(index>=0 && index<(projListBox->GetItemCount()-1))
	{
		projListBox->SwapProject(index);
		m_batch->SwapProject(index);
		if(projListBox->GetSelectedIndex()==index)
			projListBox->Select(index+1);
		else
			projListBox->Select(index);
	}
}


void BatchFrame::OnProcessTerminate(wxProcessEvent & event)
{
	/*//we find the right pointer to remove
	unsigned int i = 0;
	while(i < wxGetApp().stitchFrames.GetCount() &&
	wxGetApp().stitchFrames.Item(i)->GetProjectId()!=event.GetId())
	{
		i++;
	}
	/*RunStitchFrame* framePtr = wxGetApp().stitchFrames.Item(i);
	delete framePtr;*/
	//wxGetApp().stitchFrames.RemoveAt(i);
	if(m_batch->GetRunningCount()==1)
		((wxToolBar*)FindWindow(TOOLBAR))->ToggleTool(TOOLPAUSE,false);
	event.Skip();
	/*i = wxGetApp().GetIndex(event.GetId());
	if (event.GetExitCode() != 0)
		wxGetApp().m_projList.Item(i).status=Project::FAILED;
	else		
		wxGetApp().m_projList.Item(i).status=Project::FINISHED;
	if(!m_cancelled && !((wxToolBar*)FindWindow(TOOLBAR))->GetToolState(TOOLPAUSE))
	{
		if(wxGetApp().AllDone())
		{
			if(wxGetApp().NoErrors())
				SetStatusText(_T("Project \""+wxGetApp().m_projList.Item(i).path)+_T("\" finished. Batch successfully completed."));
			else
				SetStatusText(_T("Project \""+wxGetApp().m_projList.Item(i).path)+_T("\" finished. Batch completed with errors."));
			if(((wxCheckBox*)FindWindow(CHECKSHUTDOWN))->GetValue())	//after we are finished we turn off the computer if checked
			{
				wxProgressDialog progress(_T("Initializing shutdown..."), _T("Shutting down..."),49,this,
					wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_CAN_SKIP);
				progress.Fit();
				int i = 0;
				bool skip = false;
				while(progress.Update(i, _T("Shutting down..."),&skip))
				{	
					if(skip || i==50)
					{
						wxShutdown(wxSHUTDOWN_POWEROFF);
					}
					i++;
					GetThread()->Sleep(200);
				}
				progress.Close();
			}
		}
		else
		{
			if(((wxCheckBox*)FindWindow(CHECKPARALLEL))->GetValue())	//if we are running in parallel
			{
				//the last executed process in parallel runs next
				if(wxGetApp().GetRunningCount() == 0)
				{
					SetStatusText(_T("Project \""+wxGetApp().m_projList.Item(i).path)+_T("\" finished. Running next project..."));
					RunNextInBatch();
				}
				else
				{
					SetStatusText(_T("Project \""+wxGetApp().m_projList.Item(i).path)+_T("\" finished. Waiting for all in parallel to complete..."));
				}
			}
			else
			{
				SetStatusText(_T("Project \""+wxGetApp().m_projList.Item(i).path)+_T("\" finished. Running next project..."));
				RunNextInBatch();
			}
		}
	}
	else
	{	//after all processes have ended on a cancel, we reset the boolean back to false
		if(wxGetApp().stitchFrames.GetCount()==0)
		{
			m_cancelled=false;
		}
	}*/
}

void BatchFrame::OnSizeChange(wxSizeEvent &event)
{
	if(this->IsMaximized())
		wxConfigBase::Get()->Write(wxT("/BatchFrame/Max"), 1);
	else
	{
		wxConfigBase::Get()->Write(wxT("/BatchFrame/Max"), 0);
		wxConfigBase::Get()->Write(wxT("/BatchFrame/Width"), event.GetSize().GetWidth());
		wxConfigBase::Get()->Write(wxT("/BatchFrame/Height"), event.GetSize().GetHeight());
	}
		
	if(this->GetSizer()!=NULL)
	{
		this->Layout();
	}
}
