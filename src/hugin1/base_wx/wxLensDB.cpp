// -*- c-basic-offset: 4 -*-
/** @file wxLensDB.cpp
 *
 *  @brief dialogs for loading and saving information from/to lens database
 *
 *  @author T. Modes
 *
 */

/*  This program is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "panoinc_WX.h"
#include "panoinc.h"
#include "wxLensDB.h"
#include "lensdb/LensDB.h"
#include "platform.h"
#include "base_wx/wxPlatform.h"
#include "panodata/ImageVariableTranslate.h"
#include "panodata/ImageVariableGroup.h"
#include "base_wx/PanoCommand.h"
#include <set>

/** dialog for loading lens parameter from lens database */
class LoadLensDBDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    explicit LoadLensDBDialog(wxWindow *parent);
    void SetLensName(std::string lensname);
    std::string GetLensName() const;
    void SetFocalLength(double focal);
    double GetFocalLength() const;
    void SetAperture(double aperture);
    double GetAperture() const;
    void SetSubjectDistance(double distance);
    double GetSubjectDistance() const;
    bool GetLoadDistortion() const;
    bool GetLoadVignetting() const;

protected:
    /** Saves current state of all checkboxes when closing dialog with Ok */
    void OnOk(wxCommandEvent & e);
    void OnCheckChanged(wxCommandEvent & e);

private:
    void FillLensList();
    wxChoice *m_lenslist;
    wxCheckBox *m_loadDistortion;
    wxCheckBox *m_loadVignetting;
    double m_focal;
    double m_aperture;
    double m_distance;
    HuginBase::LensDB::LensList m_lensNames;
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(LoadLensDBDialog,wxDialog)
    EVT_BUTTON(wxID_OK, LoadLensDBDialog::OnOk)
    EVT_CHOICE(XRCID("load_lens_lenschoice"), LoadLensDBDialog::OnCheckChanged)
    EVT_CHECKBOX(XRCID("load_lens_distortion"), LoadLensDBDialog::OnCheckChanged)
    EVT_CHECKBOX(XRCID("load_lens_vignetting"), LoadLensDBDialog::OnCheckChanged)
END_EVENT_TABLE()

LoadLensDBDialog::LoadLensDBDialog(wxWindow *parent)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("load_lens_dlg"));

    //set parameters
    wxConfigBase * config = wxConfigBase::Get();
    // get display size
    int dx,dy;
    wxDisplaySize(&dx,&dy);
    int w = config->Read(wxT("/LoadLensDialog/width"),-1l);
    int h = config->Read(wxT("/LoadLensDialog/height"),-1l);
    if (w>0 && w<=dx && h>0 && h<=dy)
    {
        SetClientSize(w,h);
    }
    else
    {
        Fit();
    }
    //position
    int x = config->Read(wxT("/LoadLensDialog/positionX"),-1l);
    int y = config->Read(wxT("/LoadLensDialog/positionY"),-1l);
    if ( y >= 0 && x >= 0) 
    {
        this->Move(x, y);
    } 
    else 
    {
        this->Move(0, 44);
    };
    bool b;
    config->Read(wxT("/LoadLensDialog/loadDistortion"), &b, true);
    m_loadDistortion = XRCCTRL(*this, "load_lens_distortion", wxCheckBox);
    m_loadDistortion->SetValue(b);
    config->Read(wxT("/LoadLensDialog/loadVignetting"), &b, true);
    m_loadVignetting = XRCCTRL(*this, "load_lens_vignetting", wxCheckBox);
    m_loadVignetting->SetValue(b);
    m_lenslist=XRCCTRL(*this,"load_lens_lenschoice", wxChoice);
    FillLensList();
};

void LoadLensDBDialog::FillLensList()
{
    if (HuginBase::LensDB::LensDB::GetSingleton().GetLensNames(true, true, false, m_lensNames))
    {
        wxArrayString lensnames;
        for (HuginBase::LensDB::LensList::const_iterator it = m_lensNames.begin(); it != m_lensNames.end(); ++it)
        {
            wxString s((*it).c_str(), wxConvLocal);
            wxString cam = s.AfterFirst(wxT('|'));
            if (!cam.empty())
            {
                s = wxString::Format(_("Camera %s (%s)"), cam.c_str(), s.BeforeFirst(wxT('|')).c_str());
            };
            lensnames.Add(s);
        };
        m_lenslist->Append(lensnames);
    };
};

void LoadLensDBDialog::SetLensName(std::string lensname)
{
    if (!lensname.empty() && !m_lensNames.empty())
    {
        HuginBase::LensDB::LensList::const_iterator it=std::find(m_lensNames.begin(), m_lensNames.end(), lensname);
        if (it != m_lensNames.end())
        {
            m_lenslist->SetSelection(it - m_lensNames.begin());
        };
    };
    wxCommandEvent dummy;
    OnCheckChanged(dummy);
};

std::string LoadLensDBDialog::GetLensName() const
{
    return m_lensNames[m_lenslist->GetSelection()];
};

void LoadLensDBDialog::SetFocalLength(double focal)
{
    m_focal=focal;
    XRCCTRL(*this,"load_lens_focallength",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_focal,1));
};

double LoadLensDBDialog::GetFocalLength() const
{
    return m_focal;
};

void LoadLensDBDialog::SetAperture(double aperture)
{
    m_aperture=aperture;
    XRCCTRL(*this,"load_lens_aperture",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_aperture,1));
};

double LoadLensDBDialog::GetAperture() const
{
    return m_aperture;
};

void LoadLensDBDialog::SetSubjectDistance(double distance)
{
    m_distance=distance;
    XRCCTRL(*this,"load_lens_distance",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_distance,0));
};

double LoadLensDBDialog::GetSubjectDistance() const
{
    return m_distance;
};

bool LoadLensDBDialog::GetLoadDistortion() const
{
    return m_loadDistortion->GetValue();
};

bool LoadLensDBDialog::GetLoadVignetting() const
{
    return m_loadVignetting->GetValue();
};

// utility functions
bool str2double(wxWindow* parent, wxString s, double & d)
{
    if (!hugin_utils::stringToDouble(std::string(s.mb_str(wxConvLocal)), d)) 
    {
        wxMessageBox(wxString::Format(_("The input \"%s\" is not a valid number."),s.c_str()),_("Warning"), wxOK | wxICON_ERROR, parent);
        return false;
    }
    return true;
}

void LoadLensDBDialog::OnOk(wxCommandEvent & e)
{
    if(!m_loadDistortion->GetValue() && !m_loadVignetting->GetValue())
    {
        return;
    };
    if(m_lenslist->GetSelection()==wxNOT_FOUND)
    {
        wxBell();
        return;
    };
    if(!str2double(this,XRCCTRL(*this,"load_lens_focallength",wxTextCtrl)->GetValue(),m_focal))
    {
        return;
    };
    if(m_loadVignetting->GetValue())
    {
        wxString val = XRCCTRL(*this, "load_lens_aperture", wxTextCtrl)->GetValue().Trim();
        if (val.empty())
        {
            m_aperture = 0;
        }
        else
        {
            if (!str2double(this, val, m_aperture))
            {
                return;
            };
        };
        val = XRCCTRL(*this, "load_lens_distance", wxTextCtrl)->GetValue().Trim();
        if (val.empty())
        {
            m_distance = 0;
        }
        else
        {
            if (!str2double(this, val, m_distance))
            {
                return;
            };
        };
    };
    //store selected options
    wxConfigBase * config = wxConfigBase::Get();
    wxSize sz = this->GetClientSize();
    config->Write(wxT("/LoadLensDialog/width"), sz.GetWidth());
    config->Write(wxT("/LoadLensDialog/height"), sz.GetHeight());
    wxPoint ps = this->GetPosition();
    config->Write(wxT("/LoadLensDialog/positionX"), ps.x);
    config->Write(wxT("/LoadLensDialog/positionY"), ps.y);
    config->Write(wxT("/LoadLensDialog/loadDistortion"),m_loadDistortion->GetValue());
    config->Write(wxT("/LoadLensDialog/loadVignetting"),m_loadVignetting->GetValue());
    config->Flush(); 
    e.Skip();
};

void LoadLensDBDialog::OnCheckChanged(wxCommandEvent & e)
{
    int sel=m_lenslist->GetSelection();
    XRCCTRL(*this,"wxID_OK",wxButton)->Enable(sel!=wxNOT_FOUND && (m_loadDistortion->GetValue() || m_loadVignetting->GetValue()));
};

bool ApplyLensDBParameters(wxWindow * parent, HuginBase::Panorama *pano, HuginBase::UIntSet images, PanoCommand::PanoCommand*& cmd)
{
    LoadLensDBDialog dlg(parent);
    const HuginBase::SrcPanoImage & img=pano->getImage(*images.begin());
    dlg.SetLensName(img.getDBLensName());
    dlg.SetFocalLength(img.getExifFocalLength());
    dlg.SetAperture(img.getExifAperture());
    dlg.SetSubjectDistance(img.getExifDistance());
    if(dlg.ShowModal()==wxID_OK)
    {
        HuginBase::LensDB::LensDB & lensDB=HuginBase::LensDB::LensDB::GetSingleton();
        const double focal=dlg.GetFocalLength();
        const std::string lensname = dlg.GetLensName();
        std::vector<PanoCommand::PanoCommand*> cmds;
        HuginBase::BaseSrcPanoImage::Projection proj;
        if(lensDB.GetProjection(lensname, proj))
        {
            cmds.push_back(new PanoCommand::ChangeImageProjectionCmd(*pano,images,proj));
        };
        vigra::Rect2D cropRect;
        if(lensDB.GetCrop(lensname, focal, img.getSize(), cropRect))
        {
            cmds.push_back(new PanoCommand::ChangeImageCropModeCmd(*pano, images, img.isCircularCrop() ? HuginBase::BaseSrcPanoImage::CROP_CIRCLE : HuginBase::BaseSrcPanoImage::CROP_RECTANGLE));
            cmds.push_back(new PanoCommand::ChangeImageCropRectCmd(*pano, images, cropRect));
        };
        //load lens distortion from database
        if(dlg.GetLoadDistortion())
        {
            double hfov;
            if (lensDB.GetFov(lensname, focal, hfov))
            {
                // calculate FOV for given image, take different aspect ratios into account
                const double newFocal = HuginBase::SrcPanoImage::calcFocalLength(img.getProjection(), hfov, img.getCropFactor(), vigra::Size2D(3000, 2000));
                const double newFov = HuginBase::SrcPanoImage::calcHFOV(img.getProjection(), newFocal, img.getCropFactor(), img.getSize());
                std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> linkedVariables;
                linkedVariables.insert(HuginBase::ImageVariableGroup::IVE_HFOV);
                cmds.push_back(new PanoCommand::ChangePartImagesLinkingCmd(*pano, images, linkedVariables,
                    true,HuginBase::StandardImageVariableGroups::getLensVariables()));
                cmds.push_back(new PanoCommand::ChangeImageHFOVCmd(*pano, images, newFov));
            };
            std::vector<double> dist;
            if(lensDB.GetDistortion(lensname, focal, dist))
            {
                if (dist.size() == 3)
                {
                    dist.push_back(1.0 - dist[0] - dist[1] - dist[2]);
                    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> linkedVariables;
                    linkedVariables.insert(HuginBase::ImageVariableGroup::IVE_RadialDistortion);
                    cmds.push_back(new PanoCommand::ChangePartImagesLinkingCmd(*pano, images, linkedVariables,
                        true, HuginBase::StandardImageVariableGroups::getLensVariables()));
                    cmds.push_back(new PanoCommand::ChangeImageRadialDistortionCmd(*pano, images, dist));
                };
            };
        };
        if(dlg.GetLoadVignetting())
        {
            std::vector<double> vig;
            if (lensDB.GetVignetting(lensname, focal, dlg.GetAperture(), dlg.GetSubjectDistance(), vig))
            {
                if (vig.size() == 4)
                {
                    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> linkedVariables;
                    linkedVariables.insert(HuginBase::ImageVariableGroup::IVE_RadialVigCorrCoeff);
                    cmds.push_back(new PanoCommand::ChangePartImagesLinkingCmd(*pano, images, linkedVariables,
                        true, HuginBase::StandardImageVariableGroups::getLensVariables()));
                    cmds.push_back(new PanoCommand::ChangeImageRadialVigCorrCoeffCmd(*pano, images, vig));
                };
            };
        };
        cmd=new PanoCommand::CombinedPanoCommand(*pano, cmds);
        return true;
    };
    return false;
};

/** dialog for saving lens parameter into lens database */
class SaveLensDBDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    explicit SaveLensDBDialog(wxWindow *parent);
    void SetCameraMaker(std::string maker);
    std::string GetCameraMaker() const;
    void SetCameraModel(std::string model);
    std::string GetCameraModel() const;
    void SetLensName(std::string lensname);
    std::string GetLensName() const;
    std::string GetLensMaker() const;
    void SetFocalLength(double focal);
    double GetFocalLength() const;
    void SetAperture(double aperture);
    double GetAperture() const;
    void SetSubjectDistance(double distance);
    double GetSubjectDistance() const;
    bool GetSaveDistortion() const;
    bool GetSaveVignetting() const;
    void DeactivateSaveVignetting();

protected:
    /** Saves current state of all checkboxes when closing dialog with Ok */
    void OnOk(wxCommandEvent & e);
    void OnCheckChanged(wxCommandEvent & e);

private:
    wxCheckBox *m_saveDistortion;
    wxCheckBox *m_saveVignetting;
    double m_focal;
    double m_aperture;
    double m_distance;
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(SaveLensDBDialog,wxDialog)
    EVT_BUTTON(wxID_OK, SaveLensDBDialog::OnOk)
    EVT_CHECKBOX(XRCID("save_lens_distortion"), SaveLensDBDialog::OnCheckChanged)
    EVT_CHECKBOX(XRCID("save_lens_vignetting"), SaveLensDBDialog::OnCheckChanged)
END_EVENT_TABLE()

SaveLensDBDialog::SaveLensDBDialog(wxWindow *parent)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("save_lens_dlg"));

    //set parameters
    wxConfigBase * config = wxConfigBase::Get();
    // get display size
    int dx,dy;
    wxDisplaySize(&dx,&dy);
    int w = config->Read(wxT("/SaveLensDialog/width"),-1l);
    int h = config->Read(wxT("/SaveLensDialog/height"),-1l);
    if (w>0 && w<=dx && h>0 && h<=dy)
    {
        SetClientSize(w,h);
    }
    else
    {
        Fit();
    }
    //position
    int x = config->Read(wxT("/SaveLensDialog/positionX"),-1l);
    int y = config->Read(wxT("/SaveLensDialog/positionY"),-1l);
    if ( y >= 0 && x >= 0) 
    {
        this->Move(x, y);
    } 
    else 
    {
        this->Move(0, 44);
    };
    bool b;
    config->Read(wxT("/SaveLensDialog/saveDistortion"),&b,true);
    m_saveDistortion=XRCCTRL(*this,"save_lens_distortion",wxCheckBox);
    m_saveDistortion->SetValue(b);
    config->Read(wxT("/SaveLensDialog/saveVignetting"),&b,true);
    m_saveVignetting=XRCCTRL(*this,"save_lens_vignetting",wxCheckBox);
    m_saveVignetting->SetValue(b);
};

void SaveLensDBDialog::SetCameraMaker(std::string maker)
{
    if(!maker.empty())
    {
        XRCCTRL(*this,"save_lens_camera_maker",wxTextCtrl)->SetValue(wxString(maker.c_str(), wxConvLocal));
    };
};

std::string SaveLensDBDialog::GetCameraMaker() const
{
    return std::string(XRCCTRL(*this,"save_lens_camera_maker",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

void SaveLensDBDialog::SetCameraModel(std::string model)
{
    if(!model.empty())
    {
        XRCCTRL(*this,"save_lens_camera_model",wxTextCtrl)->SetValue(wxString(model.c_str(), wxConvLocal));
    };
};

std::string SaveLensDBDialog::GetCameraModel() const
{
    return std::string(XRCCTRL(*this,"save_lens_camera_model",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

void SaveLensDBDialog::SetLensName(std::string lensname)
{
    if(!lensname.empty())
    {
        XRCCTRL(*this,"save_lens_name",wxTextCtrl)->SetValue(wxString(lensname.c_str(), wxConvLocal));
    };
};

std::string SaveLensDBDialog::GetLensName() const
{
    return std::string(XRCCTRL(*this,"save_lens_name",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

std::string SaveLensDBDialog::GetLensMaker() const
{
    return std::string(XRCCTRL(*this,"save_lens_maker",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

void SaveLensDBDialog::SetFocalLength(double focal)
{
    m_focal=focal;
    XRCCTRL(*this,"save_lens_focallength",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_focal,1));
};

double SaveLensDBDialog::GetFocalLength() const
{
    return m_focal;
};

void SaveLensDBDialog::SetAperture(double aperture)
{
    m_aperture=aperture;
    XRCCTRL(*this,"save_lens_aperture",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_aperture,1));
};

double SaveLensDBDialog::GetAperture() const
{
    return m_aperture;
};

void SaveLensDBDialog::SetSubjectDistance(double distance)
{
    m_distance=distance;
    XRCCTRL(*this,"save_lens_distance",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_distance,0));
};

double SaveLensDBDialog::GetSubjectDistance() const
{
    return m_distance;
};

bool SaveLensDBDialog::GetSaveDistortion() const
{
    return m_saveDistortion->GetValue();
};

bool SaveLensDBDialog::GetSaveVignetting() const
{
    return m_saveVignetting->GetValue();
};

void SaveLensDBDialog::DeactivateSaveVignetting()
{
    m_saveVignetting->SetValue(false);
    m_saveVignetting->Disable();
};

void SaveLensDBDialog::OnOk(wxCommandEvent & e)
{
    if(!m_saveDistortion->GetValue() && !m_saveVignetting->GetValue())
    {
        return;
    };
    if(GetLensName().empty() && (GetCameraMaker().empty() || GetCameraModel().empty()))
    {
        wxMessageBox(_("There is too little information for saving data into database. Please check your input!"),_("Warning"),wxOK|wxICON_ERROR,this);
        return;
    };
    if(!str2double(this,XRCCTRL(*this,"save_lens_focallength",wxTextCtrl)->GetValue(),m_focal))
    {
        return;
    };
    if(m_saveVignetting->GetValue())
    {
        wxString val = XRCCTRL(*this, "save_lens_aperture", wxTextCtrl)->GetValue().Trim();
        if (val.empty())
        {
            m_aperture = 0;
        }
        else
        {
            if (!str2double(this, val, m_aperture))
            {
                return;
            };
        };
        val = XRCCTRL(*this, "save_lens_distance", wxTextCtrl)->GetValue().Trim();
        if (val.empty())
        {
            m_distance = 0;
        }
        else
        {
            if (!str2double(this, val, m_distance))
            {
                return;
            };
        };
    };
    //store selected options
    wxConfigBase * config = wxConfigBase::Get();
    wxSize sz = this->GetClientSize();
    config->Write(wxT("/SaveLensDialog/width"), sz.GetWidth());
    config->Write(wxT("/SaveLensDialog/height"), sz.GetHeight());
    wxPoint ps = this->GetPosition();
    config->Write(wxT("/SaveLensDialog/positionX"), ps.x);
    config->Write(wxT("/SaveLensDialog/positionY"), ps.y);
    config->Write(wxT("/SaveLensDialog/saveDistortion"),m_saveDistortion->GetValue());
    if(m_saveVignetting->IsEnabled())
    {
        config->Write(wxT("/SaveLensDialog/saveVignetting"),m_saveVignetting->GetValue());
    };
    config->Flush(); 
    e.Skip();
};

void SaveLensDBDialog::OnCheckChanged(wxCommandEvent & e)
{
    XRCCTRL(*this,"wxID_OK",wxButton)->Enable(m_saveDistortion->GetValue() || m_saveVignetting->GetValue());
};

bool SaveLensParameters(wxWindow * parent, const HuginBase::SrcPanoImage& img, bool includeVignetting)
{
    HuginBase::LensDB::LensDB& lensDB=HuginBase::LensDB::LensDB::GetSingleton();
    // show dialog
    SaveLensDBDialog lensDlg(parent);
    lensDlg.SetCameraMaker(img.getExifMake());
    lensDlg.SetCameraModel(img.getExifModel());
    lensDlg.SetLensName(img.getDBLensName());
    lensDlg.SetFocalLength(img.getExifFocalLength());
    lensDlg.SetAperture(img.getExifAperture());
    lensDlg.SetSubjectDistance(img.getExifDistance());
    if (!includeVignetting)
    {
        lensDlg.DeactivateSaveVignetting();
    };
    if (lensDlg.ShowModal() != wxID_OK)
    {
        return false;
    };
    const std::string camMaker = lensDlg.GetCameraMaker();
    const std::string camModel = lensDlg.GetCameraModel();
    std::string lensname = lensDlg.GetLensName();
    const double focal = lensDlg.GetFocalLength();
    if (lensname.empty())
    {
        //empty lensname, assuming it is a compact camera
        lensname = camMaker + "|" + camModel;
    };
    // unknown crop factor, remember it
    if (img.getExifCropFactor() < 0.1 && !camMaker.empty() && !camModel.empty())
    {
        lensDB.SaveCameraCrop(camMaker, camModel, img.getCropFactor());
    };
    if (lensDlg.GetSaveDistortion())
    {
        const double newFocallength = HuginBase::SrcPanoImage::calcFocalLength(img.getProjection(), img.getHFOV(), img.getCropFactor(), img.getSize());
        const double newHFOV = HuginBase::SrcPanoImage::calcHFOV(img.getProjection(), newFocallength, img.getCropFactor(), vigra::Size2D(3000, 2000));
        // save information with higher weight
        if (!lensDB.SaveLensFov(lensname, lensDlg.GetFocalLength(), newHFOV, 75))
        {
            wxMessageBox(_("Could not save information into database."), _("Error"), wxOK | wxICON_ERROR, parent);
            return false;
        };
        if (!lensDB.SaveDistortion(lensname, focal, img.getRadialDistortion(), 75))
        {
            wxMessageBox(_("Could not save information into database."), _("Error"), wxOK | wxICON_ERROR, parent);
            return false;
        };
    };
    if(lensDlg.GetSaveVignetting())
    {
        if (!lensDB.SaveVignetting(lensname, focal, lensDlg.GetAperture(), lensDlg.GetSubjectDistance(), img.getRadialVigCorrCoeff(), 75))
        {
            wxMessageBox(_("Could not save information into database."), _("Error"), wxOK | wxICON_ERROR, parent);
            return false;
        };
    };
    return true;
};

