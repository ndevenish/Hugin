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
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
//#include "hugin/ImageCache.h"
//#include "hugin/CPEditorPanel.h"
//#include "hugin/List.h"
//#include "hugin/LensPanel.h"
//#include "hugin/ImagesPanel.h"
#include "hugin/PanoPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "hugin/Events.h"
#include "PT/Panorama.h"

using namespace PT;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PanoPanel, wxWindow)
    EVT_TEXT_ENTER ( XRCID("panorama_panel"),PanoPanel::PanoChanged )

  EVT_CHOICE ( XRCID("pano_choice_panoType"),PanoPanel::ProjectionChanged )
  EVT_CHOICE ( XRCID("pano_choice_interpolator"),PanoPanel::InterpolatorChanged)
  EVT_COMBOBOX ( XRCID("pano_val_hfov"),PanoPanel::HFOVChanged )
  EVT_TEXT_ENTER ( XRCID("pano_val_gamma"),PanoPanel::GammaChanged )

  EVT_COMBOBOX ( XRCID("pano_val_previewWidth"),PanoPanel::PreviewWidthChanged )
  EVT_BUTTON   ( XRCID("pano_button_preview"),PanoPanel::DoPreview )

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
    pano->addObserver(this);

    opt = new PanoramaOptions();

    // loading xrc resources in selfcreated this panel
    wxXmlResource::Get()->LoadPanel ( this, wxT("panorama_panel")); //);

    DEBUG_TRACE("")
}


PanoPanel::~PanoPanel(void)
{
    pano.removeObserver(this);
    DEBUG_TRACE("");
}


void PanoPanel::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNr)
{
}

void PanoPanel::PanoChanged ( wxCommandEvent & e )
{

    DEBUG_INFO ( "" )
}

void PanoPanel::ProjectionChanged ( wxCommandEvent & e )
{
    int lt = XRCCTRL(*this, "pano_choice_panoType",
                                   wxChoice)->GetSelection();
    wxString Ip ("Hallo");
    switch ( lt ) {
        case RECTILINEAR:       Ip = _("Rectlinear"); break;
        case CYLINDRICAL:       Ip = _("Cylindrical"); break;
        case EQUIRECTANGULAR:   Ip = _("Equirectangular"); break;
    }
    

    DEBUG_INFO ( Ip )
}

void PanoPanel::InterpolatorChanged ( wxCommandEvent & e )
{
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

    DEBUG_INFO ( Ip )
}

void PanoPanel::HFOVChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    int lt = XRCCTRL(*this, "pano_val_hfov", wxComboBox)
                            ->GetSelection() ; 
    XRCCTRL(*this, "pano_val_hfov", wxComboBox)
                            ->GetString(lt).ToDouble(val) ; 

    DEBUG_INFO ( ": " << *val )
    delete val;
}

void PanoPanel::GammaChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    wxString text = XRCCTRL(*this, "pano_val_gamma", wxTextCtrl)->GetValue();
    text.ToDouble( val );

    DEBUG_INFO ( wxString::Format (": %f", *val) )
}
// --
void PanoPanel::PreviewWidthChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    int lt = XRCCTRL(*this, "pano_val_previewWidth", wxComboBox)
                            ->GetSelection() ; 
    XRCCTRL(*this, "pano_val_previewWidth", wxComboBox)
                            ->GetString(lt).ToDouble(val) ; 

    DEBUG_INFO ( ": " << *val )
    delete val;
}

void PanoPanel::DoPreview ( wxCommandEvent & e )
{

    DEBUG_INFO ( "" )
}
// --
void PanoPanel::FinalFormatChanged ( wxCommandEvent & e )
{
    //Interpolator from PanoramaMemento.h
    int lt = XRCCTRL(*this, "pano_choice_formatFinal",
                                   wxChoice)->GetSelection();
    wxString Ip ("Hallo");
    switch ( lt ) {
        case JPEG:            Ip = _("JPEG (*.jpg)"); break;
        case PNG:         Ip = _("PNG (*.png)"); break;
        case TIFF:         Ip = _("TIFF (*.tif)"); break;
        case TIFF_mask:          Ip = _("TIFF_mask (*.tif)"); break;
        case TIFF_nomask:         Ip = _("TIFF_nomask (*.tif)"); break;
        case PICT:          Ip = _("PICT Bitmap (*.bmp)"); break;
        case PSD: Ip = _("PSD Photoshop (*.psd)"); break;
        case PSD_mask:         Ip = _("PSD_mask"); break;
        default :   Ip = wxString::Format ("%d",lt); break;
    }

    DEBUG_INFO ( Ip )
}

void PanoPanel::WidthChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    int lt = XRCCTRL(*this, "pano_val_width", wxComboBox)
                            ->GetSelection() ; 
    XRCCTRL(*this, "pano_val_width", wxComboBox)
                            ->GetString(lt).ToDouble(val) ; 

    DEBUG_INFO ( ": " << *val )
    delete val;
}

void PanoPanel::HeightChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    int lt = XRCCTRL(*this, "pano_val_height", wxComboBox)
                            ->GetSelection() ; 
    XRCCTRL(*this, "pano_val_height", wxComboBox)
                            ->GetString(lt).ToDouble(val) ; 

    DEBUG_INFO ( ": " << *val )
    delete val;
}

void PanoPanel::JpegQChanged ( wxCommandEvent & e )
{
    double * val = new double ();
    int lt = XRCCTRL(*this, "pano_val_jpegQuality", wxComboBox)
                            ->GetSelection() ; 
    XRCCTRL(*this, "pano_val_jpegQuality", wxComboBox)
                            ->GetString(lt).ToDouble(val) ; 

    DEBUG_INFO ( ": " << *val )
    delete val;
}

void PanoPanel::JpegPChanged ( wxCommandEvent & e )
{
    int lt = XRCCTRL(*this, "pano_bool_jpegProgressive", wxCheckBox)
                            ->GetValue() ; 

    DEBUG_INFO ( ": " << lt )
}

void PanoPanel::Stitch ( wxCommandEvent & e )
{
/*    GlobalCmdHist::getInstance().addCommand(
        new PT::StitchCmd( pano, *opt )
        );*/
    DEBUG_INFO ( ": " )
}

//------------------------------------------------------------------------------



