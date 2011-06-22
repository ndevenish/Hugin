// -*- c-basic-offset: 4 -*-

/** @file PythonProgess.cpp
 *
 *  @brief implementation of Python progress window
 *
 *  @author T. Modes
 *
 */

/*
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

#include "base_wx/platform.h"
#include "hugin/config_defaults.h"
#include "hugin/PythonProgress.h"
#include "hugin_script_interface/hpi.h"
#include "hugin/huginApp.h"

// a random id, hope this doesn't break something..
enum {
    ID_PYTHON_CLOSE = wxID_HIGHEST + 147,
    ID_PYTHON_CLOSE_WHEN_DONE,
    ID_PYTHON_FINISHED,
};

#if wxCHECK_VERSION(2,9,0)
    wxDEFINE_EVENT(EVT_PYTHON_FINISHED,wxCommandEvent);
#else
#if _WINDOWS && defined Hugin_shared
    DEFINE_LOCAL_EVENT_TYPE(EVT_PYTHON_FINISHED)
#else
    DEFINE_EVENT_TYPE(EVT_PYTHON_FINISHED)
#endif
#endif

BEGIN_EVENT_TABLE(PythonProgress, wxDialog)
    EVT_BUTTON(ID_PYTHON_CLOSE,PythonProgress::OnCloseButton)
    EVT_CLOSE(PythonProgress::OnClose)
    EVT_COMMAND(wxID_ANY, EVT_PYTHON_FINISHED, PythonProgress::OnPythonFinished)
END_EVENT_TABLE()

PythonProgress::PythonProgress(wxWindow* parent, PT::Panorama &pano, wxString scriptfile) : 
    wxDialog(parent,wxID_ANY,_("Running Python script"),wxDefaultPosition, wxDefaultSize,wxCAPTION | wxSYSTEM_MENU | wxRESIZE_BORDER),
      m_pano(pano)
{
    m_success=false;
    m_scriptfile=scriptfile;
    wxBoxSizer* topSizer=new wxBoxSizer(wxVERTICAL);
    wxTextCtrl* textCtrl=new wxTextCtrl(this,wxID_ANY,wxEmptyString,wxDefaultPosition,wxSize(350,250),wxTE_MULTILINE | wxTE_READONLY);
    m_redirect=new wxStreamToTextRedirector(textCtrl);
    topSizer->Add(textCtrl,1,wxALL|wxEXPAND,6);
    wxBoxSizer* sizer2=new wxBoxSizer(wxHORIZONTAL);
    m_closeWhenDone=new wxCheckBox(this,ID_PYTHON_CLOSE_WHEN_DONE,_("Close window after running script"));
    long value=wxConfigBase::Get()->Read(wxT("/PythonProgressWindow/CloseWhenDone"),0l);
    m_closeWhenDone->SetValue(value!=0);
    sizer2->Add(m_closeWhenDone,1,wxALL|wxALIGN_CENTRE_VERTICAL,6);
    sizer2->AddStretchSpacer(1);
    m_closeButton=new wxButton(this,ID_PYTHON_CLOSE,_("Close"));
    m_closeButton->Enable(false);
    sizer2->Add(m_closeButton,0,wxALL|wxALIGN_CENTRE_VERTICAL,6);
    topSizer->Add(sizer2,0,wxEXPAND);
    SetSizerAndFit(topSizer);
    RestoreFramePosition(this,wxT("PythonProgressWindow"));
    Layout();
    CenterOnScreen();
};

PythonProgress::~PythonProgress()
{
    StoreFramePosition(this,wxT("PythonProgressWindow"));
    wxConfigBase* config=wxConfigBase::Get();
    if(m_closeWhenDone->GetValue())
    {
        config->Write(wxT("/PythonProgressWindow/CloseWhenDone"),1l);
    }
    else
    {
        config->Write(wxT("/PythonProgressWindow/CloseWhenDone"),0l);
    };
    config->Flush();
    delete m_redirect;
};

bool PythonProgress::RunScript()
{
#if wxCHECK_VERSION(2,9,0)
    if(CreateThread(wxTHREAD_JOINABLE)!=wxTHREAD_NO_ERROR)
#else
    if(wxThreadHelper::Create()!=wxTHREAD_NO_ERROR)
#endif
    {
        wxLogError(wxT("Could not create the worker thread!"));
        return false;
    }
    if (GetThread()->Run()!=wxTHREAD_NO_ERROR)
    {
        wxLogError(wxT("Could not run the worker thread!"));
        return false;
    }
    return true;
};

const HuginBase::PanoramaMemento PythonProgress::GetPanoramaMemento()
{
    return m_pano.getMemento();
};

void PythonProgress::OnCloseButton(wxCommandEvent & e)
{
    if(m_success)
    {
        EndModal(wxID_OK);
    }
    else
    {
        EndModal(wxID_CANCEL);
    };
};

void PythonProgress::OnClose(wxCloseEvent & e)
{
    if (e.CanVeto() && GetThread() && GetThread()->IsRunning())
    {
        e.Veto();
        return;
    }
    e.Skip();
};

void PythonProgress::OnPythonFinished(wxCommandEvent &e)
{
    m_success=(e.GetInt()==0);
    m_closeButton->Enable(true);
    Refresh();
    if(m_success)
    {
        if(m_closeWhenDone->GetValue())
        {
            wxCommandEvent dummy;
            OnCloseButton(dummy);
        };
    }
    else
    {
        wxMessageBox(wxString::Format(wxT("Script returned %d"),e.GetInt()),_("Result"), wxICON_INFORMATION);
    };
};

wxThread::ExitCode PythonProgress::Entry()
{
    wxCriticalSectionLocker lock(m_panoCS);
    int success = hpi::callhpi((const char *)m_scriptfile.mb_str(HUGIN_CONV_FILENAME), 1, "HuginBase::Panorama*", &m_pano);
    //notify window in thread-safe way
#if wxCHECK_VERSION(2,9,0)
    wxCommandEvent* cmdEvent=new wxCommandEvent(EVT_PYTHON_FINISHED,wxID_ANY);
    cmdEvent->SetInt(success);
    wxQueueEvent(this,cmdEvent);
#else
    wxCommandEvent cmdEvent(EVT_PYTHON_FINISHED,wxID_ANY);
    cmdEvent.SetInt(success);
    wxPostEvent(this,cmdEvent);
#endif
    return (wxThread::ExitCode)0;
};

PythonWithImagesProgress::PythonWithImagesProgress(wxWindow* parent, PT::Panorama &pano, HuginBase::UIntSet images, wxString scriptfile)
    : PythonProgress(parent,pano,scriptfile)
{
    m_images=images;
};

wxThread::ExitCode PythonWithImagesProgress::Entry()
{
    wxCriticalSectionLocker lock(m_panoCS);
    HuginBase::UIntSet images=m_images;
    int success = hpi::callhpi((const char *)m_scriptfile.mb_str(HUGIN_CONV_FILENAME), 2, 
        "HuginBase::Panorama*", &m_pano, "HuginBase::UIntSet*", &images);
    //notify window in thread-safe way
#if wxCHECK_VERSION(2,9,0)
    wxCommandEvent* cmdEvent=new wxCommandEvent(EVT_PYTHON_FINISHED,wxID_ANY);
    cmdEvent->SetInt(success);
    wxQueueEvent(this,cmdEvent);
#else
    wxCommandEvent cmdEvent(EVT_PYTHON_FINISHED,wxID_ANY);
    cmdEvent.SetInt(success);
    wxPostEvent(this,cmdEvent);
#endif
    return (wxThread::ExitCode)0;
};
