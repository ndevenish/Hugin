// -*- c-basic-offset: 4 -*-

/** @file PTBatcherGUI.cpp
 *
 *  @brief Batch processor for Hugin with GUI
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: PTBatcherGUI.cpp 3322 2008-08-16 5:00:07Z mkuder $
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

#include "PTBatcherGUI.h"

// make wxwindows use this class as the main application
IMPLEMENT_APP(PTBatcherGUI)

BEGIN_EVENT_TABLE(PTBatcherGUI, wxApp)
	EVT_LIST_ITEM_ACTIVATED(PROJLISTBOX,PTBatcherGUI::OnItemActivated)
	EVT_KEY_DOWN(PTBatcherGUI::OnKeyDown)
END_EVENT_TABLE()

bool PTBatcherGUI::OnInit()
{
	// Required to access the preferences of hugin
    SetAppName(wxT("hugin"));

    m_locale.Init(wxLANGUAGE_DEFAULT);

    // setup the environment for the different operating systems
#if defined __WXMSW__
    wxString huginExeDir = getExePath(argv[0]);

    wxString huginRoot;
    wxFileName::SplitPath(huginExeDir, &huginRoot, NULL, NULL);
	m_xrcPrefix = wxString(huginRoot + wxT("/share/hugin/xrc/"));
	
	// locale setup
    m_locale.AddCatalogLookupPathPrefix(huginRoot + wxT("/share/locale"));

    progs = getPTProgramsConfig(huginExeDir, wxConfigBase::Get());

#elif defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    {
		progs = getPTProgramsConfig(wxT(""), wxConfigBase::Get());
        wxString exec_path = MacGetPathToBundledExecutableFile(CFSTR("nona"));	 
        if(exec_path != wxT(""))
        {
            progs.nona = exec_path.mb_str(HUGIN_CONV_FILENAME);
			m_xrcPrefix = exec_path + wxT("/");
        }
		else
			wxMessageBox(_("xrc directory not found in bundle"), _("Fatal Error"));
		
        exec_path = MacGetPathToBundledExecutableFile(CFSTR("hugin_hdrmerge"));	 
        if(exec_path != wxT(""))	 
        {
            progs.hdrmerge = exec_path.mb_str(HUGIN_CONV_FILENAME);
        }

        wxString thePath = MacGetPathToBundledResourceFile(CFSTR("locale"));
        if(thePath != wxT(""))
            m_locale.AddCatalogLookupPathPrefix(thePath);
        else {
            wxMessageBox(_("Translations not found in bundle"), _("Fatal Error"));
            return false;
        }
    }
#else
    // add the locale directory specified during configure
	m_xrcPrefix = wxT(INSTALL_XRC_DIR);
    m_locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
    PTPrograms progs = getPTProgramsConfig(wxT(""), wxConfigBase::Get());
#endif
    // update incompatible configuration entries.
    updateHuginConfig(wxConfigBase::Get());

    // set the name of locale recource to look for
    m_locale.AddCatalog(wxT("hugin"));

    // parse arguments
    static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
      { wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("show this help message"),
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
     // { wxCMD_LINE_OPTION, wxT("t"), wxT("threads"),  wxT("number of threads"),
     //        wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
      { wxCMD_LINE_SWITCH, wxT("b"), wxT("batch"),  wxT("run batch immediately") },
	  { wxCMD_LINE_SWITCH, wxT("p"), wxT("parallel"),  wxT("run batch projects in parallel") },
	  { wxCMD_LINE_SWITCH, wxT("d"), wxT("delete"),  wxT("delete *.pto files after stitching") },
	  { wxCMD_LINE_SWITCH, wxT("o"), wxT("overwrite"),  wxT("overwrite previous files without asking") },
	  { wxCMD_LINE_SWITCH, wxT("s"), wxT("shutdown"),  wxT("shutdown computer after batch is complete") },
	  { wxCMD_LINE_SWITCH, wxT("v"), wxT("verbose"),  wxT("show verbose output when processing projects") },
      { wxCMD_LINE_PARAM,  NULL, NULL, _("<project1 <output prefix>> <project2 <output prefix>> <project3"),
        wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL + wxCMD_LINE_PARAM_MULTIPLE },
      { wxCMD_LINE_NONE }
    };
    wxCmdLineParser parser(cmdLineDesc, argc, argv);

    switch ( parser.Parse() ) {
		case -1: // -h or --help was given, and help displayed so exit
			return false;
			break;
		case 0:  // all is well
			break;
		default:
			wxLogError(_("Syntax error in parameters detected, aborting."));
			return false;
			break;
    }

	/*    wxString scriptFile;
#ifdef __WXMAC__
    m_macFileNameToOpenOnStart = wxT("");
    wxYield();
    scriptFile = m_macFileNameToOpenOnStart;
    
    // bring myself front (for being called from command line)
    {
        ProcessSerialNumber selfPSN;
        OSErr err = GetCurrentProcess(&selfPSN);
        if (err == noErr)
        {
            SetFrontProcess(&selfPSN);
        }
    }
#endif*/

	m_frame = new BatchFrame(_("Batch Processor"),&m_locale,m_xrcPrefix);
	m_frame->Show(true);
	//m_frame->SetLocaleAndXRC(&m_locale,m_xrcPrefix);
	//projectsRunning=0;
	unsigned int count = 0;
	bool projectSpecified = false;
	//we collect all parameters - all project files <and their output prefixes>
	while(parser.GetParamCount()>count)
	{
		wxString param = parser.GetParam(count);
		count++;
		if(!projectSpecified)	//next parameter must be new script file
		{
			wxFileName name(param);
			Project *proj = new Project(param,name.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + name.GetName());
			projList.Add(proj);
			m_frame->SetStatusText(_("Added project "+param));
			
			m_frame->projListBox->AppendProject(proj);
			projectSpecified = true;
		}
		else	//parameter could be previous project's output prefix
		{
			wxFileName fn(param);
			if(!fn.HasExt())	//if there is no extension we have a prefix
			{
				projList.Last().prefix = param;
				m_frame->projListBox->ReloadProject(m_frame->projListBox->GetItemCount()-1,((Project*)&projList.Last()));
				projectSpecified = false;
			}
			else
			{
				wxString ext = fn.GetExt();
				//we may still have a prefix, but with added image extension
				if (ext.CmpNoCase(wxT("jpg")) == 0 || ext.CmpNoCase(wxT("jpeg")) == 0|| 
					ext.CmpNoCase(wxT("tif")) == 0|| ext.CmpNoCase(wxT("tiff")) == 0 || 
					ext.CmpNoCase(wxT("png")) == 0 || ext.CmpNoCase(wxT("exr")) == 0 ||
					ext.CmpNoCase(wxT("pnm")) == 0 || ext.CmpNoCase(wxT("hdr")) == 0) 
				{
					//extension will be removed before stitch, so there is no need to do it now
					projList.Last().prefix = param;
					m_frame->projListBox->ReloadProject(m_frame->projListBox->GetItemCount()-1,((Project*)&projList.Last()));
					projectSpecified = false;
				}
				else //if parameter has a different extension we presume it is a new script file
				{
				//removed -- Now we generate prefixes automatically, so there is no need to ask user
					/*
					//we must set a prefix for the previous project 
					//we select the last project in GUI list, so the user is shown which
					
					m_frame->projListBox->Select(m_frame->projListBox->GetItemCount()-1);

					wxFileDialog dlg(0,_("Specify output prefix for project "+projList.Last().path),
							 wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")),
							 wxT(""), wxT(""),
							 wxSAVE, wxDefaultPosition);
					dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
					if (dlg.ShowModal() == wxID_OK) {
						wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
						projList.Last().prefix = dlg.GetPath();
						m_frame->projListBox->ReloadProject(m_frame->projListBox->GetItemCount()-1,((Project*)&projList.Last()));
					} else { // bail
						wxLogError( _("Output prefix not specified"));
					}
					//deselect the last project in list

					m_frame->projListBox->Deselect(m_frame->projListBox->GetItemCount()-1);*/

					//we add the new project
					wxFileName name(param);
					Project *proj = new Project(param,name.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + name.GetName());
					projList.Add(proj);
					m_frame->SetStatusText(_("Added project "+param));
					m_frame->projListBox->AppendProject(proj);
					projectSpecified = true;
				}
			} //else of if(!fn.HasExt())
		}
	}

	if (parser.Found(wxT("d")) ) {
		((wxCheckBox*)m_frame->FindWindow(CHECKDELETE))->SetValue(true);
		wxConfigBase::Get()->Write(wxT("/BatchFrame/DeleteCheck"), 1);
    }
	else{
		int del = wxConfigBase::Get()->Read(wxT("/BatchFrame/DeleteCheck"), (long)0);
		if(del==0)
			((wxCheckBox*)m_frame->FindWindow(CHECKDELETE))->SetValue(false);
		else
			((wxCheckBox*)m_frame->FindWindow(CHECKDELETE))->SetValue(true);
    }
	if (parser.Found(wxT("p")) ) {
		((wxCheckBox*)m_frame->FindWindow(CHECKPARALLEL))->SetValue(true);
		wxConfigBase::Get()->Write(wxT("/BatchFrame/ParallelCheck"), 1);
    }
	else{
		int par = wxConfigBase::Get()->Read(wxT("/BatchFrame/ParallelCheck"), (long)0);
		if(par==0)
			((wxCheckBox*)m_frame->FindWindow(CHECKPARALLEL))->SetValue(false);
		else
			((wxCheckBox*)m_frame->FindWindow(CHECKPARALLEL))->SetValue(true);
	}
	if (parser.Found(wxT("s")) ) {
		((wxCheckBox*)m_frame->FindWindow(CHECKSHUTDOWN))->SetValue(true);
		wxConfigBase::Get()->Write(wxT("/BatchFrame/ShutdownCheck"), 1);
    }
	else{
		int shtdwn = wxConfigBase::Get()->Read(wxT("/BatchFrame/ShutdownCheck"), (long)0);
		if(shtdwn==0)
			((wxCheckBox*)m_frame->FindWindow(CHECKSHUTDOWN))->SetValue(false);
		else
			((wxCheckBox*)m_frame->FindWindow(CHECKSHUTDOWN))->SetValue(true);
	}
	if (parser.Found(wxT("o")) ) {
		((wxCheckBox*)m_frame->FindWindow(CHECKOVERWRITE))->SetValue(true);
		wxConfigBase::Get()->Write(wxT("/BatchFrame/OverwriteCheck"), 1);
    }
	else{
		int overwrite = wxConfigBase::Get()->Read(wxT("/BatchFrame/OverwriteCheck"), (long)0);
		if(overwrite==0)
			((wxCheckBox*)m_frame->FindWindow(CHECKOVERWRITE))->SetValue(false);
		else
			((wxCheckBox*)m_frame->FindWindow(CHECKOVERWRITE))->SetValue(true);
	}
	if (parser.Found(wxT("v")) ) {
		((wxCheckBox*)m_frame->FindWindow(CHECKVERBOSE))->SetValue(true);
		wxConfigBase::Get()->Write(wxT("/BatchFrame/VerboseCheck"), 1);
    }
	else{
		int overwrite = wxConfigBase::Get()->Read(wxT("/BatchFrame/VerboseCheck"), (long)0);
		if(overwrite==0)
			((wxCheckBox*)m_frame->FindWindow(CHECKVERBOSE))->SetValue(false);
		else
			((wxCheckBox*)m_frame->FindWindow(CHECKVERBOSE))->SetValue(true);
	}
	m_frame->PropagateDefaults();
	if (parser.Found(wxT("b")) ) {
		m_frame->RunBatch();
    }
	return true;
}

void PTBatcherGUI::OnItemActivated(wxListEvent &event)
{
	wxCommandEvent dummy;
	m_frame->OnButtonOpenWithHugin(dummy);
}

void PTBatcherGUI::OnKeyDown(wxKeyEvent &event)
{
	wxCommandEvent dummy;
	switch(event.GetKeyCode())
	{
	case WXK_DELETE:	
		m_frame->OnButtonRemoveFromList(dummy);
		event.Skip();
		break;
	case WXK_INSERT:
		m_frame->OnButtonAddToList(dummy);
		event.Skip();
		break;
	case WXK_ESCAPE:
		m_frame->OnButtonCancel(dummy);
		event.Skip();
		break;
	default:
		event.Skip();
		break;
	}
	
}

#ifdef __WXMAC__
// wx calls this method when the app gets "Open file" AppleEvent 
void PTBatcherGUI::MacOpenFile(const wxString &fileName)
{
    m_macFileNameToOpenOnStart = fileName;
}
#endif
