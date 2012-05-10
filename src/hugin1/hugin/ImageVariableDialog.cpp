// -*- c-basic-offset: 4 -*-

/** @file ImageVariableDialog.cpp
 *
 *  @brief implementation of dialog to edit image variables
 *
 *  @author T. Modes
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

#include "hugin/ImageVariableDialog.h"
#include "base_wx/wxPlatform.h"
#include "panoinc.h"
#include <map>
#include <string>

#include "hugin/huginApp.h"
#include "CommandHistory.h"

using namespace HuginBase;

BEGIN_EVENT_TABLE(ImageVariableDialog,wxDialog)
    EVT_BUTTON(wxID_OK, ImageVariableDialog::OnOk)
END_EVENT_TABLE()

ImageVariableDialog::ImageVariableDialog(wxWindow *parent, PT::Panorama* pano, HuginBase::UIntSet imgs)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("image_variables_dialog"));

#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    wxConfigBase * cfg = wxConfigBase::Get();
    //position
    int x = cfg->Read(wxT("/ImageVariablesDialog/positionX"),-1l);
    int y = cfg->Read(wxT("/ImageVariablesDialog/positionY"),-1l);
    if ( y >= 0 && x >= 0) 
    {
        this->Move(x, y);
    } 
    else 
    {
        this->Move(0, 44);
    };
    m_pano=pano;
    m_images=imgs;
    InitValues();
};

wxTextCtrl* GetImageVariableControl(const wxWindow* parent, const char* varname)
{
    return wxStaticCast(
                parent->FindWindow(
                    wxXmlResource::GetXRCID(
                        wxString(wxT("image_variable_")).append(wxString(varname, wxConvLocal)).c_str()
                    )
                ), wxTextCtrl
           );
};

void ImageVariableDialog::InitValues()
{
    if(m_images.size()==0)
    {
        return;
    };
    BaseSrcPanoImage::ResponseType responseType= m_pano->getImage(*m_images.begin()).getResponseType();
    bool identical=true;

    for(UIntSet::const_iterator it=m_images.begin();it!=m_images.end() && identical;it++)
    {
        identical=(responseType==m_pano->getImage(*it).getResponseType());
    };
    if(identical)
    {
        XRCCTRL(*this, "image_variable_responseType", wxChoice)->SetSelection(responseType);
    };

    int degDigits = wxConfigBase::Get()->Read(wxT("/General/DegreeFractionalDigitsEdit"),3);
    int pixelDigits = wxConfigBase::Get()->Read(wxT("/General/PixelFractionalDigitsEdit"),2);
    int distDigitsEdit = wxConfigBase::Get()->Read(wxT("/General/DistortionFractionalDigitsEdit"),5);
    
    VariableMapVector imgVarVector=m_pano->getVariables();

    for (const char** varname = m_varNames; *varname != 0; ++varname)
    {
        // update parameters
        int ndigits = distDigitsEdit;
        if (strcmp(*varname, "y") == 0 || strcmp(*varname, "p") == 0 ||
            strcmp(*varname, "r") == 0 || strcmp(*varname, "TrX") == 0 ||
            strcmp(*varname, "TrY") == 0 || strcmp(*varname, "TrZ") == 0 )
        {
            ndigits=degDigits;
        };
        if (strcmp(*varname, "v") == 0 || strcmp(*varname, "d") == 0 ||
            strcmp(*varname, "e") == 0 )
        {
            ndigits = pixelDigits;
        }
        double val=const_map_get(imgVarVector[*m_images.begin()],*varname).getValue();
        bool identical=true;
        for(UIntSet::const_iterator it=m_images.begin();it!=m_images.end() && identical;it++)
        {
            identical=(val==const_map_get(imgVarVector[*it],*varname).getValue());
        };
        if(identical)
        {
            GetImageVariableControl(this, *varname)->SetValue(hugin_utils::doubleTowxString(val,ndigits));
        };
    };
};

void ImageVariableDialog::SetGuiLevel(GuiLevel newLevel)
{
    // translation
    XRCCTRL(*this, "image_variable_text_translation", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_translation_x", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_translation_y", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_translation_z", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_TrX", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_TrY", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_TrZ", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
    // shear
    XRCCTRL(*this, "image_variable_text_shear", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_shear_g", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_shear_t", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_g", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_t", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
};

bool ImageVariableDialog::ApplyNewVariables()
{
    std::vector<PT::PanoCommand*> commands;
    VariableMap varMap;
    for (const char** varname = m_varNames; *varname != 0; ++varname)
    {
        wxString s=GetImageVariableControl(this, *varname)->GetValue();
        if(!s.empty())
        {
            double val;
            if(str2double(s,val))
            {
                varMap.insert(std::make_pair(std::string(*varname), Variable(std::string(*varname), val)));
            }
            else
            {
                wxLogError(_("Value must be numeric."));
                return false;
            };
        };
    };
    int sel=XRCCTRL(*this, "image_variable_responseType", wxChoice)->GetSelection();
    if(sel!=wxNOT_FOUND)
    {
        std::vector<SrcPanoImage> SrcImgs;
        for (UIntSet::const_iterator it=m_images.begin(); it!=m_images.end(); it++)
        {
            HuginBase::SrcPanoImage img=m_pano->getSrcImage(*it);
            img.setResponseType((SrcPanoImage::ResponseType)sel);
            SrcImgs.push_back(img);
        }
        commands.push_back(new PT::UpdateSrcImagesCmd( *m_pano, m_images, SrcImgs ));
    }
    if(varMap.size()>0)
    {
        for(UIntSet::const_iterator it=m_images.begin();it!=m_images.end();it++)
        {
            commands.push_back(
                new PT::UpdateImageVariablesCmd(*m_pano,*it,varMap)
                );
        };
    };
    if(commands.size()>0)
    {
        GlobalCmdHist::getInstance().addCommand(new PT::CombinedPanoCommand(*m_pano, commands));
        return true;
    }
    else
    {
        return false;
    };
};

void ImageVariableDialog::OnOk(wxCommandEvent & e)
{
    if(ApplyNewVariables())
    {
        wxConfigBase * cfg = wxConfigBase::Get();
        wxPoint ps = this->GetPosition();
        cfg->Write(wxT("/ImageVariablesDialog/positionX"), ps.x);
        cfg->Write(wxT("/ImageVariablesDialog/positionY"), ps.y);
        cfg->Flush();
        e.Skip();
    };
};

const char *ImageVariableDialog::m_varNames[] = { "y", "p", "r", "TrX", "TrY", "TrZ", 
                                  "v", "a", "b", "c", "d", "e", "g", "t",
                                  "Eev", "Er", "Eb", 
                                  "Vb", "Vc", "Vd", "Vx", "Vy",
                                  "Ra", "Rb", "Rc", "Rd", "Re", 0};
