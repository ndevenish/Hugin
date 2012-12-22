// -*- c-basic-offset: 4 -*-
/** @file wxLensDB.cpp
 *
 *  @brief dialogs for loading and saving information from/to lensfun database
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "panoinc_WX.h"
#include "panoinc.h"
#include "wxLensDB.h"
#include "lensdb/LensDB.h"
#include "platform.h"
#include "base_wx/wxPlatform.h"
#include "panodata/ImageVariableTranslate.h"

using namespace std;

/** dialog for loading lens parameter from lensfun database */
class LoadLensDBDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    LoadLensDBDialog(wxWindow *parent);
    void SetCameraMaker(std::string camMaker);
    std::string GetCameraMaker() const;
    void SetCameraModel(std::string camModel);
    std::string GetCameraModel() const;
    void SetLensName(std::string lensname);
    std::string GetLensName() const;
    /** activates the lens in database for query information */
    bool ActivateSelectedLens();
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
    /** handler for searching for lenses in database */
    void OnSearch(wxCommandEvent & e);
    void OnCheckChanged(wxCommandEvent & e);

private:
    wxListBox *m_lenslist;
    wxCheckBox *m_fuzzySearch;
    wxCheckBox *m_loadDistortion;
    wxCheckBox *m_loadVignetting;
    double m_focal;
    double m_aperture;
    double m_distance;
    HuginBase::LensDB::LensDBList m_dblenslist;
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(LoadLensDBDialog,wxDialog)
    EVT_BUTTON(wxID_OK, LoadLensDBDialog::OnOk)
    EVT_BUTTON(XRCID("load_lens_search"), LoadLensDBDialog::OnSearch)
    EVT_LISTBOX(XRCID("load_lens_list"), LoadLensDBDialog::OnCheckChanged)
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
    config->Read(wxT("/LoadLensDialog/fuzzySearch"),&b,false);
    XRCCTRL(*this,"load_lens_fuzzy",wxCheckBox)->SetValue(b);
    config->Read(wxT("/LoadLensDialog/loadDistortion"),&b,true);
    m_loadDistortion=XRCCTRL(*this,"load_lens_distortion",wxCheckBox);
    m_loadDistortion->SetValue(b);
    config->Read(wxT("/LoadLensDialog/loadVignetting"),&b,true);
    m_loadVignetting=XRCCTRL(*this,"load_lens_vignetting",wxCheckBox);
    m_loadVignetting->SetValue(b);
    m_lenslist=XRCCTRL(*this,"load_lens_list",wxListBox);
};

void LoadLensDBDialog::SetCameraMaker(std::string camMaker)
{
    if(!camMaker.empty())
    {
        XRCCTRL(*this,"load_lens_camera_maker",wxTextCtrl)->SetValue(wxString(camMaker.c_str(), wxConvLocal));
    };
};

std::string LoadLensDBDialog::GetCameraMaker() const
{
    return std::string(XRCCTRL(*this,"load_lens_camera_maker",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

void LoadLensDBDialog::SetCameraModel(std::string camModel)
{
    if(!camModel.empty())
    {
        XRCCTRL(*this,"load_lens_camera_model",wxTextCtrl)->SetValue(wxString(camModel.c_str(), wxConvLocal));
    };
};

std::string LoadLensDBDialog::GetCameraModel() const
{
    return std::string(XRCCTRL(*this,"load_lens_camera_model",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

void LoadLensDBDialog::SetLensName(std::string lensname)
{
    if(!lensname.empty())
    {
        XRCCTRL(*this,"load_lens_name",wxTextCtrl)->SetValue(wxString(lensname.c_str(), wxConvLocal));
    };
    wxCommandEvent dummy;
    OnSearch(dummy);
};

std::string LoadLensDBDialog::GetLensName() const
{
    return std::string(XRCCTRL(*this,"load_lens_name",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

bool LoadLensDBDialog::ActivateSelectedLens()
{
    int index=m_lenslist->GetSelection();
    if(index!=wxNOT_FOUND)
    {
        HuginBase::LensDB::LensDB::GetSingleton().SetActiveLens(m_dblenslist.GetLens(index));
        return true;
    }
    else
    {
        return false;
    };
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
        if(!str2double(this,XRCCTRL(*this,"load_lens_aperture",wxTextCtrl)->GetValue(),m_aperture))
        {
            return;
        };
        if(!str2double(this,XRCCTRL(*this,"load_lens_distance",wxTextCtrl)->GetValue(),m_distance))
        {
            return;
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
    config->Write(wxT("/LoadLensDialog/fuzzySearch"),XRCCTRL(*this,"load_lens_fuzzy",wxCheckBox)->GetValue());
    config->Write(wxT("/LoadLensDialog/loadDistortion"),m_loadDistortion->GetValue());
    config->Write(wxT("/LoadLensDialog/loadVignetting"),m_loadVignetting->GetValue());
    config->Flush(); 
    e.Skip();
};

void LoadLensDBDialog::OnSearch(wxCommandEvent & e)
{
    XRCCTRL(*this,"load_lens_status",wxStaticText)->SetLabel(wxT(""));
    if(!GetLensName().empty() || (!GetCameraModel().empty()))
    {
        m_lenslist->Clear();
        bool fuzzy=XRCCTRL(*this,"load_lens_fuzzy",wxCheckBox)->GetValue();
        if(HuginBase::LensDB::LensDB::GetSingleton().FindLenses(GetCameraMaker(),GetCameraModel(),GetLensName(),m_dblenslist,fuzzy))
        {
            wxArrayString items;
            for(size_t i=0; i<m_dblenslist.GetLensCount(); i++)
            {
                wxString lensname=wxString(m_dblenslist.GetLensName(i).c_str(),wxConvLocal);
                //now translate a string part
                lensname.Replace(wxT("Focal length multiplier:"), _("Focal length multiplier:"), true);
                items.push_back(lensname);
            };
            m_lenslist->InsertItems(items,0);
            m_lenslist->DeselectAll();
            XRCCTRL(*this,"load_lens_status",wxStaticText)->SetLabel(wxString::Format(_("%d lenses found."),items.size()));
        }
        else
        {
            XRCCTRL(*this,"load_lens_status",wxStaticText)->SetLabel(_("No lens found."));
        };
    }
    else
    {
        wxBell();
    };
    XRCCTRL(*this,"wxID_OK",wxButton)->Enable(false);
};

void LoadLensDBDialog::OnCheckChanged(wxCommandEvent & e)
{
    int sel=m_lenslist->GetSelection();
    XRCCTRL(*this,"wxID_OK",wxButton)->Enable(sel!=wxNOT_FOUND && (m_loadDistortion->GetValue() || m_loadVignetting->GetValue()));
};

bool ApplyLensDBParameters(wxWindow * parent, PT::Panorama *pano, HuginBase::UIntSet images, PT::PanoCommand*& cmd)
{
    LoadLensDBDialog dlg(parent);
    const HuginBase::SrcPanoImage & img=pano->getImage(*images.begin());
    dlg.SetCameraMaker(img.getExifMake());
    dlg.SetCameraModel(img.getExifModel());
    dlg.SetLensName(img.getExifLens());
    dlg.SetFocalLength(img.getExifFocalLength());
    dlg.SetAperture(img.getExifAperture());
    dlg.SetSubjectDistance(img.getExifDistance());
    if(dlg.ShowModal()==wxID_OK)
    {
        HuginBase::LensDB::LensDB & lensDB=HuginBase::LensDB::LensDB::GetSingleton();
        if(dlg.ActivateSelectedLens())
        {
            double focal=dlg.GetFocalLength();
            std::vector<PT::PanoCommand*> cmds;
            HuginBase::BaseSrcPanoImage::Projection proj;
            if(lensDB.GetProjection(proj))
            {
                cmds.push_back(new PT::ChangeImageProjectionCmd(*pano,images,proj));
            };
            HuginBase::BaseSrcPanoImage::CropMode cropMode;
            hugin_utils::FDiff2D cropLeftTop;
            hugin_utils::FDiff2D cropRightBottom;
            if(lensDB.GetCrop(focal, cropMode, cropLeftTop, cropRightBottom))
            {
                cmds.push_back(new PT::ChangeImageCropModeCmd(*pano, images, cropMode));
                if(cropMode!=HuginBase::BaseSrcPanoImage::NO_CROP)
                {
                    vigra::Rect2D cropRect;
                    const HuginBase::SrcPanoImage & img=pano->getImage(*images.begin());
                    int width=img.getSize().width();
                    int height=img.getSize().height();
                    if(width>height)
                    {
                        cropRect=vigra::Rect2D(cropLeftTop.x*width,cropLeftTop.y*height,cropRightBottom.x*width,cropRightBottom.y*height);
                    }
                    else
                    {
                        cropRect=vigra::Rect2D((1.0-cropRightBottom.y)*width,cropLeftTop.x*height,(1.0-cropLeftTop.y)*width,cropRightBottom.x*height);
                    };
                    cmds.push_back(new PT::ChangeImageCropRectCmd(*pano, images, cropRect));
                };
            };
            //load lens distortion from database
            if(dlg.GetLoadDistortion())
            {
                double hfov;
                if(lensDB.GetFov(focal,hfov))
                {
                    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> linkedVariables;
                    linkedVariables.insert(HuginBase::ImageVariableGroup::IVE_HFOV);
                    cmds.push_back(new PT::ChangePartImagesLinkingCmd(*pano, images, linkedVariables,
                        true,HuginBase::StandardImageVariableGroups::getLensVariables()));
                    cmds.push_back(new PT::ChangeImageHFOVCmd(*pano, images, hfov));
                };
                std::vector<double> dist;
                if(lensDB.GetDistortion(focal,dist))
                {
                    if(dist.size()==3)
                    {
                        dist.push_back(1.0-dist[0]-dist[1]-dist[2]);
                        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> linkedVariables;
                        linkedVariables.insert(HuginBase::ImageVariableGroup::IVE_RadialDistortion);
                        cmds.push_back(new PT::ChangePartImagesLinkingCmd(*pano, images, linkedVariables,
                            true,HuginBase::StandardImageVariableGroups::getLensVariables()));
                        cmds.push_back(new PT::ChangeImageRadialDistortionCmd(*pano,images,dist));
                    };
                };
            };
            if(dlg.GetLoadVignetting())
            {
                std::vector<double> vig;
                if(lensDB.GetVignetting(focal,dlg.GetAperture(),dlg.GetSubjectDistance(),vig))
                {
                    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> linkedVariables;
                    linkedVariables.insert(HuginBase::ImageVariableGroup::IVE_RadialVigCorrCoeff);
                    cmds.push_back(new PT::ChangePartImagesLinkingCmd(*pano, images, linkedVariables,
                        true,HuginBase::StandardImageVariableGroups::getLensVariables()));
                    cmds.push_back(new PT::ChangeImageRadialVigCorrCoeffCmd(*pano,images,vig));
                };
            };
            cmd=new PT::CombinedPanoCommand(*pano, cmds);
            return true;
        };
    };
    return false;
};

/** dialog for saving lens parameter into lensfun database */
class SaveLensDBDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    SaveLensDBDialog(wxWindow *parent);
    void SetCameraMaker(std::string maker);
    std::string GetCameraMaker() const;
    void SetCameraModel(std::string model);
    std::string GetCameraModel() const;
    void SetLensName(std::string lensname);
    std::string GetLensName() const;
    std::string GetLensMaker() const;
    void SetLensMount(std::string mount);
    std::string GetLensMount() const;
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

void SaveLensDBDialog::SetLensMount(std::string mount)
{
    if(!mount.empty())
    {
        XRCCTRL(*this,"save_lens_mount",wxTextCtrl)->SetValue(wxString(mount.c_str(), wxConvLocal));
    };
};

std::string SaveLensDBDialog::GetLensMount() const
{
    return std::string(XRCCTRL(*this,"save_lens_mount",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
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
    if(GetLensName().empty() && GetCameraMaker().empty() && GetCameraModel().empty())
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
        if(!str2double(this,XRCCTRL(*this,"save_lens_aperture",wxTextCtrl)->GetValue(),m_aperture))
        {
            return;
        };
        if(!str2double(this,XRCCTRL(*this,"save_lens_distance",wxTextCtrl)->GetValue(),m_distance))
        {
            return;
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

bool ShowFileDialogWithWarning(wxFileDialog &dlg,wxString userDBPath)
{
    if (dlg.ShowModal()==wxID_OK)
    {
        wxFileName filename(dlg.GetPath());
        if(!filename.HasExt())
        {
            filename.SetExt(wxT("xml"));
        };
        if(filename.GetPath()!=userDBPath)
        {
            if(wxMessageBox(wxString::Format(_("You selected the folder \"%s\" to save your database file.\nThis is not the default folder. You won't be able to automatically load this information back into Hugin.\nThe default folder for the database files is \"%s\".\nDo you want to proceed anyway?"),filename.GetPath().c_str(),userDBPath.c_str()),
                _("Warning"),wxYES_NO,dlg.GetParent())==wxNO)
            {
                return ShowFileDialogWithWarning(dlg,userDBPath);
            };
        };
        return true;
    }
    else
    {
        return false;
    };
};

//build mount name from camera maker and camera model
std::string BuildMountName(std::string maker, std::string model)
{
    std::string result;
    for(size_t i=0;i<maker.length();i++)
    {
        if(isalpha(maker[i]))
        {
            result+=tolower(maker[i]);
        }
        else
        {
            break;
        };
    };
    for(size_t i=0;i<model.length();i++)
    {
        if(isalnum(model[i]))
        {
            result+=model[i];
        };
    };
    return result;
};

bool SaveLensParameters(wxWindow * parent, const HuginBase::SrcPanoImage& img, bool includeVignetting)
{
    HuginBase::LensDB::LensDB& lensDB=HuginBase::LensDB::LensDB::GetSingleton();
    wxString userDBPath=wxString(lensDB.GetUserDBPath().c_str(), wxConvLocal);
    if(!wxFileName::DirExists(userDBPath))
    {
        wxFileName::Mkdir(userDBPath, 511, wxPATH_MKDIR_FULL);
    };
    wxFileDialog dlg(parent,
                        _("Save lens into database file"),
                        userDBPath, wxT(""), 
                        _("Lensfun database files (*.xml)|*.xml"),
                        wxFD_SAVE, wxDefaultPosition);
    if(ShowFileDialogWithWarning(dlg,userDBPath))
    {
        SaveLensDBDialog lensDlg(parent);
        lensDlg.SetCameraMaker(img.getExifMake());
        lensDlg.SetCameraModel(img.getExifModel());
        lensDlg.SetLensName(img.getExifLens());
        lensDlg.SetFocalLength(img.getExifFocalLength());
        lensDlg.SetAperture(img.getExifAperture());
        lensDlg.SetSubjectDistance(img.getExifDistance());
        std::string mount;
        if(lensDB.GetCameraMount(img.getExifMake(), img.getExifModel(), mount))
        {
            lensDlg.SetLensMount(mount);
        };
        if(!includeVignetting)
        {
            lensDlg.DeactivateSaveVignetting();
        };
        if(lensDlg.ShowModal()==wxID_OK)
        {
            std::string filename=std::string(dlg.GetPath().mb_str(HUGIN_CONV_FILENAME));
            std::string lensname=lensDlg.GetLensName();
            if(lensname.empty())
            {
                //empty lensname, assuming it is a compact camera
                lensname="Standard";
            };
            mount=lensDlg.GetLensMount();
            if(!lensDlg.GetCameraMaker().empty() && !lensDlg.GetCameraModel().empty())
            {
                if(mount.empty())
                {
                    //checking, if camera is already in database
                    if(!lensDB.GetCameraMount(lensDlg.GetCameraMaker(), lensDlg.GetCameraModel(), mount))
                    {
                        //unknown camera, build mount name and save into database
                        mount=BuildMountName(lensDlg.GetCameraMaker(), lensDlg.GetCameraModel());
                        if(!lensDB.SaveCameraCrop(filename, lensDlg.GetCameraMaker(), lensDlg.GetCameraModel(), mount, img.getExifCropFactor()))
                        {
                            wxMessageBox(_("Could not save information into database file."),_("Error"),wxOK|wxICON_ERROR,parent);
                            return false;
                        };
                    };
                }
                else
                {
                    //mount given, check if camera is already in database
                    std::string s;
                    if(!lensDB.GetCameraMount(lensDlg.GetCameraMaker(), lensDlg.GetCameraModel(), s))
                    {
                        if(!lensDB.SaveCameraCrop(filename, lensDlg.GetCameraMaker(), lensDlg.GetCameraModel(), mount, img.getExifCropFactor()))
                        {
                            wxMessageBox(_("Could not save information into database file."),_("Error"),wxOK|wxICON_ERROR,parent);
                            return false;
                        };
                    };
                };
            };

            int e=lensDB.BeginSaveLens(filename, lensDlg.GetLensMaker(), lensname, mount, img.getProjection(), img.getExifCropFactor());
            if(e==0)
            {
                double focal=lensDlg.GetFocalLength();
                if(img.getCropMode()!=HuginBase::SrcPanoImage::NO_CROP)
                {
                    vigra::Rect2D cropRect=img.getCropRect();
                    hugin_utils::FDiff2D cropLeftTop;
                    hugin_utils::FDiff2D cropRightBottom;
                    int width=img.getSize().width();
                    int height=img.getSize().height();
                    if(width>height)
                    {
                        cropLeftTop=hugin_utils::FDiff2D((double)cropRect.left()/width,(double)cropRect.top()/height);
                        cropRightBottom=hugin_utils::FDiff2D((double)cropRect.right()/width,(double)cropRect.bottom()/height);
                    }
                    else
                    {
                        cropLeftTop=hugin_utils::FDiff2D((double)cropRect.top()/height,1.0-(double)cropRect.right()/width);
                        cropRightBottom=hugin_utils::FDiff2D((double)cropRect.bottom()/height,1.0-(double)cropRect.left()/width);
                    };
                    lensDB.SaveCrop(focal,img.getCropMode(),cropLeftTop,cropRightBottom);
                };
                if(lensDlg.GetSaveDistortion())
                {
                    lensDB.SaveHFOV(focal,img.getHFOV());
                    lensDB.SaveDistortion(focal,img.getRadialDistortion());
                };
                if(includeVignetting && lensDlg.GetSaveVignetting())
                {
                    lensDB.SaveVignetting(focal,lensDlg.GetAperture(),lensDlg.GetSubjectDistance(),img.getRadialVigCorrCoeff());
                };
                if(lensDB.EndSaveLens())
                {
                    lensDB.ReloadUserPart();
                    return true;
                }
                else
                {
                    wxMessageBox(_("Could not save information into database file."),_("Error"),wxOK|wxICON_ERROR,parent);
                };
            }
            else
            {
                if(e==1)
                {
                    wxMessageBox(wxString::Format(_("Could not initialize database."),_("Error"),wxOK|wxICON_ERROR,parent));
                }
                else
                {
                    wxMessageBox(wxString::Format(_("The current selected lens does not match with the information about this lens in the selected database file.\nCould not proceed.\n(Error code: %d)"),e),
                        _("Error"),wxOK|wxICON_ERROR,parent);
                };
                return false;
            };
        };
    }
    return false;
};

/** dialog for saving lens parameter into lensfun database */
class SaveCamDBDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    SaveCamDBDialog(wxWindow *parent);
    void SetCameraMaker(std::string maker);
    std::string GetCameraMaker() const;
    void SetCameraModel(std::string model);
    std::string GetCameraModel() const;
    void SetCameraMount(std::string mount);
    std::string GetCameraMount() const;

protected:
    /** Saves current state of all checkboxes when closing dialog with Ok */
    void OnOk(wxCommandEvent & e);

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(SaveCamDBDialog,wxDialog)
    EVT_BUTTON(wxID_OK, SaveCamDBDialog::OnOk)
END_EVENT_TABLE()

SaveCamDBDialog::SaveCamDBDialog(wxWindow *parent)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("save_cam_dlg"));

    //set parameters
    wxConfigBase * config = wxConfigBase::Get();
    //position
    int x = config->Read(wxT("/SaveCamDialog/positionX"),-1l);
    int y = config->Read(wxT("/SaveCamDialog/positionY"),-1l);
    if ( y >= 0 && x >= 0) 
    {
        this->Move(x, y);
    } 
    else 
    {
        this->Move(0, 44);
    };
};

void SaveCamDBDialog::SetCameraMaker(std::string maker)
{
    if(!maker.empty())
    {
        XRCCTRL(*this,"save_cam_maker",wxTextCtrl)->SetValue(wxString(maker.c_str(), wxConvLocal));
    };
};

std::string SaveCamDBDialog::GetCameraMaker() const
{
    return std::string(XRCCTRL(*this,"save_cam_maker",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

void SaveCamDBDialog::SetCameraModel(std::string model)
{
    if(!model.empty())
    {
        XRCCTRL(*this,"save_cam_model",wxTextCtrl)->SetValue(wxString(model.c_str(), wxConvLocal));
    };
};

std::string SaveCamDBDialog::GetCameraModel() const
{
    return std::string(XRCCTRL(*this,"save_cam_model",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

void SaveCamDBDialog::SetCameraMount(std::string mount)
{
    if(!mount.empty())
    {
        XRCCTRL(*this,"save_cam_mount",wxTextCtrl)->SetValue(wxString(mount.c_str(), wxConvLocal));
    };
};

std::string SaveCamDBDialog::GetCameraMount() const
{
    return std::string(XRCCTRL(*this,"save_cam_mount",wxTextCtrl)->GetValue().Trim().mb_str(wxConvLocal));
};

void SaveCamDBDialog::OnOk(wxCommandEvent & e)
{
    if(GetCameraMaker().empty())
    {
        wxMessageBox(_("The maker field contains only an empty string."),_("Warning"),wxOK|wxICON_ERROR,this);
        return;
    };
    if(GetCameraModel().empty())
    {
        wxMessageBox(_("The model field contains only an empty string."),_("Warning"),wxOK|wxICON_ERROR,this);
        return;
    };
    //store selected options
    wxConfigBase * config = wxConfigBase::Get();
    wxPoint ps = this->GetPosition();
    config->Write(wxT("/SaveCamDialog/positionX"), ps.x);
    config->Write(wxT("/SaveCamDialog/positionY"), ps.y);
    config->Flush(); 
    e.Skip();
};

bool SaveCameraCropFactor(wxWindow * parent, const HuginBase::SrcPanoImage& img)
{
    HuginBase::LensDB::LensDB& lensDB=HuginBase::LensDB::LensDB::GetSingleton();
    wxString userDBPath=wxString(lensDB.GetUserDBPath().c_str(), wxConvLocal);
    if(!wxFileName::DirExists(userDBPath))
    {
        wxFileName::Mkdir(userDBPath, 511, wxPATH_MKDIR_FULL);
    };
    wxFileDialog dlg(parent,
                        _("Save camera into database file"),
                        userDBPath, wxT(""), 
                        _("Lensfun database files (*.xml)|*.xml"),
                        wxFD_SAVE, wxDefaultPosition);
    if(ShowFileDialogWithWarning(dlg,userDBPath))
    {
        SaveCamDBDialog camDlg(parent);
        camDlg.SetCameraMaker(img.getExifMake());
        camDlg.SetCameraModel(img.getExifModel());
        std::string mount;
        if(lensDB.GetCameraMount(img.getExifMake(), img.getExifModel(), mount))
        {
            camDlg.SetCameraMount(mount);
        };
        if(camDlg.ShowModal()==wxID_OK)
        {
            if(lensDB.SaveCameraCrop(std::string(dlg.GetPath().mb_str(HUGIN_CONV_FILENAME)),
                camDlg.GetCameraMaker(),camDlg.GetCameraModel(),camDlg.GetCameraMount(),img.getExifCropFactor()))
            {
                return true;
            }
            else
            {
                wxMessageBox(_("Could not save information into database file."),_("Error"),wxOK|wxICON_ERROR,parent);
            };
        };
    };
    return false;
};

