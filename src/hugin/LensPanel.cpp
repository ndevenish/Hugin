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
using namespace utils;

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
    EVT_LIST_ITEM_SELECTED ( XRCID("images_list2_unknown"),LensPanel::LensChanged )
END_EVENT_TABLE()

// The lens of the pano for reading - the local edit_Lens is for writing.
#define EDIT_LENS pano.getLens (pano.getImage(lensEdit_RefImg).getLens())

#define SET_WXTEXTCTRL_TEXT( xrc_name , variable_name )                        \
{                                                                              \
    std::string number;                                                        \
    if ( ( wxString::Format ("%f", EDIT_LENS.variable_name) != "nan" ) &&      \
         ( EDIT_LENS.variable_name != 0.0 ) ) {                                \
      number =        doubleToString ( EDIT_LENS.variable_name );              \
      XRCCTRL(*this, xrc_name, wxTextCtrl)->SetValue( number.c_str() );        \
    } else {                                                                   \
      XRCCTRL(*this, xrc_name, wxTextCtrl)->SetValue( "" ) ;                   \
    }                                                                          \
    DEBUG_INFO( xrc_name << " = " << number);                                  \
}

// Define a constructor for the Lenses Panel
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
    DEBUG_TRACE("");
}

void LensPanel::LensChanged ( wxListEvent & e )
{
    DEBUG_TRACE("");
    lensEdit_RefImg = e.GetIndex();

    lens_edit->SetImages ( e );
//    DEBUG_INFO ( "hier is item: " << wxString::Format("%d", lensEdit_RefImg) );
}

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(LensEdit, wxWindow) //wxEvtHandler)
//    EVT_LIST_ITEM_SELECTED( XRCID("images_list2_unknown"), LensEdit::SetImages )
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

    // connects the LensProjectionFormat/PanoramaMemento.h ComboBox
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
    lensGui_dirty = FALSE;
    // FIXME with no image in pano hugin crashes
    DEBUG_INFO ( wxString::Format ("Lensesnumber: %d", pano->getNrOfLenses()) )

    for ( int i = 0 ; i < 512 ; i++ )
      imgNr[i] = 0;

    DEBUG_TRACE("");;
}


LensEdit::~LensEdit(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
    DEBUG_TRACE("");
}


/*void LensEdit::LensChanged ( wxListEvent & e )
{
//    DEBUG_TRACE("####################################################################################");
}*/

void LensEdit::updateHFOV()
{
//    DEBUG_TRACE("");

/*    if ( ( wxString::Format ("%f", edit_Lens->HFOV) != "nan" ) &&
         ( edit_Lens->HFOV != 0.0 ) ) {
      number =        doubleToString (
                        pano.getLens (pano.getImage(lensEdit_RefImg).getLens()).
                 HFOV );
      XRCCTRL(*this, "lens_val_HFOV", wxTextCtrl)->SetValue( number.c_str() );*/
    SET_WXTEXTCTRL_TEXT( "lens_val_HFOV" , HFOV )
//    }

    std::string number;
    if ( EDIT_LENS.focalLength != 0 ) {
      number = doubleToString ( EDIT_LENS.  focalLength *
                                EDIT_LENS.  focalLengthConversionFactor  );
      XRCCTRL(*this, "lens_val_focalLength",
                      wxTextCtrl)->SetValue( number.c_str() );
    } else {
      XRCCTRL(*this, "lens_val_focalLength",
                      wxTextCtrl)->SetValue( "" );
    }
}


void LensEdit::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
    int lt = XRCCTRL(*this, "lens_type_combobox", wxComboBox)->GetSelection();
    DEBUG_INFO ( wxString::Format ("lensEdit_RefImg %d Lenstype %d",lensEdit_RefImg, lt) )
}

void LensEdit::ChangePano ( )
{
//    DEBUG_TRACE( "lensGui_dirty = " << lensGui_dirty )
    if ( lensGui_dirty ) {
      imgNr[0] = 0;             // reset
      for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
        if ( images_list2->GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
          imgNr[0] += 1;
          imgNr[imgNr[0]] = Nr; //(unsigned int)Nr;
        }
      }
//      DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );

/**
 *   - Look if a lens is not needed anywhere else and erase it eventually.
 *   - Check if a lens is elsewhere needed and create a new lens, copy edit_lens *     to the new one, assign it to pano and set all edited images to new lens.
 *   - Reset the lensGui_dirty flag.
 */
      int eraseLensNr[512]; eraseLensNr[0] = 0;
      unsigned int notSelImgNr[512]; notSelImgNr[0] = 0;
      for ( int i = 0 ; i < 512 ; i++ )
        notSelImgNr[i] = 0;
      bool new_Lens = FALSE;
//      DEBUG_INFO( "new_Lens = " << new_Lens )
      for ( unsigned int all_img = 0
            ; all_img < (unsigned int) pano.getNrOfImages() ; all_img++ ) {
        bool selected_image = FALSE;
        for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
          // Are You in the group of selected images?
          if ( imgNr[i] == all_img )
            selected_image = TRUE;
        }
        if ( !selected_image ) { // But we ask for not selected - indirect.
            notSelImgNr[0]++;      // increase the number of not selected images
            notSelImgNr[notSelImgNr[0]] = all_img; // add the image number
        }
      }
//      DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",notSelImgNr[0], notSelImgNr[1],notSelImgNr[2], notSelImgNr[3], notSelImgNr[4]) );

      // ask for no longer needed lenses to erase
      for ( unsigned int i = 1 ; i <= imgNr[0] ; i++ ) {
        bool erase_Lens = TRUE;
        for ( unsigned int j = 1; notSelImgNr[0] >= j ; j++ ) {
            // is this lens inside of imgNr in notSelImgNr too
            if ( pano.getImage(notSelImgNr[j]).getLens() ==
                 pano.getImage(imgNr[i]).getLens() )
              erase_Lens = FALSE;
        }
        if ( erase_Lens ) {
          // Now we can simply erase, the images get theyre new lenses later.
          DEBUG_INFO( "will removeLens  " << pano.getImage(imgNr[i]).getLens() << " of " << pano.getNrOfLenses() )
//          pano.removeLens(imgNr[i]); // FIXME ?????
//          eraseLensNr[0] += 1;
//          eraseLensNr[eraseLensNr[0]] = pano.getImage(imgNr[i]).getLens();
        }
      }
      // create a new lens?
      for ( unsigned int j = 1; notSelImgNr[0] >= j ; j++ ) {
//        DEBUG_INFO("notSelImgNr = "<<notSelImgNr[j]<<" ref "<<lensEdit_RefImg)
        // Is the new lens in the group of the not selected images?
        if ( pano.getImage(lensEdit_RefImg).getLens() ==
             pano.getImage(notSelImgNr[j]).getLens() )
          new_Lens = TRUE;
//        DEBUG_INFO( "new_Lens = " << new_Lens )
      }

      unsigned int lensNr;
      // set the edit_lens
      if ( new_Lens )
        lensNr = pano.addLens (EDIT_LENS); // lensEdit_RefImg must be set before
      else
        lensNr =  pano.getImage(lensEdit_RefImg).getLens();

      // All list images get the new lens assigned.
      for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        pano.setLens (imgNr[i], lensNr);
      }

    }


    // FIXME support separate lenses
//      EDIT_LENS->update ( pano.getLens(pano.getImage(lensEdit_RefImg).getLens()) );

//    DEBUG_INFO ( "hier is item: " << wxString::Format("%d",lensEdit_RefImg) );

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, pano.getImage(lensEdit_RefImg).getLens(),*edit_Lens )
         );

    lensGui_dirty = FALSE;

    // update gui
//    int id (XRCID("lens_dialog"));
//    wxListEvent  e;
//    e.SetId(id);
//    LensChanged (e);

    DEBUG_TRACE( "" )
}



// Here we change the pano.
void LensEdit::LensTypeChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      DEBUG_TRACE ("")
      // uses enum LensProjectionFormat from PanoramaMemento.h
      int var = XRCCTRL(*this, "lens_type_combobox",
                                   wxComboBox)->GetSelection();
      edit_Lens->projectionFormat = (Lens::LensProjectionFormat) (var);

      ChangePano ();
      DEBUG_INFO ( wxString::Format ("lensEdit_RefImg %d lens %d Lenstype %d",lensEdit_RefImg,pano.getImage(lensEdit_RefImg).getLens(),var) )
    }
//    int lensEdit_RefImg = images_list2->GetSelectedImage();
}

void LensEdit::HFOVChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      DEBUG_TRACE ("")
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_HFOV", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->HFOV = *var;
      edit_Lens->focalLength = 18.0 / tan( edit_Lens->HFOV * M_PI / 360);
      edit_Lens->focalLength = edit_Lens->focalLength / edit_Lens->focalLengthConversionFactor;

      ChangePano ();
      updateHFOV();

      delete var;
    }

//    edit_Lens->update ( pano.getLens(pano.getImage(lensEdit_RefImg).getLens()) );

    DEBUG_TRACE ("")
}

void LensEdit::focalLengthChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
     wxString text=XRCCTRL(*this,"lens_val_focalLength",wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->focalLength = *var / edit_Lens->focalLengthConversionFactor;
      edit_Lens->HFOV = 2.0 * atan((36/2)
                      / (edit_Lens->focalLength
                         * edit_Lens->focalLengthConversionFactor))
                      * 180/M_PI;

      ChangePano ();
      updateHFOV();

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensEdit::aChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_a", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->a = *var;

      ChangePano ();
      SET_WXTEXTCTRL_TEXT( "lens_val_a"  , a )

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensEdit::bChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_b", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->b = *var;

      ChangePano ();
      SET_WXTEXTCTRL_TEXT( "lens_val_b"  , b )

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensEdit::cChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_c", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->c = *var;

      ChangePano ();
      SET_WXTEXTCTRL_TEXT( "lens_val_c"  , c )

     delete var;
    }
    DEBUG_TRACE ("")
}

void LensEdit::dChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_d", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->d = *var;

      ChangePano ();
      SET_WXTEXTCTRL_TEXT( "lens_val_d"  , d )

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensEdit::eChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_e", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->e = *var;

      ChangePano ();
      SET_WXTEXTCTRL_TEXT( "lens_val_e"  , e )

      delete var;
    }
    DEBUG_TRACE ("")
}

// This function is called whenever the image selection changes.
void LensEdit::SetImages ( wxListEvent & e )
{
    DEBUG_TRACE("");
    // One image is our prefered image.
    lensEdit_RefImg = e.GetIndex();

    // Now create an new reference lens from the lensEdit_RefImg image.
    delete edit_Lens;
    edit_Lens = new Lens(EDIT_LENS);

    // FIXME should get a bottom to update
//    edit_Lens->readEXIF(pano.getImage(lensEdit_RefImg).getFilename().c_str());

    // update gui
//    LensChanged (e);
    // set the values from the mainly focused image lens.
    XRCCTRL(*this, "lens_type_combobox", wxComboBox)->SetSelection( EDIT_LENS.
                                         projectionFormat  );
    updateHFOV();
    SET_WXTEXTCTRL_TEXT( "lens_val_a"  , a )
    SET_WXTEXTCTRL_TEXT( "lens_val_b"  , b )
    SET_WXTEXTCTRL_TEXT( "lens_val_c"  , c )
    SET_WXTEXTCTRL_TEXT( "lens_val_d"  , d )
    SET_WXTEXTCTRL_TEXT( "lens_val_e"  , e )


    /**  - Look if we may need a new lens.
      *   - If so set the lensGui_dirty flag for later remembering.
      */
    if ( images_list2->GetSelectedItemCount() != (int) pano.getNrOfImages() )
      lensGui_dirty = TRUE;


    DEBUG_TRACE("");
}



