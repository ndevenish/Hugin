// -*- c-basic-offset: 4 -*-

/** @file ImagesPanel.cpp
 *
 *  @brief implementation of ImagesPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#include <config.h>
#include "panoinc_WX.h"
#include "panoinc.h"
#include <time.h>

#include "base_wx/platform.h"
#include <vector>
#include <map>

//#include <vigra_ext/PointMatching.h>
//#include <vigra_ext/LoweSIFT.h>

#include "hugin/ImagesPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "icpfind/AutoCtrlPointCreator.h"
#include "hugin/config_defaults.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/PTWXDlg.h"
#include <algorithms/control_points/CleanCP.h>
#include <PT/PTOptimise.h>

#include <panodata/StandardImageVariableGroups.h>

// Celeste header
#include "Celeste.h"
#include "CelesteGlobals.h"
#include "Utilities.h"

using namespace PT;
using namespace hugin_utils;
using namespace vigra;
using namespace vigra_ext;
using namespace std;

ImgPreview * canvas;

#define m_XRCID(str_id) \
    wxXmlResource::GetXRCID(str_id)
#define m_XRCCTRL(window, id, type) \
    ((type*)((window).FindWindow(m_XRCID(id))))


//------------------------------------------------------------------------------
#define GET_VAR(val) pano->getVariable(orientationEdit_RefImg).val.getValue()

BEGIN_EVENT_TABLE(ImagesPanel, wxPanel)
//    EVT_SIZE   ( ImagesPanel::OnSize )
//    EVT_MOUSE_EVENTS ( ImagesPanel::OnMouse )
//    EVT_MOTION ( ImagesPanel::ChangePreview )
//	EVT_SPLITTER_SASH_POS_CHANGED(XRCID("image_panel_splitter"), ImagesPanel::OnPositionChanged)
    EVT_LIST_ITEM_SELECTED( XRCID("images_list_unknown"),
                            ImagesPanel::ListSelectionChanged )
    EVT_LIST_ITEM_DESELECTED( XRCID("images_list_unknown"),
                            ImagesPanel::ListSelectionChanged )
    EVT_BUTTON     ( XRCID("images_opt_anchor_button"), ImagesPanel::OnOptAnchorChanged)
    EVT_BUTTON     ( XRCID("images_color_anchor_button"), ImagesPanel::OnColorAnchorChanged)
    EVT_BUTTON     ( XRCID("images_feature_matching"), ImagesPanel::SIFTMatching)
    EVT_BUTTON     ( XRCID("images_cp_clean"), ImagesPanel::OnCleanCP)
    EVT_BUTTON     ( XRCID("images_remove_cp"), ImagesPanel::OnRemoveCtrlPoints)
    EVT_BUTTON     ( XRCID("images_reset_pos"), ImagesPanel::OnResetImagePositions)
    EVT_BUTTON     ( XRCID("action_remove_images"),  ImagesPanel::OnRemoveImages)
    EVT_BUTTON     ( XRCID("images_move_image_down"),  ImagesPanel::OnMoveImageDown)
    EVT_BUTTON     ( XRCID("images_move_image_up"),  ImagesPanel::OnMoveImageUp)
    EVT_BUTTON	   ( XRCID("images_celeste_button"), ImagesPanel::OnCelesteButton)
    EVT_BUTTON	   ( XRCID("images_new_stack"), ImagesPanel::OnNewStack)
    EVT_BUTTON	   ( XRCID("images_change_stack"), ImagesPanel::OnChangeStack)
    EVT_TEXT_ENTER ( XRCID("images_text_y"), ImagesPanel::OnVarTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_p"), ImagesPanel::OnVarTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_r"), ImagesPanel::OnVarTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_X"), ImagesPanel::OnVarTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_Y"), ImagesPanel::OnVarTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_Z"), ImagesPanel::OnVarTextChanged )
    EVT_CHECKBOX   ( XRCID("images_check_link"), ImagesPanel::OnImageLinkChanged )
    EVT_COMMAND    (wxID_ANY, EVT_IMAGE_ADD, ImagesPanel::OnAddImages )
    EVT_COMMAND    (wxID_ANY, EVT_IMAGE_DEL, ImagesPanel::OnRemoveImages )
END_EVENT_TABLE()

ImagesPanel::ImagesPanel()
{
    pano = 0;
    m_restoreLayoutOnResize = false;
}

bool ImagesPanel::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                      long style, const wxString& name)
{
    if (! wxPanel::Create(parent, id, pos, size, style, name)) {
        return false;
    }

    wxXmlResource::Get()->LoadPanel(this, wxT("images_panel"));
    wxPanel * panel = XRCCTRL(*this, "images_panel", wxPanel);
    images_list = XRCCTRL(*this, "images_list_unknown", ImagesListImage);
    assert(images_list);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer(topsizer);

    DEBUG_TRACE("");

    m_showImgNr = INT_MAX;

    m_optAnchorButton = XRCCTRL(*this, "images_opt_anchor_button", wxButton);
    DEBUG_ASSERT(m_optAnchorButton);

    m_colorAnchorButton = XRCCTRL(*this, "images_color_anchor_button", wxButton);
    DEBUG_ASSERT(m_colorAnchorButton);
    m_matchingButton = XRCCTRL(*this, "images_feature_matching", wxButton);
    DEBUG_ASSERT(m_matchingButton);
    m_cleaningButton = XRCCTRL(*this, "images_cp_clean", wxButton);
    DEBUG_ASSERT(m_cleaningButton);
    m_removeCPButton = XRCCTRL(*this, "images_remove_cp", wxButton);
    DEBUG_ASSERT(m_removeCPButton);
    m_moveUpButton = XRCCTRL(*this, "images_move_image_up", wxButton);
    DEBUG_ASSERT(m_moveUpButton);
    m_moveDownButton = XRCCTRL(*this, "images_move_image_down", wxButton);
    DEBUG_ASSERT(m_moveDownButton);
    m_stackNewButton = XRCCTRL(*this, "images_new_stack", wxButton);
    DEBUG_ASSERT(m_stackNewButton);
    m_stackChangeButton = XRCCTRL(*this, "images_change_stack", wxButton);
    DEBUG_ASSERT(m_stackChangeButton);
    
    m_linkCheckBox = XRCCTRL(*this, "images_check_link", wxCheckBox);

    m_img_ctrls = XRCCTRL(*this, "image_control_panel", wxPanel);
    DEBUG_ASSERT(m_img_ctrls);

    m_CPDetectorChoice = XRCCTRL(*this, "cpdetector_settings", wxChoice);

    // Image Preview
    m_smallImgCtrl = XRCCTRL(*this, "images_selected_image", wxStaticBitmap);
    DEBUG_ASSERT(m_smallImgCtrl);

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    XRCCTRL(*this, "images_text_y", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_r", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_p", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_X", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_Y", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_Z", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));

    m_empty.Create(0,0);
//    m_empty.LoadFile(huginApp::Get()->GetXRCPath() +
//                     wxT("data/") + wxT("transparent.png"),
//                     wxBITMAP_TYPE_PNG);
    m_smallImgCtrl->SetBitmap(m_empty);

    wxListEvent ev;
    ListSelectionChanged(ev);
    DEBUG_TRACE("end");

//    SetAutoLayout(false);

    wxConfigBase* config=wxConfigBase::Get();
    m_degDigits = config->Read(wxT("/General/DegreeFractionalDigitsEdit"),3);
    //read autopano generator settings
    cpdetector_config.Read(config,huginApp::Get()->GetDataPath()+wxT("default.setting"));
    //write current autopano generator settings
    cpdetector_config.Write(config);
    config->Flush();
    cpdetector_config.FillControl(m_CPDetectorChoice,true);
    Layout();

    return true;
}

void ImagesPanel::Init(Panorama * panorama)
{
    pano = panorama;
    images_list->Init(pano);
    // observe the panorama
    pano->addObserver(this);
}

ImagesPanel::~ImagesPanel()
{
    DEBUG_TRACE("dtor");

    XRCCTRL(*this, "images_text_y", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "images_text_r", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "images_text_p", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "images_text_X", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "images_text_Y", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "images_text_Z", wxTextCtrl)->PopEventHandler(true);
/*
    delete(m_tkf);
*/
    pano->removeObserver(this);
    delete images_list;
    DEBUG_TRACE("dtor end");
}

void ImagesPanel::RestoreLayout()
{
	DEBUG_TRACE("");
    int winWidth, winHeight;
    GetClientSize(&winWidth, &winHeight);
    DEBUG_INFO( "image panel: " << winWidth <<"x"<< winHeight);

}

// We need to override the default handling of size events because the
// sizers set the virtual size but not the actual size. We reverse
// the standard handling and fit the child to the parent rather than
// fitting the parent around the child

void ImagesPanel::OnSize( wxSizeEvent & e )
{
    int winWidth, winHeight;
    GetClientSize(&winWidth, &winHeight);
    DEBUG_INFO( "image panel: " << winWidth <<"x"<< winHeight );
    UpdatePreviewImage();

    e.Skip();
}

void ImagesPanel::OnPositionChanged(wxSplitterEvent& e)
{
//	DEBUG_INFO("Sash Position now:" << e.GetSashPosition() << " or: " << m_img_splitter->GetSashPosition());
    e.Skip();
}

void ImagesPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & _imgNr)
{
    DEBUG_TRACE("");

    // update text field if selected
    const UIntSet & selected = images_list->GetSelected();
    DEBUG_DEBUG("nr of sel Images: " << selected.size());
    if (pano.getNrOfImages() == 0)
    {
        DisableImageCtrls();
        m_removeCPButton->Disable();
        m_matchingButton->Disable();
    } else {
        m_removeCPButton->Enable();
        m_matchingButton->Enable();
    }
    if (pano.getNrOfImages()>1)
    {
        if(pano.getNrOfCtrlPoints()>2)
            m_cleaningButton->Enable();
        else
            m_cleaningButton->Disable();
    }
    else
        m_cleaningButton->Disable();

    if (selected.size() == 1 &&
        *(selected.begin()) < pano.getNrOfImages() &&
        set_contains(_imgNr, *(selected.begin())))
    {
        DEBUG_DEBUG("Img Nr: " << *(selected.begin()));
        ShowImgParameters( *(selected.begin()) );
    }
    else
    {
        ClearImgParameters( );
    }

    DEBUG_TRACE("");
}

void ImagesPanel::ChangePano ( std::string type, double var )
{
    Variable img_var(type,var);
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetVariableCmd(*pano, images_list->GetSelected(), img_var)
        );
}

// #####  Here start the eventhandlers  #####

/** run sift matching on selected images, and add control points */
void ImagesPanel::SIFTMatching(wxCommandEvent & e)
{
    UIntSet selImg = images_list->GetSelected();
    //if only one image is selected, run detector on all images, except for linefind
    wxString progName=cpdetector_config.settings[m_CPDetectorChoice->GetSelection()].GetProg().Lower();
    if ((selImg.size()==0) || (selImg.size()==1 && progName.Find(wxT("linefind"))==wxNOT_FOUND))
    {
        // add all images.
        selImg.clear();
        unsigned int nImg = pano->getNrOfImages();
        for (unsigned int i=0; i < nImg; i++) {
            selImg.insert(i);
        }
    }

    if (selImg.size() == 0) {
        return;
    }

    long nFeatures = XRCCTRL(*this, "images_points_per_overlap"
                            , wxSpinCtrl)->GetValue();

    AutoCtrlPointCreator matcher;
    CPVector cps = matcher.automatch(cpdetector_config.settings[m_CPDetectorChoice->GetSelection()],
        *pano, selImg, nFeatures,this);
    wxString msg;
    wxMessageBox(wxString::Format(_("Added %lu control points"), (unsigned long) cps.size()), _("Control point detector result"),wxOK|wxICON_INFORMATION,this);
    GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointsCmd(*pano, cps)
                                           );

};

void ImagesPanel::OnCleanCP(wxCommandEvent & e)
{
    if (pano->getNrOfImages()<2)
        return;

    if (pano->getNrOfCtrlPoints()<2)
        return;

    deregisterPTWXDlgFcn();
    // work around a flaw in wxProgresDialog that results in incorrect layout
    // by pre-allocting sufficient horizontal space
    ProgressReporterDialog progress(2, _("Cleaning Control points"), _("Checking pairwise")+wxString((wxChar)' ',10),this, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME);
    UIntSet CPremove=getCPoutsideLimit_pair(*pano,2.0);
    
    unsigned int NrRemoved=CPremove.size();
    if(NrRemoved>0)
    {
        GlobalCmdHist::getInstance().addCommand(
                        new PT::RemoveCtrlPointsCmd(*pano,CPremove)
                        );
        
    };
    CPremove.clear();
    
    //check for unconnected images
    CPGraph graph;
    createCPGraph(*pano, graph);
    CPComponents comps;
    int n=findCPComponents(graph, comps);
    progress.increaseProgress(1, std::wstring(wxString(_("Checking whole project")).wc_str(wxConvLocal)));
    if (n <= 1)
    {
        CPremove=getCPoutsideLimit(*pano,2.0);
    }
    progress.increaseProgress(1, std::wstring(wxString(_("Finished cleaning")).wc_str(wxConvLocal)));
    registerPTWXDlgFcn(MainFrame::Get());
    if(CPremove.size()>0)
    {
        GlobalCmdHist::getInstance().addCommand(
                        new PT::RemoveCtrlPointsCmd(*pano,CPremove)
                        );
        NrRemoved+=CPremove.size();
    };
    wxMessageBox(wxString::Format(_("Removed %u control points"), NrRemoved), _("Cleaning"),wxOK|wxICON_INFORMATION,this);
};

void ImagesPanel::OnVarTextChanged ( wxCommandEvent & e )
{

    if ( images_list->GetSelected().size() > 0 ) {

	std::string varname;
	double val;

	
	const char vars[] = "rpyXYZ";
	for (const char * var = vars; *var; var++) {
	    wxString ctrl_name(wxT("images_text_"));
	    ctrl_name.Append(wxChar(*var));
	    if (e.GetId() == wxXmlResource::GetXRCID(ctrl_name)){

        wxString text = m_XRCCTRL(*this, ctrl_name, wxTextCtrl)->GetValue().Trim();
        if(text.IsEmpty())
        {
            return;
        };

		// hack to add the T to the x. This should really use SrcPanoImage instead..
		char name[4];
		if (*var > 'Z') {
		    name[0] = *var; name[1] = 0;
		} else {
		    name[0] = 'T'; name[1]='r'; name[2]=*var; name[3] = 0;
		}
		if (!str2double(text, val)){
		    DEBUG_NOTICE("Value (" << text << ") for var " << name << " must be numeric.");
		    wxLogError(_("Value must be numeric."));
		    return;
		}
		Variable img_var(name, val);
		GlobalCmdHist::getInstance().addCommand(
		    new PT::SetVariableCmd(*pano, images_list->GetSelected(), img_var));
	    }
	}
    }
}

void ImagesPanel::OnImageLinkChanged(wxCommandEvent &e )
{
    // link or unlink yaw, pitch and roll for selected stacks.
    bool inherit = e.IsChecked();
    
    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
    variables.insert(HuginBase::ImageVariableGroup::IVE_Roll);
    variables.insert(HuginBase::ImageVariableGroup::IVE_Pitch);
    variables.insert(HuginBase::ImageVariableGroup::IVE_Yaw);

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangePartImagesLinkingCmd(*pano, images_list->GetSelected(),
                                             variables, inherit,
                HuginBase::StandardImageVariableGroups::getStackVariables())
        );
}

void ImagesPanel::OnOptAnchorChanged(wxCommandEvent &e )
{
    const UIntSet & sel = images_list->GetSelected();
    if ( sel.size() == 1 ) {
        // set first image to be the anchor
        PanoramaOptions opt = pano->getOptions();
        opt.optimizeReferenceImage = *(sel.begin());

        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
    }
}

void ImagesPanel::OnColorAnchorChanged(wxCommandEvent &e )
{
    const UIntSet & sel = images_list->GetSelected();
    if ( sel.size() == 1 ) {
        // set first image to be the anchor
        PanoramaOptions opt = pano->getOptions();
        opt.colorReferenceImage = *(sel.begin());
		// Set the color correction mode so that the anchor image is persisted
		if (opt.colorCorrection == 0) {
		  opt.colorCorrection = (PanoramaOptions::ColorCorrection) 1;
		}
		DEBUG_INFO("Color reference image is now: " << opt.colorReferenceImage);
		DEBUG_INFO("Color correction mode : " << opt.colorCorrection);
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( *pano, opt )
            );
    }
	
}


void ImagesPanel::ListSelectionChanged(wxListEvent & e)
{
    DEBUG_TRACE(e.GetIndex());
    const UIntSet & sel = images_list->GetSelected();
    DEBUG_DEBUG("selected Images: " << sel.size());
    if (sel.size() > 0) {
        DEBUG_DEBUG("first sel. image: " << *(sel.begin()));
    }
    if (sel.size() == 0) {
        // nothing to edit
        DisableImageCtrls();
        ClearImgExifInfo();
    } else {
        // enable edit
        EnableImageCtrls();
        if (sel.size() == 1) {
            unsigned int imgNr = *(sel.begin());
            // single selection, show its parameters
            ShowImgParameters(imgNr);
            m_optAnchorButton->Enable();
            m_colorAnchorButton->Enable();
        } else {
            DEBUG_DEBUG("Multiselection, or no image selected");
            // multiselection, clear all values
            // we don't know which images parameters to show.
            ClearImgParameters();
            m_optAnchorButton->Disable();
            m_colorAnchorButton->Disable();
        }
        bool movePossible=*(sel.begin())+sel.size()-1 == *(sel.rbegin());
        m_moveDownButton->Enable(movePossible);
        m_moveUpButton->Enable(movePossible);
    }
}


// ######  Here end the eventhandlers  #####

void ImagesPanel::DisableImageCtrls()
{
    // disable controls
    XRCCTRL(*this, "images_text_y", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_r", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_p", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_X", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_Y", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_Z", wxTextCtrl) ->Disable();
    m_smallImgCtrl->SetBitmap(m_empty);
    m_smallImgCtrl->GetParent()->Layout();
    m_smallImgCtrl->Refresh();
    m_optAnchorButton->Disable();
    m_colorAnchorButton->Disable();
    m_moveDownButton->Disable();
    m_moveUpButton->Disable();
    XRCCTRL(*this, "images_reset_pos", wxButton)->Disable();
    XRCCTRL(*this, "action_remove_images", wxButton)->Disable();
    XRCCTRL(*this, "images_celeste_button", wxButton)->Disable();
    m_stackNewButton->Disable();
    m_stackChangeButton->Disable();
    m_linkCheckBox->Disable();
}

void ImagesPanel::EnableImageCtrls()
{
    // enable control if not already enabled
    if (XRCCTRL(*this, "images_text_y", wxTextCtrl)->Enable()) {
        XRCCTRL(*this, "images_text_r", wxTextCtrl) ->Enable();
        XRCCTRL(*this, "images_text_p", wxTextCtrl) ->Enable();
        XRCCTRL(*this, "images_text_X", wxTextCtrl) ->Enable();
        XRCCTRL(*this, "images_text_Y", wxTextCtrl) ->Enable();
        XRCCTRL(*this, "images_text_Z", wxTextCtrl) ->Enable();
        m_moveDownButton->Enable();
        m_moveUpButton->Enable();
        XRCCTRL(*this, "images_reset_pos", wxButton)->Enable();
        XRCCTRL(*this, "action_remove_images", wxButton)->Enable();
	XRCCTRL(*this, "images_celeste_button", wxButton)->Enable();
        m_stackNewButton->Enable();
        m_stackChangeButton->Enable();
        m_linkCheckBox->Enable();
    }
}

void ImagesPanel::ShowImgParameters(unsigned int imgNr)
{
    const VariableMap & vars = pano->getImageVariables(imgNr);

    std::string val;
    val = doubleToString(const_map_get(vars,"y").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_y", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));

    val = doubleToString(const_map_get(vars,"p").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_p", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));

    val = doubleToString(const_map_get(vars,"r").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_r", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));

    val = doubleToString(const_map_get(vars,"TrX").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_X", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));

    val = doubleToString(const_map_get(vars,"TrY").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_Y", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));

    val = doubleToString(const_map_get(vars,"TrZ").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_Z", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));
    
    m_linkCheckBox->SetValue(pano->getImage(imgNr).YawisLinked());
    
    ShowExifInfo(imgNr);
    ShowImage(imgNr);
}


void ImagesPanel::ShowExifInfo(unsigned int imgNr)
{
    SrcPanoImage img = pano->getSrcImage(imgNr);

    double focalLength = 0;
    double cropFactor = 0;
    const bool applyExposureValue = FALSE;
    initImageFromFile(img,focalLength,cropFactor,applyExposureValue);

    std::string val;
    wxString s;
    val = img.getFilename();
    XRCCTRL(*this, "images_filename",wxStaticText) ->
        SetLabel(wxFileName(wxString(val.c_str(),HUGIN_CONV_FILENAME)).GetFullName());

    val = img.getExifMake();
    if(val!="Unknown")
    {
        XRCCTRL(*this, "images_camera_make",wxStaticText)->SetLabel(wxString(val.c_str(),wxConvLocal));
    };

    val = img.getExifModel();
    if(val!="Unknown")
    {
        XRCCTRL(*this, "images_camera_model",wxStaticText)->SetLabel(wxString(val.c_str(),wxConvLocal));
    };

    val = img.getExifLens();
    if(val!="Unknown")
    {
        XRCCTRL(*this, "images_lens",wxStaticText)->SetLabel(wxString(val.c_str(),wxConvLocal));
    };

    struct tm exifdatetime;
    if(img.getExifDateTime(&exifdatetime)==0)
    {
        wxDateTime s_datetime=wxDateTime(exifdatetime);
        s=s_datetime.Format();
    }
    else
        s = wxString(img.getExifDate().c_str(),wxConvLocal);
    XRCCTRL(*this, "images_capture_date",wxStaticText)->SetLabel(s);

    if(img.getExifFocalLength()>0.0)
        if(img.getExifFocalLength35()>0.0)
            s = wxString::Format(wxT("%0.1f mm (%0.0f mm)"),img.getExifFocalLength(),img.getExifFocalLength35());
        else
            s = wxString::Format(wxT("%0.1f mm"),img.getExifFocalLength());
    else
        s = wxEmptyString;
    XRCCTRL(*this, "images_focal_length",wxStaticText)->SetLabel(s);

    if(img.getExifAperture()>0)
        s=wxString::Format(wxT("F%.1f"),img.getExifAperture());
    else
        s=wxEmptyString;
    XRCCTRL(*this, "images_aperture",wxStaticText)->SetLabel(s);

    if(img.getExifExposureTime()>0.5) 
        if(img.getExifExposureTime()>=1.0) 
            if(img.getExifExposureTime()>=10.0) 
                s=wxString::Format(wxT("%3.0f s"),img.getExifExposureTime());
            else
                s=wxString::Format(wxT("%1.1f s"),img.getExifExposureTime());
        else
            s=wxString::Format(wxT("%1.2f s"),img.getExifExposureTime());
    else {
        if (img.getExifExposureTime() > 1e-9) {
            s=wxString::Format(wxT("1/%0.0f s"),1.0/img.getExifExposureTime());
        } else {
            //Sanity
            s=wxT("");
        }
    }
        
    XRCCTRL(*this, "images_shutter_speed",wxStaticText)->SetLabel(s);

    if(img.getExifISO()>0)
        s=wxString::Format(wxT("%0.0f"),img.getExifISO());
    else
        s=wxEmptyString;
    XRCCTRL(*this, "images_iso",wxStaticText)->SetLabel(s);

    XRCCTRL(*this, "images_shutter_speed",wxStaticText)->GetParent()->Layout();
    Refresh();
}


void ImagesPanel::ClearImgParameters()
{
    XRCCTRL(*this, "images_text_y", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_r", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_p", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_X", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_Y", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_Z", wxTextCtrl) ->Clear();

    m_smallImgCtrl->SetBitmap(m_empty);
    m_smallImgCtrl->GetParent()->Layout();
    m_smallImgCtrl->Refresh();
    ClearImgExifInfo();
}

void ImagesPanel::ClearImgExifInfo()
{
    XRCCTRL(*this, "images_filename", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_camera_make", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_camera_model", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_lens", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_capture_date", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_focal_length", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_aperture", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_shutter_speed", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_iso", wxStaticText) ->SetLabel(wxT(""));
}


void ImagesPanel::ShowImage(unsigned int imgNr)
{
    m_showImgNr = imgNr;
    UpdatePreviewImage();
}


void ImagesPanel::UpdatePreviewImage()
{
    if (m_showImgNr < 0 || m_showImgNr >= pano->getNrOfImages()) {
        return;
    }
    ImageCache::EntryPtr cacheEntry = ImageCache::getInstance().getSmallImageIfAvailable(
            pano->getImage(m_showImgNr).getFilename());
    if (!cacheEntry.get())
    {
        // image currently isn't loaded.
        // Instead of loading and displaying the image now, request it for
        // later. Then the user can switch between images in the list quickly,
        // even when not all images previews are in the cache.
        thumbnail_request = ImageCache::getInstance().requestAsyncSmallImage(
                pano->getImage(m_showImgNr).getFilename());
        // When the image is ready, try this function again.
        thumbnail_request->ready.connect(
            boost::bind(&ImagesPanel::UpdatePreviewImage, this));
    } else {
        // forget any request now the image has loaded.
        thumbnail_request = ImageCache::RequestPtr();
        wxImage img = imageCacheEntry2wxImage(cacheEntry); 

        double iRatio = img.GetWidth() / (double) img.GetHeight();

        wxSize sz;
        // estimate image size
        
        sz = m_smallImgCtrl->GetContainingSizer()->GetSize();
        double sRatio = (double)sz.GetWidth() / sz.GetHeight();
        if (iRatio > sRatio) {
            // image is wider than screen, display landscape
            sz.SetHeight((int) (sz.GetWidth() / iRatio));
        } else {
            // portrait
            sz.SetWidth((int) (sz.GetHeight() * iRatio));
        }
        // Make sure the size is positive:
        // on a small window, m_smallImgCtrl can have 0 width.
        sz.IncTo(wxSize(1,1));
        wxImage scaled = img.Scale(sz.GetWidth(),sz.GetHeight());
        m_smallImgCtrl->SetBitmap(wxBitmap(scaled));
        m_smallImgCtrl->GetParent()->Layout();
        m_smallImgCtrl->Refresh();
    }
}


#if 0
void ImagesPanel::ShowImage(unsigned int imgNr)
{
    // show preview image

    // static bitmap behaves strangely under windows,
    // we need to resize it to its maximum size every time before
    // a new image is set, else it will keep the old size..
    // however, I'm not sure how this should be done with wxWindows


    const wxImage * img = ImageCache::getInstance().getSmallImage(
    pano->getImage(imgNr).getFilename());

    double iRatio = (double)img->GetWidth() / img->GetHeight();

    wxSize sz;
#ifdef USE_WX253
    // get width from splitter window
    wxSize szs = m_img_splitter->GetSize();
    wxSize sz1 = m_img_splitter->GetWindow1()->GetSize();
    wxSize sz2 = m_img_splitter->GetWindow2()->GetSize();

    // estimate image size
    sz = m_img_splitter->GetWindow2()->GetSize();
    int maxw = sz.GetWidth() - 40;
    int maxh = sz.GetHeight() - m_smallImgCtrl->GetPosition().y - 20;
    sz.SetHeight(std::max(maxh, 20));
    sz.SetWidth(std::max(maxw, 20));
    double sRatio = (double)sz.GetWidth() / sz.GetHeight();
    if (iRatio > sRatio) {
        // image is wider than screen, display landscape
        sz.SetHeight((int) (sz.GetWidth() / iRatio));
    } else {
        // portrait
        sz.SetWidth((int) (sz.GetHeight() * iRatio));
    }

#else
    // get size from parent panel (its just there, to provide the size..
    wxPanel * imgctrlpanel = XRCCTRL(*this, "images_selected_image_panel", wxPanel);
    DEBUG_ASSERT(imgctrlpanel);
    wxSize sz = imgctrlpanel->GetSize();
    DEBUG_DEBUG("imgctrl panel size: " << sz.x << "," << sz.y);
    double sRatio = (double)sz.GetWidth() / sz.GetHeight();

    if (iRatio > sRatio) {
        // image is wider than screen, display landscape
        sz.SetHeight((int) (sz.GetWidth() / iRatio));
    } else {
        // portrait
        sz.SetWidth((int) (sz.GetHeight() * iRatio));
    }
#if (wxMAJOR_VERSION == 2 && wxMINOR_VERSION == 4)
    imgctrlpanel->Clear();
#else
    imgctrlpanel->ClearBackground();
#endif
    imgctrlpanel->SetSize(sz.GetWidth(),sz.GetHeight());

#endif

    wxImage scaled = img->Scale(sz.GetWidth(),sz.GetHeight());
    m_smallImgCtrl->SetSize(sz.GetWidth(),sz.GetHeight());
    m_smallImgCtrl->SetBitmap(wxBitmap(scaled));
}
#endif

void ImagesPanel::OnResetImagePositions(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    const UIntSet & selImg = images_list->GetSelected();
    unsigned int nSelImg =  selImg.size();
    if ( nSelImg > 0 ) {
        VariableMapVector vars(nSelImg);
        unsigned int i=0;
        for (UIntSet::const_iterator it = selImg.begin();
             it != selImg.end(); ++it )
        {
            vars[i].insert(make_pair("y", Variable("y",0.0)));
            vars[i].insert(make_pair("p", Variable("p",0.0)));
            vars[i].insert(make_pair("r", Variable("r",pano->getSrcImage(*it).getExifOrientation())));
            vars[i].insert(make_pair("TrX", Variable("TrX",0.0)));
            vars[i].insert(make_pair("TrY", Variable("TrY",0.0)));
            vars[i].insert(make_pair("TrZ", Variable("TrZ",0.0)));
            i++;
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateImagesVariablesCmd( *pano, selImg, vars ));
    }

}

void ImagesPanel::OnAddImages(wxCommandEvent &e)
{
    MainFrame::Get()->OnAddImages(e);
};

void ImagesPanel::OnRemoveImages(wxCommandEvent & e)
{
    DEBUG_TRACE("");

    UIntSet selImg = images_list->GetSelected();
    vector<string> filenames;
    for (UIntSet::iterator it = selImg.begin(); it != selImg.end(); ++it) {
        filenames.push_back(pano->getImage(*it).getFilename());
    }
    //deselect images if multiple image were selected
    if(selImg.size()>1)
    {
        images_list->DeselectAll();
    };
    DEBUG_TRACE("Sending remove images command");
    GlobalCmdHist::getInstance().addCommand(
        new PT::RemoveImagesCmd(*pano, selImg)
        );

    DEBUG_TRACE("Removing " << filenames.size() << " images from cache");
    for (vector<string>::iterator it = filenames.begin();
         it != filenames.end(); ++it)
    {
        ImageCache::getInstance().removeImage(*it);
    }
}


void ImagesPanel::OnRemoveCtrlPoints(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    UIntSet selImg = images_list->GetSelected();
    if ( selImg.size() == 0) {
        // add all images.
        selImg.clear();
        unsigned int nImg = pano->getNrOfImages();
        for (unsigned int i=0; i < nImg; i++) {
            selImg.insert(i);
        }
    }


    UIntSet cpsToDelete;
    const CPVector & cps = pano->getCtrlPoints();
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it){
        if (set_contains(selImg, (*it).image1Nr) &&
            set_contains(selImg, (*it).image2Nr) )
        {
            cpsToDelete.insert(it - cps.begin());
        }
    }
    int r =wxMessageBox(wxString::Format(_("Really Delete %lu control points?"),
                                         (unsigned long int) cpsToDelete.size()),
                        _("Delete Control Points"),
                        wxICON_QUESTION | wxYES_NO);
    if (r == wxYES) {
        GlobalCmdHist::getInstance().addCommand(
            new PT::RemoveCtrlPointsCmd( *pano, cpsToDelete ));
    }
}

void ImagesPanel::OnMoveImageDown(wxCommandEvent & e)
{
    UIntSet selImg = images_list->GetSelected();
    size_t num = selImg.size();
    if(num>0)
    {
        //last selected image
        unsigned int i1 = *selImg.rbegin();
        //image to move into
        unsigned int i2 = i1+1;
        //Test if there is room to move images down
        if (i2 < pano->getNrOfImages() )
        {
            //Group repeated move commands within undo history
            std::vector<PanoCommand*> cmds;
            for (size_t i=0; i<num; i++)
            {
                cmds.push_back(new SwapImagesCmd(*pano,i2, i1));
                i1--;
                i2--;
             }
            GlobalCmdHist::getInstance().addCommand(
                new PT::CombinedPanoCommand(*pano, cmds));
            // set new selection
            images_list->SelectImageRange(*selImg.begin()+1,*selImg.begin()+num);
            // Bring the focus back to the button.
            m_moveDownButton->SetFocus();
        };
    };
};

void ImagesPanel::OnMoveImageUp(wxCommandEvent & e)
{
    UIntSet selImg = images_list->GetSelected();
    size_t num = selImg.size();
    if(num>0)
    {
        //first selected image
        size_t i1 = *selImg.begin();
        //image to move into. 
        size_t i2 = i1-1;
        //Test if there is room to move images up
        if (i1 > 0 )
        {
            //Group repeated move commands within undo history
            std::vector<PanoCommand*> cmds;
            for (size_t i=0; i<num; i++)
            {
                cmds.push_back(new SwapImagesCmd(*pano,i2, i1));
                i1++;
                i2++;
            }
            GlobalCmdHist::getInstance().addCommand(
                    new PT::CombinedPanoCommand(*pano, cmds));
            // set new selection
            images_list->SelectImageRange(*selImg.begin()-1,*selImg.begin()+num-2);
            // Bring the focus back to the button.
            m_moveUpButton->SetFocus();
        };
    };
};

void ImagesPanel::ReloadCPDetectorSettings()
{
    cpdetector_config.Read(); 
    cpdetector_config.FillControl(m_CPDetectorChoice,true);
    m_CPDetectorChoice->InvalidateBestSize();
    m_CPDetectorChoice->GetParent()->Layout();
    Refresh();
};

void ImagesPanel::OnNewStack(wxCommandEvent & e)
{
    /** @todo it is possibly better to link just the Stack variable,
     * since the majority of stacks need self alignment, and this links angles.
     */
    GlobalCmdHist::getInstance().addCommand
    (
        new PT::NewPartCmd
        (
            *pano, images_list->GetSelected(),
            HuginBase::StandardImageVariableGroups::getStackVariables()
        )
    );
}

void ImagesPanel::OnChangeStack(wxCommandEvent & e)
{
    // ask user for stack number, if there are enoguh.
    HuginBase::StandardImageVariableGroups variableGroups(*pano);
    if (variableGroups.getStacks().getNumberOfParts() == 1)
    {
        wxLogError(_("Your project must have at least two stacks before you can assign images to a different stack."));
        return;
    }
    long nr = wxGetNumberFromUser(
                            _("Enter new stack number"),
                            _("stack number"),
                            _("Change stack number"), 0, 0,
                            variableGroups.getStacks().getNumberOfParts()-1
                                 );
    if (nr >= 0) {
        // user accepted
        GlobalCmdHist::getInstance().addCommand
        (
            new PT::ChangePartNumberCmd
            (   
                *pano, images_list->GetSelected(), nr,
                HuginBase::StandardImageVariableGroups::getStackVariables()
            )
        );
    }
}

IMPLEMENT_DYNAMIC_CLASS(ImagesPanel, wxPanel)

ImagesPanelXmlHandler::ImagesPanelXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *ImagesPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, ImagesPanel)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow( cp);

    return cp;
}

bool ImagesPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("ImagesPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(ImagesPanelXmlHandler, wxXmlResourceHandler)

void ImagesPanel::OnCelesteButton(wxCommandEvent & e)
{
    const UIntSet & selImg = images_list->GetSelected();

    if ( selImg.size() == 0)
    {
        DEBUG_WARN("Cannot run celeste without at least one point");
    }
    else
    {	
        ProgressReporterDialog progress(selImg.size()+2, _("Running Celeste"), _("Running Celeste"),this);
        MainFrame::Get()->SetStatusText(_("searching for cloud-like control points..."),0);
        progress.increaseProgress(1.0, std::wstring(wxString(_("Loading model file")).wc_str(wxConvLocal)));

        struct celeste::svm_model* model=MainFrame::Get()->GetSVMModel();
        if(model==NULL)
        {
            MainFrame::Get()->SetStatusText(wxT(""),0);
            return;
        };

        // Get Celeste parameters
        wxConfigBase *cfg = wxConfigBase::Get();
        // SVM threshold
        double threshold = HUGIN_CELESTE_THRESHOLD;
        cfg->Read(wxT("/Celeste/Threshold"), &threshold, HUGIN_CELESTE_THRESHOLD);

        // Mask resolution - 1 sets it to fine
        bool t = (cfg->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER) == 0);
        int radius=(t)?10:20;
        DEBUG_TRACE("Running Celeste");

        UIntSet cpsToRemove;
        for (UIntSet::const_iterator it=selImg.begin(); it!=selImg.end(); it++)
        {
            // Image to analyse
            HuginBase::CPointVector cps=pano->getCtrlPointsVectorForImage(*it);
            if(cps.size()==0)
            {
                progress.increaseProgress(1.0, std::wstring(wxString(_("Running Celeste")).wc_str(wxConvLocal)));
                continue;
            };
            ImageCache::EntryPtr img=ImageCache::getInstance().getImage(pano->getImage(*it).getFilename());
            vigra::UInt16RGBImage in;
            if(img->image16->width()>0)
            {
                in.resize(img->image16->size());
                vigra::copyImage(srcImageRange(*(img->image16)),destImage(in));
            }
            else
            {
                ImageCache::ImageCacheRGB8Ptr im8=img->get8BitImage();
                in.resize(im8->size());
                vigra::transformImage(srcImageRange(*im8),destImage(in),vigra::functor::Arg1()*vigra::functor::Param(65535/255));
            };
            UIntSet cloudCP=celeste::getCelesteControlPoints(model,in,cps,radius,threshold,800);
            in.resize(0,0);
            if(cloudCP.size()>0)
            {
                for(UIntSet::const_iterator it2=cloudCP.begin();it2!=cloudCP.end();it2++)
                {
                    cpsToRemove.insert(*it2);
                };
            };
            progress.increaseProgress(1.0, std::wstring(wxString(_("Running Celeste")).wc_str(wxConvLocal)));
        };

        if(cpsToRemove.size()>0)
        {
            GlobalCmdHist::getInstance().addCommand(
                new PT::RemoveCtrlPointsCmd(*pano,cpsToRemove)
                );
        };

        progress.increaseProgress(1.0, std::wstring(wxString(_("Running Celeste")).wc_str(wxConvLocal)));
        wxMessageBox(wxString::Format(_("Removed %lu control points"), (unsigned long int) cpsToRemove.size()), _("Celeste result"),wxOK|wxICON_INFORMATION,this);
        DEBUG_TRACE("Finished running Celeste");
        MainFrame::Get()->SetStatusText(wxT(""),0);
    }
}

