// -*- c-basic-offset: 4 -*-

/** @file LensPanel.cpp
 *
 *  @brief implementation of LensPanel Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de>
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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>
#include <wx/imaglist.h>

#include "PT/PanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/List.h"
#include "hugin/LensPanel.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "PT/Panorama.h"

using namespace PT;

// Image Icons
extern wxImageList * img_icons;
// Image Icons biger
extern wxImageList * img_bicons;

// image preview
extern wxBitmap * p_img;

ImgPreview * lens_canvas;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(LensPanel, wxEvtHandler)
//    EVT_LIST_ITEM_SELECTED( XRCID("images_list2_unknown"), LensPanel::itemSelected )
    EVT_LEFT_DOWN ( LensPanel::ChangePreview )
    EVT_COMBOBOX ( XRCID("lens_type_combobox"), LensPanel::LensTypeChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_HFOV"), LensPanel::HFOVChanged )
    EVT_TEXT_ENTER ( XRCID("lensval_focalLength"),LensPanel::focalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_a"), LensPanel::aChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_b"), LensPanel::bChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_c"), LensPanel::cChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_d"), LensPanel::dChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_e"), LensPanel::eChanged )
END_EVENT_TABLE()


// Define a constructor for the Images Panel
LensPanel::LensPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : pano(*pano)
{
    pano->addObserver(this);

    images_list2 = new List (parent, pano, lens_layout);
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_list2_unknown"),
               images_list2 );
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("lens_list_unknown"),
               wxXmlResource::Get()->LoadPanel (parent, wxT("lens_dialog")) );

    images_list2->AssignImageList(img_icons, wxIMAGE_LIST_SMALL );

    // connects the ProjectionFormat/PanoramaMemento.h ComboBox
//    wxXmlResource::Get()->LoadObject (cb, parent, wxT("lens_type_combobox"), wxT("wxComboBox") );
    cb = XRCCTRL( *parent, "lens_type_combobox", wxComboBox );

    p_img = new wxBitmap( 0, 0 );
    DEBUG_TRACE("");;

}


LensPanel::~LensPanel(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
    delete images_list2;
    DEBUG_TRACE("");
}


void LensPanel::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
}

void LensPanel::ChangePreview ( wxListEvent & e )
{
    DEBUG_TRACE ("")
    long item (1);
    DEBUG_INFO ( "hier: is item %ld" << wxString::Format("%ld", item) );
}

void LensPanel::LensTypeChanged ( wxCommandEvent & e )
{
    // uses enum ProjectionFormat from PanoramaMemento.h
    cb->GetSelection();
    int image = images_list2->GetSelectedImage();
    DEBUG_INFO ( wxString::Format ("%d",image) )
}

void LensPanel::HFOVChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
}

void LensPanel::focalLengthChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
}

void LensPanel::aChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
}

void LensPanel::bChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
}

void LensPanel::cChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
}

void LensPanel::dChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
}

void LensPanel::eChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE ("")
}


