// -*- c-basic-offset: 4 -*-

/** @file FailedProjectsDialog.cpp
 *
 *	@brief implementation of failed projects dialog
 *
 *  @author Thomas Modes
 *
 */

/*  This is free software; you can redistribute it and/or
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

#include "FailedProjectsDialog.h"
#include "base_wx/wxPlatform.h"
#include "panoinc.h"
#include "Batch.h"

BEGIN_EVENT_TABLE(FailedProjectsDialog,wxDialog)
    EVT_LISTBOX(XRCID("failed_list"),FailedProjectsDialog::OnSelectProject)
END_EVENT_TABLE()

FailedProjectsDialog::FailedProjectsDialog(wxWindow* parent,Batch* batch,wxString xrcPrefix)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this,parent,wxT("failed_project_dialog"));

#ifdef __WXMSW__
    wxIcon myIcon(xrcPrefix+ wxT("data/ptbatcher.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(xrcPrefix + wxT("data/ptbatcher.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    m_batch=batch;

    m_list=XRCCTRL(*this,"failed_list",wxListBox);
    m_log=XRCCTRL(*this,"failed_log",wxTextCtrl);

    //fill list
    for(unsigned int i=0; i<batch->GetFailedProjectsCount(); i++)
    {
        m_list->AppendString(batch->GetFailedProjectName(i));
    };
    if(m_list->GetCount()>0)
    {
        m_list->SetSelection(0);
        wxCommandEvent dummy;
        OnSelectProject(dummy);
    };

    //set parameters
    wxConfigBase* config = wxConfigBase::Get();
    // restore position and size
    int dx,dy;
    wxDisplaySize(&dx,&dy);
    bool maximized = config->Read(wxT("/FailedProjectsDialog/maximized"), 0l) != 0;
    if (maximized)
    {
        this->Maximize();
    }
    else
    {
        //size
        int w = config->Read(wxT("/FailedProjectsDialog/width"),-1l);
        int h = config->Read(wxT("/FailedProjectsDialog/height"),-1l);
        if (w > 0 && w <= dx)
        {
            this->SetClientSize(w,h);
        }
        else
        {
            this->Fit();
        }
        //splitter position
        int splitter_pos=config->Read(wxT("/FailedProjectsDialog/splitterPos"),-1l);
        wxSplitterWindow* splitWindow=XRCCTRL(*this,"failed_splitter",wxSplitterWindow);
        if(splitter_pos>0 && splitter_pos<splitWindow->GetSize().GetWidth())
        {
            splitWindow->SetSashPosition(splitter_pos);
        };
        //position
        int x = config->Read(wxT("/FailedProjectsDialog/positionX"),-1l);
        int y = config->Read(wxT("/FailedProjectsDialog/positionY"),-1l);
        if ( y >= 0 && x >= 0 && x < dx && y < dy)
        {
            this->Move(x, y);
        }
        else
        {
            this->Move(0, 44);
        }
    }
};

FailedProjectsDialog::~FailedProjectsDialog()
{
    wxConfigBase* config=wxConfigBase::Get();
    if(!this->IsMaximized())
    {
        wxSize sz = this->GetClientSize();
        config->Write(wxT("/FailedProjectsDialog/width"), sz.GetWidth());
        config->Write(wxT("/FailedProjectsDialog/height"), sz.GetHeight());
        wxPoint ps = this->GetPosition();
        config->Write(wxT("/FailedProjectsDialog/positionX"), ps.x);
        config->Write(wxT("/FailedProjectsDialog/positionY"), ps.y);
        config->Write(wxT("/FailedProjectsDialog/maximized"), 0);
    }
    else
    {
        config->Write(wxT("/FailedProjectsDialog/maximized"), 1l);
    };
    config->Write(wxT("/FailedProjectsDialog/splitterPos"), XRCCTRL(*this,"failed_splitter",wxSplitterWindow)->GetSashPosition());
};

void FailedProjectsDialog::OnSelectProject(wxCommandEvent& e)
{
    int sel=m_list->GetSelection();
    m_log->Clear();
    if(sel!=wxNOT_FOUND)
    {
        wxString logfile=m_batch->GetFailedProjectLog(sel);
        if(!logfile.IsEmpty())
        {
            if(wxFileName::FileExists(logfile))
            {
                m_log->LoadFile(logfile);
            }
        };
    };
};
