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
//#include <wx/imaglist.h>

#include "PT/PanoCommand.h"
#include "PT/PanoramaMemento.h"
#include "PT/Panorama.h"
#include "hugin/config.h"
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
    EVT_SIZE   ( PanoPanel::FitParent )
//    EVT_TEXT_ENTER ( XRCID("panorama_panel"),PanoPanel::PanoChanged )

  EVT_BUTTON   ( XRCID("pano_make_dialog"),PanoPanel::DoDialog )


  EVT_BUTTON   ( XRCID("pano_optimizer_start"),PanoPanel::DoOptimization )

  EVT_CHOICE ( XRCID("pano_choice_panoType"),PanoPanel::ProjectionChanged )
  EVT_CHOICE ( XRCID("pano_choice_interpolator"),PanoPanel::InterpolatorChanged)
  EVT_COMBOBOX ( XRCID("pano_val_hfov"),PanoPanel::HFOVChanged )
  EVT_TEXT_ENTER ( XRCID("pano_val_gamma"),PanoPanel::GammaChanged )

  EVT_COMBOBOX ( XRCID("pano_val_previewWidth"),PanoPanel::PreviewWidthChanged )
  EVT_BUTTON   ( XRCID("pano_button_preview"),PanoPanel::DoPreview )
  EVT_CHECKBOX ( XRCID("pano_cb_auto_preview"),PanoPanel::AutoPreview )
  EVT_CHECKBOX ( XRCID("pano_cb_panoviewer_enabled"),
                                               PanoPanel::panoviewerEnabled )

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
//    PanoChanged (e);
    auto_preview = FALSE;
    panoviewer_enabled = TRUE;
    panoviewer_started = FALSE;

    changePano = FALSE;

    wxCommandEvent e;
    PreviewWidthChanged (e);

    pano->addObserver (this);

    DEBUG_TRACE("")
}


PanoPanel::~PanoPanel(void)
{
    pano.removeObserver(this);
    DEBUG_TRACE("");
}

void PanoPanel::FitParent( wxSizeEvent & e )
{
//    wxSize new_size = GetSize();
//    wxSize new_size = GetParent()->GetSize();
//    XRCCTRL(*this, "panorama_panel", wxPanel)->SetSize ( new_size );
//    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}

void PanoPanel::DoDialog ( wxCommandEvent & e )
{
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
                    ->SetHelpText("tear off this tab");
    } else {
        DEBUG_TRACE("" <<   GetName() );
        DEBUG_TRACE("" <<   GetParent()->GetName() );
        DEBUG_TRACE("" <<   GetGrandParent()->GetName() );
        DEBUG_TRACE("" <<   GetParent()->GetGrandParent()->GetLabel() );
        DEBUG_TRACE("" <<   frame->GetLabel() );
      wxSize new_size = frame->GetSize();
      wxPoint new_point = frame->GetPosition();
      new_point.y = new_point.y + new_size.GetHeight()
#if defined(__WXGTK__)
      +25 // for the status bar
#endif
      ;
      pano_dlg = new PanoDialog (this, new_point,
                         XRCCTRL(*this, "panorama_panel", wxPanel)->GetSize(),
                         &pano);
      pano_dlg->SetModal(FALSE);
      pano_dlg_run = TRUE;
      XRCCTRL(*this, "pano_make_dialog", wxPanel)
                    ->SetHelpText("Close child window");
      XRCCTRL(*pano_dlg->pp, "pano_make_dialog", wxPanel)
                    ->SetHelpText("Close child window");
      UIntSet i;
      pano_dlg->pp->panoramaImagesChanged (pano, i); // initialize

    }
    DEBUG_TRACE("");
}

void PanoPanel::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
    opt = pano.getOptions();
    if ( !changePano ) {
      changePano = TRUE;

      DEBUG_INFO (XRCID("pano_button_preview"))
// --
      PanoOptionsChanged ();
      int id (XRCID("pano_button_preview"));
      wxCommandEvent  e;// = new wxCommandEvent( /*id , wxEVT_COMMAND_BUTTON_CLICKED*/ );
      e.SetId(id);
      if (auto_preview && !self_pano_dlg && !panoviewer_started)
        DoPreview (e);
//    PanoChanged (e);
//      if (panoviewer_started)
//        panoviewer_started = FALSE;
    }
    changePano = FALSE;
}

void PanoPanel::Optimize (OptimizeVector & optvars, PanoramaOptions & output)
{
    GlobalCmdHist::getInstance().addCommand(
        new PT::OptimizeCmd( pano, optvars, output)
        );
}

void PanoPanel::DoOptimization (wxCommandEvent & e)
{
    // TODO ask which variables to optimize - > wxComboBox pano_optimizer_level 
    Optimize(*optset, opt); 
}

void PanoPanel::DoPreview ( wxCommandEvent & e )
{

    if (!self_pano_dlg) {
      int old_previewWidth = previewWidth;
      Panorama preview_pano = pano;
      PanoramaOptions preview_opt = opt;
      if ( !panoviewer_enabled ) {
        previewWidth = 256;
      }
      preview_opt.width = previewWidth;
      preview_opt.height = previewWidth / 2;
      wxString outputFormat = preview_opt.outputFormat.c_str();
      preview_opt.outputFormat = "JPEG";
      if ( outputFormat != "JPEG" )
        preview_opt.quality = 100;

      if (!panoviewer_enabled) {
        // Set the preview image accordingly to ImagesPanel.cpp
        for (unsigned int imgNr=0; imgNr < pano.getNrOfImages(); imgNr++){
          wxFileName fn = (wxString)pano.getImage(imgNr).getFilename().c_str();
          std::stringstream filename;
          filename << fn.GetPath(wxPATH_GET_SEPARATOR|wxPATH_GET_VOLUME).c_str()
                  <<_("preview")<<"_"<< imgNr <<".ppm" ;
          PanoImage image = preview_pano.getImage(imgNr);
          image.setFilename( filename.str() ) ;
          preview_pano.setImage(imgNr, image);
          DEBUG_INFO ("rendering preview image: "<< filename.str())
        }
      }

      preview_pano.clearObservers();
      Process process(false);
      preview_pano.runStitcher(process, preview_opt);
      process.wait();

      previewWidth = old_previewWidth;

      // Send panoViewer the name of our image
      if ( panoviewer_enabled ) {
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
void PanoPanel::AutoPreview ( wxCommandEvent & e )
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


#define OPT_TO_COMBOBOX( xml_combo, type ) \
{ \
    lt = XRCCTRL(*this, xml_combo , wxComboBox) \
                            -> FindString( DoubleToString (opt.type) ) ; \
    std::string number = XRCCTRL(*this, xml_combo, wxComboBox) \
                            ->GetString(lt).c_str(); \
    std::stringstream compare_s; \
    compare_s << opt.type; \
    if ( atof(number.c_str()) != atof(compare_s.str().c_str()) ) { \
      XRCCTRL(*this, xml_combo, wxComboBox) \
                            -> SetValue(compare_s.str().c_str()); \
    } else { \
      XRCCTRL(*this, xml_combo, wxComboBox) \
                            ->SetSelection(lt) ; \
    } \
}

void PanoPanel::PanoOptionsChanged ( void )
{
    DEBUG_INFO ( "updating the other PanoPanel " << self_pano_dlg )
    int lt;
//    UIntSet imgNr;

//    GammaChanged (e);
    XRCCTRL(*this, "pano_val_gamma", wxTextCtrl)
                ->SetValue( wxString::Format ("%f", opt.gamma) );
//    HFOVChanged (e);
/*    DEBUG_INFO ( "updating HFOV " << opt.HFOV )
    lt = XRCCTRL(*this, "pano_val_hfov", wxComboBox)
                            -> FindString( DoubleToString (opt.HFOV) ) ;
    DEBUG_INFO ( "updating HFOV " << opt.HFOV <<" "<< lt )
    std::string number = XRCCTRL(*this, "pano_val_hfov", wxComboBox)
                            ->GetString(lt).c_str();
    std::stringstream compare_s;
    compare_s << opt.HFOV;
    if ( atof(number.c_str()) != atof(compare_s.str().c_str()) ) {
      DEBUG_INFO ( "updating HFOV " << compare_s.str().c_str() )
      XRCCTRL(*this, "pano_val_hfov", wxComboBox)
                            -> SetValue(compare_s.str().c_str());
    } else {
      XRCCTRL(*this, "pano_val_hfov", wxComboBox)
                            ->SetSelection(lt) ;
      DEBUG_INFO ( "updating HFOV " << opt.HFOV <<" "<< lt )
    }*/
    OPT_TO_COMBOBOX ( "pano_val_hfov", HFOV )
//    InterpolatorChanged (e);
    XRCCTRL(*this, "pano_choice_interpolator", wxChoice)
                            ->SetSelection(opt.interpolator);
//    ProjectionChanged (e);
    XRCCTRL(*this, "pano_choice_panoType", wxChoice)
                            ->SetSelection(opt.projectionFormat);
//    PreviewWidthChanged (e);
//    FinalFormatChanged (e);
    lt = XRCCTRL(*this, "pano_choice_formatFinal", wxChoice)
                        ->FindString(opt.outputFormat.c_str());
    XRCCTRL(*this, "pano_choice_formatFinal", wxChoice)
                            ->SetSelection(lt);
//    WidthChanged (e);
    lt = XRCCTRL(*this, "pano_val_width", wxComboBox)
                            ->FindString(wxString::Format ("%d", opt.width)) ;
    XRCCTRL(*this, "pano_val_width", wxComboBox)
                            ->SetSelection(lt) ;
    Width = opt.width;
//    HeightChanged (e);
    lt = XRCCTRL(*this, "pano_val_height", wxComboBox)
                            ->FindString(wxString::Format ("%d", opt.height)) ;
    XRCCTRL(*this, "pano_val_height", wxComboBox)
                            ->SetSelection(lt) ;
    Height = opt.height;
//    JpegQChanged (e);
    lt = XRCCTRL(*this, "pano_val_jpegQuality", wxComboBox)
                            ->FindString(wxString::Format ("%d", opt.quality)) ;
    XRCCTRL(*this, "pano_val_jpegQuality", wxComboBox)
                            ->SetSelection(lt) ;
//    JpegPChanged (e);
    XRCCTRL(*this, "pano_bool_jpegProgressive", wxCheckBox)
                            ->SetValue( opt.progressive ) ;
    
    DEBUG_TRACE("");
}

void PanoPanel::PanoChanged ( wxCommandEvent & e )
{

    DEBUG_TRACE("");
    GammaChanged (e);
    HFOVChanged (e);
    InterpolatorChanged (e);
    ProjectionChanged (e);

    PreviewWidthChanged (e);

    FinalFormatChanged (e);
    WidthChanged (e);
    HeightChanged (e);
    JpegQChanged (e);
    JpegPChanged (e);

    DEBUG_TRACE ( "" )
}

void PanoPanel::ProjectionChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      int lt = XRCCTRL(*this, "pano_choice_panoType",
                                     wxChoice)->GetSelection();
      wxString Ip ("Hallo");
      switch ( lt ) {
          case RECTILINEAR:       Ip = _("Rectlinear"); break;
          case CYLINDRICAL:       Ip = _("Cylindrical"); break;
          case EQUIRECTANGULAR:   Ip = _("Equirectangular"); break;
      }

      opt.projectionFormat = (ProjectionFormat) lt;
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
          case POLY_3:            Ip = _("Poly3 (Bicubic)"); break;
          case SPLINE_16:         Ip = _("Spline 16"); break;
          case SPLINE_36:         Ip = _("Spline 36"); break;
          case SINC_256:          Ip = _("Sinc 256"); break;
          case SPLINE_64:         Ip = _("Spline 64"); break;
          case BILINEAR:          Ip = _("Bilinear"); break;
          case NEAREST_NEIGHBOUR: Ip = _("Nearest neighbour"); break;
          case SINC_1024:         Ip = _("Sinc 1024"); break;
      }
  
      opt.interpolator = (Interpolator) lt;
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
// --
void PanoPanel::PreviewWidthChanged ( wxCommandEvent & e )
{
    if ( ! changePano ) {
      double * val = new double ();
      int lt = XRCCTRL(*this, "pano_val_previewWidth", wxComboBox)
                              ->GetSelection() ;
      XRCCTRL(*this, "pano_val_previewWidth", wxComboBox)
                              ->GetString(lt).ToDouble(val) ;
  
      previewWidth = (int)*val;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );
  
      DEBUG_INFO ( ": " << *val )
      delete val;
    }
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
          case JPEG:        Ip = wxT("JPEG"); break;
          case PNG:         Ip = wxT("PNG"); break;
          case TIFF:        Ip = wxT("TIFF"); break;
          case TIFF_mask:   Ip = wxT("TIFF_m"); break;
          case TIFF_nomask: Ip = wxT("TIFF_nomask"); break;
          case PICT:        Ip = wxT("PICT"); break;
          case PSD:         Ip = wxT("PSD"); break;
          case PSD_mask:    Ip = wxT("PSD_mask"); break;
          case PSD_nomask:  Ip = wxT("PSD_nomask"); break;
          case PAN:         Ip = wxT("PAN"); break;
          case IVR:         Ip = wxT("IVR"); break;
          case IVR_java:    Ip = wxT("IVR_java"); break;
          case VRML:        Ip = wxT("VRML"); break;
          case QTVR:        Ip = wxT("QTVR"); break;
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
      double * val = new double ();
      int lt = XRCCTRL(*this, "pano_val_height", wxComboBox)
                              ->GetSelection() ;
      XRCCTRL(*this, "pano_val_height", wxComboBox)
                              ->GetString(lt).ToDouble(val) ;
  
      opt.height = Height = (int) *val;
      GlobalCmdHist::getInstance().addCommand(
          new PT::SetPanoOptionsCmd( pano, opt )
          );
  
      DEBUG_INFO ( ": " << *val )
      delete val;
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

    GlobalCmdHist::getInstance().addCommand(
        new PT::StitchCmd( pano, opt )
        );

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
    // FIXME let the window close and move together with the MainFrame like DDD
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

