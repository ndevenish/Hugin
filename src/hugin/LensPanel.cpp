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
#include <wx/spinctrl.h>

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
#define EDIT_LENS pano.getLens (lensEditRef_lensNr)

#define SET_WXTEXTCTRL_TEXT( xrc_name , variable )                  \
{                                                                              \
    std::string number;                                                        \
    if ( ( wxString::Format ("%f", variable)\
           != "nan" ) &&  ( variable != 0.0 ) ) {      \
      number = doubleToString ( variable ); \
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

    lens_edit->SetImages ( e );
//    DEBUG_INFO ( "hier is item: " << wxString::Format("%d", lensEdit_ReferenceImage) );
}

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(LensEdit, wxWindow) //wxEvtHandler)
//    EVT_LIST_ITEM_SELECTED( XRCID("images_list2_unknown"), LensEdit::SetImages )
//    EVT_LIST_ITEM_SELECTED ( XRCID("images_list2_unknown"),LensPanel::LensChanged )
    EVT_COMBOBOX ( XRCID("lens_combobox_number"), LensEdit::LensSelected )
    EVT_COMBOBOX ( XRCID("lens_type_combobox"), LensEdit::LensTypeChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_HFOV"), LensEdit::HFOVChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_focalLength"),LensEdit::focalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_a"), LensEdit::aChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_b"), LensEdit::bChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_c"), LensEdit::cChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_d"), LensEdit::dChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_e"), LensEdit::eChanged )
    EVT_CHECKBOX ( XRCID("images_inherit_hfov"), LensEdit::SetInheritHfov )
    EVT_SPINCTRL ( XRCID("images_spin_hfov"), LensEdit::SetInheritHfov )
    EVT_CHECKBOX ( XRCID("images_optimize_hfov"), LensEdit::SetOptimizeHfov )
    EVT_CHECKBOX ( XRCID("images_inherit_a"), LensEdit::SetInheritA )
    EVT_SPINCTRL ( XRCID("images_spin_a"), LensEdit::SetInheritA )
    EVT_CHECKBOX ( XRCID("images_optimize_a"), LensEdit::SetOptimizeA )
    EVT_CHECKBOX ( XRCID("images_inherit_b"), LensEdit::SetInheritB )
    EVT_SPINCTRL ( XRCID("images_spin_b"), LensEdit::SetInheritB )
    EVT_CHECKBOX ( XRCID("images_optimize_b"), LensEdit::SetOptimizeB )
    EVT_CHECKBOX ( XRCID("images_inherit_c"), LensEdit::SetInheritC )
    EVT_SPINCTRL ( XRCID("images_spin_c"), LensEdit::SetInheritC )
    EVT_CHECKBOX ( XRCID("images_optimize_c"), LensEdit::SetOptimizeC )
    EVT_CHECKBOX ( XRCID("images_inherit_d"), LensEdit::SetInheritD )
    EVT_SPINCTRL ( XRCID("images_spin_d"), LensEdit::SetInheritD )
    EVT_CHECKBOX ( XRCID("images_optimize_d"), LensEdit::SetOptimizeD )
    EVT_CHECKBOX ( XRCID("images_inherit_e"), LensEdit::SetInheritE )
    EVT_SPINCTRL ( XRCID("images_spin_e"), LensEdit::SetInheritE )
    EVT_CHECKBOX ( XRCID("images_optimize_e"), LensEdit::SetOptimizeE )
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


// selected a lens as edit_Lens
void LensEdit::LensSelected ( wxCommandEvent & e )
{
    int lens = XRCCTRL(*this, "lens_combobox_number", wxComboBox)
                                 ->GetSelection();
    update_edit_LensGui ( lens );
}

void LensEdit::update_edit_LensGui ( int lens )
{
    DEBUG_TRACE("");
      lensEditRef_lensNr = lens;

      // Now create an new reference lens from the lensEdit_ReferenceImage image.
      delete edit_Lens;
      edit_Lens = new Lens( pano.getLens (lensEditRef_lensNr) );

      // FIXME should get a bottom to update
//    edit_Lens->readEXIF(pano.getImage(lensEdit_ReferenceImage).getFilename().c_str());

      // update gui
      XRCCTRL(*this, "lens_type_combobox", wxComboBox)->SetSelection(  
                      pano.getLens (lensEditRef_lensNr).  projectionFormat  );
      updateHFOV();
      SET_WXTEXTCTRL_TEXT( "lens_val_a" , EDIT_LENS.a )
      SET_WXTEXTCTRL_TEXT( "lens_val_b" , EDIT_LENS.b )
      SET_WXTEXTCTRL_TEXT( "lens_val_c" , EDIT_LENS.c )
      SET_WXTEXTCTRL_TEXT( "lens_val_d" , EDIT_LENS.d )
      SET_WXTEXTCTRL_TEXT( "lens_val_e" , EDIT_LENS.e )

      // label the panel
      std::stringstream label;
      label << lensEditRef_lensNr ;
      XRCCTRL(*this, "lens_val_number", wxStaticText)->SetLabel( 
                            label.str().c_str() );

/*    if ( EDIT_LENS.HFOV.isLinked() )
      XRCCTRL(*this, "images_inherit_hfov" ,wxCheckBox)->SetValue(TRUE);
    else
      XRCCTRL(*this, "images_inherit_hfov" ,wxCheckBox)->SetValue(FALSE);
  */    
    DEBUG_TRACE("");
}

void LensEdit::updateHFOV()
{
//    DEBUG_TRACE("");
    SET_WXTEXTCTRL_TEXT( "lens_val_HFOV" , EDIT_LENS.HFOV )

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
    if ( (int)pano.getNrOfImages() != images_list2->GetItemCount() ) {
      wxListEvent e;
      SetImages( e );
    }
    int lt = XRCCTRL(*this, "lens_type_combobox", wxComboBox)->GetSelection();
    DEBUG_INFO ( wxString::Format (" Lenstype %d", lt) )
}

void LensEdit::ChangePano ( )
{
    DEBUG_TRACE( "lensGui_dirty = " << lensGui_dirty )
    changePano = TRUE;
    if ( lensGui_dirty ) {
//      DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );

/**
 *   - Look if a lens is not needed anywhere else and erase it eventually.
 *   - Check if a lens is elsewhere needed and if so, create a new lens, copy
 *     edit_lens to the new one, assign it to pano and set all edited images to
 *     new lens.
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
        // Is the new lens in the group of the not selected images?
        if ( lensEditRef_lensNr == (int)pano.getImage(notSelImgNr[j]).getLens())
          new_Lens = TRUE;
//        DEBUG_INFO( "new_Lens = " << new_Lens )
      }

      unsigned int lensNr;
      // set the edit_lens
      if ( new_Lens )
        lensNr = pano.addLens (EDIT_LENS); // lensEditRef_lensNr must set before
      else
        lensNr =  lensEditRef_lensNr;

      // All list images get the new lens assigned.
      for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        pano.setLens (imgNr[i], lensNr);
      }

    }

    // TODO decide wether to take the selected lens or not

    if ( XRCCTRL(*this, "lens_cb_apply", wxCheckBox)->IsChecked() ) {
        lensEditRef_lensNr = XRCCTRL(*this, "lens_combobox_number", wxComboBox)
                                 ->GetSelection();
        for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
          pano.setLens (imgNr[i], lensEditRef_lensNr);
        }
    }


    // FIXME support separate lenses
//      EDIT_LENS->update ( pano.getLens(lensEditRef_lensNr) );

//    DEBUG_INFO ( "hier is item: " << wxString::Format("%d",lensEdit_ReferenceImage) );

    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, lensEditRef_lensNr,*edit_Lens )
         );

    lensGui_dirty = FALSE;
    changePano = FALSE;

    DEBUG_TRACE( "" )
}

// Here we change the pano.
void LensEdit::LensTypeChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      DEBUG_TRACE ("")
      // uses enum Lens::LensProjectionFormat from PanoramaMemento.h
      int var = XRCCTRL(*this, "lens_type_combobox",
                                   wxComboBox)->GetSelection();
      edit_Lens->projectionFormat = (Lens::LensProjectionFormat) (var);

      ChangePano ();
      DEBUG_INFO ( wxString::Format ("lens %d Lenstype %d",lensEditRef_lensNr,var) )
    }
//    int lensEdit_ReferenceImage = images_list2->GetSelectedImage();
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

//    edit_Lens->update ( pano.getLens(lensEditRef_lensNr) );

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
      SET_WXTEXTCTRL_TEXT( "lens_val_a" , EDIT_LENS.a )

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
      SET_WXTEXTCTRL_TEXT( "lens_val_b" , EDIT_LENS.b )

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
      SET_WXTEXTCTRL_TEXT( "lens_val_c" , EDIT_LENS.c )

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
      SET_WXTEXTCTRL_TEXT( "lens_val_d" , EDIT_LENS.d )

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
      SET_WXTEXTCTRL_TEXT( "lens_val_e" , EDIT_LENS.e )

      delete var;
    }
    DEBUG_TRACE ("")
}

// Inheritance + Optimization

void LensEdit::SetInherit( std::string type )
{
//    "roll", "images_inherit_roll", "images_spin_roll", "images_optimize_roll" 
    // requisites
    int var (-1);
    std::string command;
    std::string xml_inherit, xml_optimize, xml_spin;
    ImageVariables new_var;

    if ( imgNr[0] > 0 ) { // dont work on an empty image
      // set xml resource names
      xml_inherit = "images_inherit_"; xml_inherit.append(type);
      xml_optimize = "images_optimize_"; xml_optimize.append(type);
      xml_spin = "images_spin_"; xml_spin.append(type);
      // for all images
      for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        new_var = pano.getVariable(imgNr[i]);
        // set inheritance from image with number ...
        var = XRCCTRL(*this, xml_spin .c_str(),wxSpinCtrl)->GetValue();
        DEBUG_INFO("var("<<var<<") == I("<<(int)imgNr[i]<<")  :  | pano");
        // Shall we inherit?
        if ( XRCCTRL(*this, xml_inherit .c_str(),wxCheckBox)->IsChecked()
             && (pano.getNrOfImages() != 1) ) { // single image cannot inherit
            // We are conservative and ask better once more. 
            if ( type == "hfov" ) {
              // test for unselfish inheritance
              if ( var != (int)imgNr[i] ) {
                new_var.HFOV.link(var);
                optset->at(imgNr[i]).HFOV = FALSE;
              } else { // search for another possible link image
                if ( (((int)new_var. HFOV .getLink() > var) && (var != 0)) 
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.HFOV.link(var);
                } else {
                  var++; new_var.HFOV.link(var);
                }
              }
            }
            if ( type == "a" ) {
              if ( var != (int)imgNr[i] ) {
                new_var.a.link(var);
                optset->at(imgNr[i]).a = FALSE;
              } else { // search for another possible link image
                if ( ((int)new_var. a .getLink() > var) && (var != 0)
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.a.link(var);
                } else {
                  var++; new_var.a.link(var);
                }
              }
            }
            if ( type == "b" ) {
              if ( var != (int)imgNr[i] ) {
                new_var.b.link(var);
                optset->at(imgNr[i]).b = FALSE;
              } else { // search for another possible link image
                if ( ((int)new_var. b .getLink() > var) && (var != 0)
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.b.link(var);
                } else {
                  var++; new_var.b.link(var);
                }
              }
            }
            if ( type == "c" ) {
              if ( var != (int)imgNr[i] ) {
                new_var.c.link(var);
                optset->at(imgNr[i]).c = FALSE;
              } else { // search for another possible link image
                if ( ((int)new_var. c .getLink() > var) && (var != 0)
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.c.link(var);
                } else {
                  var++; new_var.c.link(var);
                }
              }
            }
            if ( type == "d" ) {
              if ( var != (int)imgNr[i] ) {
                new_var.d.link(var);
                optset->at(imgNr[i]).d = FALSE;
              } else { // search for another possible link image
                if ( ((int)new_var. d .getLink() > var) && (var != 0)
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.d.link(var);
                } else {
                  var++; new_var.d.link(var);
                }
              }
            }
            if ( type == "e" ) {
              if ( var != (int)imgNr[i] ) {
                new_var.e.link(var);
                optset->at(imgNr[i]).e = FALSE;
              } else { // search for another possible link image
                if ( ((int)new_var. e .getLink() > var) && (var != 0)
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.e.link(var);
                } else {
                  var++; new_var.e.link(var);
                }
              }
            }
            // ... and set controls
            XRCCTRL(*this, xml_spin .c_str(),wxSpinCtrl)->SetValue(var);
            XRCCTRL(*this, xml_optimize .c_str(),wxCheckBox)->SetValue(FALSE);
            DEBUG_INFO("var("<<var<<") == I("<<(int)imgNr[i]<<")  :  | pano");
            // local ImageVariables finished, save to pano
            pano.updateVariables( imgNr[i], new_var ); 
          // unset inheritance
          } else {
            if ( type == "hfov" )
              new_var.HFOV.unlink();
            if ( type == "a" )
              new_var.a.unlink();
            if ( type == "b" )
              new_var.b.unlink();
            if ( type == "c" )
              new_var.c.unlink();
            if ( type == "d" )
              new_var.d.unlink();
            if ( type == "e" )
              new_var.e.unlink();
            XRCCTRL(*this,xml_inherit.c_str(),wxCheckBox)->SetValue(FALSE);
          }
          // local ImageVariables finished, save to pano
          pano.updateVariables( imgNr[i], new_var ); 
          // set optimization
          if (XRCCTRL(*this,xml_optimize.c_str(),wxCheckBox)->IsChecked()){
            if ( type == "hfov" ) {
              optset->at(imgNr[i]).HFOV = TRUE;
            }
            if ( type == "a" ) {
              optset->at(imgNr[i]).a = TRUE;
            }
            if ( type == "b" ) {
              optset->at(imgNr[i]).b = TRUE;
            }
            if ( type == "c" ) {
              optset->at(imgNr[i]).c = TRUE;
            }
            if ( type == "d" ) {
              optset->at(imgNr[i]).d = TRUE;
            }
            if ( type == "e" ) {
              optset->at(imgNr[i]).e = TRUE;
            }
            XRCCTRL(*this,xml_inherit.c_str(),wxCheckBox)->SetValue(FALSE);
          // unset optimization
          } else if(!XRCCTRL(*this,xml_optimize.c_str(),wxCheckBox)->IsChecked()){
            if ( type == "hfov" )
              optset->at(imgNr[i]).HFOV = FALSE;
            if ( type == "a" )
              optset->at(imgNr[i]).a = FALSE;
            if ( type == "b" )
              optset->at(imgNr[i]).b = FALSE;
            if ( type == "c" )
              optset->at(imgNr[i]).c = FALSE;
            if ( type == "d" )
              optset->at(imgNr[i]).d = FALSE;
            if ( type == "e" )
              optset->at(imgNr[i]).e = FALSE;
          }
        }

        // activate an undoable command, not for the optimize settings
        GlobalCmdHist::getInstance().addCommand(
           new PT::UpdateImageVariablesCmd(pano, imgNr[imgNr[0]], pano.getVariable(imgNr[imgNr[0]]))
           );
    }
    DEBUG_INFO( type.c_str() << " end" )
}

void LensEdit::SetInheritHfov( wxCommandEvent & e )
{
    SetInherit ( "hfov" );
}
void LensEdit::SetInheritA( wxCommandEvent & e )
{
    SetInherit ( "a" );
}
void LensEdit::SetInheritB( wxCommandEvent & e )
{
    SetInherit ( "b" );
}
void LensEdit::SetInheritC( wxCommandEvent & e )
{
    SetInherit ( "c" );
}
void LensEdit::SetInheritD( wxCommandEvent & e )
{
    SetInherit ( "d" );
}
void LensEdit::SetInheritE( wxCommandEvent & e )
{
    SetInherit ( "e" );
}

void LensEdit::SetOptimizeHfov( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_hfov" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "hfov" );
}
void LensEdit::SetOptimizeA( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_a" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "a" );
}
void LensEdit::SetOptimizeB( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_b" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "b" );
}
void LensEdit::SetOptimizeC( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_c" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "c" );
}
void LensEdit::SetOptimizeD( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_d" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "d" );
}
void LensEdit::SetOptimizeE( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_e" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "e" );
}



// This function is called whenever the image selection changes.
void LensEdit::SetImages ( wxListEvent & e )
{
    DEBUG_TRACE("changePano = " << changePano );
    if ( changePano == FALSE ) {

      // set link controls
      #define SET_SPIN_RANGE( type ) \
      XRCCTRL(*this, type ,wxSpinCtrl)->   \
                 SetRange( 0 , (int) pano.getNrOfImages() - 1); 
      SET_SPIN_RANGE ("images_spin_hfov")
      SET_SPIN_RANGE ("images_spin_a")
      SET_SPIN_RANGE ("images_spin_b")
      SET_SPIN_RANGE ("images_spin_c")
      SET_SPIN_RANGE ("images_spin_d")
      SET_SPIN_RANGE ("images_spin_e")

      // One image is our prefered image.
      int lensEdit_ReferenceImage = e.GetIndex();
      bool lensEdit_ReferenceImage_valid = FALSE;
      // get the selection
      imgNr[0] = 0;             // reset
      for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
        if ( images_list2->GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
          imgNr[0] += 1;
          imgNr[imgNr[0]] = Nr; //(unsigned int)Nr;
          if ( lensEdit_ReferenceImage == Nr )
            lensEdit_ReferenceImage_valid = TRUE;
        }
      }
      // We need a valid image to take the lens from.
      if ( !lensEdit_ReferenceImage_valid ) {
        DEBUG_WARN( "leafing early !!!");
        return;
      }

      // We remember the number of the lens on wich we work.
      lensEditRef_lensNr = pano.getImage(lensEdit_ReferenceImage). getLens();

      // Now create a new reference lens from the lensEdit_ReferenceImage image.
      // and update the gui
      update_edit_LensGui ( lensEditRef_lensNr );

      // set the selectable lenses
      // TODO let give names for it, type in and take it as a new alias
      XRCCTRL(*this, "lens_combobox_number", wxComboBox)->Clear();
      for ( unsigned int i = 0 ; i < pano.getNrOfLenses() ; i++) {
          std::stringstream sstr;
          sstr << i;
          XRCCTRL(*this, "lens_combobox_number", wxComboBox)
                                   -> Append( sstr.str().c_str() );
      }
      XRCCTRL(*this, "lens_combobox_number", wxComboBox)
                               -> SetSelection(lensEditRef_lensNr);

      /**  - Look if we may need a new lens.
        *   - If so set the lensGui_dirty flag for later remembering.
        */
      if ( images_list2->GetSelectedItemCount() != (int) pano.getNrOfImages() )
        lensGui_dirty = TRUE;

    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) << " lens:"<< lensEditRef_lensNr);
//    DEBUG_TRACE("");
}



