// -*- c-basic-offset: 4 -*-

/** @file List.cpp
 *
 *  @brief implementation of List Class
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

#include "panoinc.h"
#include "panoinc_WX.h"
#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/spinctrl.h>
#include <wx/imaglist.h>

#include "hugin/ImagesList.h"
#include "hugin/config.h"
#include "hugin/ImageCache.h"

using namespace PT;
using namespace utils;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ImagesList, wxListCtrl)
    EVT_LIST_ITEM_SELECTED(-1, ImagesList::OnItemSelected)
    EVT_LIST_ITEM_DESELECTED(-1, ImagesList::OnItemDeselected)
END_EVENT_TABLE()

// Define a constructor for the Images Panel
ImagesList::ImagesList( wxWindow* parent, Panorama* pano)
    : wxListCtrl(parent, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxSUNKEN_BORDER),   //|wxLC_HRULES),
    pano(*pano)
{
    DEBUG_TRACE("");

    InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25 );

    // get a good size for the images
    wxPoint sz(1,14);
    sz = ConvertDialogToPixels(sz);
    m_iconHeight = sz.y;
    DEBUG_DEBUG("icon Height: " << m_iconHeight);

    m_smallIcons = new wxImageList(m_iconHeight, m_iconHeight);
    AssignImageList(m_smallIcons,wxIMAGE_LIST_SMALL);
    pano->addObserver(this);
    DEBUG_TRACE("");
}

ImagesList::~ImagesList(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
}

void ImagesList::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    Freeze();

    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrItems = GetItemCount();

    // remove items for nonexisting images
    for (int i=nrItems-1; i>=(int)nrImages; i--)
    {
        DEBUG_DEBUG("Deleting list item " << i);

        // deselect item
        DEBUG_DEBUG("item state before: " << GetItemState(i,wxLIST_STATE_SELECTED));
        SetItemState(i,0, wxLIST_STATE_SELECTED);
        DEBUG_DEBUG("item state after: " << GetItemState(i,wxLIST_STATE_SELECTED));
        
        RemoveItem(i);
        m_smallIcons->Remove(i);
    }

    // update existing items
//    if ( nrImages >= nrItems ) {
        for(UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it){
            if (*it >= nrItems) {
                // create new item.
                DEBUG_DEBUG("creating " << *it);
                CreateItem(*it);

                wxBitmap small0;
                createIcon(small0, *it, m_iconHeight);
                m_smallIcons->Add(small0);
            } else {
                // update existing item
                DEBUG_DEBUG("updating item" << *it);
                UpdateItem(*it);

                wxBitmap small0;
                createIcon(small0, *it, m_iconHeight);
                m_smallIcons->Replace(*it, small0);
            }
            ImageCache::getInstance().softFlush();
        }
//    }

    // set new column widths
    for ( int j=0; j < GetColumnCount() ; j++ ) {
        SetColumnWidth(j, wxLIST_AUTOSIZE);
        if ( GetColumnWidth(j) < 40 )
            SetColumnWidth(j, 40);
    }

    Thaw();
}

void ImagesList::createIcon(wxBitmap & bitmap, unsigned int imgNr, unsigned int size)
{
    wxImage * s_img = ImageCache::getInstance().getSmallImage(
        pano.getImage(imgNr).getFilename());

    float w = s_img->GetWidth();
    float h = s_img->GetHeight();

    // create scaled versions
    int bW,bH;

    if ( h > w ) {
        // protrait
        bW = (int) nearbyint(h/w * size);
        bH = size;
    } else {
        bW = size;
        bH = (int) nearbyint(h/w * size);
    }
    wxImage img = s_img->Scale(bW, bH);
    img.SaveFile("test.pnm");
    bitmap = img.ConvertToBitmap();
}

void ImagesList::CreateItem(unsigned int imgNr)
{
    DEBUG_DEBUG("creating item " << imgNr);
    // create the new row
    InsertItem ( imgNr, wxString::Format("%d",imgNr), imgNr );
    UpdateItem(imgNr);
}

void ImagesList::RemoveItem(unsigned int imgNr)
{
    // call wxListCtrl's removal function
    wxListCtrl::DeleteItem(imgNr);
}

void ImagesList::OnItemSelected ( wxListEvent & e )
{
    DEBUG_TRACE(e.GetIndex());
    selectedItems.insert((int)e.GetIndex());
    // allow other parents to receive this event
    e.Skip();
}

void ImagesList::OnItemDeselected ( wxListEvent & e )
{
    DEBUG_TRACE(e.GetIndex());
    selectedItems.erase((int)e.GetIndex());
    // allow other parents to receive this event
    e.Skip();
}

#if 0
void ImagesList::OnItemSelected ( wxListEvent & e )
{
    DEBUG_TRACE("");

    // publicate the selected items
    selectedImages.clear();

    for ( int Nr=GetItemCount()-1 ; Nr>=0 ; --Nr ) {
        if ( GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
            selectedImages.insert(Nr);
        }
    }
    // notify other parties about changed notification..

    // let others recieve the event too
    e.Skip(true);
    DEBUG_TRACE("end");
}
#endif

ImagesListImage::ImagesListImage(wxWindow * parent, Panorama * pano)
    : ImagesList(parent, pano)
{
    InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 200 );
    InsertColumn( 2, _("width"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 3, _("height"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 4, _("yaw (y)"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 5, _("pitch (p)"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 6, _("roll (r)"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 7, _("Anchor"), wxLIST_FORMAT_RIGHT, 40 );
}

void ImagesListImage::UpdateItem(unsigned int imgNr)
{
    DEBUG_DEBUG("update image list item " << imgNr);
    DEBUG_ASSERT((int)imgNr < GetItemCount());
    const PanoImage & img = pano.getImage(imgNr);
    wxFileName fn(img.getFilename().c_str());
    VariableMap var = pano.getImageVariables(imgNr);

//    wxLogMessage(wxString::Format("updating image list item %d, filename %s",imgNr, fn.GetFullName()));

    SetItem(imgNr, 1, fn.GetFullName() );
    SetItem(imgNr, 2, wxString::Format("%d", img.getWidth()));
    SetItem(imgNr, 3, wxString::Format("%d", img.getHeight()));
    SetItem(imgNr, 4, doubleToString(map_get(var,"y").getValue()).c_str());
    SetItem(imgNr, 5, doubleToString( map_get(var,"p").getValue()).c_str());
    SetItem(imgNr, 6, doubleToString( map_get(var,"r").getValue()).c_str());
    char flags[] = "--";
    if (pano.getOptions().optimizeReferenceImage == imgNr) {
        flags[0]='A';
    }
    if (pano.getOptions().colorReferenceImage == imgNr) {
        flags[1]='C';
    }
    SetItem(imgNr,7, flags);
}

ImagesListLens::ImagesListLens(wxWindow * parent, Panorama * pano)
    : ImagesList(parent, pano)
{
    InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 180 );
    InsertColumn( 2, _("Lens no."), wxLIST_FORMAT_LEFT, 40);
    InsertColumn( 3, _("Lens type (f)"), wxLIST_FORMAT_LEFT, 100 );
    InsertColumn( 4, _("hfov (v)"), wxLIST_FORMAT_RIGHT, 80 );
    InsertColumn( 5, _("a"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 6, _("b"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 7, _("c"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 8, _("d"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 9, _("e"), wxLIST_FORMAT_RIGHT, 40 );

}

void ImagesListLens::UpdateItem(unsigned int imgNr)
{
    const PanoImage & img = pano.getImage(imgNr);
    wxFileName fn(img.getFilename().c_str());
    SetItem(imgNr, 1, fn.GetFullName() );
    SetItem(imgNr, 2, wxString::Format("%d",img.getLensNr()));

    VariableMap var = pano.getImageVariables(imgNr);
    const Lens & lens = pano.getLens( img.getLensNr());
    wxString ps;
    switch ( (int) lens.  projectionFormat  ) {
    case Lens::RECTILINEAR:          ps << _("Normal (rectlinear)"); break;
    case Lens::PANORAMIC:            ps << _("Panoramic"); break;
    case Lens::CIRCULAR_FISHEYE:     ps << _("Circular"); break;
    case Lens::FULL_FRAME_FISHEYE:   ps << _("Full frame"); break;
    case Lens::EQUIRECTANGULAR_LENS: ps << _("Equirectangular"); break;
    }
    SetItem(imgNr, 3, ps);
    SetItem(imgNr, 4, doubleToString( map_get(var, "v").getValue()).c_str());
    SetItem(imgNr, 5, doubleToString( map_get(var, "a").getValue()).c_str());
    SetItem(imgNr, 6, doubleToString( map_get(var, "b").getValue()).c_str());
    SetItem(imgNr, 7, doubleToString( map_get(var, "c").getValue()).c_str());
    SetItem(imgNr, 8, doubleToString( map_get(var, "d").getValue()).c_str());
    SetItem(imgNr, 9, doubleToString( map_get(var, "e").getValue()).c_str());
}

