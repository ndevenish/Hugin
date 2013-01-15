// -*- c-basic-offset: 4 -*-

/** @file ResetDialog.cpp
 *
 *	@brief implementation of ResetDialog class
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "hugin/ResetDialog.h"
#include "base_wx/wxPlatform.h"
#include "panoinc.h"

#include "hugin/huginApp.h"

BEGIN_EVENT_TABLE(ResetDialog,wxDialog)
    EVT_BUTTON(wxID_OK, ResetDialog::OnOk)
    EVT_CHECKBOX(XRCID("reset_exposure"), ResetDialog::OnSelectExposure)
END_EVENT_TABLE()

ResetDialog::ResetDialog(wxWindow *parent)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("reset_dialog"));

#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    //set parameters
    wxConfigBase * cfg = wxConfigBase::Get();
    bool check;
    cfg->Read(wxT("/ResetDialog/ResetPosition"),&check,true);
    XRCCTRL(*this,"reset_pos",wxCheckBox)->SetValue(check);
    cfg->Read(wxT("/ResetDialog/ResetFOV"),&check,true);
    XRCCTRL(*this,"reset_fov",wxCheckBox)->SetValue(check);
    cfg->Read(wxT("/ResetDialog/ResetLens"),&check,true);
    XRCCTRL(*this,"reset_lens",wxCheckBox)->SetValue(check);
    cfg->Read(wxT("/ResetDialog/ResetExposure"),&check,true);
    XRCCTRL(*this,"reset_exposure",wxCheckBox)->SetValue(check);
    int exp_param;
    cfg->Read(wxT("/ResetDialog/ResetExposureParam"),&exp_param,0);
    XRCCTRL(*this,"combo_exposure",wxComboBox)->Select(exp_param);
    wxCommandEvent dummy;
    OnSelectExposure(dummy);
    cfg->Read(wxT("/ResetDialog/ResetColor"),&check,true);
    XRCCTRL(*this,"reset_color",wxCheckBox)->SetValue(check);
    cfg->Read(wxT("/ResetDialog/ResetVignetting"),&check,true);
    XRCCTRL(*this,"reset_vignetting",wxCheckBox)->SetValue(check);
    cfg->Read(wxT("/ResetDialog/ResetResponse"),&check,true);
    XRCCTRL(*this,"reset_response",wxCheckBox)->SetValue(check);
    //position
    int x = cfg->Read(wxT("/ResetDialog/positionX"),-1l);
    int y = cfg->Read(wxT("/ResetDialog/positionY"),-1l);
    if ( y >= 0 && x >= 0) 
    {
        this->Move(x, y);
    } 
    else 
    {
        this->Move(0, 44);
    };
};

void ResetDialog::LimitToGeometric()
{
    XRCCTRL(*this,"reset_exposure",wxCheckBox)->Show(false);
    XRCCTRL(*this,"combo_exposure",wxComboBox)->Show(false);
    XRCCTRL(*this,"reset_color",wxCheckBox)->Show(false);
    XRCCTRL(*this,"reset_vignetting",wxCheckBox)->Show(false);
    XRCCTRL(*this,"reset_response",wxCheckBox)->Show(false);
    GetSizer()->Fit(this);
};

void ResetDialog::LimitToPhotometric()
{
    XRCCTRL(*this,"reset_pos",wxCheckBox)->Show(false);
    XRCCTRL(*this,"reset_fov",wxCheckBox)->Show(false);
    XRCCTRL(*this,"reset_lens",wxCheckBox)->Show(false);
    GetSizer()->Fit(this);
};

void ResetDialog::OnOk(wxCommandEvent & e)
{
    wxConfigBase * cfg = wxConfigBase::Get();
    wxPoint ps = this->GetPosition();
    cfg->Write(wxT("/ResetDialog/positionX"), ps.x);
    cfg->Write(wxT("/ResetDialog/positionY"), ps.y);
    cfg->Write(wxT("/ResetDialog/ResetPosition"),GetResetPos());
    cfg->Write(wxT("/ResetDialog/ResetFOV"),GetResetFOV());
    cfg->Write(wxT("/ResetDialog/ResetLens"),GetResetLens());
    cfg->Write(wxT("/ResetDialog/ResetExposure"),GetResetExposure());
    int exp_param;
    exp_param=XRCCTRL(*this,"combo_exposure",wxComboBox)->GetSelection();
    cfg->Write(wxT("/ResetDialog/ResetExposureParam"),exp_param);
    cfg->Write(wxT("/ResetDialog/ResetColor"),GetResetColor());
    cfg->Write(wxT("/ResetDialog/ResetVignetting"),GetResetVignetting());
    cfg->Write(wxT("/ResetDialog/ResetResponse"),GetResetResponse());
    cfg->Flush();
    e.Skip();
};

void ResetDialog::OnSelectExposure(wxCommandEvent & e)
{
    if(XRCCTRL(*this, "reset_exposure", wxCheckBox)->GetValue())
        XRCCTRL(*this,"combo_exposure",wxComboBox)->Enable();
    else
        XRCCTRL(*this,"combo_exposure",wxComboBox)->Disable();
};

bool ResetDialog::GetResetPos()
{
    return XRCCTRL(*this, "reset_pos", wxCheckBox)->GetValue();
};

bool ResetDialog::GetResetFOV()
{
    return XRCCTRL(*this, "reset_fov", wxCheckBox)->GetValue();
};

bool ResetDialog::GetResetLens()
{
    return XRCCTRL(*this, "reset_lens", wxCheckBox)->GetValue();
};

bool ResetDialog::GetResetExposure()
{
    return XRCCTRL(*this, "reset_exposure", wxCheckBox)->GetValue();
};

bool ResetDialog::GetResetExposureToExif()
{
    if(!GetResetExposure())
        return false;
    return XRCCTRL(*this, "combo_exposure", wxComboBox)->GetSelection()==0;
};

bool ResetDialog::GetResetColor()
{
    return XRCCTRL(*this, "reset_color", wxCheckBox)->GetValue();
};

bool ResetDialog::GetResetVignetting()
{
    return XRCCTRL(*this, "reset_vignetting", wxCheckBox)->GetValue();
};

bool ResetDialog::GetResetResponse()
{
    return XRCCTRL(*this, "reset_response", wxCheckBox)->GetValue();
};
