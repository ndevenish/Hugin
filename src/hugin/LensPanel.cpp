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

BEGIN_EVENT_TABLE(LensPanel, wxWindow) //wxEvtHandler)
    EVT_SIZE   ( LensPanel::FitParent )
//    EVT_LIST_ITEM_SELECTED( XRCID("images_list2_unknown"), LensPanel::SetImages )
//    EVT_LIST_ITEM_SELECTED ( XRCID("images_list2_unknown"),LensPanel::LensChanged )
    EVT_COMBOBOX ( XRCID("lens_combobox_number"), LensPanel::LensSelected )
    EVT_BUTTON ( XRCID("lens_button_apply"), LensPanel::LensApply )
    EVT_COMBOBOX (XRCID("lens_val_projectionFormat"),LensPanel::LensTypeChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_HFOV"), LensPanel::HFOVChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_focalLength"),LensPanel::focalLengthChanged)
    EVT_TEXT_ENTER ( XRCID("lens_val_a"), LensPanel::aChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_b"), LensPanel::bChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_c"), LensPanel::cChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_d"), LensPanel::dChanged )
    EVT_TEXT_ENTER ( XRCID("lens_val_e"), LensPanel::eChanged )
    EVT_CHECKBOX ( XRCID("images_inherit_HFOV"), LensPanel::SetInheritHfov )
    EVT_SPINCTRL ( XRCID("images_spin_HFOV"), LensPanel::SetInheritHfov )
    EVT_CHECKBOX ( XRCID("images_optimize_HFOV"), LensPanel::SetOptimizeHfov )
    EVT_CHECKBOX ( XRCID("images_inherit_a"), LensPanel::SetInheritA )
    EVT_SPINCTRL ( XRCID("images_spin_a"), LensPanel::SetInheritA )
    EVT_CHECKBOX ( XRCID("images_optimize_a"), LensPanel::SetOptimizeA )
    EVT_CHECKBOX ( XRCID("images_inherit_b"), LensPanel::SetInheritB )
    EVT_SPINCTRL ( XRCID("images_spin_b"), LensPanel::SetInheritB )
    EVT_CHECKBOX ( XRCID("images_optimize_b"), LensPanel::SetOptimizeB )
    EVT_CHECKBOX ( XRCID("images_inherit_c"), LensPanel::SetInheritC )
    EVT_SPINCTRL ( XRCID("images_spin_c"), LensPanel::SetInheritC )
    EVT_CHECKBOX ( XRCID("images_optimize_c"), LensPanel::SetOptimizeC )
    EVT_CHECKBOX ( XRCID("images_inherit_d"), LensPanel::SetInheritD )
    EVT_SPINCTRL ( XRCID("images_spin_d"), LensPanel::SetInheritD )
    EVT_CHECKBOX ( XRCID("images_optimize_d"), LensPanel::SetOptimizeD )
    EVT_CHECKBOX ( XRCID("images_inherit_e"), LensPanel::SetInheritE )
    EVT_SPINCTRL ( XRCID("images_spin_e"), LensPanel::SetInheritE )
    EVT_CHECKBOX ( XRCID("images_optimize_e"), LensPanel::SetOptimizeE )
END_EVENT_TABLE()

// Define a constructor for the Lenses Panel

LensPanel::LensPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(*pano)
{
    DEBUG_TRACE("ctor");
    pano->addObserver(this);

    // This controls must called by xrc handler and after it we play with it.
    wxXmlResource::Get()->LoadPanel (this, wxT("lens_panel"));

    // The following control creates itself. We dont care about xrc loading.
    images_list2 = new List (parent, pano, lens_layout);
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_list2_unknown"),
               images_list2 );
    images_list2->AssignImageList(img_icons, wxIMAGE_LIST_SMALL );

    p_img = new wxBitmap( 0, 0 );

    // intermediate event station
    PushEventHandler( new MyEvtHandler((size_t) 2) );

    // with no image in pano hugin crashes
    edit_Lens = new Lens ();
    AddLensCmd ( *pano, *edit_Lens );

    DEBUG_INFO ( wxString::Format ("Lensesnumber: %d", pano->getNrOfLenses()) )

    for ( int i = 0 ; i < 512 ; i++ )
      imgNr[i] = 0;

    DEBUG_TRACE("");;
}


LensPanel::~LensPanel(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
    delete images_list2;
    DEBUG_TRACE("");
}

void LensPanel::FitParent( wxSizeEvent & e )
{
    wxSize new_size = GetSize();
//    wxSize new_size = GetParent()->GetSize();
    XRCCTRL(*this, "lens_panel", wxPanel)->SetSize ( new_size );
//    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}

// selected a lens as edit_Lens
void LensPanel::LensSelected ( wxCommandEvent & e )
{
    int lens = XRCCTRL(*this, "lens_combobox_number", wxComboBox)
                                 ->GetSelection();
    update_edit_LensGui ( lens );
}

// apply edit_Lens to selected images
void LensPanel::LensApply ( wxCommandEvent & e )
{
    DEBUG_TRACE("");

    lensEditRef_lensNr = XRCCTRL(*this, "lens_combobox_number", wxComboBox)
                                 ->GetSelection();
        
    update_edit_LensGui ( lensEditRef_lensNr );

    for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        GlobalCmdHist::getInstance().addCommand(
            new PT::ChangeLensCmd( pano, imgNr[i], *edit_Lens )
            );
          //pano.setLens (imgNr[i], lensEditRef_lensNr);
    }
    DEBUG_TRACE("");
}

void LensPanel::update_edit_LensGui ( int lens )
{
    DEBUG_TRACE("");
      lensEditRef_lensNr = lens;

      // Now create an new reference lens from the lensEdit_ReferenceImage image.
      delete edit_Lens;
      edit_Lens = new Lens( pano.getLens (lensEditRef_lensNr) );

      // FIXME should get a bottom to update
//    edit_Lens->readEXIF(pano.getImage(lensEdit_ReferenceImage).getFilename().c_str());

      // update gui
      XRCCTRL(*this, "lens_val_projectionFormat", wxComboBox)->SetSelection(  
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

    // set inherit and from wich image,  dis-/enable optimize checkboxes
    // We expect ImageNr = lensEditRef_lensNr !
    std::string xml_inherit, xml_optimize, xml_spin;
    ImageVariables new_var ( pano.getVariable( lensEditRef_lensNr ) );
#define SET_IMHERIT_OPTIMIZE( type_str, type ) \
{ \
    xml_inherit = "images_inherit_"; xml_inherit.append(type_str); \
    xml_optimize = "images_optimize_"; xml_optimize.append(type_str); \
    xml_spin = "images_spin_"; xml_spin.append(type_str); \
    if ( new_var. type .isLinked() ) {\
      XRCCTRL(*this, xml_inherit .c_str(),wxCheckBox)->SetValue(TRUE); \
      int var = (int)new_var. type .getLink(); \
         XRCCTRL(*this, xml_spin.c_str(), wxSpinCtrl) ->SetValue(var); \
    } else { \
      XRCCTRL(*this, xml_inherit .c_str(),wxCheckBox)->SetValue(FALSE); \
    } \
    if (optset->at(lensEditRef_lensNr). type == TRUE ) { \
        XRCCTRL(*this, xml_optimize .c_str(), wxCheckBox) ->SetValue(TRUE); \
    } else { \
        XRCCTRL(*this, xml_optimize .c_str(), wxCheckBox) ->SetValue(FALSE); \
    } \
}
    SET_IMHERIT_OPTIMIZE ( "HFOV" , HFOV )
    SET_IMHERIT_OPTIMIZE ( "a" , a )
    SET_IMHERIT_OPTIMIZE ( "b" , b )
    SET_IMHERIT_OPTIMIZE ( "c" , c )
    SET_IMHERIT_OPTIMIZE ( "d" , d )
    SET_IMHERIT_OPTIMIZE ( "e" , e )
      
    DEBUG_TRACE("");
}

void LensPanel::updateHFOV()
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


void LensPanel::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
    if ( (int)pano.getNrOfImages() != images_list2->GetItemCount() ) {
      wxListEvent e;
      SetImages( e );
    }
    int lt = XRCCTRL(*this, "lens_val_projectionFormat", wxComboBox)->GetSelection();
//    DEBUG_INFO ( wxString::Format (" Lenstype %d", lt) )
    DEBUG_INFO ( wxString::Format (" Lenstype %d", lt) )
}

void LensPanel::ChangePano ( std::string type, double var )
{
//    DEBUG_TRACE( "" )
    changePano = TRUE;

    // we ask for each images Lens
    Lens new_Lens;
    for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        new_Lens = pano.getLens(imgNr[i]);
        // set values
        if ( type == "projectionFormat" ) {
          new_Lens.projectionFormat = (PT::Lens::LensProjectionFormat)var;
        }
        if ( type == "HFOV" ) {
          new_Lens.HFOV = var;
        }
        if ( type == "a" ) {
          new_Lens.a = var;
        }
        if ( type == "b" ) {
          new_Lens.b = var;
        }
        if ( type == "c" ) {
          new_Lens.c = var;
        }
        if ( type == "d" ) {
          new_Lens.d = var;
        }
        if ( type == "e" ) {
          new_Lens.e = var;
        }
        GlobalCmdHist::getInstance().addCommand(
//          new PT::UpdateImageVariablesCmd(pano, imgNr[imgNr[0]], new_Lens)
          new PT::ChangeLensCmd( pano, imgNr[i], new_Lens )
          );

//        pano.updateVariables( imgNr[i], new_Lens ); 

        DEBUG_INFO( type <<" for image "<< imgNr[i] );
    }

/*    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeLensCmd( pano, //lensEditRef_lensNr,*edit_Lens )
         );
*/
    changePano = FALSE;

    DEBUG_TRACE( "" )
}

// Here we change the pano.
void LensPanel::LensTypeChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      DEBUG_TRACE ("")
      // uses enum Lens::LensProjectionFormat from PanoramaMemento.h
      int var = XRCCTRL(*this, "lens_val_projectionFormat",
                                   wxComboBox)->GetSelection();
      edit_Lens->projectionFormat = (Lens::LensProjectionFormat) (var);

      ChangePano ( "projectionFormat", (double)var );
      DEBUG_INFO ( wxString::Format ("lens %d Lenstype %d",lensEditRef_lensNr,var) )
    }
//    int lensEdit_ReferenceImage = images_list2->GetSelectedImage();
}

void LensPanel::HFOVChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      DEBUG_TRACE ("")
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_HFOV", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->HFOV = *var;
      edit_Lens->focalLength = 18.0 / tan( edit_Lens->HFOV * M_PI / 360);
      edit_Lens->focalLength = edit_Lens->focalLength / edit_Lens->focalLengthConversionFactor;

      ChangePano ( "HFOV", *var );
      updateHFOV();

      delete var;
    }

//    edit_Lens->update ( pano.getLens(lensEditRef_lensNr) );

    DEBUG_TRACE ("")
}

void LensPanel::focalLengthChanged ( wxCommandEvent & e )
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

      ChangePano ( "HFOV", edit_Lens->HFOV );
      updateHFOV();

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensPanel::aChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_a", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->a = *var;

      ChangePano ( "a", *var );
      SET_WXTEXTCTRL_TEXT( "lens_val_a" , EDIT_LENS.a )

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensPanel::bChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_b", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->b = *var;

      ChangePano ( "b", *var );
      SET_WXTEXTCTRL_TEXT( "lens_val_b" , EDIT_LENS.b )

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensPanel::cChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_c", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->c = *var;

      ChangePano ( "c", *var );
      SET_WXTEXTCTRL_TEXT( "lens_val_c" , EDIT_LENS.c )

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensPanel::dChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_d", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->d = *var;

      ChangePano ( "d", *var );
      SET_WXTEXTCTRL_TEXT( "lens_val_d" , EDIT_LENS.d )

      delete var;
    }
    DEBUG_TRACE ("")
}

void LensPanel::eChanged ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      double * var = new double ();
      wxString text = XRCCTRL(*this, "lens_val_e", wxTextCtrl)->GetValue();
      text.ToDouble( var );

      edit_Lens->e = *var;

      ChangePano ( "e", *var );
      SET_WXTEXTCTRL_TEXT( "lens_val_e" , EDIT_LENS.e )

      delete var;
    }
    DEBUG_TRACE ("")
}

// Inheritance + Optimization

void LensPanel::SetInherit( std::string type )
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
            if ( type == "HFOV" ) {
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
          // unset inheritance
          } else {
            if ( type == "HFOV" )
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
//          pano.updateVariables( imgNr[i], new_var ); 
          // set optimization
          if (XRCCTRL(*this,xml_optimize.c_str(),wxCheckBox)->IsChecked()){
            if ( type == "HFOV" ) {
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
            if ( type == "HFOV" )
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
          GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateImageVariablesCmd(pano, imgNr[i], new_var)
            );
        }

        // activate an undoable command, not for the optimize settings
/*        GlobalCmdHist::getInstance().addCommand(
           new PT::UpdateImageVariablesCmd(pano, imgNr[i], pano.getVariable(imgNr[imgNr[0]]))
           );*/
    }
    DEBUG_INFO( type.c_str() << " end" )
}

void LensPanel::SetInheritHfov( wxCommandEvent & e )
{
    SetInherit ( "HFOV" );
}
void LensPanel::SetInheritA( wxCommandEvent & e )
{
    SetInherit ( "a" );
}
void LensPanel::SetInheritB( wxCommandEvent & e )
{
    SetInherit ( "b" );
}
void LensPanel::SetInheritC( wxCommandEvent & e )
{
    SetInherit ( "c" );
}
void LensPanel::SetInheritD( wxCommandEvent & e )
{
    SetInherit ( "d" );
}
void LensPanel::SetInheritE( wxCommandEvent & e )
{
    SetInherit ( "e" );
}

void LensPanel::SetOptimizeHfov( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_HFOV" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "HFOV" );
}
void LensPanel::SetOptimizeA( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_a" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "a" );
}
void LensPanel::SetOptimizeB( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_b" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "b" );
}
void LensPanel::SetOptimizeC( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_c" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "c" );
}
void LensPanel::SetOptimizeD( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_d" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "d" );
}
void LensPanel::SetOptimizeE( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_e" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "e" );
}



// This function is called whenever the image selection changes.
void LensPanel::SetImages ( wxListEvent & e )
{
    DEBUG_TRACE("changePano = " << changePano );
    if ( changePano == FALSE ) {

      // set link controls
      #define SET_SPIN_RANGE( type ) \
      XRCCTRL(*this, type ,wxSpinCtrl)->   \
                 SetRange( 0 , (int) pano.getNrOfImages() - 1); 
      SET_SPIN_RANGE ("images_spin_HFOV")
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

    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) << " lens:"<< lensEditRef_lensNr);
//    DEBUG_TRACE("");
}



