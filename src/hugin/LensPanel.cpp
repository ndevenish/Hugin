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
//wxImageList * img_icons;
// Image Icons biger
//wxImageList * img_bicons;

// image preview
extern wxBitmap * p_img;

ImgPreview * lens_canvas;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(LensPanel, LensPanel)
    EVT_LEFT_DOWN ( LensPanel::ChangePreview )
END_EVENT_TABLE()


// Define a constructor for the Images Panel
LensPanel::LensPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : pano(*pano)
{
    pano->addObserver(this);

    images_list = new List (parent, pano);
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_list_unknown"),
               images_list );

    images_list->InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25 );
    images_list->InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 255 );
//    images_list->InsertColumn( 2, _("width"), wxLIST_FORMAT_RIGHT, 60 );
//    images_list->InsertColumn( 3, _("height"), wxLIST_FORMAT_RIGHT, 60 );
//    images_list->InsertColumn( 4, _("No."), wxLIST_FORMAT_RIGHT, 30 );

    p_img = new wxBitmap( 0, 0 );
/*    wxPanel * img_p = XRCCTRL(*parent, "Lens_preview_unknown", wxPanel);
    wxPaintDC dc (img_p);

    lens_canvas = new ImgPreview(img_p, wxPoint(0, 0), wxSize(128, 128));
*/    DEBUG_TRACE("");;

}


LensPanel::~LensPanel(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
//    delete p_img; // Dont know how to check for existing
    p_img = (wxBitmap *) NULL;
    delete images_list;
    DEBUG_TRACE("");
}


void LensPanel::panoramaChanged (PT::Panorama &pano)
{
    canvas->Refresh();
    DEBUG_TRACE("");
}

void LensPanel::ChangePreview ( wxListEvent & e )
{
    DEBUG_TRACE ("")
    long item (1);
    DEBUG_INFO ( "hier: is item %ld" << wxString::Format("%ld", item) );
}


