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

#include "base_wx/platform.h"
#include <vector>
#include <map>

//#include <vigra_ext/PointMatching.h>
//#include <vigra_ext/LoweSIFT.h>

#include "hugin/ImagesPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/TextKillFocusHandler.h"
#include "base_wx/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/AutoCtrlPointCreator.h"
#include "hugin/config_defaults.h"
#include "base_wx/MyProgressDialog.h"

// Celeste header
#include "Celeste.h"
#include "CelesteGlobals.h"
#include "Utilities.h"

using namespace PT;
using namespace utils;
using namespace vigra;
using namespace vigra_ext;
using namespace std;

ImgPreview * canvas;

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
    EVT_BUTTON     ( XRCID("images_remove_cp"), ImagesPanel::OnRemoveCtrlPoints)
    EVT_BUTTON     ( XRCID("images_reset_pos"), ImagesPanel::OnResetImagePositions)
    EVT_BUTTON     ( XRCID("action_remove_images"),  ImagesPanel::OnRemoveImages)
    EVT_BUTTON     ( XRCID("images_move_image_down"),  ImagesPanel::OnMoveImageDown)
    EVT_BUTTON     ( XRCID("images_move_image_up"),  ImagesPanel::OnMoveImageUp)
    EVT_BUTTON	   ( XRCID("images_celeste_button"), ImagesPanel::OnCelesteButton)
    EVT_TEXT_ENTER ( XRCID("images_text_yaw"), ImagesPanel::OnYawTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_pitch"), ImagesPanel::OnPitchTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_roll"), ImagesPanel::OnRollTextChanged )
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
    m_removeCPButton = XRCCTRL(*this, "images_remove_cp", wxButton);
    DEBUG_ASSERT(m_removeCPButton);
    m_moveUpButton = XRCCTRL(*this, "images_move_image_up", wxButton);
    DEBUG_ASSERT(m_moveUpButton);
    m_moveDownButton = XRCCTRL(*this, "images_move_image_down", wxButton);
    DEBUG_ASSERT(m_moveDownButton);

    m_img_ctrls = XRCCTRL(*this, "image_control_panel", wxPanel);
    DEBUG_ASSERT(m_img_ctrls);

    m_CPDetectorChoice = XRCCTRL(*this, "cpdetector_settings", wxChoice);

    // Image Preview
    m_smallImgCtrl = XRCCTRL(*this, "images_selected_image", wxStaticBitmap);
    DEBUG_ASSERT(m_smallImgCtrl);

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_roll", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));

    m_empty.LoadFile(huginApp::Get()->GetXRCPath() +
                     wxT("data/") + wxT("druid.images.128.png"),
                     wxBITMAP_TYPE_PNG);
    wxStaticBitmap * bmp = XRCCTRL(*this, "images_selected_image", wxStaticBitmap);
    DEBUG_ASSERT(bmp);
    bmp->SetBitmap(m_empty);

    wxListEvent ev;
    ListSelectionChanged(ev);
    DEBUG_TRACE("end");

//    SetAutoLayout(false);

    wxConfigBase* config=wxConfigBase::Get();
    m_degDigits = config->Read(wxT("/General/DegreeFractionalDigitsEdit"),3);
    //read autopano generator settings
    cpdetector_config.Read(config);
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

    XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "images_text_roll", wxTextCtrl)->PopEventHandler(true);
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl)->PopEventHandler(true);
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
    if ( selImg.size() < 2) {
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
    wxMessageBox(wxString::Format(_("Added %d control points"), cps.size()), _("Autopano result"),wxOK|wxICON_INFORMATION,this);
    GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointsCmd(*pano, cps)
                                           );

};

// Yaw by text -> double
void ImagesPanel::OnYawTextChanged ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
        wxString text = XRCCTRL(*this, "images_text_yaw"
                                , wxTextCtrl) ->GetValue();
        if (text == wxT("")) {
            return;
        }
        DEBUG_INFO ("yaw = " << text );

        double val;
        if (!str2double(text, val)) {
//        if (!text.ToDouble(&val)) {
            wxLogError(_("Value must be numeric."));
            return;
        }
        ChangePano ( "y" , val );

    }
}

void ImagesPanel::OnPitchTextChanged ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
        wxString text = XRCCTRL(*this, "images_text_pitch"
                                , wxTextCtrl) ->GetValue();
        DEBUG_INFO ("pitch = " << text );
        if (text == wxT("")) {
            return;
        }

        double val;
        if (!str2double(text, val)) {
//        if (!text.ToDouble(&val)) {
            wxLogError(_("Value must be numeric."));
            return;
        }
        ChangePano ( "p" , val );
    }
}

void ImagesPanel::OnRollTextChanged ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
        wxString text = XRCCTRL(*this, "images_text_roll"
                                , wxTextCtrl) ->GetValue();
        DEBUG_INFO ("roll = " << text );
        if (text == wxT("")) {
            return;
        }

        double val;
        if (!str2double(text, val)) {
//        if (!text.ToDouble(&val)) {
            wxLogError(_("Value must be numeric."));
            return;
        }
        ChangePano ( "r" , val );
    }
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
    } else {
        // enable edit
        EnableImageCtrls();
        if (sel.size() == 1) {
            unsigned int imgNr = *(sel.begin());
            // single selection, show its parameters
            ShowImgParameters(imgNr);
            m_optAnchorButton->Enable();
            m_colorAnchorButton->Enable();
            m_moveDownButton->Enable();
            m_moveUpButton->Enable();
        } else {
            DEBUG_DEBUG("Multiselection, or no image selected");
            // multiselection, clear all values
            // we don't know which images parameters to show.
            ClearImgParameters();
            m_optAnchorButton->Disable();
            m_colorAnchorButton->Disable();
            m_moveDownButton->Disable();
            m_moveUpButton->Disable();
        }
    }
}


// ######  Here end the eventhandlers  #####

void ImagesPanel::DisableImageCtrls()
{
    // disable controls
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->Disable();
    XRCCTRL(*this, "images_selected_image", wxStaticBitmap)->
    SetBitmap(m_empty);
    m_optAnchorButton->Disable();
    m_colorAnchorButton->Disable();
    m_moveDownButton->Disable();
    m_moveUpButton->Disable();
    XRCCTRL(*this, "images_reset_pos", wxButton)->Disable();
    XRCCTRL(*this, "action_remove_images", wxButton)->Disable();
    XRCCTRL(*this, "images_celeste_button", wxButton)->Disable();
}

void ImagesPanel::EnableImageCtrls()
{
    // enable control if not already enabled
    if (XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->Enable()) {
        XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->Enable();
        XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->Enable();
        m_moveDownButton->Enable();
        m_moveUpButton->Enable();
        XRCCTRL(*this, "images_reset_pos", wxButton)->Enable();
        XRCCTRL(*this, "action_remove_images", wxButton)->Enable();
	XRCCTRL(*this, "images_celeste_button", wxButton)->Enable();
    }
}

void ImagesPanel::ShowImgParameters(unsigned int imgNr)
{
    const VariableMap & vars = pano->getImageVariables(imgNr);

    std::string val;
    val = doubleToString(const_map_get(vars,"y").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));

    val = doubleToString(const_map_get(vars,"p").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));

    val = doubleToString(const_map_get(vars,"r").getValue(),m_degDigits);
    XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->SetValue(wxString(val.c_str(), wxConvLocal));

    ShowImage(imgNr);
    ShowExifInfo(imgNr);
}


void ImagesPanel::ShowExifInfo(unsigned int imgNr)
{
    SrcPanoImage img = pano->getSrcImage(imgNr);

    double focalLength = 0;
    double cropFactor = 0;
    const bool applyExposureValue = FALSE;
    initImageFromFile(img,focalLength,cropFactor,applyExposureValue);

    std::string val;
    val = img.getFilename();
    XRCCTRL(*this, "images_filename",wxStaticText) ->
        SetLabel(wxFileName(wxString(val.c_str(),HUGIN_CONV_FILENAME)).GetFullName());

    val = img.getExifMake();
    XRCCTRL(*this, "images_camera_make",wxStaticText) ->
        SetLabel(wxString(val.c_str(),wxConvLocal));

    val = img.getExifModel();
    XRCCTRL(*this, "images_camera_model",wxStaticText) ->
        SetLabel(wxString(val.c_str(),wxConvLocal));

    val = img.getExifDate();
    XRCCTRL(*this, "images_capture_date",wxStaticText) ->
        SetLabel(wxString(val.c_str(),wxConvLocal));

    val = doubleToString(img.getExifFocalLength(),1);
    XRCCTRL(*this, "images_focal_length",wxStaticText) ->
        SetLabel(wxString(val.c_str(),wxConvLocal));

    val = doubleToString(img.getExifAperture(),1);
    XRCCTRL(*this, "images_aperture",wxStaticText) ->
        SetLabel(wxString(val.c_str(),wxConvLocal));

    val = doubleToString(img.getExifExposureTime(),5);
    XRCCTRL(*this, "images_shutter_speed",wxStaticText) ->
        SetLabel(wxString(val.c_str(),wxConvLocal));
}


void ImagesPanel::ClearImgParameters()
{
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->Clear();

    XRCCTRL(*this, "images_selected_image", wxStaticBitmap)->
        SetBitmap(m_empty);

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
    ImageCache::EntryPtr cacheEntry = ImageCache::getInstance().getSmallImage(
            pano->getImage(m_showImgNr).getFilename());
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
    wxImage scaled = img.Scale(sz.GetWidth(),sz.GetHeight());
    m_smallImgCtrl->SetBitmap(wxBitmap(scaled));
    m_smallImgCtrl->Refresh();
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
            vars[i].insert(make_pair("r", Variable("r",0.0)));
            i++;
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateImagesVariablesCmd( *pano, selImg, vars ));
    }

}

void ImagesPanel::OnRemoveImages(wxCommandEvent & e)
{
    DEBUG_TRACE("");

    UIntSet selImg = images_list->GetSelected();
    vector<string> filenames;
    for (UIntSet::iterator it = selImg.begin(); it != selImg.end(); ++it) {
        filenames.push_back(pano->getImage(*it).getFilename());
    }
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
    int r =wxMessageBox(wxString::Format(_("Really Delete %d control points?"),
                                         cpsToDelete.size()),
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
    if ( selImg.size() == 1) {
        unsigned int i1 = *selImg.begin();
        unsigned int i2 = i1+1;
        if (i2 < pano->getNrOfImages() ) {
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

void ImagesPanel::ReloadCPDetectorSettings()
{
    cpdetector_config.Read(); 
    cpdetector_config.FillControl(m_CPDetectorChoice,true);
    m_CPDetectorChoice->InvalidateBestSize();
    m_CPDetectorChoice->GetParent()->Layout();
    Refresh();
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

void ImagesPanel::OnCelesteButton(wxCommandEvent & e)
{
    const UIntSet & selImg = images_list->GetSelected();
    unsigned int total_removed = 0;

    if ( selImg.size() == 0)
    {
        DEBUG_WARN("Cannot run celeste without at least one point");
    }
    else
    {	
        ProgressReporterDialog progress(selImg.size()+1, _("Running Celeste"), _("Running Celeste"),this);

        DEBUG_TRACE("Running Celeste");
        // set numeric locale to C, for correct number output
        char * old_locale = setlocale(LC_NUMERIC,NULL);
        setlocale(LC_NUMERIC,"C");

        // determine file name of SVM model file
        // get XRC path from application
        wxString wxstrModelFileName = huginApp::Get()->GetDataPath() + wxT(HUGIN_CELESTE_MODEL);
        // convert wxString to string
        string strModelFileName(wxstrModelFileName.mb_str(wxConvUTF8));
		
        // SVM model file
        if (! wxFile::Exists(wxstrModelFileName) ) {
            wxMessageBox(_("Celeste model expected in ") + wxstrModelFileName +_(" not found, Hugin needs to be properly installed." ), _("Fatal Error"));
            return ;
        }

        for (UIntSet::const_iterator itr = selImg.begin(); itr != selImg.end(); ++itr) {

            progress.increaseProgress(1.0, std::wstring(wxString(_("Running Celeste")).wc_str(wxConvLocal)));

            const CPVector & controlPoints = pano->getCtrlPoints();
            unsigned int removed = 0;
            const unsigned int imgNr = *itr;

            gNumLocs = 0;
            for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
                PT::ControlPoint point = *it;
                if (imgNr == point.image1Nr){
                    gNumLocs++;				
                }
                if (imgNr == point.image2Nr){
                    gNumLocs++;				
                }
            }		

            // Create the storage matrix
            gLocations = CreateMatrix( (int)0, gNumLocs, 2);
            unsigned int glocation_counter = 0;
            unsigned int cp_counter = 0;	
            vector<unsigned int> global_cp_nr;

            for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
                PT::ControlPoint point = *it;

                if (imgNr == point.image1Nr){
                    //cout << "---imgNr = " << imgNr << " point.image1Nr = " << point.image1Nr << endl;	
                    gLocations[glocation_counter][0] = (int)point.x1;
                    gLocations[glocation_counter][1] = (int)point.y1;
                    global_cp_nr.push_back(cp_counter);	
                    glocation_counter++;				
                }
                if (imgNr == point.image2Nr){
                    //cout << "---imgNr = " << imgNr << " point.image1Nr = " << point.image1Nr << endl;	
                    gLocations[glocation_counter][0] = (int)point.x2;
                    gLocations[glocation_counter][1] = (int)point.y2;
                    global_cp_nr.push_back(cp_counter);	
                    glocation_counter++;				
                }
                cp_counter++;	
            }

            // SVM threshold
            double threshold = HUGIN_CELESTE_THRESHOLD;
            wxConfigBase::Get()->Read(wxT("/Celeste/Threshold"), &threshold, HUGIN_CELESTE_THRESHOLD);

            // Mask resolution - 1 sets it to fine
            bool t = (wxConfigBase::Get()->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER) != 0);
            if (t){
                //cerr <<"---Celeste--- Using small filter" << endl;
                gRadius = 10;
                spacing = (gRadius * 2) + 1;
            }

            // Image to analyse
            string imagefile = pano->getImage(*itr).getFilename();

            // Print progress
            MainFrame::Get()->SetStatusText(_("searching for cloud-like control points..."),0);

            // Vector to store Gabor filter responses
            vector<double> svm_responses_im;
            string mask_format = "PNG";
            unsigned int mask = 0;

            // Get responses
            get_gabor_response(imagefile, mask, strModelFileName, threshold, mask_format, svm_responses_im);

            MainFrame::Get()->SetStatusText(_("classifying control points..."),0);

            // Print SVM results
            for (unsigned int c = 0; c < svm_responses_im.size(); c++){

                unsigned int pNr = global_cp_nr[c] - removed;	

                if (svm_responses_im[c] >= threshold){

                    DEBUG_DEBUG("about to delete point " << pNr);
                    GlobalCmdHist::getInstance().addCommand(
                        new PT::RemoveCtrlPointCmd(*pano,pNr)
                        );
                    removed++;	
                    total_removed++;					
                    cout << "CP: " << c << "\tSVM Score: " << svm_responses_im[c] << "\tremoved." << endl;
                }
            }
            if (removed)
            {
                cout << endl;
            }
        }
        MainFrame::Get()->SetStatusText(_(""),0);

        // reset locale
        setlocale(LC_NUMERIC,old_locale);	
    }

    wxMessageBox(wxString::Format(_("Removed %d control points"), total_removed), _("Celeste result"),wxOK|wxICON_INFORMATION,this);
}

