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
#include "hugin/Events.h"
#include "PT/Panorama.h"

using namespace PT;

// Image Icons
extern wxImageList * img_icons;
// Image Icons biger
extern wxImageList * img_bicons;

// image preview
extern wxBitmap * p_img;

// pointer to the list control
List* images_list2;

ImgPreview * lens_canvas;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(LensPanel, wxWindow) //wxEvtHandler)
//    EVT_LIST_ITEM_SELECTED( XRCID("images_list2_unknown"), LensPanel::itemSelected )
    EVT_LIST_ITEM_SELECTED ( XRCID("images_list2_unknown"),LensPanel::LensChanged )
/*    EVT_COMBOBOX ( XRCID("lens_type_combobox"), LensPanel::LensTypeChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_HFOV"), LensPanel::HFOVChanged )
    EVT_TEXT_ENTER ( XRCID("lensval_focalLength"),LensPanel::focalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_a"), LensPanel::aChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_b"), LensPanel::bChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_c"), LensPanel::cChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_d"), LensPanel::dChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_e"), LensPanel::eChanged )*/
END_EVENT_TABLE()


// Define a constructor for the Images Panel
LensPanel::LensPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : pano(*pano)
{
    DEBUG_TRACE("ctor");
    pano->addObserver(this);

    // The following control creates itself. We dont care about xrc loading.
    images_list2 = new List (parent, pano, lens_layout);
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_list2_unknown"),
               images_list2 );
    images_list2->AssignImageList(img_icons, wxIMAGE_LIST_SMALL );

    // This controls must called by xrc handler and after it we play with it.
    // It could as well be done in the LensEdit class.
    lens_edit    = new LensEdit (parent, pano);
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("lens_list_unknown"),
               lens_edit);
//               wxXmlResource::Get()->LoadPanel (parent, wxT("lens_dialog")) );

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

void LensPanel::LensChanged ( wxListEvent & e )
{
    image = e.GetIndex();

    lens_edit->LensChanged ( e );
    DEBUG_INFO ( "hier is item: " << wxString::Format("%d", image) );
}

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(LensEdit, wxWindow) //wxEvtHandler)
//    EVT_LIST_ITEM_SELECTED( XRCID("images_list2_unknown"), LensPanel::itemSelected )
//    EVT_LIST_ITEM_SELECTED ( XRCID("images_list2_unknown"),LensPanel::LensChanged )
    EVT_COMBOBOX ( XRCID("lens_type_combobox"), LensEdit::LensTypeChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_HFOV"), LensEdit::HFOVChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_focalLength"),LensEdit::focalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_a"), LensEdit::aChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_b"), LensEdit::bChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_c"), LensEdit::cChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_d"), LensEdit::dChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_e"), LensEdit::eChanged )
END_EVENT_TABLE()

LensEdit::LensEdit(wxWindow *parent, /*const wxPoint& pos, const wxSize& size,*/ Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, 0),//)wxSUNKEN_BORDER),
      pano(*pano)
{
    pano->addObserver(this);

    // connects the ProjectionFormat/PanoramaMemento.h ComboBox
//    ProjectionFormat_cb = XRCCTRL(*this, "lens_type_combobox", wxComboBox);

    // show the lens editing controls
    wxXmlResource::Get()->LoadPanel (this, wxT("lens_dialog"));
//    wxXmlResource::Get()->AttachUnknownControl (
//               wxT("lens_list_unknown"),
//               wxXmlResource::Get()->LoadPanel (this, wxT("lens_dialog")) );

    // intermediate event station
    PushEventHandler( new MyEvtHandler((size_t) 2) );

    edit_Lens = new Lens ();
    AddLensCmd ( *pano, *edit_Lens );
    // FIXME with no image in pano hugin crashes
    DEBUG_INFO ( wxString::Format ("Lensesnumber: %d", pano->getNrOfLenses()) )

    DEBUG_TRACE("");;

}


LensEdit::~LensEdit(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
    DEBUG_TRACE("");
}

void LensEdit::LensChanged ( wxListEvent & e )
{
    image = e.GetIndex();

    // FIXME should get a bottom to update
    edit_Lens->readEXIF( pano.getImage(image).getFilename().c_str() );

    // FIXME support separate lenses
    edit_Lens->update ( pano.getLens(pano.getImage(image).getLens()) );

    XRCCTRL(*this, "lens_type_combobox", wxComboBox)->SetSelection(
                   edit_Lens->projectionFormat
           );
    // FIXME format better
    XRCCTRL(*this, "lens_val_HFOV", wxTextCtrl)->SetValue(
                   wxString::Format ( "%f", edit_Lens->HFOV)
           );
    // FIXME format better
    XRCCTRL(*this, "lens_val_focalLength", wxTextCtrl)->SetValue(
                   wxString::Format ( "%f", edit_Lens->focalLength )
           );
    XRCCTRL(*this, "lens_val_a", wxTextCtrl)->SetValue(
                   wxString::Format ( "%f", edit_Lens->a )
           );
    XRCCTRL(*this, "lens_val_b", wxTextCtrl)->SetValue(
                   wxString::Format ( "%f", edit_Lens->b )
           );
    XRCCTRL(*this, "lens_val_c", wxTextCtrl)->SetValue(
                   wxString::Format ( "%f", edit_Lens->c )
           );
    XRCCTRL(*this, "lens_val_d", wxTextCtrl)->SetValue(
                   wxString::Format ( "%f", edit_Lens->d )
           );
    XRCCTRL(*this, "lens_val_e", wxTextCtrl)->SetValue(
                   wxString::Format ( "%f", edit_Lens->e )
           );

    DEBUG_INFO ( "hier is item: " << wxString::Format("%d", image) );
}

void LensEdit::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
/*    XRCCTRL(*this, "lens_type_combobox", wxComboBox)->SetSelection(
                   pano.getLens(pano.getImage(image).getLens()).projectionFormat
           );*/
    int lt = XRCCTRL(*this, "lens_type_combobox", wxComboBox)->GetSelection();
    DEBUG_INFO ( wxString::Format ("image %d Lenstype %d",image, lt) )
}


void LensEdit::LensTypeChanged ( wxCommandEvent & e )
{
    // uses enum ProjectionFormat from PanoramaMemento.h
    int lt = XRCCTRL(*this, "lens_type_combobox",
                                   wxComboBox)->GetSelection();
    edit_Lens->projectionFormat = (ProjectionFormat) (lt);

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(image).getLens(),*edit_Lens )
        );
//    int image = images_list2->GetSelectedImage();
    DEBUG_INFO ( wxString::Format ("image %d Lenstype %d",image,lt) )
}

void LensEdit::HFOVChanged ( wxCommandEvent & e )
{
    // FIXME beautify this function and write a macro
    double * val = new double ();
    wxString text = XRCCTRL(*this, "lens_val_HFOV", wxTextCtrl)->GetValue();
    text.ToDouble( val );

    edit_Lens->HFOV = *val;

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(image).getLens(),*edit_Lens )
        );

    DEBUG_TRACE ("")
}

void LensEdit::focalLengthChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    wxString text=XRCCTRL(*this,"lens_val_focalLength", wxTextCtrl)->GetValue();
    text.ToDouble( val );

    edit_Lens->focalLength = *val;

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(image).getLens(),*edit_Lens )
        );

    DEBUG_TRACE ("")
}

void LensEdit::aChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    wxString text = XRCCTRL(*this, "lens_val_a", wxTextCtrl)->GetValue();
    text.ToDouble( val );

    edit_Lens->a = *val;

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(image).getLens(),*edit_Lens )
        );

    DEBUG_TRACE ("")
}

void LensEdit::bChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    wxString text = XRCCTRL(*this, "lens_val_b", wxTextCtrl)->GetValue();
    text.ToDouble( val );

    edit_Lens->b = *val;

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(image).getLens(),*edit_Lens )
        );

    DEBUG_TRACE ("")
}

void LensEdit::cChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    wxString text = XRCCTRL(*this, "lens_val_c", wxTextCtrl)->GetValue();
    text.ToDouble( val );

    edit_Lens->c = *val;

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(image).getLens(),*edit_Lens )
        );

    DEBUG_TRACE ("")
}

void LensEdit::dChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    wxString text = XRCCTRL(*this, "lens_val_d", wxTextCtrl)->GetValue();
    text.ToDouble( val );

    edit_Lens->d = *val;

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(image).getLens(),*edit_Lens )
        );

    DEBUG_TRACE ("")
}

void LensEdit::eChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    wxString text = XRCCTRL(*this, "lens_val_e", wxTextCtrl)->GetValue();
    text.ToDouble( val );

    edit_Lens->e = *val;

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(image).getLens(),*edit_Lens )
        );

    DEBUG_TRACE ("")
}


