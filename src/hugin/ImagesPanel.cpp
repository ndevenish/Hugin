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

#include "panoinc.h"
#include "panoinc_WX.h"
#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>    // needed on mingw
#include <wx/imaglist.h>
#include <wx/spinctrl.h>

#include "hugin/ImagesPanel.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/ImageOrientationFrame.h"

using namespace PT;
using namespace utils;

ImgPreview * canvas;

//------------------------------------------------------------------------------
#define GET_VAR(val) pano.getVariable(orientationEdit_RefImg).val.getValue()

BEGIN_EVENT_TABLE(ImagesPanel, wxWindow)
    EVT_SIZE   ( ImagesPanel::FitParent )
//    EVT_MOUSE_EVENTS ( ImagesPanel::OnMouse )
//    EVT_MOTION ( ImagesPanel::ChangePreview )
    EVT_LIST_ITEM_SELECTED( XRCID("images_list_unknown"),
                            ImagesPanel::ListSelectionChanged )
    EVT_LIST_ITEM_SELECTED( XRCID("images_list_unknown"),
                            ImagesPanel::ListSelectionChanged )
    EVT_BUTTON     ( XRCID("images_opt_anchor_button"), ImagesPanel::OnOptAnchorChanged)
    EVT_BUTTON     ( XRCID("images_set_orientation_button"), ImagesPanel::OnSelectAnchorPosition)
    EVT_BUTTON     ( XRCID("images_color_anchor_button"), ImagesPanel::OnColorAnchorChanged)
    EVT_TEXT_ENTER ( XRCID("images_text_yaw"), ImagesPanel::OnYawTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_pitch"), ImagesPanel::OnPitchTextChanged )
    EVT_TEXT_ENTER ( XRCID("images_text_roll"), ImagesPanel::OnRollTextChanged )
END_EVENT_TABLE()


// Define a constructor for the Images Panel
ImagesPanel::ImagesPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(*pano)
{
    DEBUG_TRACE("");

    wxXmlResource::Get()->LoadPanel (this, wxT("images_panel"));
    DEBUG_TRACE("");

    images_list = new ImagesListImage (parent, pano);
    wxXmlResource::Get()->AttachUnknownControl (
        "images_list_unknown",
        images_list );

    DEBUG_TRACE("");

    m_optAnchorButton = XRCCTRL(*this, "images_opt_anchor_button", wxButton);
    DEBUG_ASSERT(m_optAnchorButton);

    m_setAnchorOrientButton = XRCCTRL(*this, "images_set_orientation_button", wxButton);
    DEBUG_ASSERT(m_setAnchorOrientButton);
    m_colorAnchorButton = XRCCTRL(*this, "images_color_anchor_button", wxButton);
    DEBUG_ASSERT(m_colorAnchorButton);
    // Image Preview

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_roll", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl)->PushEventHandler(new TextKillFocusHandler(this));

    m_empty.LoadFile(MainFrame::Get()->GetXRCPath() +
                     "/data/" "druid.images.128.png",
                     wxBITMAP_TYPE_PNG);
    XRCCTRL(*this, "images_selected_image", wxStaticBitmap)->
        SetBitmap(m_empty);

    wxListEvent ev;
    ListSelectionChanged(ev);
    pano->addObserver(this);
    DEBUG_TRACE("end");
}


ImagesPanel::~ImagesPanel(void)
{
    DEBUG_TRACE("dtor");

    // FIXME crashes.. don't know why
/*
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->PopEventHandler();
    XRCCTRL(*this, "images_text_roll", wxTextCtrl)->PopEventHandler();
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl)->PopEventHandler();
    delete(m_tkf);
*/
    pano.removeObserver(this);
    delete images_list;
    DEBUG_TRACE("dtor end");
}

void ImagesPanel::FitParent( wxSizeEvent & e )
{
    wxSize new_size = GetSize();
    XRCCTRL(*this, "images_panel", wxPanel)->SetSize ( new_size );
//    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}


void ImagesPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & _imgNr)
{
    DEBUG_TRACE("");

    // update text field if selected
    const UIntSet & selected = images_list->GetSelected();
    if (selected.size() == 1 &&
        *(selected.begin()) < pano.getNrOfImages() &&
        set_contains(_imgNr, *(selected.begin())))
    {
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
        new PT::SetVariableCmd(pano, images_list->GetSelected(), img_var)
        );
}


// #####  Here start the eventhandlers  #####

// Yaw by text -> double
void ImagesPanel::OnYawTextChanged ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
        wxString text = XRCCTRL(*this, "images_text_yaw"
                                , wxTextCtrl) ->GetValue();
        DEBUG_INFO ("yaw = " << text );

        double val;
        if (!text.ToDouble(&val)) {
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

        double val;
        if (!text.ToDouble(&val)) {
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

        double val;
        if (!text.ToDouble(&val)) {
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
        PanoramaOptions opt = pano.getOptions();
        opt.optimizeReferenceImage = *(sel.begin());

        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
    }
}

void ImagesPanel::OnSelectAnchorPosition(wxCommandEvent & e)
{
    // first, change anchor
    OnOptAnchorChanged(e);

    // open a frame to show the image
    ImageOrientationFrame * t = new ImageOrientationFrame(this, pano);
    t->Show();
}

void ImagesPanel::OnColorAnchorChanged(wxCommandEvent &e )
{
    const UIntSet & sel = images_list->GetSelected();
    if ( sel.size() == 1 ) {
        // set first image to be the anchor
        PanoramaOptions opt = pano.getOptions();
        opt.colorReferenceImage = *(sel.begin());

        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
    }
}


void ImagesPanel::ListSelectionChanged(wxListEvent & e)
{
	DEBUG_TRACE(e.GetIndex());
    const UIntSet & sel = images_list->GetSelected();
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
            m_setAnchorOrientButton->Enable();
            m_colorAnchorButton->Enable();
        } else {
            // multiselection, clear all values
            // we don't know which images parameters to show.
            ClearImgParameters();
            m_optAnchorButton->Disable();
            m_setAnchorOrientButton->Disable();
            m_colorAnchorButton->Disable();
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
    m_setAnchorOrientButton->Disable();
}

void ImagesPanel::EnableImageCtrls()
{
    // enable control if not already enabled
    if (XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->Enable()) {
        XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->Enable();
        XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->Enable();
    }
}

void ImagesPanel::ShowImgParameters(unsigned int imgNr)
{
    const VariableMap & vars = pano.getImageVariables(imgNr);

    std::string val;
    val = doubleToString(const_map_get(vars,"y").getValue());
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl) ->SetValue(val.c_str());

    val = doubleToString(const_map_get(vars,"p").getValue());
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->SetValue(val.c_str());

    val = doubleToString(const_map_get(vars,"r").getValue());
    XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->SetValue(val.c_str());

    ShowImage(imgNr);
}

void ImagesPanel::ClearImgParameters()
{
    XRCCTRL(*this, "images_text_yaw", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_roll", wxTextCtrl) ->Clear();
    XRCCTRL(*this, "images_text_pitch", wxTextCtrl) ->Clear();

    XRCCTRL(*this, "images_selected_image", wxStaticBitmap)->
        SetBitmap(m_empty);
}

void ImagesPanel::ShowImage(unsigned int imgNr)
{
    // show preview image
    wxStaticBitmap * imgctrl = XRCCTRL(*this, "images_selected_image", wxStaticBitmap);
    DEBUG_ASSERT(imgctrl);
    wxSize sz = imgctrl->GetSize();
    const wxImage * img = ImageCache::getInstance().getSmallImage(
        pano.getImage(imgNr).getFilename());

    double sRatio = (double)sz.GetWidth() / sz.GetHeight();
    double iRatio = (double)img->GetWidth() / img->GetHeight();

    int w,h;
    if (iRatio > sRatio) {
        // image is wider than screen, display landscape
        w = sz.GetWidth();
        h = (int) (w / iRatio);
    } else {
        // portrait
        h = sz.GetHeight();
        w = (int) (h * iRatio);
    }
    wxImage scaled = img->Scale(w,h);
    imgctrl->SetBitmap(scaled.ConvertToBitmap());
}

