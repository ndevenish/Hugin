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

#include <config.h>
#include "panoinc_WX.h"
#include "panoinc.h"

#include "common/wxPlatform.h"
#include "hugin/ImagesList.h"
#include "hugin/ImageCache.h"

#ifdef __WXMAC__
#include "hugin/MainFrame.h"
#endif

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

    m_notifyParents = true;
    InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25 );

    // get a good size for the images
    wxPoint sz(1,11);
    sz = ConvertDialogToPixels(sz);
    m_iconHeight = sz.y;
    DEBUG_DEBUG("icon Height: " << m_iconHeight);

    m_smallIcons = new wxImageList(m_iconHeight, m_iconHeight);
    AssignImageList(m_smallIcons,wxIMAGE_LIST_SMALL);
    pano->addObserver(this);
    DEBUG_TRACE("");
    m_degDigits = wxConfigBase::Get()->Read(wxT("/General/DegreeFractionalDigits"),1);
    m_pixelDigits = wxConfigBase::Get()->Read(wxT("/General/PixelFractionalDigits"),1);
    m_distDigits = wxConfigBase::Get()->Read(wxT("/General/DistortionFractionalDigits"),3);
#ifdef __WXMAC__
    SetDropTarget(new PanoDropTarget(*pano, true));
#endif
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
    m_notifyParents = false;
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

                wxBitmap small0(m_iconHeight, m_iconHeight);
                createIcon(small0, *it, m_iconHeight);
                m_smallIcons->Add(small0);
            } else {
                // update existing item
                DEBUG_DEBUG("updating item" << *it);
                UpdateItem(*it);

                wxBitmap small0(m_iconHeight, m_iconHeight);
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
#ifdef __WXMAC__
    SetColumnWidth(0, GetColumnWidth(0) + 12); //somehow wxMac does not set the first column's width very well.
#endif

    Thaw();
    m_notifyParents = true;
    
    // HACK! need to notify clients anyway... send dummy event..
    // lets hope our clients query for the selected images with GetSelected()
    // and do not try to interpret the event.
    wxListEvent e;
    e.SetEventType(wxEVT_COMMAND_LIST_ITEM_SELECTED);
    e.m_itemIndex = -1;
    GetEventHandler()->ProcessEvent(e);
}

const UIntSet & ImagesList::GetSelected() const      
{ 
    return selectedItems; 
}


void ImagesList::createIcon(wxBitmap & bitmap, unsigned int imgNr, unsigned int size)
{
    wxImage * s_img = ImageCache::getInstance().getSmallImage(
        pano.getImage(imgNr).getFilename());
    if (!s_img->Ok()) {
        return;
    }

    float w = s_img->GetWidth();
    float h = s_img->GetHeight();

    // create scaled versions
    int bW,bH;

    if ( h > w ) {
        // portrait
        bW = roundi(w/h * size);
        bH = size;
    } else {
        // landscape
        bW = size;
        bH = roundi(h/w * size);
    }
    wxImage img = s_img->Scale(bW, bH);
    wxBitmap bimg(img);

    wxMemoryDC temp_dc;
    temp_dc.SelectObject(bitmap);
    temp_dc.Clear();
/*
    wxBitmap maskb(size, size);
    wxMemoryDC mask_dc;
    mask_dc.SelectObject(maskb);
    mask_dc.Clear();
    mask_dc.SetBrush(wxBrush("WHITE",wxSOLID));
    mask_dc.SetPen(wxPen("WHITE",wxSOLID));
*/
    if (h > w) {
        temp_dc.DrawBitmap(bimg, (size-bW)>>1 ,0);
    } else {
        temp_dc.DrawBitmap(bimg,0,(size-bH)>>1);
    }
//    wxMask * m = new wxMask(bitmap, wxColour(0,0,0));
//    bitmap.SetMask(m);
}

void ImagesList::CreateItem(unsigned int imgNr)
{
    DEBUG_DEBUG("creating item " << imgNr);
    // create the new row
    InsertItem ( imgNr, wxString::Format(wxT("%d"),imgNr), imgNr );
    UpdateItem(imgNr);
}

void ImagesList::RemoveItem(unsigned int imgNr)
{
    // call wxListCtrl's removal function
    wxListCtrl::DeleteItem(imgNr);
}

void ImagesList::SelectSingleImage(unsigned int imgNr)
{
    unsigned int nrItems = GetItemCount();
    for (unsigned int i=0; i < nrItems ; i++) {
        int selected = GetItemState(i, wxLIST_STATE_SELECTED);
        if (i != imgNr && selected) {
            SetItemState(i, 0, wxLIST_STATE_SELECTED);
        }
    }
    SetItemState(imgNr, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

void ImagesList::OnItemSelected ( wxListEvent & e )
{
    DEBUG_TRACE(e.GetIndex());
    if(e.GetIndex() == -1) return;
    selectedItems.insert((int)e.GetIndex());
    // allow other parents to receive this event, only if its
    // generated by the user, and not during an automatic update
    // triggered by a change to the panorama
    if (m_notifyParents) {
        e.Skip();
    }
}

void ImagesList::OnItemDeselected ( wxListEvent & e )
{
    DEBUG_TRACE(e.GetIndex());
    if(e.GetIndex() == -1) return;
    selectedItems.erase((int)e.GetIndex());
    // allow other parents to receive this event, only if its
    // generated by the user, and not during an automatic update
    // triggered by a change to the panorama
    if (m_notifyParents) {
        e.Skip();
    }
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
    InsertColumn( 8, _("# Ctrl Pnts"), wxLIST_FORMAT_RIGHT, 40);
}

void ImagesListImage::UpdateItem(unsigned int imgNr)
{
    DEBUG_DEBUG("update image list item " << imgNr);
    DEBUG_ASSERT((int)imgNr < GetItemCount());
    const PanoImage & img = pano.getImage(imgNr);
    wxFileName fn(wxString (img.getFilename().c_str(), *wxConvCurrent));
    VariableMap var = pano.getImageVariables(imgNr);

//    wxLogMessage(wxString::Format(_("updating image list item %d, filename %s"),imgNr, fn.GetFullName()));

    SetItem(imgNr, 1, fn.GetFullName() );
    SetItem(imgNr, 2, wxString::Format(wxT("%d"), img.getWidth()));
    SetItem(imgNr, 3, wxString::Format(wxT("%d"), img.getHeight()));
    SetItem(imgNr, 4, doubleTowxString(map_get(var,"y").getValue(),m_degDigits));
    SetItem(imgNr, 5, doubleTowxString( map_get(var,"p").getValue(),m_degDigits));
    SetItem(imgNr, 6, doubleTowxString( map_get(var,"r").getValue(),m_degDigits));
    wxChar flags[] = wxT("--");
    if (pano.getOptions().optimizeReferenceImage == imgNr) {
        flags[0]='A';
    }
    if (pano.getOptions().colorReferenceImage == imgNr) {
        flags[1]='C';
    }
    SetItem(imgNr,7, wxString(flags, *wxConvCurrent));
    // urgh.. slow.. stupid.. traverse control point list for each image..
    const CPVector & cps = pano.getCtrlPoints();
    int nCP=0;
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it) {
        if ((*it).image1Nr == imgNr || (*it).image2Nr == imgNr) {
            nCP++;
        }
    }
    SetItem(imgNr, 8, wxString::Format(wxT("%d"), nCP));
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
    InsertColumn( 10, _("g"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 11, _("t"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 12, _("Crop"), wxLIST_FORMAT_RIGHT,40);
}

void ImagesListLens::UpdateItem(unsigned int imgNr)
{
    const PanoImage & img = pano.getImage(imgNr);
    wxFileName fn(wxString (img.getFilename().c_str(), *wxConvCurrent));
    SetItem(imgNr, 1, fn.GetFullName() );
    SetItem(imgNr, 2, wxString::Format(wxT("%d"),img.getLensNr()));

    VariableMap var = pano.getImageVariables(imgNr);
    const Lens & lens = pano.getLens( img.getLensNr());
    wxString ps;
    switch ( (int) lens.getProjection() ) {
    case Lens::RECTILINEAR:          ps << _("Normal (rectilinear)"); break;
    case Lens::PANORAMIC:            ps << _("Panoramic (cylindrical)"); break;
    case Lens::CIRCULAR_FISHEYE:     ps << _("Circular fisheye"); break;
    case Lens::FULL_FRAME_FISHEYE:   ps << _("Full frame fisheye"); break;
    case Lens::EQUIRECTANGULAR:      ps << _("Equirectangular"); break;
    }
    SetItem(imgNr, 3, ps);
    SetItem(imgNr, 4, doubleTowxString( map_get(var, "v").getValue(),m_degDigits));
    SetItem(imgNr, 5, doubleTowxString( map_get(var, "a").getValue(),m_distDigits));
    SetItem(imgNr, 6, doubleTowxString( map_get(var, "b").getValue(),m_distDigits));
    SetItem(imgNr, 7, doubleTowxString( map_get(var, "c").getValue(),m_distDigits));
    SetItem(imgNr, 8, doubleTowxString( map_get(var, "d").getValue(),m_pixelDigits));
    SetItem(imgNr, 9, doubleTowxString( map_get(var, "e").getValue(),m_pixelDigits));
    SetItem(imgNr, 10, doubleTowxString( map_get(var, "g").getValue(),m_distDigits));
    SetItem(imgNr, 11, doubleTowxString( map_get(var, "t").getValue(),m_distDigits));
    wxString cropstr(wxT("-"));
    if ( img.getOptions().docrop ) {
        vigra::Rect2D c = img.getOptions().cropRect;
        cropstr.Printf(wxT("%d,%d,%d,%d"), c.left(), c.right(), c.top(), c.bottom());
    }
    SetItem(imgNr, 12, cropstr);
}

