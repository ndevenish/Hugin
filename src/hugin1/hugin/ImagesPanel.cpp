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

#include "hugin/ImagesPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/ImagesList.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "hugin/PanoOperationTree.h"
#include "hugin/PanoOperation.h"

#include <panodata/StandardImageVariableGroups.h>

using namespace PT;
using namespace hugin_utils;
using namespace vigra;
using namespace vigra_ext;
using namespace std;

#define m_XRCID(str_id) \
    wxXmlResource::GetXRCID(str_id)
#define m_XRCCTRL(window, id, type) \
    ((type*)((window).FindWindow(m_XRCID(id))))


//------------------------------------------------------------------------------
#define GET_VAR(val) pano->getVariable(orientationEdit_RefImg).val.getValue()

BEGIN_EVENT_TABLE(ImagesPanel, wxPanel)
    EVT_LIST_ITEM_SELECTED( XRCID("images_list_unknown"),
                            ImagesPanel::ListSelectionChanged )
    EVT_LIST_ITEM_DESELECTED( XRCID("images_list_unknown"),
                            ImagesPanel::ListSelectionChanged )
    EVT_BUTTON     ( XRCID("images_reset_pos"), ImagesPanel::OnResetImagePositions)
#ifndef __WXGTK__
    EVT_BUTTON     ( XRCID("action_remove_images"),  ImagesPanel::OnRemoveImages)
#endif
    EVT_BUTTON     ( XRCID("images_move_image_down"),  ImagesPanel::OnMoveImageDown)
    EVT_BUTTON     ( XRCID("images_move_image_up"),  ImagesPanel::OnMoveImageUp)
    EVT_BUTTON     ( XRCID("images_execute_operation"), ImagesPanel::OnExecuteOperation)
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
    pano = NULL;
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

    m_moveUpButton = XRCCTRL(*this, "images_move_image_up", wxButton);
    DEBUG_ASSERT(m_moveUpButton);
    m_moveDownButton = XRCCTRL(*this, "images_move_image_down", wxButton);
    DEBUG_ASSERT(m_moveDownButton);
    m_linkCheckBox = XRCCTRL(*this, "images_check_link", wxCheckBox);
    DEBUG_ASSERT(m_linkCheckBox);
    m_operationTree = XRCCTRL(*this, "images_pano_operation_tree", PanoOperationTreeCtrl);
    DEBUG_ASSERT(m_operationTree);
    m_operationTree->GenerateTree();

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

//    SetAutoLayout(false);

    wxConfigBase* config=wxConfigBase::Get();
    m_degDigits = config->Read(wxT("/General/DegreeFractionalDigitsEdit"),3);
    //read autopano generator settings
    cpdetector_config.Read(config,huginApp::Get()->GetDataPath()+wxT("default.setting"));
    //write current autopano generator settings
    cpdetector_config.Write(config);
    config->Flush();
    m_operationTree->UpdateCPDetectorItems(cpdetector_config);
    Layout();
    wxListEvent ev;
    ListSelectionChanged(ev);
    DEBUG_TRACE("end");

    return true;
}

void ImagesPanel::Init(Panorama * panorama)
{
    pano = panorama;
    images_list->Init(pano);
    // observe the panorama
    pano->addObserver(this);
}

#ifdef HUGIN_HSI
void ImagesPanel::AddPythonOperations(PluginItems items)
{
    m_operationTree->GeneratePythonTree(items);
    UIntSet images;
    m_operationTree->UpdateState(*pano,images);
};
#endif

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
    };

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
    m_operationTree->UpdateState(pano,selected);

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
            m_moveDownButton->Enable();
            m_moveUpButton->Enable();
        } else {
            DEBUG_DEBUG("Multiselection, or no image selected");
            // multiselection, clear all values
            // we don't know which images parameters to show.
            ClearImgParameters();
            m_moveDownButton->Disable();
            m_moveUpButton->Disable();
        }
    }
    if(pano!=NULL)
    {
        m_operationTree->UpdateState(*pano,sel);
    }
    else
    {
        Panorama pano2;
        UIntSet images;
        m_operationTree->UpdateState(pano2,images);
    };
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
    m_moveDownButton->Disable();
    m_moveUpButton->Disable();
    XRCCTRL(*this, "images_reset_pos", wxButton)->Disable();
#ifndef __WXGTK__
    XRCCTRL(*this, "action_remove_images", wxButton)->Disable();
#endif
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
#ifndef __WXGTK__
        XRCCTRL(*this, "action_remove_images", wxButton)->Enable();
#endif
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
    XRCCTRL(*this, "images_camera_make",wxStaticText) ->
        SetLabel(wxString(val.c_str(),wxConvLocal));

    val = img.getExifModel();
    XRCCTRL(*this, "images_camera_model",wxStaticText) ->
        SetLabel(wxString(val.c_str(),wxConvLocal));

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
    XRCCTRL(*this, "images_capture_date", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_focal_length", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_aperture", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_shutter_speed", wxStaticText) ->SetLabel(wxT(""));
    XRCCTRL(*this, "images_iso",wxStaticText)->SetLabel(wxT(""));
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
    UIntSet selImg = images_list->GetSelected();
    RemoveImageOperation remove;
    PT::PanoCommand* cmd=remove.GetCommand(this,*pano,selImg);
    if(cmd!=NULL)
    {
        DEBUG_TRACE("Sending remove images command");
        GlobalCmdHist::getInstance().addCommand(cmd);
    };
};

void ImagesPanel::OnMoveImageDown(wxCommandEvent & e)
{
    UIntSet selImg = images_list->GetSelected();
    if ( selImg.size() == 1) {
        unsigned int i1 = *selImg.begin();
        unsigned int i2 = i1+1;
        if (i2 < pano->getNrOfImages() ) {
            images_list->SetItemState(i1,0,wxLIST_STATE_SELECTED);
            GlobalCmdHist::getInstance().addCommand(
                new SwapImagesCmd(*pano,i1, i2)
            );
            // set new selection
            images_list->SelectSingleImage(i2);
            // Bring the focus back to the button.
            m_moveDownButton->CaptureMouse();
            m_moveDownButton->ReleaseMouse();
        }
    }
}

void ImagesPanel::OnMoveImageUp(wxCommandEvent & e)
{
    UIntSet selImg = images_list->GetSelected();
    if ( selImg.size() == 1) {
        unsigned int i1 = *selImg.begin();
        unsigned int i2 = i1 -1;
        if (i1 > 0) {
            images_list->SetItemState(i1,0,wxLIST_STATE_SELECTED);
            GlobalCmdHist::getInstance().addCommand(
                new SwapImagesCmd(*pano,i1, i2)
            );
            // set new selection
            images_list->SelectSingleImage(i2);
            // Bring the focus back to the button.
            m_moveUpButton->CaptureMouse();
            m_moveUpButton->ReleaseMouse();
        }
    }
}

void ImagesPanel::OnExecuteOperation(wxCommandEvent & e)
{
    wxTreeItemId id=m_operationTree->GetSelection();
    if(id.IsOk())
    {
        PanoOperation* operation=(PanoOperation*)(m_operationTree->GetItemData(id));
        if(operation!=NULL)
        {
            PT::PanoCommand* cmd=operation->GetCommand(this,*pano,images_list->GetSelected());
            if(cmd!=NULL)
            {
                GlobalCmdHist::getInstance().addCommand(cmd);
            };
        };
    };
};

void ImagesPanel::ReloadCPDetectorSettings()
{
    cpdetector_config.Read(); 
    m_operationTree->UpdateCPDetectorItems(cpdetector_config);
    m_operationTree->UpdateState(*pano,images_list->GetSelected());
};

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

