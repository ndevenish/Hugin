// -*- c-basic-offset: 4 -*-

/** @file HDRMergeOptionDialog.cpp
 *
 *	@brief implementation of dialog for hdrmerge options
 *
 *  @author Thomas Modes
 *
 *  $Id$
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "hugin/HDRMergeOptionDialog.h"
#include "common/wxPlatform.h"
#include <hugin/config_defaults.h>
#include "hugin/huginApp.h"
#include <wx/cmdline.h>

BEGIN_EVENT_TABLE(HDRMergeOptionsDialog,wxDialog)
EVT_CHOICE(XRCID("hdrmerge_option_mode"),HDRMergeOptionsDialog::OnModeChanged)
EVT_BUTTON(wxID_OK, HDRMergeOptionsDialog::OnOk)
END_EVENT_TABLE()

HDRMergeOptionsDialog::HDRMergeOptionsDialog(wxWindow *parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("hdrmerge_options_dialog"));

#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    m_mode=XRCCTRL(*this,"hdrmerge_option_mode",wxChoice);
    m_panel_avg=XRCCTRL(*this,"hdrmerge_option_panel_avg",wxPanel);
    m_panel_avgslow=XRCCTRL(*this,"hdrmerge_option_panel_avgslow",wxPanel);
    m_panel_khan=XRCCTRL(*this,"hdrmerge_option_panel_khan",wxPanel);
    m_option_c=XRCCTRL(*this,"hdrmerge_option_c",wxCheckBox);
    m_khan_iter=XRCCTRL(*this,"hdrmerge_option_khan_iter",wxTextCtrl);
    m_khan_sigma=XRCCTRL(*this,"hdrmerge_option_khan_sigma",wxTextCtrl);
    m_option_khan_af=XRCCTRL(*this,"hdrmerge_option_khan_af",wxCheckBox);
    m_option_khan_ag=XRCCTRL(*this,"hdrmerge_option_khan_ag",wxCheckBox);
    m_option_khan_am=XRCCTRL(*this,"hdrmerge_option_khan_am",wxCheckBox);
    this->CenterOnParent();
};

void HDRMergeOptionsDialog::SetCommandLineArgument(wxString cmd)
{
    m_cmd=cmd;
    if (m_cmd.IsEmpty())
        m_cmd=wxT(HUGIN_HDRMERGE_ARGS);
    m_cmd.LowerCase();
	// parse arguments
    static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
        { wxCMD_LINE_OPTION, wxT("m"), NULL, NULL, wxCMD_LINE_VAL_STRING },     
        { wxCMD_LINE_SWITCH, wxT("c"), NULL, NULL},
        { wxCMD_LINE_OPTION, wxT("i"), NULL, NULL, wxCMD_LINE_VAL_NUMBER },
        { wxCMD_LINE_OPTION, wxT("s"), NULL, NULL, wxCMD_LINE_VAL_NUMBER },
        { wxCMD_LINE_OPTION, wxT("a"), NULL, NULL, wxCMD_LINE_VAL_STRING },
        { wxCMD_LINE_NONE }
    };
    wxCmdLineParser parser;
    parser.SetDesc(cmdLineDesc);
    parser.SetCmdLine(m_cmd);
    parser.Parse(false);
    wxString param;
    if(parser.Found(wxT("m"),&param))
    {
        if(param.CmpNoCase(wxT("avg_slow"))==0)
            m_mode->SetSelection(1);
        else
        {
            if(param.CmpNoCase(wxT("khan"))==0)
                m_mode->SetSelection(2);
            else
                m_mode->SetSelection(0);
        };
    }
    else
        m_mode->SetSelection(0);
    m_option_c->SetValue(parser.Found(wxT("c")));
    long i;
    if(parser.Found(wxT("i"),&i))
        m_khan_iter->SetValue(wxString::Format(wxT("%d"),i));
    else
        m_khan_iter->SetValue(wxT("1"));
    if(parser.Found(wxT("s"),&i))
        m_khan_sigma->SetValue(wxString::Format(wxT("%d"),i));
    else
        m_khan_sigma->SetValue(wxT("30"));
    if(parser.Found(wxT("a"),&param))
    {
        m_option_khan_af->SetValue(param.Contains(wxT("f")));
        m_option_khan_ag->SetValue(param.Contains(wxT("g")));
        m_option_khan_am->SetValue(param.Contains(wxT("m")));
    }
    wxCommandEvent dummy;
    OnModeChanged(dummy);
};

bool HDRMergeOptionsDialog::BuildCommandLineArgument()
{
    int selection=m_mode->GetSelection();
    m_cmd.Clear();
    bool correct_input=true;
    long i;
    wxString errorstring(_("Invalid input\n"));
    switch(selection)
    {
        case 0:
            m_cmd.Append(wxT("-m avg"));
            if(m_option_c->IsChecked())
                m_cmd.Append(wxT(" -c"));
            break;
        case 1:
            m_cmd.Append(wxT("-m avg_slow"));
            break;
        case 2:
            m_cmd.Append(wxT("-m khan"));
            if(m_khan_iter->GetValue().ToLong(&i))
            {
                m_cmd.Append(wxString::Format(wxT(" -i %s"),m_khan_iter->GetValue()));
            }
            else
            {
                correct_input=false;
                errorstring.Append(wxString::Format(_("Input \"%s\" for %s is not a valid number\n"),
                    m_khan_iter->GetValue(),_("Iteration")));
            };
            if(m_khan_sigma->GetValue().ToLong(&i))
            {
                m_cmd.Append(wxString::Format(wxT(" -s %s"),m_khan_sigma->GetValue()));
            }
            else
            {
                correct_input=false;
                errorstring.Append(wxString::Format(_("Input \"%s\" for %s is not a valid number\n"),
                    m_khan_iter->GetValue(),_("Sigma")));
            };
            if(m_option_khan_af->IsChecked() || m_option_khan_ag->IsChecked() || 
                m_option_khan_am->IsChecked())
            {
                m_cmd.Append(wxT(" -a "));
                if(m_option_khan_af->IsChecked())
                    m_cmd.Append(wxT("f"));
                if(m_option_khan_ag->IsChecked())
                    m_cmd.Append(wxT("g"));
                if(m_option_khan_am->IsChecked())
                    m_cmd.Append(wxT("m"));
            }
            break;
    };
    if(!correct_input)
        wxMessageBox(errorstring,_("Wrong input"),wxOK | wxICON_INFORMATION);
    return correct_input;
};

void HDRMergeOptionsDialog::OnModeChanged(wxCommandEvent & e)
{
    int selection=m_mode->GetSelection();
    m_panel_avg->Show(selection==0);
    m_panel_avgslow->Show(selection==1);
    m_panel_khan->Show(selection==2);
    Fit();
};

void HDRMergeOptionsDialog::OnOk(wxCommandEvent & e)
{
    if(BuildCommandLineArgument())
        this->EndModal(wxOK);
};

