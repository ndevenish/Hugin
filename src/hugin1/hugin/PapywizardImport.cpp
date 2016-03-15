/**
* @file PapywizardImport.cpp
*
* @brief implementation of read settings from papywizard xml file
*
* @author T. Modes
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

#include "PapywizardImport.h"

#include "panoinc_WX.h"
#include "hugin_utils/utils.h"
#include <wx/xml/xml.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include "hugin/huginApp.h"
#include "base_wx/CommandHistory.h"
#include "base_wx/PanoCommand.h"
#include "base_wx/platform.h"
#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/wxPanoCommand.h"
#include "icpfind/AutoCtrlPointCreator.h"
#include "hugin/config_defaults.h"

namespace Papywizard
{

/** class which holds all read settings from a Papywizard xml file */
class PapywizardSettings
{
public:
    // read from header
    double focallength;
    double cropfactor;
    HuginBase::SrcPanoImage::Projection projection;
    // read from shoot section
    struct PapywizardImage
    {
        size_t id;
        size_t bracket;
        double yaw, pitch, roll;
    };

    std::vector<PapywizardImage> images;
    /** constructor, initialize some values */
    PapywizardSettings::PapywizardSettings()
    {
        focallength = 0;
        cropfactor = 1;
        projection = HuginBase::SrcPanoImage::RECTILINEAR;
    };

    bool HasBracketImages() const
    {
        for (size_t i = 0; i < images.size(); ++i)
        {
            if (images[i].bracket > 1)
            {
                return true;
            };
        };
        return false;
    };
};

/** parse header of papywizard file */
bool ParseHeader(wxXmlNode* root, PapywizardSettings& images)
{
    wxXmlNode* child = root->GetChildren();
    while (child)
    {
        // section <camera>
        if (child->GetName().CmpNoCase(wxT("camera")) == 0)
        {
            wxXmlNode* camChild = child->GetChildren();
            while (camChild)
            {
                // crop factor is saved as attribute in <sensor>
                if (camChild->GetName().CmpNoCase(wxT("sensor")) == 0)
                {
                    wxString number;
                    if (camChild->GetAttribute(wxT("coef"), &number))
                    {
                        if (!hugin_utils::stringToDouble(number, images.cropfactor))
                        {
                            return false;
                        };
                    };
                }
                camChild = camChild->GetNext();
            }
        }
        // section <lens>
        if (child->GetName().CmpNoCase(wxT("lens")) == 0)
        {
            // projection as type attribute
            wxString projection = child->GetAttribute(wxT("type"), wxEmptyString).Trim().Trim(false);
            if (!projection.empty())
            {
                if (projection.CmpNoCase(wxT("rectilinear")) == 0)
                {
                    images.projection = HuginBase::SrcPanoImage::RECTILINEAR;
                }
                else
                {
                    if (projection.CmpNoCase(wxT("fisheye")) == 0)
                    {
                        images.projection = HuginBase::SrcPanoImage::CIRCULAR_FISHEYE;
                    }
                    else
                    {
                        return false;
                    };
                };
            };
            // focal length as own element
            wxXmlNode* lensChild = child->GetChildren();
            while (lensChild)
            {
                if (lensChild->GetName().CmpNoCase(wxT("focal")) == 0)
                {
                    wxString focallength = lensChild->GetNodeContent().Trim().Trim(false);
                    if(!hugin_utils::stringToDouble(focallength, images.focallength))
                    {
                        return false;
                    };
                }
                lensChild = lensChild->GetNext();
            }
        };
        child = child->GetNext();
    };
    return true;
};

/** parse shoot section of papywizard file */
bool ParseShoot(wxXmlNode* root, PapywizardSettings& images)
{
    wxXmlNode* child = root->GetChildren();
    size_t id = 1;
    while (child)
    {
        if (child->GetName().CmpNoCase(wxT("pict")) == 0)
        {
            PapywizardSettings::PapywizardImage image;
            long longVal;
            wxString s;
            // check id
            if (!child->GetAttribute(wxT("id"), &s))
            {
                return false;
            };
            if (!s.ToLong(&longVal))
            {
                return false;
            };
            if (longVal < id)
            {
                return false;
            };
            image.id = longVal;
            ++id;
            // read bracket attribute
            if(!child->GetAttribute(wxT("bracket"), &s))
            {
                return false;
            };
            if (!s.ToLong(&longVal))
            {
                return false;
            };
            image.bracket = longVal;
            // now parse all position entries
            wxXmlNode* posChild = child->GetChildren();
            while (posChild)
            {
                if (posChild->GetName().CmpNoCase(wxT("position")) == 0)
                {
                    if (!posChild->GetAttribute(wxT("yaw"), &s))
                    {
                        return false;
                    };
                    if (!hugin_utils::stringToDouble(s, image.yaw))
                    {
                        return false;
                    };
                    if (!posChild->GetAttribute(wxT("pitch"), &s))
                    {
                        return false;
                    }
                    if (!hugin_utils::stringToDouble(s, image.pitch))
                    {
                        return false;
                    };
                    if (!posChild->GetAttribute(wxT("roll"), &s))
                    {
                        return false;
                    };
                    if (!hugin_utils::stringToDouble(s, image.roll))
                    {
                        return false;
                    };
                    images.images.push_back(image);
                    // we are ignoring all further entries
                    break;
                };
                posChild = posChild->GetNext();
            };
        };
        child = child->GetNext();
    };
    return true;
};

bool ParsePapywizardFile(const wxString& filename, PapywizardSettings& images)
{
    wxXmlDocument xmlFile;
    if (!xmlFile.Load(filename))
    {
        return false;
    }
    if (xmlFile.GetRoot()->GetName().CmpNoCase(wxT("papywizard")) != 0)
    {
        // not a papywizard file
        return false;
    };
    // iterate all children
    wxXmlNode* child = xmlFile.GetRoot()->GetChildren();
    while (child)
    {
        if (child->GetName().CmpNoCase(wxT("header")) == 0)
        {
            if (!ParseHeader(child, images))
            {
                return false;
            };
        };
        if (child->GetName().CmpNoCase(wxT("shoot")) == 0)
        {
            if (!ParseShoot(child, images))
            {
                return false;
            };
        }
        child = child->GetNext();
    }
    return true;
};

class PapywizardImportDialog: public wxDialog
{
public:
    /** Constructor, read from xrc ressource */
    PapywizardImportDialog(wxWindow *parent)
    {
        wxXmlResource::Get()->LoadDialog(this, parent, wxT("papywizard_import_dialog"));
#ifdef __WXMSW__
        wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"), wxBITMAP_TYPE_ICO);
#else
        wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"), wxBITMAP_TYPE_PNG);
#endif
        SetIcon(myIcon);
        m_linkPos = XRCCTRL(*this, "papywizard_link_positions", wxCheckBox);
        m_cpfind = XRCCTRL(*this, "papywizard_cpfind", wxCheckBox);
        m_cpfindParams = XRCCTRL(*this, "papywizard_cpfind_parameters", wxTextCtrl);
        m_geocpset = XRCCTRL(*this, "papywizard_geocpset", wxCheckBox);
        const wxString cpfindParams = wxConfig::Get()->Read(wxT("/PapywizardImportCpfind"), wxEmptyString);
        m_cpfindParams->SetValue(cpfindParams);
        RestoreFramePosition(this, wxT("PapywizardImportDialog"));
    };
    /** destructor, save settings */
    PapywizardImportDialog::~PapywizardImportDialog()
    {
        StoreFramePosition(this, wxT("PapywizardImportDialog"));
        if (m_cpfind->IsChecked())
        {
            wxConfig::Get()->Write(wxT("/PapywizardImportCpfind"), m_cpfindParams->GetValue());
        };
    };
    void PapywizardImportDialog::EnableStack(const bool hasStacks)
    {
        m_linkPos->Enable(hasStacks);
        m_linkPos->SetValue(hasStacks);
    };
    const bool PapywizardImportDialog::LinkStacks() const
    {
        return m_linkPos->IsEnabled() && m_linkPos->IsChecked();
    };
    const bool PapywizardImportDialog::RunCpfind() const
    {
        return m_cpfind->IsChecked();
    };
    const wxString PapywizardImportDialog::GetCPFindParam() const
    {
        return m_cpfindParams->GetValue();
    };
    const bool RunGeocpset() const
    {
        return m_geocpset->IsEnabled() && m_geocpset->IsChecked();
    };
protected:
    void OnCpfindCheck(wxCommandEvent& e)
    {
        const bool cpfindActive = m_cpfind->IsChecked();
        m_cpfindParams->Enable(cpfindActive);
        m_geocpset->Enable(cpfindActive);
    };
private:
    wxCheckBox* m_linkPos;
    wxCheckBox* m_cpfind;
    wxTextCtrl* m_cpfindParams;
    wxCheckBox* m_geocpset;
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(PapywizardImportDialog, wxDialog)
    EVT_CHECKBOX(XRCID("papywizard_cpfind"), PapywizardImportDialog::OnCpfindCheck)
END_EVENT_TABLE()

bool ImportPapywizardFile(const wxString& filename, HuginBase::Panorama& pano)
{
    PapywizardSettings papyImages;
    if (!ParsePapywizardFile(filename, papyImages))
    {
        wxMessageBox(wxString::Format(_("Could not parse file %s as Papywizard XML file."), filename.c_str()),
#ifdef __WXMSW__
            _("Hugin"),
#else
            wxT(""),
#endif
            wxOK);
        return false;
    };
    // check if number of images matches
    if(papyImages.images.size()!=pano.getNrOfImages())
    {
        wxMessageBox(wxString::Format(_("The current project does not match with the Papywizard xml file.\nThe Papywizard file \"%s\" contains %lu images, but the Hugin project contains %lu images."), filename.c_str(), static_cast<unsigned long>(papyImages.images.size()), static_cast<unsigned long>(pano.getNrOfImages())),
#ifdef __WXMSW__
            _("Hugin"),
#else
            wxT(""),
#endif
            wxOK);
        return false;
    };
    PapywizardImportDialog dialog(wxGetActiveWindow());
    dialog.EnableStack(papyImages.HasBracketImages());
    if(dialog.ShowModal()!=wxID_OK)
    { 
        return false;
    };
    // now we can build all commands
    std::vector<PanoCommand::PanoCommand *> commands;
    HuginBase::UIntSet images;
    fill_set(images, 0, pano.getNrOfImages() - 1);
    // all images share the same lens
    commands.push_back(new PanoCommand::ChangePartNumberCmd(pano, images, 0, HuginBase::StandardImageVariableGroups::getLensVariables()));
    commands.push_back(new PanoCommand::ChangeImageProjectionCmd(pano, images, papyImages.projection));
    if (papyImages.focallength > 0)
    {
        commands.push_back(new PanoCommand::UpdateCropFactorCmd(pano, images, papyImages.cropfactor));
        commands.push_back(new PanoCommand::UpdateFocalLengthCmd(pano, images, papyImages.focallength));
    };
    // remove all existing stacks
    for (size_t i = 1; i < pano.getNrOfImages(); i++)
    {
        HuginBase::UIntSet imgs;
        imgs.insert(i);
        commands.push_back(new PanoCommand::NewPartCmd(pano, imgs, HuginBase::StandardImageVariableGroups::getStackVariables()));
    };
    // create stacks
    if (papyImages.HasBracketImages())
    {
        size_t stackNr = 0;
        size_t imgNr = 0;
        while (imgNr < pano.getNrOfImages())
        {
            HuginBase::UIntSet imgs;
            do
            {
                imgs.insert(imgNr);
                imgNr++;
            } while (imgNr < pano.getNrOfImages() && papyImages.images[imgNr].bracket != 1);
            commands.push_back(new PanoCommand::ChangePartNumberCmd(pano, imgs, stackNr, HuginBase::StandardImageVariableGroups::getStackVariables()));
            stackNr++;
        };
        // unlink position if wished by user
        if (!dialog.LinkStacks())
        {
            std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
            variables.insert(HuginBase::ImageVariableGroup::IVE_Yaw);
            variables.insert(HuginBase::ImageVariableGroup::IVE_Pitch);
            variables.insert(HuginBase::ImageVariableGroup::IVE_Roll);
            variables.insert(HuginBase::ImageVariableGroup::IVE_X);
            variables.insert(HuginBase::ImageVariableGroup::IVE_Y);
            variables.insert(HuginBase::ImageVariableGroup::IVE_Z);
            variables.insert(HuginBase::ImageVariableGroup::IVE_TranslationPlaneYaw);
            variables.insert(HuginBase::ImageVariableGroup::IVE_TranslationPlanePitch);
            commands.push_back(new PanoCommand::ChangePartImagesLinkingCmd(pano, images, variables, false, HuginBase::StandardImageVariableGroups::getStackVariables()));
        }
    };
    // now set the positions    
    HuginBase::VariableMapVector variables = pano.getVariables();
    for (size_t i = 0; i < papyImages.images.size(); ++i)
    {
        map_get(variables[i], "y").setValue(papyImages.images[i].yaw);
        map_get(variables[i], "p").setValue(papyImages.images[i].pitch);
        map_get(variables[i], "r").setValue(papyImages.images[i].roll);
        map_get(variables[i], "TrX").setValue(0);
        map_get(variables[i], "TrY").setValue(0);
        map_get(variables[i], "TrZ").setValue(0);
        map_get(variables[i], "Tpy").setValue(0);
        map_get(variables[i], "Tpp").setValue(0);
        // fov gets updated when focal length is set, this has not yet been happen
        // so remove fov from list to prevent overwritting with old value, because
        // variables have not yet been updated yet
        variables[i].erase("v");
    };
    commands.push_back(new PanoCommand::UpdateVariablesCmd(pano, variables));
    PanoCommand::GlobalCmdHist::getInstance().addCommand(new PanoCommand::CombinedPanoCommand(pano, commands));
    
    // now create the cp
    if (dialog.RunCpfind())
    {
        //save project into temp directory
        wxString tempDir = wxConfig::Get()->Read(wxT("tempDir"), wxT(""));
        if (!tempDir.IsEmpty())
        {
            if (tempDir.Last() != wxFileName::GetPathSeparator())
            {
                tempDir.Append(wxFileName::GetPathSeparator());
            }
        };
        wxFileName scriptFileName(wxFileName::CreateTempFileName(tempDir + wxT("hp")));
        std::ofstream script(scriptFileName.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
        script.exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), images, false);
        script.close();
        // build command queue
        const wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        HuginQueue::CommandQueue* commands=new HuginQueue::CommandQueue();
        const wxString quotedProject(HuginQueue::wxEscapeFilename(scriptFileName.GetFullPath()));
        commands->push_back(new HuginQueue::NormalCommand(HuginQueue::GetInternalProgram(exePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), wxT("cpfind")),
            dialog.GetCPFindParam() + wxT(" --prealigned -o ") + quotedProject + wxT(" ") + quotedProject, _("Searching for control points...")));
        if (dialog.RunGeocpset())
        {
            commands->push_back(new HuginQueue::NormalCommand(HuginQueue::GetInternalProgram(exePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), wxT("geocpset")),
                wxT("-o ") + quotedProject + wxT(" ") + quotedProject, _("Connecting overlapping images")));
        };
        //execute queue
        int ret = MyExecuteCommandQueue(commands, wxGetActiveWindow(), _("Searching control points"));
        //read back panofile
        PanoCommand::GlobalCmdHist::getInstance().addCommand(new PanoCommand::wxLoadPTProjectCmd(pano,
            (const char *)scriptFileName.GetFullPath().mb_str(HUGIN_CONV_FILENAME),
            (const char *)scriptFileName.GetPath(wxPATH_NATIVE | wxPATH_GET_SEPARATOR).mb_str(HUGIN_CONV_FILENAME),
            false, false));
        //delete temporary files
        wxRemoveFile(scriptFileName.GetFullPath());
    };
    return true;
}

} // namespace Papywizard

