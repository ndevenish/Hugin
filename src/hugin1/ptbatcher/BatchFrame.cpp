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
#include "PTBatcherGUI.h"
#include "FindPanoDialog.h"
#include "FailedProjectsDialog.h"
#ifdef __WXMSW__
#include <powrprof.h>
#pragma comment(lib, "PowrProf.lib")
#endif

/* file drag and drop handler method */
bool BatchDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    BatchFrame* MyBatchFrame = wxGetApp().GetFrame();
    if (!MyBatchFrame)
    {
        return false;
    }
    if(filenames.GetCount()==0)
    {
        return false;
    }
    for(unsigned int i=0; i< filenames.GetCount(); i++)
    {
        wxFileName file(filenames[i]);
        if(file.HasExt())
        {
            if (file.GetExt().CmpNoCase(wxT("pto")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("ptp")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("pts")) == 0 )
            {
                if(file.FileExists())
                {
                    MyBatchFrame->AddToList(filenames[i]);
                }
            };
        }
        else
        {
            if(file.DirExists())
            {
                MyBatchFrame->AddDirToList(filenames[i]);
            }
        };
    };
    return true;
};

enum
{
    wxEVT_COMMAND_RELOAD_BATCH,
    wxEVT_COMMAND_UPDATE_LISTBOX
};

BEGIN_EVENT_TABLE(BatchFrame, wxFrame)
    EVT_TOOL(XRCID("tool_clear"),BatchFrame::OnButtonClear)
    EVT_TOOL(XRCID("tool_open"),BatchFrame::OnButtonOpenBatch)
    EVT_TOOL(XRCID("tool_save"),BatchFrame::OnButtonSaveBatch)
    EVT_TOOL(XRCID("tool_start"),BatchFrame::OnButtonRunBatch)
    EVT_TOOL(XRCID("tool_skip"),BatchFrame::OnButtonSkip)
    EVT_TOOL(XRCID("tool_pause"),BatchFrame::OnButtonPause)
    EVT_TOOL(XRCID("tool_cancel"),BatchFrame::OnButtonCancel)
    EVT_TOOL(XRCID("tool_add"),BatchFrame::OnButtonAddToStitchingQueue)
    EVT_TOOL(XRCID("tool_remove"),BatchFrame::OnButtonRemoveFromList)
    EVT_TOOL(XRCID("tool_adddir"),BatchFrame::OnButtonAddDir)
    EVT_MENU(XRCID("menu_add"),BatchFrame::OnButtonAddToStitchingQueue)
    EVT_MENU(XRCID("menu_add_assistant"),BatchFrame::OnButtonAddToAssistantQueue)
    EVT_MENU(XRCID("menu_remove"),BatchFrame::OnButtonRemoveFromList)
    EVT_MENU(XRCID("menu_adddir"),BatchFrame::OnButtonAddDir)
    EVT_MENU(XRCID("menu_searchpano"), BatchFrame::OnButtonSearchPano)
    EVT_MENU(XRCID("menu_open"),BatchFrame::OnButtonOpenBatch)
    EVT_MENU(XRCID("menu_save"),BatchFrame::OnButtonSaveBatch)
    EVT_MENU(XRCID("menu_clear"),BatchFrame::OnButtonClear)
    EVT_MENU(XRCID("menu_exit"),BatchFrame::OnUserExit)
    EVT_MENU(XRCID("menu_help"),BatchFrame::OnButtonHelp)
    EVT_BUTTON(XRCID("button_addcommand"),BatchFrame::OnButtonAddCommand)
    EVT_BUTTON(XRCID("button_remove"),BatchFrame::OnButtonRemoveComplete)
    EVT_BUTTON(XRCID("button_prefix"),BatchFrame::OnButtonChangePrefix)
    EVT_BUTTON(XRCID("button_reset"),BatchFrame::OnButtonReset)
    EVT_BUTTON(XRCID("button_resetall"),BatchFrame::OnButtonResetAll)
    EVT_BUTTON(XRCID("button_edit"),BatchFrame::OnButtonOpenWithHugin)
    EVT_BUTTON(XRCID("button_move_up"),BatchFrame::OnButtonMoveUp)
    EVT_BUTTON(XRCID("button_move_down"),BatchFrame::OnButtonMoveDown)
    EVT_CHECKBOX(XRCID("cb_parallel"), BatchFrame::OnCheckParallel)
    EVT_CHECKBOX(XRCID("cb_overwrite"), BatchFrame::OnCheckOverwrite)
    EVT_CHOICE(XRCID("choice_end"), BatchFrame::OnChoiceEnd)
    EVT_CHECKBOX(XRCID("cb_verbose"), BatchFrame::OnCheckVerbose)
    EVT_CHECKBOX(XRCID("cb_autoremove"), BatchFrame::OnCheckAutoRemove)
    EVT_CHECKBOX(XRCID("cb_autostitch"), BatchFrame::OnCheckAutoStitch)
    EVT_CHECKBOX(XRCID("cb_savelog"), BatchFrame::OnCheckSaveLog)
    EVT_END_PROCESS(-1, BatchFrame::OnProcessTerminate)
    EVT_CLOSE(BatchFrame::OnClose)
    EVT_MENU(wxEVT_COMMAND_RELOAD_BATCH, BatchFrame::OnReloadBatch)
    EVT_MENU(wxEVT_COMMAND_UPDATE_LISTBOX, BatchFrame::OnUpdateListBox)
    EVT_COMMAND(wxID_ANY, EVT_BATCH_FAILED, BatchFrame::OnBatchFailed)
    EVT_COMMAND(wxID_ANY, EVT_INFORMATION, BatchFrame::OnBatchInformation)
    EVT_COMMAND(wxID_ANY, EVT_UPDATE_PARENT, BatchFrame::OnRefillListBox)
    EVT_ICONIZE(BatchFrame::OnMinimize)
END_EVENT_TABLE()

BatchFrame::BatchFrame(wxLocale* locale, wxString xrc)
{
    this->SetLocaleAndXRC(locale,xrc);
    m_cancelled = false;
    m_closeThread = false;
#ifndef __WXMSW__
    m_help=0;
#endif

    //load xrc resources
    wxXmlResource::Get()->LoadFrame(this, (wxWindow* )NULL, wxT("batch_frame"));
    // load our menu bar
#ifdef __WXMAC__
    wxApp::s_macExitMenuItemId = XRCID("menu_exit");
    wxApp::s_macHelpMenuTitleName = _("&Help");
#endif
    SetMenuBar(wxXmlResource::Get()->LoadMenuBar(this, wxT("batch_menu")));

    // create tool bar
    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, wxT("batch_toolbar")));

    CreateStatusBar(1);
    SetStatusText(_("Not doing much..."));

    // set the minimize icon
#ifdef __WXMSW__
    wxIcon myIcon(m_xrcPrefix + wxT("data/ptbatcher.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(m_xrcPrefix + wxT("data/ptbatcher.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

#if defined __WXMSW__
    //show tray icon only on windows
    //because not all window manager on *nix provide a usable tray area
#if wxCHECK_VERSION(2,9,0)
    if(wxTaskBarIcon::IsAvailable())
    {
        m_tray=new BatchTaskBarIcon();
        m_tray->SetIcon(myIcon,_("Hugin's Batch processor"));
    }
    else
    {
        m_tray=NULL;
    };
#else
    m_tray=new BatchTaskBarIcon();
    if(m_tray->IsOk())
    {
        m_tray->SetIcon(myIcon,_("Hugin's Batch processor"));
    }
    else
    {
        m_tray=NULL;
    };
#endif
#else
    m_tray=NULL;
#endif
    m_batch = new Batch(this);
#if wxCHECK_VERSION(2,9,4)
    if(wxGetKeyState(WXK_COMMAND))
#else
    if(wxGetKeyState(WXK_CONTROL))
#endif
    {
#ifdef __WXMAC__
        wxString text(_("You have pressed the Command key."));
#else
        wxString text(_("You have pressed the Control key."));
#endif
        text.Append(wxT("\n"));
        text.Append(_("Should the loading of the batch queue be skipped?"));
        if(wxMessageBox(text, 
#ifdef __WXMSW__
            wxT("PTBatcherGUI"),
#else
            wxEmptyString,
#endif
            wxYES_NO | wxICON_EXCLAMATION, NULL)==wxNO)
        {
            m_batch->LoadTemp();
        };
    }
    else
    {
        m_batch->LoadTemp();
    };
    if(m_batch->GetLastFile().length()==0)
    {
        m_batch->SaveTemp();
    }
    projListBox = XRCCTRL(*this,"project_listbox",ProjectListBox);
    // fill at end list box, check which options are available
    m_endChoice = XRCCTRL(*this, "choice_end", wxChoice);
    m_endChoice->Clear();
    m_endChoice->Append(_("Do nothing"), (void*)Batch::DO_NOTHING);
    m_endChoice->Append(_("Close PTBatcherGUI"), (void*)Batch::CLOSE_PTBATCHERGUI);
#if !defined __WXMAC__ && !defined __WXOSX_COCOA__
    // there is no wxShutdown for wxMac
    m_endChoice->Append(_("Shutdown computer"), (void*)Batch::SHUTDOWN);
#endif
#ifdef __WXMSW__
    SYSTEM_POWER_CAPABILITIES pwrCap;
    if (GetPwrCapabilities(&pwrCap))
    {
        if (pwrCap.SystemS3)
        {
            m_endChoice->Append(_("Suspend computer"), (void*)Batch::SUSPEND);
        };
        if (pwrCap.SystemS4 && pwrCap.HiberFilePresent)
        {
            m_endChoice->Append(_("Hibernate computer"), (void*)Batch::HIBERNATE);
        };
    };
#endif

    //wxThreadHelper::Create is deprecated in wxWidgets 2.9+, but its
    // replacement, CreateThread, does not exist in 2.8. Pick one
    // depending on the version to a avoid compiler warning(2.9) or error(2.8).
#if wxCHECK_VERSION(2, 9, 0)
    this->wxThreadHelper::CreateThread();
#else
    this->wxThreadHelper::Create();
#endif
    this->GetThread()->Run();
    //TO-DO: include a batch or project progress gauge?
    projListBox->Fill(m_batch);
    SetDropTarget(new BatchDropTarget());
}

void* BatchFrame::Entry()
{
    //we constantly poll the working dir for new files and wait a bit on each loop
    while(!m_closeThread)
    {
        //don't access GUI directly in this function (function is running as separate thread)
        //check, if ptbt file was changed
        wxFileName aFile(m_batch->GetLastFile());
        if(!aFile.FileExists())
        {
#if wxCHECK_VERSION(3,0,0)
            // for thread safety wxWidgets recommends using wxQueueEvent
            // but this function is not available for older wxWidgets versions
            wxQueueEvent(this, new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, wxEVT_COMMAND_RELOAD_BATCH));
#else
            wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxEVT_COMMAND_RELOAD_BATCH);
            wxPostEvent(this, evt);
#endif
        }
        else
        {
            wxDateTime create;
            aFile.GetTimes(NULL,NULL,&create);
            if(create.IsLaterThan(m_batch->GetLastFileDate()))
            {
#if wxCHECK_VERSION(3,0,0)
                wxQueueEvent(this, new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, wxEVT_COMMAND_RELOAD_BATCH));
#else
                wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxEVT_COMMAND_RELOAD_BATCH);
                wxPostEvent(this, evt);
#endif
            };
        };
        //update project list box
#if wxCHECK_VERSION(3,0,0)
        wxQueueEvent(this, new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, wxEVT_COMMAND_UPDATE_LISTBOX));
#else
        wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxEVT_COMMAND_UPDATE_LISTBOX);
        wxPostEvent(this, evt);
#endif
        GetThread()->Sleep(1000);
    }
    return 0;
}

bool BatchFrame::IsRunning()
{
    return m_batch->IsRunning();
};

bool BatchFrame::IsPaused()
{
    return m_batch->IsPaused();
};

void BatchFrame::OnUpdateListBox(wxCommandEvent& event)
{
    wxFileName tempFile;
    bool change = false;
    for(int i = 0; i< m_batch->GetProjectCount(); i++)
    {
        if(m_batch->GetProject(i)->id >= 0 && m_batch->GetStatus(i)!=Project::FINISHED)
        {
            tempFile.Assign(m_batch->GetProject(i)->path);
            if(tempFile.FileExists())
            {
                wxDateTime modify;
                modify=tempFile.GetModificationTime();
                if(m_batch->GetProject(i)->skip)
                {
                    change = true;
                    m_batch->GetProject(i)->skip = false;
                    m_batch->SetStatus(i,Project::WAITING);
                    projListBox->ReloadProject(projListBox->GetIndex(m_batch->GetProject(i)->id),m_batch->GetProject(i));
                }
                else if(!modify.IsEqualTo(m_batch->GetProject(i)->modDate))
                {
                    change = true;
                    m_batch->GetProject(i)->modDate = modify;
                    m_batch->GetProject(i)->ResetOptions();
                    if(m_batch->GetProject(i)->target==Project::STITCHING)
                    {
                        m_batch->SetStatus(i,Project::WAITING);
                    };
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
        {
            change = true;
        }
    }
    if(change)
    {
        m_batch->SaveTemp();
    }
};

void BatchFrame::OnReloadBatch(wxCommandEvent& event)
{
    m_batch->ClearBatch();
    m_batch->LoadTemp();
    projListBox->DeleteAllItems();
    projListBox->Fill(m_batch);
    SetStatusText(wxT(""));
};

void BatchFrame::OnUserExit(wxCommandEvent& event)
{
    Close(true);
};

void BatchFrame::OnButtonAddCommand(wxCommandEvent& event)
{
    wxTextEntryDialog dlg(this,_("Please enter the command-line application to execute:"),_("Enter application"));
    wxTheApp->SetEvtHandlerEnabled(false);
    if(dlg.ShowModal() == wxID_OK)
    {
        wxString line = dlg.GetValue();
        m_batch->AddAppToBatch(line);
        //SetStatusText(_T("Added application"));
        projListBox->AppendProject(m_batch->GetProject(m_batch->GetProjectCount()-1));
        m_batch->SaveTemp();
    }
    wxTheApp->SetEvtHandlerEnabled(true);
}

void BatchFrame::OnButtonAddDir(wxCommandEvent& event)
{
    wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/actualPath"),wxT(""));
    wxDirDialog dlg(this,
                    _("Specify a directory to search for projects in"),
                    defaultdir, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    dlg.SetPath(wxConfigBase::Get()->Read(wxT("/BatchFrame/actualPath"),wxT("")));
    if (dlg.ShowModal() == wxID_OK)
    {
        wxConfig::Get()->Write(wxT("/BatchFrame/actualPath"), dlg.GetPath());  // remember for later
        AddDirToList(dlg.GetPath());
    };
}

void BatchFrame::OnButtonSearchPano(wxCommandEvent& e)
{
    FindPanoDialog findpano_dlg(this,m_xrcPrefix);
    findpano_dlg.ShowModal();
};

void BatchFrame::OnButtonAddToStitchingQueue(wxCommandEvent& event)
{
    wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/actualPath"),wxT(""));
    wxFileDialog dlg(0,
                     _("Specify project source file(s)"),
                     defaultdir, wxT(""),
                     _("Project files (*.pto)|*.pto|All files (*)|*"),
                     wxFD_OPEN | wxFD_MULTIPLE, wxDefaultPosition);
    dlg.SetDirectory(defaultdir);

    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString paths;
        dlg.GetPaths(paths);
#ifdef __WXGTK__
        //workaround a bug in GTK, see https://bugzilla.redhat.com/show_bug.cgi?id=849692 and http://trac.wxwidgets.org/ticket/14525
        wxConfig::Get()->Write(wxT("/BatchFrame/actualPath"), wxPathOnly(paths[0]));  // remember for later
#else
        wxConfig::Get()->Write(wxT("/BatchFrame/actualPath"), dlg.GetDirectory());  // remember for later
#endif
        for(unsigned int i=0; i<paths.GetCount(); i++)
        {
            AddToList(paths.Item(i));
        }
        m_batch->SaveTemp();
    };
}

void BatchFrame::OnButtonAddToAssistantQueue(wxCommandEvent& event)
{
    wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/actualPath"),wxT(""));
    wxFileDialog dlg(0,
                     _("Specify project source file(s)"),
                     defaultdir, wxT(""),
                     _("Project files (*.pto)|*.pto|All files (*)|*"),
                     wxFD_OPEN | wxFD_MULTIPLE, wxDefaultPosition);
    dlg.SetDirectory(defaultdir);

    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString paths;
        dlg.GetPaths(paths);
#ifdef __WXGTK__
        //workaround a bug in GTK, see https://bugzilla.redhat.com/show_bug.cgi?id=849692 and http://trac.wxwidgets.org/ticket/14525
        wxConfig::Get()->Write(wxT("/BatchFrame/actualPath"), wxPathOnly(paths[0]));  // remember for later
#else
        wxConfig::Get()->Write(wxT("/BatchFrame/actualPath"), dlg.GetDirectory());  // remember for later
#endif

        for(unsigned int i=0; i<paths.GetCount(); i++)
        {
            AddToList(paths.Item(i),Project::DETECTING);
        }
        m_batch->SaveTemp();
    };
}

void BatchFrame::AddDirToList(wxString aDir)
{
    //we traverse all subdirectories of chosen path
    DirTraverser traverser;
    wxDir dir(aDir);
    dir.Traverse(traverser);
    wxArrayString projects = traverser.GetProjectFiles();
    for(unsigned int i=0; i<projects.GetCount(); i++)
    {
        m_batch->AddProjectToBatch(projects.Item(i));
        Project* proj=m_batch->GetProject(m_batch->GetProjectCount()-1);
        if(!proj->isAligned)
        {
            proj->target=Project::DETECTING;
        };
        projListBox->AppendProject(proj);
    };
    m_batch->SaveTemp();
    SetStatusText(_("Added projects from dir ")+aDir);
};

void BatchFrame::AddToList(wxString aFile,Project::Target target)
{
    wxFileName name(aFile);
    m_batch->AddProjectToBatch(aFile,wxT(""),target);
    wxString s;
    switch(target)
    {
        case Project::STITCHING:
            s=wxString::Format(_("Add project %s to stitching queue."),aFile.c_str());
            break;
        case Project::DETECTING:
            s=wxString::Format(_("Add project %s to assistant queue."),aFile.c_str());
            break;
    };
    SetStatusInformation(s,true);
    projListBox->AppendProject(m_batch->GetProject(m_batch->GetProjectCount()-1));
    m_batch->SaveTemp();
}


void BatchFrame::OnButtonCancel(wxCommandEvent& event)
{
    GetToolBar()->ToggleTool(XRCID("tool_pause"),false);
    m_cancelled = true;
    m_batch->CancelBatch();
    SetStatusInformation(_("Batch stopped"),true);
}

void BatchFrame::OnButtonChangePrefix(wxCommandEvent& event)
{
    int selIndex = projListBox->GetSelectedIndex();
    if(selIndex != -1)
    {
        if(projListBox->GetSelectedProjectTarget()==Project::STITCHING)
        {
            wxFileName prefix(projListBox->GetSelectedProjectPrefix());
            wxFileDialog dlg(0,_("Specify output prefix for project ")+projListBox->GetSelectedProject(),
                             prefix.GetPath(),
                             prefix.GetFullName(), wxT(""),
                             wxFD_SAVE, wxDefaultPosition);
            if (dlg.ShowModal() == wxID_OK)
            {
                while(containsInvalidCharacters(dlg.GetPath()))
                {
                    wxArrayString list;
                    list.Add(dlg.GetPath());
                    ShowFilenameWarning(this, list);
                    if(dlg.ShowModal()!=wxID_OK)
                    {
                        return;
                    }
                };
                wxFileName prefix(dlg.GetPath());
                while (!prefix.IsDirWritable())
                {
                    wxMessageBox(wxString::Format(_("You have no permissions to write in folder \"%s\".\nPlease select another folder for the final output."), prefix.GetPath().c_str()),
#ifdef __WXMSW__
                        wxT("PTBatcherGUI"),
#else
                        wxT(""),
#endif
                        wxOK | wxICON_INFORMATION);
                    if (dlg.ShowModal() != wxID_OK)
                    {
                        return;
                    };
                    prefix = dlg.GetPath();
                };

                wxString outname(dlg.GetPath());
                ChangePrefix(selIndex,outname);
                //SetStatusText(_T("Changed prefix for "+projListBox->GetSelectedProject()));
                m_batch->SaveTemp();
            }
        }
        else
        {
            SetStatusText(_("The prefix of an assistant target cannot be changed."));
            wxBell();
        };
    }
    else
    {
        SetStatusText(_("Please select a project"));
    }
}

void BatchFrame::ChangePrefix(int index,wxString newPrefix)
{
    int i;
    if(index!=-1)
    {
        i=index;
    }
    else
    {
        i=m_batch->GetProjectCount()-1;
    }
    m_batch->ChangePrefix(i,newPrefix);
    projListBox->ChangePrefix(i,newPrefix);
}

void BatchFrame::OnButtonClear(wxCommandEvent& event)
{
    int returnCode = m_batch->ClearBatch();
    if(returnCode == 0)
    {
        projListBox->DeleteAllItems();
    }
    else if(returnCode == 2)
    {
        m_cancelled = true;
        projListBox->DeleteAllItems();
        if(GetToolBar()->GetToolState(XRCID("tool_pause")))
        {
            GetToolBar()->ToggleTool(XRCID("tool_pause"),false);
        }
    }
    m_batch->SaveTemp();
}

void BatchFrame::OnButtonHelp(wxCommandEvent& event)
{
    DEBUG_TRACE("");
#ifdef __WXMSW__
    GetHelpController().DisplaySection(wxT("Hugin_Batch_Processor.html"));
#else
    if (m_help == 0)
    {
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
        // On Mac, xrc/data/help_LOCALE should be in the bundle as LOCALE.lproj/help
        // which we can rely on the operating sytem to pick the right locale's.
        wxString strFile = MacGetPathToBundledResourceFile(CFSTR("help"));
        if(strFile!=wxT(""))
        {
            strFile += wxT("/hugin.hhp");
        }
        else
        {
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
        }
        else
        {
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
#endif
}

void BatchFrame::OnButtonMoveDown(wxCommandEvent& event)
{
    SwapProject(projListBox->GetSelectedIndex());
    m_batch->SaveTemp();
}

void BatchFrame::OnButtonMoveUp(wxCommandEvent& event)
{
    SwapProject(projListBox->GetSelectedIndex()-1);
    m_batch->SaveTemp();
}

void BatchFrame::OnButtonOpenBatch(wxCommandEvent& event)
{
    wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/batchPath"),wxT(""));
    wxFileDialog dlg(0,
                     _("Specify batch file to open"),
                     defaultdir, wxT(""),
                     _("Batch files (*.ptb)|*.ptb;|All files (*)|*"),
                     wxFD_OPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxConfig::Get()->Write(wxT("/BatchFrame/batchPath"), dlg.GetDirectory());  // remember for later
        int clearCode = m_batch->LoadBatchFile(dlg.GetPath());
        //1 is error code for not clearing batch
        if(clearCode!=1)
        {
            /*if(clearCode==2) //we just cancelled the batch, so we need to try loading again
            	m_batch->LoadBatchFile(dlg.GetPath());*/
            projListBox->DeleteAllItems();
            projListBox->Fill(m_batch);
            m_batch->SaveTemp();
        }
    }
}

void BatchFrame::OnButtonOpenWithHugin(wxCommandEvent& event)
{
#ifdef __WINDOWS__
    wxString huginPath = getExePath(wxGetApp().argv[0])+wxFileName::GetPathSeparator();
#else
    wxString huginPath = _T("");	//we call hugin directly without path on linux
#endif
    if(projListBox->GetSelectedIndex()!=-1)
        if(projListBox->GetText(projListBox->GetSelectedIndex(),0).Cmp(_T(""))!=0)
#ifdef __WXMAC__
            wxExecute(_T("open -b net.sourceforge.hugin.hugin \"" + projListBox->GetSelectedProject()+_T("\"")));
#else
            wxExecute(huginPath+_T("hugin \"" + projListBox->GetSelectedProject()+_T("\" -notips")));
#endif
        else
        {
            SetStatusText(_("Cannot open app in Hugin."));
        }
    else
    {
        //ask user if he/she wants to load an empty project
        wxMessageDialog message(this,_("No project selected. Open Hugin without project?"),
#ifdef _WINDOWS
                                _("PTBatcherGUI"),
#else
                                wxT(""),
#endif
                                wxYES | wxNO | wxICON_INFORMATION );
        if(message.ShowModal() == wxID_YES)
        {
#ifdef __WXMAC__
            wxExecute(_T("open -b net.sourceforge.hugin.hugin"));
#else
            wxExecute(huginPath+_T("hugin"));
#endif
        }
    }
}

void BatchFrame::OnButtonPause(wxCommandEvent& event)
{
    if(m_batch->GetRunningCount()>0)
    {
        if(!m_batch->IsPaused())
        {
            m_batch->PauseBatch();
            GetToolBar()->ToggleTool(XRCID("tool_pause"),true);
            SetStatusInformation(_("Batch paused"),true);
        }
        else
        {
            m_batch->PauseBatch();
            GetToolBar()->ToggleTool(XRCID("tool_pause"),false);
            SetStatusInformation(_("Continuing batch..."),true);
        }
    }
    else //if no projects are running, we deactivate the button
    {
        GetToolBar()->ToggleTool(XRCID("tool_pause"),false);
    }
}

void BatchFrame::OnButtonRemoveComplete(wxCommandEvent& event)
{
    bool removeErrors=false;
    if(!m_batch->NoErrors())
    {
        wxMessageDialog message(this,_("There are failed projects in the list.\nRemove them too?"),
#ifdef _WINDOWS
                                _("PTBatcherGUI"),
#else
                                wxT(""),
#endif
                                wxYES | wxNO | wxICON_INFORMATION );
        if(message.ShowModal()==wxID_YES)
        {
            removeErrors=true;
        }
    }
    for(int i=projListBox->GetItemCount()-1; i>=0; i--)
    {
        if(m_batch->GetStatus(i)==Project::FINISHED ||
                (removeErrors && m_batch->GetStatus(i)==Project::FAILED))
        {
            projListBox->Deselect(i);
            projListBox->DeleteItem(i);
            m_batch->RemoveProjectAtIndex(i);
        }
    }
    m_batch->SaveTemp();
}

void BatchFrame::OnButtonRemoveFromList(wxCommandEvent& event)
{
    int selIndex = projListBox->GetSelectedIndex();
    if(selIndex != -1)
    {
        if(m_batch->GetStatus(selIndex)==Project::RUNNING || m_batch->GetStatus(selIndex)==Project::PAUSED)
        {
            wxMessageDialog message(this, _("Cannot remove project in progress.\nDo you want to cancel it?"),
#ifdef _WINDOWS
                                    _("PTBatcherGUI"),
#else
                                    wxT(""),
#endif
                                    wxYES | wxCANCEL | wxICON_INFORMATION);
            if(message.ShowModal()==wxID_YES)
            {
                OnButtonSkip(event);
            }
        }
        else
        {
            SetStatusText(_("Removed project ")+projListBox->GetSelectedProject());
            projListBox->Deselect(selIndex);
            projListBox->DeleteItem(selIndex);
            m_batch->RemoveProjectAtIndex(selIndex);
            m_batch->SaveTemp();
        }
    }
    else
    {
        SetStatusText(_("Please select a project to remove"));
    }
}


void BatchFrame::OnButtonReset(wxCommandEvent& event)
{
    int selIndex = projListBox->GetSelectedIndex();
    if(selIndex != -1)
    {
        if(m_batch->GetStatus(selIndex)==Project::RUNNING || m_batch->GetStatus(selIndex)==Project::PAUSED)
        {
            wxMessageDialog message(this, _("Cannot reset project in progress.\nDo you want to cancel it?"),
#ifdef _WINDOWS
                                    _("PTBatcherGUI"),
#else
                                    wxT(""),
#endif
                                    wxYES | wxCANCEL | wxICON_INFORMATION);
            if(message.ShowModal()==wxID_YES)
            {
                OnButtonSkip(event);
            }
        }
        else
        {
            m_batch->SetStatus(selIndex,Project::WAITING);
            SetStatusText(_("Reset project ")+projListBox->GetSelectedProject());
        }
    }
    else
    {
        SetStatusText(_("Please select a project to reset"));
    }
    m_batch->SaveTemp();
}

void BatchFrame::OnButtonResetAll(wxCommandEvent& event)
{
    if(m_batch->GetRunningCount()!=0)
    {
        wxMessageDialog message(this, _("Cannot reset projects in progress.\nDo you want to cancel the batch?"),
#ifdef _WINDOWS
                                _("PTBatcherGUI"),
#else
                                wxT(""),
#endif
                                wxYES | wxCANCEL | wxICON_INFORMATION);
        if(message.ShowModal()==wxID_YES)
        {
            OnButtonCancel(event);
        }
    }
    else
    {
        for(int i=projListBox->GetItemCount()-1; i>=0; i--)
        {
            m_batch->SetStatus(i,Project::WAITING);
        }
    }
    m_batch->SaveTemp();
}

void BatchFrame::OnButtonRunBatch(wxCommandEvent& event)
{
    if(m_batch->IsPaused())
    {
        //m_batch->paused = false;
        OnButtonPause(event);
    }
    else
    {
        RunBatch();
    }
}

void BatchFrame::OnButtonSaveBatch(wxCommandEvent& event)
{
    wxString defaultdir = wxConfigBase::Get()->Read(wxT("/BatchFrame/batchPath"),wxT(""));
    wxFileDialog dlg(0,
                     _("Specify batch file to save"),
                     defaultdir, wxT(""),
                     _("Batch file (*.ptb)|*.ptb;|All files (*)|*"),
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
    {
        wxConfig::Get()->Write(wxT("/BatchFrame/batchPath"), dlg.GetDirectory());  // remember for later
        m_batch->SaveBatchFile(dlg.GetPath());
    }
}

void BatchFrame::OnButtonSkip(wxCommandEvent& event)
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
                {
                    GetToolBar()->ToggleTool(XRCID("tool_pause"),false);
                }
                for(int i=0; i<m_batch->GetRunningCount(); i++)
                {
                    if(m_batch->GetStatus(selIndex)==Project::PAUSED
                            && m_batch->CompareProjectsInLists(i,selIndex))
                    {
                        m_batch->CancelProject(i);
                    }
                }
            }
            else
            {
                //we go through all running projects to find the one with the same id as chosen one
                for(int i=0; i<m_batch->GetRunningCount(); i++)
                {
                    if(m_batch->GetStatus(selIndex)==Project::RUNNING
                            && m_batch->CompareProjectsInLists(i,selIndex))
                    {
                        m_batch->CancelProject(i);
                    }
                }
            }
        }
        else
        {
            m_batch->SetStatus(selIndex,Project::FAILED);
        }
    }
}

void SelectEndTask(wxControlWithItems* list, const Batch::EndTask endTask)
{
    for (unsigned int i = 0; i<list->GetCount(); i++)
    {
        if (static_cast<Batch::EndTask>((size_t)list->GetClientData(i)) == endTask)
        {
            list->SetSelection(i);
            return;
        };
    };
    list->SetSelection(0);
};

void BatchFrame::SetCheckboxes()
{
    wxConfigBase* config=wxConfigBase::Get();
    int i;
    i=config->Read(wxT("/BatchFrame/ParallelCheck"), 0l);
    XRCCTRL(*this,"cb_parallel",wxCheckBox)->SetValue(i!=0);
    m_batch->parallel=(i!=0);
    // read older version
#if defined __WXMAC__ || defined __WXOSX_COCOA__
    i = 0;
#else
    i=config->Read(wxT("/BatchFrame/ShutdownCheck"), 0l);
#endif
    if (i != 0)
    {
        SelectEndTask(m_endChoice, Batch::SHUTDOWN);
        m_batch->atEnd = Batch::SHUTDOWN;
    };
    config->DeleteEntry(wxT("/BatchFrame/ShutdownCheck"));
    // now read current version
    i = config->Read(wxT("/BatchFrame/AtEnd"), 0l);
#if defined __WXMAC__ || defined __WXOSX_COCOA__
    // wxWidgets for MacOS does not support wxShutdown
    if (i == Batch::SHUTDOWN)
    {
        i = 0;
    };
#endif
    m_batch->atEnd = static_cast<Batch::EndTask>(i);
    SelectEndTask(m_endChoice, m_batch->atEnd);
    i=config->Read(wxT("/BatchFrame/OverwriteCheck"), 0l);
    XRCCTRL(*this,"cb_overwrite",wxCheckBox)->SetValue(i!=0);
    m_batch->overwrite=(i!=0);
    i=config->Read(wxT("/BatchFrame/VerboseCheck"), 0l);
    XRCCTRL(*this,"cb_verbose",wxCheckBox)->SetValue(i!=0);
    m_batch->verbose=(i!=0);
    i=config->Read(wxT("/BatchFrame/AutoRemoveCheck"), 0l);
    XRCCTRL(*this,"cb_autoremove",wxCheckBox)->SetValue(i!=0);
    m_batch->autoremove=(i!=0);
    i=config->Read(wxT("/BatchFrame/AutoStitchCheck"), 0l);
    XRCCTRL(*this,"cb_autostitch",wxCheckBox)->SetValue(i!=0);
    m_batch->autostitch=(i!=0);
    i=config->Read(wxT("/BatchFrame/SaveLog"), 0l);
    XRCCTRL(*this, "cb_savelog",wxCheckBox)->SetValue(i!=0);
    m_batch->saveLog=(i!=0);
};

bool BatchFrame::GetCheckParallel()
{
    return XRCCTRL(*this,"cb_parallel",wxCheckBox)->IsChecked();
};

Batch::EndTask BatchFrame::GetEndTask()
{
    return static_cast<Batch::EndTask>((size_t)m_endChoice->GetClientData(m_endChoice->GetSelection()));
};

bool BatchFrame::GetCheckOverwrite()
{
    return XRCCTRL(*this,"cb_overwrite",wxCheckBox)->IsChecked();
};

bool BatchFrame::GetCheckVerbose()
{
    return XRCCTRL(*this,"cb_verbose",wxCheckBox)->IsChecked();
};

bool BatchFrame::GetCheckAutoRemove()
{
    return XRCCTRL(*this,"cb_autoremove",wxCheckBox)->IsChecked();
};

bool BatchFrame::GetCheckAutoStitch()
{
    return XRCCTRL(*this,"cb_autostitch",wxCheckBox)->IsChecked();
};

bool BatchFrame::GetCheckSaveLog()
{
    return XRCCTRL(*this,"cb_savelog",wxCheckBox)->IsChecked();
};

void BatchFrame::OnCheckOverwrite(wxCommandEvent& event)
{
    if(event.IsChecked())
    {
        m_batch->overwrite = true;
        wxConfigBase::Get()->Write(wxT("/BatchFrame/OverwriteCheck"), 1l);
    }
    else
    {
        m_batch->overwrite = false;
        wxConfigBase::Get()->Write(wxT("/BatchFrame/OverwriteCheck"), 0l);
    }
}

void BatchFrame::OnCheckParallel(wxCommandEvent& event)
{
    if(event.IsChecked())
    {
        m_batch->parallel = true;
        wxConfigBase::Get()->Write(wxT("/BatchFrame/ParallelCheck"), 1l);
    }
    else
    {
        m_batch->parallel = false;
        wxConfigBase::Get()->Write(wxT("/BatchFrame/ParallelCheck"), 0l);
    }
}

void BatchFrame::OnChoiceEnd(wxCommandEvent& event)
{
    m_batch->atEnd = static_cast<Batch::EndTask>((size_t)m_endChoice->GetClientData(event.GetSelection()));
    wxConfigBase::Get()->Write(wxT("/BatchFrame/AtEnd"), static_cast<long>(m_batch->atEnd));
}

void BatchFrame::OnCheckVerbose(wxCommandEvent& event)
{
    if(event.IsChecked())
    {
        m_batch->verbose = true;
        wxConfigBase::Get()->Write(wxT("/BatchFrame/VerboseCheck"), 1l);
    }
    else
    {
        m_batch->verbose = false;
        wxConfigBase::Get()->Write(wxT("/BatchFrame/VerboseCheck"), 0l);
    }
    m_batch->ShowOutput(m_batch->verbose);
}

void BatchFrame::SetInternalVerbose(bool newVerbose)
{
    m_batch->verbose=newVerbose;
};

void BatchFrame::OnCheckAutoRemove(wxCommandEvent& event)
{
    m_batch->autoremove=event.IsChecked();
    wxConfigBase* config=wxConfigBase::Get();
    if(m_batch->autoremove)
    {
        config->Write(wxT("/BatchFrame/AutoRemoveCheck"), 1l);
    }
    else
    {
        config->Write(wxT("/BatchFrame/AutoRemoveCheck"), 0l);
    }
    config->Flush();
};

void BatchFrame::OnCheckAutoStitch(wxCommandEvent& event)
{
    m_batch->autostitch=event.IsChecked();
    wxConfigBase* config=wxConfigBase::Get();
    if(m_batch->autostitch)
    {
        config->Write(wxT("/BatchFrame/AutoStitchCheck"), 1l);
    }
    else
    {
        config->Write(wxT("/BatchFrame/AutoStitchCheck"), 0l);
    }
    config->Flush();
};

void BatchFrame::OnCheckSaveLog(wxCommandEvent& event)
{
    m_batch->saveLog=event.IsChecked();
    wxConfigBase* config=wxConfigBase::Get();
    if(m_batch->saveLog)
    {
        config->Write(wxT("/BatchFrame/SaveLog"), 1l);
    }
    else
    {
        config->Write(wxT("/BatchFrame/SaveLog"), 0l);
    }
    config->Flush();
};

void BatchFrame::OnClose(wxCloseEvent& event)
{
    //save windows position
    wxConfigBase* config=wxConfigBase::Get();
    if(IsMaximized())
    {
        config->Write(wxT("/BatchFrame/Max"), 1l);
        config->Write(wxT("/BatchFrame/Minimized"), 0l);
    }
    else
    {
        config->Write(wxT("/BatchFrame/Max"), 0l);
        if(m_tray!=NULL && !IsShown())
        {
            config->Write(wxT("/BatchFrame/Minimized"), 1l);
        }
        else
        {
            config->Write(wxT("/BatchFrame/Minimized"), 0l);
            config->Write(wxT("/BatchFrame/Width"), GetSize().GetWidth());
            config->Write(wxT("/BatchFrame/Height"), GetSize().GetHeight());
        };
    }
    config->Flush();
    m_closeThread = true;
    if (this->GetThread() && this->GetThread()->IsRunning())
    {
        this->GetThread()->Wait();
    };
#ifndef __WXMSW__
    delete m_help;
#endif
    if(m_tray!=NULL)
    {
        delete m_tray;
        m_tray = NULL;
    }
    this->Destroy();
}

void BatchFrame::PropagateDefaults()
{
    m_batch->parallel=GetCheckParallel();
    m_batch->atEnd = GetEndTask();
    m_batch->overwrite=GetCheckOverwrite();
    m_batch->verbose=GetCheckVerbose();
    m_batch->autoremove=GetCheckAutoRemove();
    m_batch->autostitch=GetCheckAutoStitch();
}

void BatchFrame::RunBatch()
{
    if(!IsRunning())
    {
        SetStatusInformation(_("Starting batch"),true);
    }
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
        {
            projListBox->Select(index+1);
        }
        else
        {
            projListBox->Select(index);
        }
    }
}


void BatchFrame::OnProcessTerminate(wxProcessEvent& event)
{
    if(m_batch->GetRunningCount()==1)
    {
        GetToolBar()->ToggleTool(XRCID("tool_pause"),false);
    }
    event.Skip();
}

void BatchFrame::RestoreSize()
{
    //get saved size
    wxConfigBase* config=wxConfigBase::Get();
    int width = config->Read(wxT("/BatchFrame/Width"), -1l);
    int height = config->Read(wxT("/BatchFrame/Height"), -1l);
    int max = config->Read(wxT("/BatchFrame/Max"), -1l);
    int min = config->Read(wxT("/BatchFrame/Minimized"), -1l);
    if((width != -1) && (height != -1))
    {
        SetSize(width,height);
    }
    else
    {
        SetSize(600,400);
    }

    if(max==1)
    {
        Maximize();
    };
    m_startedMinimized=(m_tray!=NULL) && (min==1);
}

void BatchFrame::OnBatchFailed(wxCommandEvent& event)
{
    if(m_batch->GetFailedProjectsCount()>0)
    {
        FailedProjectsDialog failedProjects_dlg(this,m_batch,m_xrcPrefix);
        failedProjects_dlg.ShowModal();
    };
};

void BatchFrame::OnBatchInformation(wxCommandEvent& e)
{
    SetStatusInformation(e.GetString(),true);
};

void BatchFrame::SetStatusInformation(wxString status,bool showBalloon)
{
    SetStatusText(status);
    if(m_tray!=NULL && showBalloon)
    {
#if defined __WXMSW__ && wxUSE_TASKBARICON_BALLOONS && wxCHECK_VERSION(2,9,0)
        m_tray->ShowBalloon(_("PTBatcherGUI"),status,5000,wxICON_INFORMATION);
#else
#ifndef __WXMAC__
        // the balloon does not work correctly on MacOS; it gets the focus
        // and can not be closed
        if(!IsShown())
        {
            TaskBarBalloon* balloon=new TaskBarBalloon(_("PTBatcherGUI"),status);
            balloon->showBalloon(5000);
        };
#endif
#endif
    };
};

void BatchFrame::OnMinimize(wxIconizeEvent& e)
{
    //hide/show window in taskbar when minimizing
    if(m_tray!=NULL)
    {
#if wxCHECK_VERSION(2,9,0)
        Show(!e.IsIconized());
        //switch off verbose output if PTBatcherGUI is in tray/taskbar
        if(e.IsIconized())
#else
        Show(!e.Iconized());
        //switch off verbose output if PTBatcherGUI is in tray/taskbar
        if(e.Iconized())
#endif
        {
            m_batch->verbose=false;
        }
        else
        {
            m_batch->verbose=XRCCTRL(*this,"cb_verbose",wxCheckBox)->IsChecked();
        };
        m_batch->ShowOutput(m_batch->verbose);
    }
    else //don't hide window if no tray icon
    {
        e.Skip();
    };
};

void BatchFrame::UpdateBatchVerboseStatus()
{
    m_batch->verbose=XRCCTRL(*this,"cb_verbose",wxCheckBox)->IsChecked();
    m_batch->ShowOutput(m_batch->verbose);
};

void BatchFrame::OnRefillListBox(wxCommandEvent& event)
{
    int index=projListBox->GetSelectedIndex();
    int id=-1;
    if(index!=-1)
    {
        id=projListBox->GetProjectId(index);
    };
    projListBox->DeleteAllItems();
    projListBox->Fill(m_batch);
    if(id!=-1)
    {
        index=projListBox->GetIndex(id);
        if(index!=-1)
        {
            projListBox->Select(index);
        };
    };
};
