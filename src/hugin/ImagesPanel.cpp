// -*- c-basic-offset: 4 -*-

/** @file ImagesPanel.cpp
 *
 *  @brief implementation of ImagesPanel Class
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
//#include <wx/image.h>               // wxImage
//#include <wx/imagpng.h>             // for about html
#include <wx/listctrl.h>	// needed on mingw
#include <wx/imaglist.h>
#include <wx/spinctrl.h>

#include "PT/PanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/List.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/Events.h"
#include "hugin/huginApp.h"
#include "PT/Panorama.h"

#include "common/utils.h"
using namespace PT;
using namespace utils;

// Image Icons
wxImageList * img_icons;
// Image Icons biger
wxImageList * img_bicons;

// image preview
wxBitmap * p_img = (wxBitmap *) NULL;

ImgPreview * canvas;

//------------------------------------------------------------------------------
#define GET_VAR(val) pano.getVariable(orientationEdit_RefImg).val.getValue()

BEGIN_EVENT_TABLE(ImagesPanel, wxWindow)
    EVT_SIZE   ( ImagesPanel::FitParent )
//    EVT_MOUSE_EVENTS ( ImagesPanel::OnMouse )
//    EVT_MOTION ( ImagesPanel::ChangePreview )
    EVT_SLIDER ( XRCID("images_slider_yaw"),ImagesPanel::SetYaw )
    EVT_SLIDER ( XRCID("images_slider_pitch"),ImagesPanel::SetPitch )
    EVT_SLIDER ( XRCID("images_slider_roll"),ImagesPanel::SetRoll )
    EVT_TEXT_ENTER ( XRCID("images_text_yaw"), ImagesPanel::SetYawText )
    EVT_TEXT_ENTER ( XRCID("images_text_pitch"), ImagesPanel::SetPitchText )
    EVT_TEXT_ENTER ( XRCID("images_text_roll"), ImagesPanel::SetRollText )
    EVT_CHECKBOX ( XRCID("images_inherit_yaw"), ImagesPanel::SetInheritYaw )
    EVT_SPINCTRL ( XRCID("images_spin_yaw"), ImagesPanel::SetInheritYaw )
    EVT_CHECKBOX ( XRCID("images_inherit_pitch"),ImagesPanel::SetInheritPitch)
    EVT_SPINCTRL ( XRCID("images_spin_pitch"), ImagesPanel::SetInheritPitch )
    EVT_CHECKBOX ( XRCID("images_inherit_roll"), ImagesPanel::SetInheritRoll )
    EVT_SPINCTRL ( XRCID("images_spin_roll"), ImagesPanel::SetInheritRoll )
    EVT_CHECKBOX ( XRCID("images_optimize_yaw"), ImagesPanel::SetOptimizeYaw )
    EVT_CHECKBOX ( XRCID("images_optimize_pitch"),ImagesPanel::SetOptimizePitch)
    EVT_CHECKBOX ( XRCID("images_optimize_roll"), ImagesPanel::SetOptimizeRoll )
END_EVENT_TABLE()


// Define a constructor for the Images Panel
ImagesPanel::ImagesPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, wxEXPAND|wxGROW),
      pano(*pano)
{
    DEBUG_TRACE("");

    wxXmlResource::Get()->LoadPanel (this, wxT("images_panel"));
    DEBUG_TRACE("");

    images_list = new List (parent, pano, images_layout);
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_list_unknown"),
               images_list );

    DEBUG_TRACE("");

    PushEventHandler( new MyEvtHandler((size_t) 3) );

    // Image Preview

    img_icons = new wxImageList(24,24);
    img_bicons = new wxImageList(128,128);
#if 1
    images_list->AssignImageList(img_icons, wxIMAGE_LIST_SMALL );
#else
    images_list->AssignImageList(img_bicons, wxIMAGE_LIST_NORMAL );
#endif

    DEBUG_TRACE("");
    p_img = new wxBitmap( 0, 0 );
    wxPanel * img_p = XRCCTRL(*parent, "img_preview_unknown", wxPanel);
    wxPaintDC dc (img_p);

    DEBUG_TRACE("");
    canvas = new ImgPreview(img_p, wxPoint(0, 0), wxSize(256, 128), pano);

    DEBUG_TRACE("end");
    pano->addObserver(this);

    for ( int i = 0 ; i < 512 ; i++ )
      imgNr[i] = 0;
 
    changePano = FALSE;

    DEBUG_TRACE("end");
}


ImagesPanel::~ImagesPanel(void)
{
    pano.removeObserver(this);
//    delete p_img; // Dont know how to check for existing
    p_img = (wxBitmap *) NULL;
    delete images_list;
    DEBUG_TRACE("");
}

void ImagesPanel::FitParent( wxSizeEvent & e )
{
    wxSize new_size = GetSize();
//    wxSize new_size = GetParent()->GetSize();
    XRCCTRL(*this, "images_panel", wxPanel)->SetSize ( new_size );
//    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}

//void ImagesPanel::panoramaChanged (PT::Panorama &pano)
void ImagesPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & _imgNr)
{
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );

    // Is something gone or more here? 
    if ( (int)pano.getNrOfImages() != images_list2->GetItemCount() ) {
      imgNr[0] = 0;

      img_icons->RemoveAll();
      img_bicons->RemoveAll();
    DEBUG_INFO( "after delete are " << wxString::Format("%d", img_icons->GetImageCount() ) << " images inside img_icons")
    // start the loop for every selected filename
      for ( int i = 0 ; i <= (int)pano.getNrOfImages() - 1 ; i++ ) {
        wxFileName fn = (wxString)pano.getImage(i).getFilename().c_str();

//        wxImage * img = ImageCache::getInstance().getImage(
//            pano.getImage(i).getFilename());

        // preview selected images
        wxImage * s_img = ImageCache::getInstance().getImageSmall(
            pano.getImage(i).getFilename());

        // right image preview
        wxImage r_img;
        // left list control big icon
        wxImage b_img;
        // left list control icon
        wxImage i_img;
        if ( s_img->GetHeight() > s_img->GetWidth() ) {
          r_img = s_img->Scale( (int)((float)s_img->GetWidth()/
                                      (float)s_img->GetHeight()*128.0),
                                128);
          b_img = s_img->Scale( (int)((float)s_img->GetWidth()/
                                      (float)s_img->GetHeight()*64.0),
                                64);
          i_img = s_img->Scale( (int)((float)s_img->GetWidth()/
                                      (float)s_img->GetHeight()*20.0),
                                20);
        } else {
          r_img = s_img->Scale( 128, 
                                (int)((float)s_img->GetHeight()/
                                      (float)s_img->GetWidth()*128.0));
          b_img = r_img.Scale( 64,
                                (int)((float)r_img.GetHeight()/
                                      (float)r_img.GetWidth()*64.0));
          i_img = r_img.Scale( 20,
                                (int)((float)r_img.GetHeight()/
                                      (float)r_img.GetWidth()*20.0));

        }
        std::stringstream filename;
        filename << _("preview") << "_" << i << ".ppm" ;
        r_img.SaveFile(filename.str().c_str(), wxBITMAP_TYPE_PNM);
        DEBUG_INFO( "saving for preview: " << filename.str().c_str() )

        delete p_img;
        p_img = new wxBitmap( r_img.ConvertToBitmap() );
        canvas->Refresh();

        img_icons->Add( i_img.ConvertToBitmap() );
        img_bicons->Add( r_img.ConvertToBitmap() );
        DEBUG_INFO( "now " << wxString::Format("%d", img_icons->GetImageCount() ) << " images inside img_icons")

      }

      if ( pano.getNrOfImages() == 0 ) {
        delete p_img;
        p_img = new wxBitmap(0,0);
      }
      canvas->Refresh();

      wxListEvent e;
      SetImages( e );  // refresh the list settings of this class
    }


    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
    DEBUG_TRACE("");
}

void ImagesPanel::ChangePano ( std::string type, double var )
{
    changePano = TRUE;
//    DEBUG_TRACE("");
// DEBUG_TRACE("imgNr = "<<imgNr[0]<<" "<<imgNr[1]<<" "<<imgNr[2]<<" "<<imgNr[3]);

    ImageVariables new_var = pano.getVariable (orientationEdit_RefImg);
    // set all images with the same distances (only for yaw)
    double yaw_diff  = var - new_var.yaw.getValue();

    UIntSet imagesNr;
    VariablesVector vars;
    for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        imagesNr.insert (imgNr[i]);
        new_var = pano.getVariable(imgNr[i]);
        // set values
        if ( type == "yaw" ) {
          new_var.yaw.setValue(new_var.yaw.getValue() + yaw_diff);
          DEBUG_INFO( "var "<< var <<"  yaw_diff "<< yaw_diff )
        }
        if ( type == "yaw_text" ) {
          new_var.yaw.setValue(var);
        }
        if ( type == "pitch" ) {
          new_var.pitch.setValue(var);
        }
        if ( type == "roll" ) {
          new_var.roll.setValue(var);
        }
        vars.insert (vars.begin(), new_var);
        DEBUG_INFO( type <<" for image "<< imgNr[i] );
    }
        DEBUG_INFO( "");
    GlobalCmdHist::getInstance().addCommand(
          new PT::UpdateImagesVariablesCmd(pano, imagesNr, vars)
          );

    changePano = FALSE;

//    DEBUG_TRACE( "end" );
}


// #####  Here start the eventhandlers  #####

// Yaw by slider -> int -> double  -- roughly 
void ImagesPanel::SetYaw ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      int var = XRCCTRL(*this, "images_slider_yaw", wxSlider) ->GetValue();
      DEBUG_INFO ("yaw = " << var )

      char text[16];
      sprintf( text, "%d ", var );
      XRCCTRL(*this, "images_stext_orientation", wxStaticText) ->SetLabel(text);
      XRCCTRL(*this,"images_text_yaw",wxTextCtrl)->SetValue( doubleToString (
                                              (double) var ).c_str() );

      ChangePano ( "yaw" , (double) var );

    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}
void ImagesPanel::SetPitch ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      int var = XRCCTRL(*this,"images_slider_pitch",wxSlider) ->GetValue() * -1;
      DEBUG_INFO ("pitch = " << var )

      char text[16];
      sprintf( text, "%d", var );
      XRCCTRL(*this, "images_stext_orientation", wxStaticText) ->SetLabel(text);

      ChangePano ( "pitch" , (double) var );

      XRCCTRL(*this,"images_text_pitch",wxTextCtrl)->SetValue( doubleToString (
                                              (double) var ).c_str() );
    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}
void ImagesPanel::SetRoll ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      int var = XRCCTRL(*this, "images_slider_roll", wxSlider) ->GetValue();
      DEBUG_INFO ("roll = " << var )

      char text[16];
      sprintf( text, "%d", var );
      XRCCTRL(*this, "images_stext_roll", wxStaticText) ->SetLabel(text);

      ChangePano ( "roll" , (double) var );

      XRCCTRL(*this, "images_text_roll", wxTextCtrl)->SetValue( doubleToString (
                                              (double) var ).c_str() );
    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}
void ImagesPanel::SetYawPitch ( double coord_x, double coord_y ) {
    wxCommandEvent e;
    SetYaw (e);
    SetPitch (e);
}

// Yaw by text -> double
void ImagesPanel::SetYawText ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      wxString text = XRCCTRL(*this, "images_text_yaw"
                  , wxTextCtrl) ->GetValue();
      DEBUG_INFO ("yaw = " << text )

      double * var = new double ();
      text.ToDouble (var);

      char c[48];
      sprintf ( c, "%d", (int) *var );
      XRCCTRL(*this,"images_stext_orientation",wxStaticText)->SetLabel(c);

      ChangePano ( "yaw_text" , *var );

      XRCCTRL(*this,"images_slider_yaw",wxSlider) ->SetValue( (int) *var );

      delete var;
    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}
void ImagesPanel::SetPitchText ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      wxString text = XRCCTRL(*this, "images_text_pitch"
                  , wxTextCtrl) ->GetValue();
      DEBUG_INFO ("pitch = " << text )

      double * var = new double ();
      text.ToDouble (var);

      char c[48];
      sprintf ( c, "%d", (int) *var );
      XRCCTRL(*this,"images_stext_orientation",wxStaticText)->SetLabel(c);

      ChangePano ( "pitch" , *var );

      XRCCTRL(*this,"images_slider_pitch",wxSlider)->SetValue( (int) *var * -1);

      delete var;
    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}
void ImagesPanel::SetRollText ( wxCommandEvent & e )
{
    if ( imgNr[0] > 0 ) {
      wxString text = XRCCTRL(*this, "images_text_roll"
                  , wxTextCtrl) ->GetValue();
      DEBUG_INFO ("roll = " << text )

      double * var = new double ();
      text.ToDouble (var);

      char c[48];
      sprintf ( c, "%d", (int) *var );
      XRCCTRL(*this,"images_stext_roll",wxStaticText)->SetLabel(c);

      ChangePano ( "roll" , *var );

      XRCCTRL(*this,"images_slider_roll",wxSlider) ->SetValue( (int) *var );

      delete var;
    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}

// Inheritance + Optimization

void ImagesPanel::SetInherit( std::string type )
{
//    "roll", "images_inherit_roll", "images_spin_roll", "images_optimize_roll" 
    // requisites
    int var (-1);
//    std::string command;
    std::string xml_inherit, xml_optimize, xml_spin;
    ImageVariables new_var;

    if ( imgNr[0] > 0 ) { // dont work on an empty image
      // set xml resource names
      xml_inherit = "images_inherit_"; xml_inherit.append(type);
      xml_optimize = "images_optimize_"; xml_optimize.append(type);
      xml_spin = "images_spin_"; xml_spin.append(type);
      // for all images
      UIntSet imagesNr;
      VariablesVector vars;
      for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        imagesNr.insert (imgNr[i]);
        new_var = pano.getVariable(imgNr[i]);
        // set inheritance from image with number ...
        var = XRCCTRL(*this, xml_spin .c_str(),wxSpinCtrl)->GetValue();
        DEBUG_INFO("var("<<var<<") == I("<<(int)imgNr[i]<<")  :  | pano");
        // Shall we inherit?
        if ( XRCCTRL(*this, xml_inherit .c_str(),wxCheckBox)->IsChecked()
             && (pano.getNrOfImages() != 1) ) { // single image cannot inherit
            // We are conservative and ask better once more. 
            if ( type == "yaw" ) {
              // test for unselfish inheritance
              if ( var != (int)imgNr[i] ) {
                new_var.yaw.link(var);
                optset->at(imgNr[i]).yaw = FALSE;
              } else { // search for another possible link image
                if ( (((int)new_var. yaw .getLink() > var) && (var != 0)) 
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.yaw.link(var);
                } else {
                  var++; new_var.yaw.link(var);
                }
              }
            }
            if ( type == "pitch" ) {
              if ( var != (int)imgNr[i] ) {
                new_var.pitch.link(var);
                optset->at(imgNr[i]).pitch = FALSE;
              } else { // search for another possible link image
                if ( ((int)new_var. pitch .getLink() > var) && (var != 0)
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.pitch.link(var);
                } else {
                  var++; new_var.pitch.link(var);
                }
              }
            }
            if ( type == "roll" ) {
              if ( var != (int)imgNr[i] ) {
                new_var.roll.link(var);
                optset->at(imgNr[i]).roll = FALSE;
              } else { // search for another possible link image
                if ( ((int)new_var. roll .getLink() > var) && (var != 0)
                     || (imgNr[i] == pano.getNrOfImages()-1) ) {
                  var--; new_var.roll.link(var);
                } else {
                  var++; new_var.roll.link(var);
                }
              }
            }
            // ... and set controls
            XRCCTRL(*this, xml_spin .c_str(),wxSpinCtrl)->SetValue(var);
            XRCCTRL(*this, xml_optimize .c_str(),wxCheckBox)->SetValue(FALSE);
            DEBUG_INFO("var("<<var<<") == I("<<(int)imgNr[i]<<")  :  | pano");
            // local ImageVariables finished, save to pano
//            pano.updateVariables( imgNr[i], new_var ); 
            DEBUG_INFO ( new_var.yaw.getLink() <<" "<< pano.getVariable(orientationEdit_RefImg).yaw.getValue() )//<<" "<< pano.getVariable(imgNr[i]).yaw.getLink() )
          // unset inheritance
          } else {
            if ( type == "yaw" )
              new_var.yaw.unlink();
            if ( type == "pitch" )
              new_var.pitch.unlink();
            if ( type == "roll" )
              new_var.roll.unlink();
            XRCCTRL(*this,xml_inherit.c_str(),wxCheckBox)->SetValue(FALSE);
          }
          // local ImageVariables finished, save to pano
//          pano.updateVariables( imgNr[i], new_var ); 
          // set optimization
          if (XRCCTRL(*this,xml_optimize.c_str(),wxCheckBox)->IsChecked()){
            if ( type == "yaw" ) {
              optset->at(imgNr[i]).yaw = TRUE;
            }
            if ( type == "pitch" ) {
              optset->at(imgNr[i]).pitch = TRUE;
            }
            if ( type == "roll" ) {
              optset->at(imgNr[i]).roll = TRUE;
            }
            XRCCTRL(*this,xml_inherit.c_str(),wxCheckBox)->SetValue(FALSE);
          // unset optimization
          } else if(!XRCCTRL(*this,xml_optimize.c_str(),wxCheckBox)->IsChecked()){
            if ( type == "yaw" )
              optset->at(imgNr[i]).yaw = FALSE;
            if ( type == "pitch" )
              optset->at(imgNr[i]).pitch = FALSE;
            if ( type == "roll" )
              optset->at(imgNr[i]).roll = FALSE;
          }
          vars.insert (vars.begin(), new_var);
        }

          // activate an undoable command, not for the optimize settings
        GlobalCmdHist::getInstance().addCommand(
          new PT::UpdateImagesVariablesCmd(pano, imagesNr, vars)
          );
    }
    DEBUG_INFO( type.c_str() << " end" )
}

void ImagesPanel::SetInheritYaw( wxCommandEvent & e )
{
    SetInherit ( "yaw" );
}
void ImagesPanel::SetInheritPitch( wxCommandEvent & e )
{
    SetInherit ( "pitch" );
}
void ImagesPanel::SetInheritRoll( wxCommandEvent & e )
{
    SetInherit ( "roll" );
}

void ImagesPanel::SetOptimizeYaw( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_yaw" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "yaw" );
}
void ImagesPanel::SetOptimizePitch( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_pitch" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "pitch" );
}
void ImagesPanel::SetOptimizeRoll( wxCommandEvent & e )
{
    XRCCTRL(*this, "images_inherit_roll" ,wxCheckBox)->SetValue(FALSE);
    SetInherit ( "roll" );
}



// ######  Here end the eventhandlers  #####


void ImagesPanel::SetImages ( wxListEvent & e )
{
    DEBUG_TRACE("changePano = " << changePano );
    if ( changePano == FALSE ) {

      // set link controls
      #define SET_SPIN_RANGE(type) \
      XRCCTRL(*this, type ,wxSpinCtrl)->   \
                 SetRange( 0 , (int) pano.getNrOfImages() - 1);
      SET_SPIN_RANGE ("images_spin_yaw")
      SET_SPIN_RANGE ("images_spin_pitch")
      SET_SPIN_RANGE ("images_spin_roll")



      // One image is our prefered image.
      orientationEdit_RefImg = e.GetIndex();

      // get the list to read from
      wxListCtrl* lst =  XRCCTRL(*this, "images_list_unknown", wxListCtrl);

      // prepare an status message
      wxString e_msg;
      int sel_I = lst->GetSelectedItemCount();
      if ( sel_I == 1 ) e_msg = _("Selected image:   ");
      else e_msg = _("Selected images:   ");

      // publicate the selected items(images)
      imgNr[0] = 0;             // reset
      for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
        if ( lst->GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
          e_msg += "  " + lst->GetItemText(Nr);
          imgNr[0] += 1;
          imgNr[imgNr[0]] = Nr;
        }
      }
      DEBUG_INFO (e_msg)

      // gui values to set
      // set sliders in the first tab
      XRCCTRL(*this, "images_text_yaw", wxTextCtrl)->SetValue(doubleToString (
                                                     GET_VAR( yaw )).c_str());
      XRCCTRL(*this, "images_slider_yaw" , wxSlider)->SetValue(
                                                (int)GET_VAR( yaw ));
      XRCCTRL(*this, "images_text_pitch", wxTextCtrl)->SetValue(doubleToString (
                                                     GET_VAR( pitch )).c_str());
      XRCCTRL(*this, "images_slider_pitch" , wxSlider)->SetValue(
                                                (int)GET_VAR( pitch ) * -1);
      XRCCTRL(*this, "images_text_roll", wxTextCtrl)->SetValue(doubleToString (
                                                     GET_VAR( roll )).c_str());
      XRCCTRL(*this, "images_slider_roll" , wxSlider)->SetValue(
                                                (int)GET_VAR( roll ));

      XRCCTRL(*this, "images_stext_orientation", wxStaticText) ->SetLabel("");
      XRCCTRL(*this, "images_stext_roll", wxStaticText) ->SetLabel("");
      // set inherit and from wich image, dis-/enable optimize checkboxes
      ImageVariables new_var ( pano.getVariable(orientationEdit_RefImg) );
      if ( new_var.yaw.isLinked() ) {
         XRCCTRL(*this, "images_inherit_yaw" , wxCheckBox) ->SetValue(TRUE);
         int var = (int)new_var. yaw .getLink();
         XRCCTRL(*this, "images_spin_yaw" , wxSpinCtrl) ->SetValue(var);
//         XRCCTRL(*this, "images_optimize_yaw" , wxCheckBox) ->Disable();
         DEBUG_INFO (var << " " << (int)new_var.yaw.isLinked() )
      } else {
         DEBUG_INFO ( (int)new_var.yaw.isLinked() << " " << (int)new_var. yaw .getLink() )
         XRCCTRL(*this, "images_inherit_yaw" , wxCheckBox) ->SetValue(FALSE);
//         XRCCTRL(*this, "images_optimize_yaw" , wxCheckBox) ->Enable();
      }
      if ( new_var.pitch.isLinked() ) {
         XRCCTRL(*this, "images_inherit_pitch" , wxCheckBox) ->SetValue(TRUE);
         int var = (int)new_var. pitch .getLink();
         XRCCTRL(*this, "images_spin_pitch" , wxSpinCtrl) ->SetValue(var);
//         XRCCTRL(*this, "images_optimize_pitch" , wxCheckBox) ->Disable();
      } else {
         XRCCTRL(*this, "images_inherit_pitch" , wxCheckBox) ->SetValue(FALSE);
//         XRCCTRL(*this, "images_optimize_pitch" , wxCheckBox) ->Enable();
      }
      if ( new_var.roll.isLinked() ) {
         XRCCTRL(*this, "images_inherit_roll" , wxCheckBox) ->SetValue(TRUE);
         int var = (int)new_var. roll .getLink();
         XRCCTRL(*this, "images_spin_roll" , wxSpinCtrl) ->SetValue(var);
//         XRCCTRL(*this, "images_optimize_roll" , wxCheckBox) ->Disable();
      } else {
         XRCCTRL(*this, "images_inherit_roll" , wxCheckBox) ->SetValue(FALSE);
//         XRCCTRL(*this, "images_optimize_roll" , wxCheckBox) ->Enable();
      }
      // enable disable optmize check boxes
      if (optset->at(orientationEdit_RefImg).yaw == TRUE ) {
         XRCCTRL(*this, "images_optimize_yaw" , wxCheckBox) ->SetValue(TRUE);
      } else {
         XRCCTRL(*this, "images_optimize_yaw" , wxCheckBox) ->SetValue(FALSE);
      }
      if (optset->at(orientationEdit_RefImg).pitch == TRUE ) {
         XRCCTRL(*this, "images_optimize_pitch" , wxCheckBox) ->SetValue(TRUE);
      } else {
         XRCCTRL(*this, "images_optimize_pitch" , wxCheckBox) ->SetValue(FALSE);
      }
      if (optset->at(orientationEdit_RefImg).roll == TRUE ) {
         XRCCTRL(*this, "images_optimize_roll" , wxCheckBox) ->SetValue(TRUE);
      } else {
         XRCCTRL(*this, "images_optimize_roll" , wxCheckBox) ->SetValue(FALSE);
      }


    }

    changePano = FALSE;
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ImgPreview, wxScrolledWindow)
    //EVT_PAINT(ImgPreview::OnPaint)
    EVT_MOUSE_EVENTS ( ImgPreview::OnMouse )
END_EVENT_TABLE()

// Define a constructor for my canvas
ImgPreview::ImgPreview(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama *pano)
  : wxScrolledWindow(parent, -1, pos, size),
    pano(*pano)
{
    DEBUG_TRACE("");
}

ImgPreview::~ImgPreview(void)
{
    p_img = (wxBitmap *) NULL;
    DEBUG_TRACE("");
}

// Define the repainting behaviour
void ImgPreview::OnDraw(wxDC & dc)
{
  if ( p_img && p_img->Ok() )
  {
    wxMemoryDC memDC;
    memDC.SelectObject(* p_img);

    // Transparent blitting if there's a mask in the bitmap
    dc.Blit(0, 0, p_img->GetWidth(), p_img->GetHeight(), & memDC,
      0, 0, wxCOPY, TRUE);

    //img_icons->Draw( 0, dc, 0, 0, wxIMAGELIST_DRAW_NORMAL, FALSE);

    memDC.SelectObject(wxNullBitmap);
  }
}

void ImgPreview::ChangePreview ( std::string filename )
{
    wxFileName fn = (wxString)filename.c_str();
    if ( fn.IsOk() && fn.FileExists() ) {
//    DEBUG_INFO ( "hier: is item " << fn.GetFullPath() );
          // right image preview
          wxImage * s_img;
          s_img = new wxImage (filename.c_str());
          wxImage r_img;

          int new_width;
          int new_height;
          if ( ((float)s_img->GetWidth() / (float)s_img->GetHeight())
                > 2.0 ) {
            new_width =  256;
            new_height = (int)((float)s_img->GetHeight()/
                                        (float)s_img->GetWidth()*256.0);
          } else {
            new_width = (int)((float)s_img->GetWidth()/
                                        (float)s_img->GetHeight()*128.0);
            new_height = 128;
          }

          r_img = s_img->Scale( new_width, new_height );
          delete p_img;
          p_img = new wxBitmap( r_img.ConvertToBitmap() );
          Refresh();
          delete s_img;
    }
//    wxPoint pos = e.GetPosition();
//    long item = HitTest( e.m_x ,e.m_y );
//    DEBUG_INFO ( "hier:" << wxString::Format(" %d is item %ld", e.GetPosition(), item) );
//    DEBUG_INFO ( "hier: is item " << filename );
}

void ImgPreview::OnMouse ( wxMouseEvent & e )
{
    if (e.Entering() || e.Leaving()) {
      PanoramaOptions opt = pano.getOptions();
      ChangePreview (opt.outfile);
//      frame->SetStatusText(wxString::Format("set %s", opt.outfile.c_str()),0);
    }

    double coord_x = (double)e.m_x/256.0*360.0 -180.0;
    double coord_y = (double)e.m_y/128.0* -180.0 + 90.0; 

    frame->SetStatusText(wxString::Format("%d°,%d°",
              (int)coord_x,
              (int)coord_y ), 1);

    // mouse yaw and pitch -> sliders
    if ( e.m_shiftDown || (e.m_shiftDown && e.m_controlDown) 
           || (!e.m_shiftDown && !e.m_controlDown)) 
      XRCCTRL(*images_panel,"images_slider_pitch",wxSlider)
                                       ->SetValue((int) -coord_y);
    if ( e.m_controlDown || (e.m_shiftDown && e.m_controlDown) 
           || (!e.m_shiftDown && !e.m_controlDown)) 
      XRCCTRL(*images_panel,"images_slider_yaw", wxSlider)
                                       ->SetValue((int) coord_x);

    wxCommandEvent event;
    // mouse yaw and pitch -> pano
    if ( e.m_leftDown ) {
      if ( (!e.m_shiftDown && !e.m_controlDown) || (e.m_shiftDown && e.m_controlDown) ) {
        images_panel->SetYawPitch( coord_x, coord_y );
      } else {
        if ( e.m_shiftDown ) {
          images_panel->SetPitch (event);
        }
        if ( e.m_controlDown ) {
          images_panel->SetYaw (event);
        }
      }
    }
    // pano yaw and pitch -> sliders
    if ((images_panel->imgNr[0] >= 1)) {
      if (e.Leaving() || (e.m_controlDown 
           && !(e.m_shiftDown && e.m_controlDown))) 
        XRCCTRL(*images_panel,"images_slider_pitch",wxSlider) ->SetValue(
             (int) pano.getVariable(images_panel->imgNr[1]) .pitch .getValue());
      if (e.Leaving() || (e.m_shiftDown
            && !(e.m_shiftDown && e.m_controlDown)))
        XRCCTRL(*images_panel,"images_slider_yaw",wxSlider) ->SetValue(
             (int) pano.getVariable(images_panel->imgNr[1]) .yaw .getValue());
    }

//    DEBUG_INFO ( "Mouse " << e.Entering() << e.Leaving());
} 

