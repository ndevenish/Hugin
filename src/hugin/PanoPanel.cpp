// -*- c-basic-offset: 4 -*-

/** @file PanoPanel.cpp
 *
 *  @brief implementation of PanoPanel Class
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
//#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/image.h>
#include <wx/spinctrl.h>

#include "PT/PanoCommand.h"
#include "PT/PanoramaMemento.h"
#include "PT/Panorama.h"
//#include "hugin/config.h"
#include "hugin/CommandHistory.h"
//#include "hugin/ImageCache.h"
//#include "hugin/CPEditorPanel.h"
//#include "hugin/List.h"
//#include "hugin/LensPanel.h"
//#include "hugin/ImagesPanel.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/PanoPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/Events.h"
#include "PT/Panorama.h"

using namespace PT;

// image preview
extern wxBitmap *p_img;
extern ImgPreview *canvas;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PanoPanel, wxWindow)
//    EVT_TEXT_ENTER ( XRCID("panorama_panel"),PanoPanel::PanoChanged )
  EVT_SIZE   ( PanoPanel::Resize )

  EVT_BUTTON   ( XRCID("pano_make_dialog"),PanoPanel::DoDialog )


  EVT_BUTTON   ( XRCID("pano_optimizer_start"),PanoPanel::DoOptimization )

  EVT_CHOICE ( XRCID("pano_choice_panoType"),PanoPanel::ProjectionChanged )
  EVT_CHOICE ( XRCID("pano_choice_interpolator"),PanoPanel::InterpolatorChanged)
  EVT_COMBOBOX ( XRCID("pano_val_hfov"),PanoPanel::HFOVChanged )
  EVT_TEXT_ENTER ( XRCID("pano_val_gamma"),PanoPanel::GammaChanged )
  EVT_CHOICE ( XRCID("pano_choice_colour_mode"),PanoPanel::ColourModeChanged)
  EVT_SPINCTRL(XRCID("pano_spin_colour_reference"),PanoPanel::ColourModeChanged)

  EVT_COMBOBOX ( XRCID("pano_val_previewWidth"),PanoPanel::previewWidthChanged )
  EVT_COMBOBOX (XRCID("pano_val_previewHeight"),PanoPanel::previewHeightChanged)
  EVT_BUTTON   ( XRCID("pano_button_preview"),PanoPanel::DoPreview )
  EVT_CHECKBOX ( XRCID("pano_cb_auto_preview"),PanoPanel::autoPreview )
  EVT_CHECKBOX ( XRCID("pano_cb_auto_optimize"),PanoPanel::autoOptimize )
  EVT_CHECKBOX ( XRCID("pano_cb_panoviewer_enabled"),
                                               PanoPanel::panoviewerEnabled )
  EVT_CHECKBOX ( XRCID("pano_cb_panoviewer_precise"),
                                               PanoPanel::panoviewerPrecise )
  EVT_SPINCTRL ( XRCID("pano_spin_single_preview"),
                                               PanoPanel::previewSingleChanged)
  EVT_CHECKBOX ( XRCID("pano_cb_single_preview"),
                                               PanoPanel::previewSingleChanged)

  EVT_CHOICE   ( XRCID("pano_choice_formatFinal"),PanoPanel::FinalFormatChanged)
  EVT_COMBOBOX ( XRCID("pano_val_width"),PanoPanel::WidthChanged )
  EVT_COMBOBOX ( XRCID("pano_val_height"),PanoPanel::HeightChanged )
  EVT_COMBOBOX ( XRCID("pano_val_jpegQuality"),PanoPanel::JpegQChanged )
  EVT_CHECKBOX ( XRCID("pano_bool_jpegProgressive"),PanoPanel::JpegPChanged )

    EVT_BUTTON     ( XRCID("pano_button_stitch"),PanoPanel::Stitch )
END_EVENT_TABLE()


// Define a constructor for the Pano Panel
PanoPanel::PanoPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : wxPanel (parent, -1, wxDefaultPosition, wxDefaultSize, 0),
      pano(*pano)
{
//    opt = new PanoramaOptions();

    // loading xrc resources in selfcreated this panel
    wxXmlResource::Get()->LoadPanel ( this, wxT("panorama_panel")); //);

    // set defaults from gui;
    auto_preview = TRUE;
    auto_optimize = TRUE;
    auto_optimize_run = FALSE;
    panoviewer_enabled = TRUE;
    panoviewer_precise = FALSE;
    panoviewer_started = FALSE;

    changePano = TRUE;

    pano->addObserver (this);

    changePano = FALSE;
    DEBUG_TRACE("")
}


PanoPanel::~PanoPanel(void)
{
    pano.removeObserver(this);
    DEBUG_TRACE("");
}

void PanoPanel::Resize( wxSizeEvent & e )
{
    if ( wxGetApp().Initialized() && pano_dlg_run && !self_pano_dlg ) {
      wxPoint pos = frame->GetPosition();
      wxSize frame_size = frame->GetSize();
      pos.y = pos.y + frame_size.GetHeight()
#if defined(__WXGTK__)
      +25 // for the status bar
#endif
      ;
      pano_dlg->Move (pos.x, pos.y);
      if ( frame->IsIconized() )
        pano_dlg->Iconize(TRUE);
      else
        pano_dlg->Maximize(TRUE);
        pano_dlg->Iconize(FALSE);
    }

}

void PanoPanel::DoDialog ( wxCommandEvent & e )
{
    DEBUG_TRACE("");
    // we open only on dialog window , next click we close the dialog
    if ( pano_dlg_run ) {
      if ( self_pano_dlg ) {
        pano_panel->pano_dlg_run = FALSE;
        pano_panel->pano_dlg->Destroy();
      } else {
        pano_dlg->Destroy();
        pano_dlg_run = FALSE;
      }
      XRCCTRL(*pano_panel, "pano_make_dialog", wxPanel)
                    ->SetToolTip(_("tear off this tab"));
    } else {
        DEBUG_TRACE("" <<   GetName() );
        DEBUG_TRACE("" <<   GetParent()->GetName() );
        DEBUG_TRACE("" <<   GetGrandParent()->GetName() );
        DEBUG_TRACE("" <<   GetParent()->GetGrandParent()->GetLabel() );
        DEBUG_TRACE("" <<   frame->GetLabel() );
      wxSize frame_size = frame->GetSize();
      wxPoint frame_pos = frame->GetPosition();
      DEBUG_INFO( "pos.x "<< frame_pos.x <<"  pos.y "<< frame_pos.y )
      frame_pos.y = frame_pos.y + frame_size.GetHeight()
#if defined(__WXGTK__)
      +25 // for the status bar
#endif
      ;
      pano_dlg = new PanoDialog (this, frame_pos,
                         XRCCTRL(*this, "panorama_panel", wxPanel)->GetSize(),
                         &pano);
      pano_dlg->SetModal(FALSE);
      pano_dlg_run = TRUE;
      XRCCTRL(*this, "pano_make_dialog", wxPanel)
                    ->SetToolTip(_("Close child window"));
      XRCCTRL(*pano_dlg->pp, "pano_make_dialog", wxPanel)
                    ->SetToolTip(_("Close child window"));
//      UIntSet i;
//      pano_dlg->pp->panoramaImagesChanged (pano, i); // initialize
      pano_dlg->pp->pano_dlg_run = TRUE;
      pano_dlg->pp->self_pano_dlg = TRUE;
      pano_dlg->pp->previewWidth = previewWidth;
      pano_dlg->pp->previewHeight = previewHeight;
      pano_dlg->pp->PanoOptionsChanged (opt);

    }
    DEBUG_TRACE("");
}

void PanoPanel::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
    opt = pano.getOptions();
    if ( !changePano ) {
      changePano = TRUE;

      // update all options for dialog and notebook tab
      PanoOptionsChanged (opt);

      if ( !auto_optimize_run ) {
        if ( auto_optimize && !self_pano_dlg ) {
          DEBUG_TRACE ("")
          auto_optimize_run = TRUE;
          wxCommandEvent e;
          DoOptimization(e);
        }

        int id (XRCID("pano_button_preview"));
        wxCommandEvent  e;
        e.SetId(id);
        if (auto_preview && !self_pano_dlg && !panoviewer_started)
          DoPreview (e);

      } else {
        auto_optimize_run = FALSE;
      }
    }
    changePano = FALSE;
}

void PanoPanel::Optimize (OptimizeVector & optvars, PanoramaOptions & output)
{
#ifdef __unix__
    GlobalCmdHist::getInstance().addCommand(
        new PT::OptimizeCmd( pano, optvars, output)
        );
#else
    wxLogError("Optimize not implemented under windows yet");
#endif

}

void PanoPanel::DoOptimization (wxCommandEvent & e)
{
    // TODO ask which variables to optimize - > wxComboBox pano_optimizer_level
    Optimize(*optset, opt);
}
void PanoPanel::autoOptimize ( wxCommandEvent & e )
{
    if ( XRCCTRL(*this, "pano_cb_auto_optimize", wxCheckBox)
         ->IsChecked() ) {
        DEBUG_INFO ("set auto optimize")
        auto_optimize = true;
    } else {
        DEBUG_INFO ("unset auto optimize")
        auto_optimize = false;
    }

    if ( pano_dlg_run ) {  // tell the other window
      if ( self_pano_dlg ) {
        pano_panel->auto_optimize = auto_optimize;
        XRCCTRL(*pano_panel, "pano_cb_auto_optimize", wxCheckBox)
                  ->SetValue(auto_optimize);
      } else {
        pano_dlg->pp->auto_optimize = auto_optimize;
        XRCCTRL(*pano_dlg->pp, "pano_cb_auto_optimize", wxCheckBox)
                  ->SetValue(auto_optimize);
      }
    }
}

void PanoPanel::DoPreview ( wxCommandEvent & e )
{

    DEBUG_TRACE("self_pano_dlg " << self_pano_dlg);
    if (!self_pano_dlg && (pano.getNrOfImages() > 0) ) {
      std::stringstream filename;
      int old_previewWidth = previewWidth;
      int old_previewHeight = previewHeight;
      Panorama preview_pano = pano;
      PanoramaOptions preview_opt = opt;
      if ( !panoviewer_enabled ) {
        previewWidth = 256;
        previewHeight = 128;
      }
      preview_opt.width = previewWidth;
      preview_opt.height = previewHeight;
      if ( previewHeight <= 0 )
        preview_opt.height = previewWidth / 2;
      wxString outputFormat = preview_opt.outputFormat.c_str();
      filename <<_("preview")<<".JPG" ;
      preview_opt.outfile = filename.str();
      preview_opt.outputFormat = "JPEG";
      if ( outputFormat != "JPEG" )
        preview_opt.quality = 100;

      // Set the preview image accordingly to ImagesPanel.cpp
      for (int imgNr=(int)pano.getNrOfImages()-1; imgNr >= 0; imgNr--) {
        if ( preview_single && !(imgNr == (int)previewSingle) ) {
          DEBUG_INFO ( "preview_single("<< preview_single <<"): " << previewSingle )
          if ( imgNr > (int)previewSingle ) {
            preview_pano.removeImage(imgNr);
          } else {
            preview_pano.removeImage(0);
          }
          DEBUG_INFO ("remove image: "<< imgNr)
        } else {
          // test for needed precision of source image
          double p_width =(double)pano.getImage((unsigned int)imgNr).getWidth()/
                         (double)pano.getImage((unsigned int)imgNr).getHeight()*
                         128.0;
          if (p_width > 128.0) p_width = 128.0;
          int source_pixels = (int)(p_width *
                            preview_opt.HFOV /
                            pano.getVariable((unsigned int)imgNr).HFOV.getValue() );
          DEBUG_INFO ( source_pixels <<" source:target "<< preview_opt.width )
          if ( ((source_pixels >= (int)preview_opt.width)
                && !panoviewer_precise) 
               || !panoviewer_enabled ) {
            wxFileName fn= (wxString)pano.getImage((unsigned int)imgNr).getFilename().c_str();
            filename.str("");
            filename
#if 0
          << fn.GetPath(wxPATH_GET_SEPARATOR|wxPATH_GET_VOLUME).c_str()
#endif
                  <<_("preview")<<"_"<< imgNr <<".ppm" ;
            PanoImage image = preview_pano.getImage((unsigned int)imgNr);
            image.setFilename( filename.str() ) ;
            preview_pano.setImage((unsigned int)imgNr, image);
            DEBUG_INFO ("rendering preview image: "<< filename.str())
          }
        }
      }

      preview_pano.clearObservers();
#ifdef __unix__
      Process process(false);
      preview_pano.runStitcher(process, preview_opt);
      process.wait();
#else
      wxLogError("Preview not implemented under windows yet");
#endif

      previewWidth = old_previewWidth;
      previewHeight = old_previewHeight;

      // Send panoViewer the name of our image
      if ( panoviewer_enabled ) {
        server->projectionFormat = preview_opt.projectionFormat;
        server->height = (int)180;
        server->width = (int)preview_opt.HFOV;
        if (  preview_opt.projectionFormat != PanoramaOptions::EQUIRECTANGULAR )
          server->resetView = TRUE;
//        server->showGrid = TRUE;
        server->SendFilename( (wxString) preview_opt.outfile.c_str() );
//        panoviewer_started = TRUE;
      }

      // Show the result image in the first tab
          // right image preview
          wxImage * s_img;
          s_img = new wxImage (preview_opt.outfile.c_str());
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
          canvas->Refresh();
          delete s_img;
    } else {  // if ( panoviewer_enabled )
      if ( pano.getNrOfImages() > 0 )
        pano_panel->DoPreview(e);
    }
/*
    if ( !preview_dlg ) {
        wxTheXmlResource->LoadDialog(preview_dlg, frame, "pano_dlg_preview");
    DEBUG_INFO ( "" )
//        preview_dlg = XRCCTRL(*this, "pano_dlg_preview", wxDialog);
    DEBUG_INFO ( "" )
        CPImageCtrl * preview_win = new CPImageCtrl(this);
    DEBUG_INFO ( ": " << opt.width )
//        wxXmlResource::Get()->AttachUnknownControl(wxT("pano_preview_unknown"),
//                                               preview_win);
    DEBUG_INFO ( "" )
        preview_dlg->ShowModal();
    DEBUG_INFO ( "" )
    }*/
    DEBUG_INFO ( "" )
}
void PanoPanel::autoPreview ( wxCommandEvent & e )
{
    if ( XRCCTRL(*this, "pano_cb_auto_preview", wxCheckBox)
         ->IsChecked() ) {
        DEBUG_INFO ("set auto preview")
        auto_preview = true;
    } else {
        DEBUG_INFO ("unset auto prievew")
        auto_preview = false;
    }

    if ( pano_dlg_run ) {  // tell the other window
      if ( self_pano_dlg ) {
        pano_panel->auto_preview = auto_preview;
        XRCCTRL(*pano_panel, "pano_cb_auto_preview", wxCheckBox)
                  ->SetValue(auto_preview);
      } else {
        pano_dlg->pp->auto_preview = auto_preview;
        XRCCTRL(*pano_dlg->pp, "pano_cb_auto_preview", wxCheckBox)
                  ->SetValue(auto_preview);
      }
    }

}
void PanoPanel::panoviewerEnabled ( wxCommandEvent & e )
{
    if ( XRCCTRL(*this, "pano_cb_panoviewer_enabled", wxCheckBox)
         ->IsChecked() ) {
        DEBUG_INFO ("set panoviewer_enabled")
        panoviewer_enabled = true;
    } else {
        DEBUG_INFO ("unset panoviewer_enabled")
        panoviewer_enabled = false;
    }

    if ( pano_dlg_run ) {  // tell the other window
      if ( self_pano_dlg ) {
        pano_panel->panoviewer_enabled = panoviewer_enabled;
        XRCCTRL(*pano_panel, "pano_cb_panoviewer_enabled", wxCheckBox)
                  ->SetValue(panoviewer_enabled);
      } else {
        pano_dlg->pp->panoviewer_enabled = panoviewer_enabled;
        XRCCTRL(*pano_dlg->pp, "pano_cb_panoviewer_enabled", wxCheckBox)
                  ->SetValue(panoviewer_enabled);
      }
    }

}
void PanoPanel::panoviewerPrecise ( wxCommandEvent & e )
{
    if ( XRCCTRL(*this, "pano_cb_panoviewer_precise", wxCheckBox)
         ->IsChecked() ) {
        DEBUG_INFO ("set panoviewer_precise")
        panoviewer_precise = true;
    } else {
        DEBUG_INFO ("unset panoviewer_precise")
        panoviewer_precise = false;
    }

    if ( pano_dlg_run ) {  // tell the other window
      if ( self_pano_dlg ) {
        pano_panel->panoviewer_precise = panoviewer_precise;
        XRCCTRL(*pano_panel, "pano_cb_panoviewer_precise", wxCheckBox)
                  ->SetValue(panoviewer_precise);
      } else {
        pano_dlg->pp->panoviewer_precise = panoviewer_precise;
        XRCCTRL(*pano_dlg->pp, "pano_cb_panoviewer_precise", wxCheckBox)
                  ->SetValue(panoviewer_precise);
      }
    }

}


#define OPT_TO_COMBOBOX( xml_combo, type ) \
{ \
    lt = XRCCTRL(*this, xml_combo , wxComboBox) \
                            -> FindString( DoubleToString (type) ) ; \
    std::string number = XRCCTRL(*this, xml_combo, wxComboBox) \
                            ->GetString(lt).c_str(); \
    std::stringstream compare_s; \
    compare_s << type; \
    if ( atof(number.c_str()) != atof(compare_s.str().c_str()) ) { \
      XRCCTRL(*this, xml_combo, wxComboBox) \
                            -> SetValue(compare_s.str().c_str()); \
    } else { \
      XRCCTRL(*this, xml_combo, wxComboBox) \
                            ->SetSelection(lt) ; \
    } \
    /*DEBUG_INFO ( "updating " << xml_combo << type )*/ \
}

void PanoPanel::PanoOptionsChanged ( PanoramaOptions &o )
{
    changePano = TRUE;
    DEBUG_INFO ( "updating the other PanoPanel " << self_pano_dlg )

    opt = o;

    int lt;
//    UIntSet imgNr;

    // setting number of available images
    XRCCTRL(*this, "pano_spin_single_preview" ,wxSpinCtrl)->
                 SetRange( 0 , (int) pano.getNrOfImages() - 1);
    XRCCTRL(*this, "pano_spin_optimizer_reference" ,wxSpinCtrl)->
                 SetRange( 0 , (int) pano.getNrOfImages() - 1);
    XRCCTRL(*this, "pano_spin_colour_reference" ,wxSpinCtrl)->
                 SetRange( 0 , (int) pano.getNrOfImages() - 1);


//    ColourModeChanged (e);
    XRCCTRL(*this, "pano_choice_colour_mode"
                  , wxChoice) ->SetSelection(opt.colorCorrection);
    XRCCTRL(*this, "pano_spin_colour_reference"
                  , wxSpinCtrl)->SetValue(opt.colorReferenceImage);
//    GammaChanged (e);
    XRCCTRL(*this, "pano_val_gamma", wxTextCtrl)
                ->SetValue( wxString::Format ("%f", opt.gamma) );
//    HFOVChanged (e);
    OPT_TO_COMBOBOX ( "pano_val_hfov", opt.HFOV )
//    InterpolatorChanged (e);
    XRCCTRL(*this, "pano_choice_interpolator", wxChoice)
                            ->SetSelection(opt.interpolator);
//    ProjectionChanged (e);
    XRCCTRL(*this, "pano_choice_panoType", wxChoice)
                            ->SetSelection(opt.projectionFormat);
//    previewWidthChanged (e);
    OPT_TO_COMBOBOX ( "pano_val_previewWidth", previewWidth )
//    previewHeightChanged (e);
    OPT_TO_COMBOBOX ( "pano_val_previewHeight", previewHeight )
//    previewSingleChanged (e);
    XRCCTRL(*this, "pano_spin_single_preview"
                  , wxSpinCtrl)->SetValue(previewSingle);
    XRCCTRL(*this, "pano_cb_single_preview", wxCheckBox)
                            ->SetValue( preview_single ) ;
//    FinalFormatChanged (e);
    lt = XRCCTRL(*this, "pano_choice_formatFinal", wxChoice)
                        ->FindString(opt.outputFormat.c_str());
    XRCCTRL(*this, "pano_choice_formatFinal", wxChoice)
                            ->SetSelection(lt);
//    WidthChanged (e);
    OPT_TO_COMBOBOX ( "pano_val_width", opt.width )
//    HeightChanged (e);
    OPT_TO_COMBOBOX ( "pano_val_height", opt.height )
//    Height = opt.height;
//    JpegQChanged (e);
    OPT_TO_COMBOBOX ( "pano_val_jpegQuality", opt.quality )
//    JpegPChanged (e);
    XRCCTRL(*this, "pano_bool_jpegProgressive", wxCheckBox)
                            ->SetValue( opt.progressive ) ;

    changePano = FALSE;
    DEBUG_TRACE("");
}

void PanoPanel::PanoChanged ( wxCommandEvent & e )
{
    DEBUG_TRACE("");

    if ( !self_pano_dlg ) {
      ColourModeChanged (e);
      GammaChanged (e);
      HFOVChanged (e);
      InterpolatorChanged (e);
      ProjectionChanged (e);

      FinalFormatChanged (e);
      WidthChanged (e);
      HeightChanged (e);
      JpegQChanged (e);
      JpegPChanged (e);

      autoPreview (e);
      autoOptimize (e);
//      optimizeAnchorChanged (e);
      panoviewerEnabled (e);
      panoviewerPrecise (e);
      previewWidthChanged (e);
      previewHeightChanged (e);
      previewSingleChanged (e);
    }

    DEBUG_TRACE ( "" )
}

void PanoPanel::ProjectionChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      int lt = XRCCTRL(*this, "pano_choice_panoType",
                                     wxChoice)->GetSelection();
      wxString Ip ("Hallo");
      switch ( lt ) {
          case PanoramaOptions::RECTILINEAR:       Ip = _("Rectlinear"); break;
          case PanoramaOptions::CYLINDRICAL:       Ip = _("Cylindrical"); break;
          case PanoramaOptions::EQUIRECTANGULAR:   Ip = _("Equirectangular"); break;
      }

      opt.projectionFormat = (PanoramaOptions::ProjectionFormat) lt;
      GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
      DEBUG_INFO ( Ip )
    }
}

void PanoPanel::InterpolatorChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      //Interpolator from PanoramaMemento.h
      int lt = XRCCTRL(*this, "pano_choice_interpolator",
                                     wxChoice)->GetSelection();
      wxString Ip ("Hallo");
      switch ( lt ) {
          case PanoramaOptions::POLY_3:            Ip = _("Poly3 (Bicubic)"); break;
          case PanoramaOptions::SPLINE_16:         Ip = _("Spline 16"); break;
          case PanoramaOptions::SPLINE_36:         Ip = _("Spline 36"); break;
          case PanoramaOptions::SINC_256:          Ip = _("Sinc 256"); break;
          case PanoramaOptions::SPLINE_64:         Ip = _("Spline 64"); break;
          case PanoramaOptions::BILINEAR:          Ip = _("Bilinear"); break;
          case PanoramaOptions::NEAREST_NEIGHBOUR: Ip = _("Nearest neighbour"); break;
          case PanoramaOptions::SINC_1024:         Ip = _("Sinc 1024"); break;
      }

      opt.interpolator = (PanoramaOptions::Interpolator) lt;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );
      DEBUG_INFO ( Ip )
    }
}

void PanoPanel::HFOVChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      double * val = new double ();
      int lt = XRCCTRL(*this, "pano_val_hfov", wxComboBox)
                              ->GetSelection() ;
      XRCCTRL(*this, "pano_val_hfov", wxComboBox)
                              ->GetString(lt).ToDouble(val) ;

      opt.HFOV = *val;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );

      DEBUG_INFO ( ": " << *val )
      delete val;
    }
}

void PanoPanel::GammaChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      double * val = new double ();
      wxString text = XRCCTRL(*this, "pano_val_gamma", wxTextCtrl)->GetValue();
      text.ToDouble( val );

      opt.gamma = *val;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );

      DEBUG_INFO ( wxString::Format (": %f", *val) )
    }
}
void PanoPanel::ColourModeChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      int colorCorrection = XRCCTRL(*this, "pano_choice_colour_mode"
                  , wxChoice) ->GetSelection();
      wxString text = XRCCTRL(*this, "pano_choice_colour_mode"
                  , wxChoice) ->GetString(colorCorrection);

      int val = XRCCTRL(*this, "pano_spin_colour_reference"
                  , wxSpinCtrl)->GetValue();
      opt.colorCorrection = (PanoramaOptions::ColorCorrection) colorCorrection;
      if ( val <= (int)pano.getNrOfImages() -1 || val == 0 )
        opt.colorReferenceImage = (unsigned int) val;
      else
        opt.colorReferenceImage = 0;

      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );

      DEBUG_INFO ( wxString::Format ( text <<" with: %d", val) )
    }
}
// --
void PanoPanel::previewWidthChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      double * val = new double ();
      int lt = XRCCTRL(*this, "pano_val_previewWidth", wxComboBox)
                              ->GetSelection() ;
      XRCCTRL(*this, "pano_val_previewWidth", wxComboBox)
                              ->GetString(lt).ToDouble(val) ;
      previewWidth = (int)*val;

      if ( pano_dlg_run ) {
        if ( self_pano_dlg ) {
          pano_panel->previewWidth = previewWidth;
          XRCCTRL(*pano_panel, "pano_val_previewWidth", wxComboBox)
                              ->SetValue(wxString::Format ("%d", previewWidth));
        } else {
          pano_dlg->pp->previewWidth = previewWidth;
          XRCCTRL(*pano_dlg->pp, "pano_val_previewWidth", wxComboBox)
                              ->SetValue(wxString::Format ("%d", previewWidth));
        }
      }

/*      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );
*/
      DEBUG_INFO ( ": " << *val )
      delete val;
    }
}

void PanoPanel::previewHeightChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      double * val = new double ();
      int lt = XRCCTRL(*this, "pano_val_previewHeight", wxComboBox)
                              ->GetSelection() ;
      XRCCTRL(*this, "pano_val_previewHeight", wxComboBox)
                              ->GetString(lt).ToDouble(val) ;
      previewHeight = (int)*val;

      if ( pano_dlg_run ) {
        if ( self_pano_dlg ) {
          pano_panel->previewHeight = previewHeight;
          XRCCTRL(*pano_panel, "pano_val_previewHeight", wxComboBox)
                             ->SetValue(wxString::Format ("%d", previewHeight));
        } else {
          pano_dlg->pp->previewHeight = previewHeight;
          XRCCTRL(*pano_dlg->pp, "pano_val_previewHeight", wxComboBox)
                             ->SetValue(wxString::Format ("%d", previewHeight));
        }
      }

/*      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );
*/
      DEBUG_INFO ( ": " << *val )
      delete val;
    }
}

void PanoPanel::previewSingleChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      int lt = XRCCTRL(*this, "pano_spin_single_preview", wxSpinCtrl)
                              ->GetValue();
      previewSingle = (unsigned int)lt;
      lt = XRCCTRL(*this, "pano_cb_single_preview", wxCheckBox)
                              ->GetValue();
      preview_single = lt;

      if ( pano_dlg_run ) {
        if ( self_pano_dlg ) {
          pano_panel->previewSingle = previewSingle;
          pano_panel->preview_single = preview_single;
          XRCCTRL(*pano_panel, "pano_spin_single_preview", wxSpinCtrl)
                             ->SetValue(previewSingle);
          XRCCTRL(*pano_panel, "pano_cb_single_preview", wxCheckBox)
                             ->SetValue(preview_single);
        } else {
          pano_dlg->pp->previewSingle = previewSingle;
          pano_dlg->pp->preview_single = preview_single;
          XRCCTRL(*pano_dlg->pp, "pano_spin_single_preview", wxSpinCtrl)
                             ->SetValue(previewSingle);
          XRCCTRL(*pano_dlg->pp, "pano_cb_single_preview", wxCheckBox)
                             ->SetValue(preview_single);
        }
      }

      // update preview
      PT::UIntSet i;
      pano_panel->panoramaImagesChanged (pano, i);

      DEBUG_INFO ( "preview_single("<< preview_single <<"): " << previewSingle )
    }
}
// --
void PanoPanel::FinalFormatChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      // FileFormat from PanoramaMemento.h
      int lt = XRCCTRL(*this, "pano_choice_formatFinal",
                                     wxChoice)->GetSelection();

      wxString Ip ("JPEG");
      switch ( lt ) {
          case PanoramaOptions::JPEG:        Ip = wxT("JPEG"); break;
          case PanoramaOptions::PNG:         Ip = wxT("PNG"); break;
          case PanoramaOptions::TIFF:        Ip = wxT("TIFF"); break;
          case PanoramaOptions::TIFF_mask:   Ip = wxT("TIFF_m"); break;
          case PanoramaOptions::TIFF_nomask: Ip = wxT("TIFF_nomask"); break;
          case PanoramaOptions::PICT:        Ip = wxT("PICT"); break;
          case PanoramaOptions::PSD:         Ip = wxT("PSD"); break;
          case PanoramaOptions::PSD_mask:    Ip = wxT("PSD_mask"); break;
          case PanoramaOptions::PSD_nomask:  Ip = wxT("PSD_nomask"); break;
          case PanoramaOptions::PAN:         Ip = wxT("PAN"); break;
          case PanoramaOptions::IVR:         Ip = wxT("IVR"); break;
          case PanoramaOptions::IVR_java:    Ip = wxT("IVR_java"); break;
          case PanoramaOptions::VRML:        Ip = wxT("VRML"); break;
          case PanoramaOptions::QTVR:        Ip = wxT("QTVR"); break;
  //      default :   Ip = wxString::Format ("%d",lt); break;
      }

      opt.outputFormat = Ip;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );

  // TODO add path for writing
      if (opt.outputFormat == "JPEG" ) {
        opt.outfile = "panorama.JPG";
      } else {
        opt.outfile = "panorama";
      }
      DEBUG_INFO ( Ip )
    }
}

void PanoPanel::WidthChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      double * val = new double ();
      int lt = XRCCTRL(*this, "pano_val_width", wxComboBox)
                              ->GetSelection() ;
      XRCCTRL(*this, "pano_val_width", wxComboBox)
                              ->GetString(lt).ToDouble(val) ;

      opt.width = Width = (int) *val;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );

      DEBUG_INFO( ": " << *val << " " << Width );
      delete val;
    }
}

void PanoPanel::HeightChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
        double val;
        int lt = XRCCTRL(*this, "pano_val_height", wxComboBox)
                              ->GetSelection() ;
        XRCCTRL(*this, "pano_val_height", wxComboBox)
                              ->GetString(lt).ToDouble(&val) ;

        opt.height = Height = (int) val;
        GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
            );
        DEBUG_INFO ( ": " << val << " " << Height )
    }
}

void PanoPanel::JpegQChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      double * val = new double ();
      int lt = XRCCTRL(*this, "pano_val_jpegQuality", wxComboBox)
                              ->GetSelection() ;
      XRCCTRL(*this, "pano_val_jpegQuality", wxComboBox)
                              ->GetString(lt).ToDouble(val) ;

      opt.quality = (int) *val;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );

      DEBUG_INFO ( ": " << *val )
      delete val;
    }
}

void PanoPanel::JpegPChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      int lt = XRCCTRL(*this, "pano_bool_jpegProgressive", wxCheckBox)
                              ->GetValue() ;

      opt.progressive = lt;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );
      DEBUG_INFO ( ": " << lt )
    }
}

void PanoPanel::Stitch ( wxCommandEvent & e )
{
    opt.width = Width;
    opt.height = Height;
    PT::SetPanoOptionsCmd( pano, opt );
#ifdef unix
    DEBUG_INFO ( "unix" )
    // for testing
    //opt.printStitcherScript( *stdout, opt);
#endif

#ifdef __unix__
    GlobalCmdHist::getInstance().addCommand(
        new PT::StitchCmd( pano, opt )
        );
#else
      wxLogError("Stitching not implemented under windows yet");
#endif


    DEBUG_INFO ( ": " << Width )
}

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PanoDialog, wxWindow)
    EVT_PAINT (PanoDialog::OnPaint )
END_EVENT_TABLE()

PanoDialog::PanoDialog (wxWindow *parent, const wxPoint& pos, const wxSize& size
              , Panorama *pano)
    : wxDialog (parent, 1000, _("Panorama settings"), pos, size,
          wxDEFAULT_DIALOG_STYLE|wxDIALOG_MODELESS|wxRESIZE_BORDER),
      pano (*pano)
{
    DEBUG_TRACE("");
    wxTheXmlResource->LoadPanel(this, wxT("pano_dialog"));
    pp = new PanoPanel( this, wxDefaultPosition, size, pano);
    wxXmlResource::Get()->AttachUnknownControl (
       wxT("pano_dialog_unknown"),
       pp);
    pp->pano_dlg_run = TRUE; // Tell pano_panel dialog is running.
    pp->self_pano_dlg = TRUE; // Tell pano_panel it is dialog.
    Show();
    DEBUG_TRACE("");
}

PanoDialog::~PanoDialog(void)
{
    pp->Close();
    this->Destroy();
}

void PanoDialog::OnPaint (wxPaintEvent & e)
{ }

void PanoDialog::DoPaint (wxPaintEvent & e)
{}

