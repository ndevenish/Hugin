// -*- c-basic-offset: 4 -*-

/** @file ImagesList.cpp
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

#include "base_wx/wxPlatform.h"
#include "hugin/ImagesList.h"
#include "base_wx/wxImageCache.h"
#include "base_wx/platform.h"

#ifdef __WXMAC__
#include "hugin/MainFrame.h"
#endif

using namespace PT;
using namespace hugin_utils;

#if wxCHECK_VERSION(2,9,0)
    wxDEFINE_EVENT(EVT_IMAGE_ADD,wxCommandEvent);
    wxDEFINE_EVENT(EVT_IMAGE_DEL,wxCommandEvent);
#else
#if _WINDOWS && defined Hugin_shared
    DEFINE_LOCAL_EVENT_TYPE(EVT_IMAGE_ADD)
    DEFINE_LOCAL_EVENT_TYPE(EVT_IMAGE_DEL)
#else
    DEFINE_EVENT_TYPE(EVT_IMAGE_ADD)
    DEFINE_EVENT_TYPE(EVT_IMAGE_DEL)
#endif
#endif

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ImagesList, wxListCtrl)
    EVT_LIST_ITEM_SELECTED(-1, ImagesList::OnItemSelected)
    EVT_LIST_ITEM_DESELECTED(-1, ImagesList::OnItemDeselected)
    EVT_LIST_COL_END_DRAG(-1, ImagesList::OnColumnWidthChange)
    EVT_CHAR(ImagesList::OnChar)
END_EVENT_TABLE()

// Define a constructor for the Images Panel
ImagesList::ImagesList()
{
    pano = 0;
}

bool ImagesList::Create(wxWindow* parent, wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
{
    DEBUG_TRACE("List");
    if (! wxListCtrl::Create(parent, id, pos, size, wxLC_REPORT | style) ) {
        return false;
    }

    DEBUG_TRACE("List, adding columns");

    m_notifyParents = true;
    InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 35 );

    // get a good size for the images
    wxPoint sz(1,11);
    sz = ConvertDialogToPixels(sz);
    m_iconHeight = sz.y;
    DEBUG_DEBUG("icon Height: " << m_iconHeight);

    //m_smallIcons = new wxImageList(m_iconHeight, m_iconHeight);
    //AssignImageList(m_smallIcons,wxIMAGE_LIST_SMALL);
    DEBUG_TRACE("");
    m_degDigits = wxConfigBase::Get()->Read(wxT("/General/DegreeFractionalDigits"),1);
    m_pixelDigits = wxConfigBase::Get()->Read(wxT("/General/PixelFractionalDigits"),1);
    m_distDigits = wxConfigBase::Get()->Read(wxT("/General/DistortionFractionalDigits"),3);
    m_singleSelect=(style & wxLC_SINGLE_SEL)!=0;
    return true;
}

void ImagesList::Init(PT::Panorama * panorama)
{
    pano = panorama;
    pano->addObserver(this);
    
    /// @todo check new didn't return NULL in non-debug builds.
    variable_groups = new HuginBase::StandardImageVariableGroups(*pano);
    DEBUG_ASSERT(variable_groups);
    
#ifdef __WXMAC__
    SetDropTarget(new PanoDropTarget(*pano, true));
#endif
}

ImagesList::~ImagesList(void)
{
    DEBUG_TRACE("");
    pano->removeObserver(this);
    delete variable_groups;
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
        //m_smallIcons->Remove(i);
    }
    
    // Make sure the part numbers are up to date before writing them to the table.
    variable_groups->update();

    // update existing items
//    if ( nrImages >= nrItems ) {
        for(UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it){
            if (*it >= nrItems) {
                // create new item.
                DEBUG_DEBUG("creating " << *it);
                wxBitmap small0(m_iconHeight, m_iconHeight);
                //createIcon(small0, *it, m_iconHeight);
                //m_smallIcons->Add(small0);

                CreateItem(*it);

            } else {
                // update existing item
                DEBUG_DEBUG("updating item" << *it);
                wxBitmap small0(m_iconHeight, m_iconHeight);
                //createIcon(small0, *it, m_iconHeight);
                //m_smallIcons->Replace(*it, small0);

                UpdateItem(*it);
            }
            ImageCache::getInstance().softFlush();
        }
//    }
    
    // Update the part numbers. We do this for every image, as the images that
    // have changed may affect the part numbers for images that haven't changed.
    for (unsigned int i = 0; i < pano.getNrOfImages(); i++)
    {
        UpdatePartNumbersForItem(i);
    }
    
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

#if 0
void ImagesList::createIcon(wxBitmap & bitmap, unsigned int imgNr, unsigned int size)
{
    ImageCache::Entry * cacheEntry = ImageCache::getInstance().getSmallImage(
            pano->getImage(imgNr).getFilename());
    if (! cacheEntry) {
        return;
    }
    wxImage * s_img = cacheEntry->image; 
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
#endif

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

void ImagesList::SelectAll()
{
    unsigned int nrItems = GetItemCount();
    for (unsigned int i=0; i < nrItems ; i++)
    {
        SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
}

void ImagesList::DeselectAll()
{
    unsigned int nrItems = GetItemCount();
    for (unsigned int i=0; i < nrItems ; i++) {
        int selected = GetItemState(i, wxLIST_STATE_SELECTED);
        if (selected) {
            SetItemState(i, 0, wxLIST_STATE_SELECTED);
        }
    }
}

void ImagesList::OnItemSelected ( wxListEvent & e )
{
    DEBUG_TRACE(e.GetIndex());
    if(e.GetIndex() < 0) return;
    selectedItems.insert((int)e.GetIndex());
    // allow other parents to receive this event, only if its
    // generated by the user, and not during an automatic update
    // triggered by a change to the panorama
    if (m_notifyParents) {
        e.Skip();
    }
}

void ImagesList::OnChar(wxKeyEvent &e)
{
    switch(e.GetKeyCode())
    {
        case WXK_INSERT:
            {
                wxCommandEvent ev(EVT_IMAGE_ADD,wxID_ANY);
                GetParent()->GetEventHandler()->AddPendingEvent(ev);
                break;
            };
        case WXK_DELETE:
            {
                wxCommandEvent ev(EVT_IMAGE_DEL,wxID_ANY);
                GetParent()->GetEventHandler()->AddPendingEvent(ev);
                break;
            };
        case 1: //Ctrl+A
            {
                if(!m_singleSelect && e.ControlDown())
                {
                    SelectAll();
                }
                break;
            }
    };
    e.Skip();
};


void ImagesList::OnItemDeselected ( wxListEvent & e )
{
    DEBUG_TRACE(e.GetIndex());
    if(e.GetIndex() < 0) return;
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

void ImagesList::OnColumnWidthChange( wxListEvent & e )
{
    if(m_configClassName != wxT(""))
    {
        int colNum = e.GetColumn();
        wxConfigBase::Get()->Write( m_configClassName+wxString::Format(wxT("/ColumnWidth%d"),colNum), GetColumnWidth(colNum) );
    }
}

void ImagesList::UpdateItem(unsigned int imgNr)
{

}

void ImagesList::UpdatePartNumbersForItem(unsigned int imgNr)
{

}

IMPLEMENT_DYNAMIC_CLASS(ImagesList, wxListCtrl)

// ============================================================================
// ============================================================================
// Image

ImagesListImage::ImagesListImage()
{

}


bool ImagesListImage::Create(wxWindow* parent, wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
{
    DEBUG_TRACE("ListImage");
    if (! ImagesList::Create(parent, id, pos, size, style, name)) {
        return false;
    }

    m_configClassName = wxT("/ImagesListImage");

    InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 200 );
    InsertColumn( 2, _("width"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 3, _("height"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 4, _("yaw (y)"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 5, _("pitch (p)"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 6, _("roll (r)"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 7, _("X (TrX)"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 8, _("Y (TrY)"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 9, _("Z (TrZ)"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 10, _("Anchor"), wxLIST_FORMAT_RIGHT, 60 );
    InsertColumn( 11, _("# Ctrl Pnts"), wxLIST_FORMAT_RIGHT, 60);
    InsertColumn( 12, _("Stack Number"), wxLIST_FORMAT_RIGHT, 60);
    
    //get saved width
    for ( int j=0; j < GetColumnCount() ; j++ )
    {
        // -1 is auto
        int width = wxConfigBase::Get()->Read(wxString::Format(m_configClassName+wxT("/ColumnWidth%d"), j ), -1);
        if(width != -1)
            SetColumnWidth(j, width);
    }
    return true;
}

void ImagesListImage::Init(PT::Panorama * panorama)
{
    ImagesList::Init(panorama);
}

void ImagesListImage::UpdateItem(unsigned int imgNr)
{
    DEBUG_DEBUG("update image list item " << imgNr);
    DEBUG_ASSERT((int)imgNr < GetItemCount());
    const SrcPanoImage & img = pano->getImage(imgNr);
    wxFileName fn(wxString (img.getFilename().c_str(), HUGIN_CONV_FILENAME));

//    wxLogMessage(wxString::Format(_("updating image list item %d, filename %s"),imgNr, fn.GetFullName()));

    SetItem(imgNr, 1, fn.GetFullName() );
    SetItem(imgNr, 2, wxString::Format(wxT("%d"), img.getSize().width()));
    SetItem(imgNr, 3, wxString::Format(wxT("%d"), img.getSize().height()));
    SetItem(imgNr, 4, doubleTowxString(img.getYaw(), m_degDigits));
    SetItem(imgNr, 5, doubleTowxString(img.getPitch(), m_degDigits));
    SetItem(imgNr, 6, doubleTowxString(img.getRoll(), m_degDigits));
    SetItem(imgNr, 7, doubleTowxString(img.getX(), m_distDigits));
    SetItem(imgNr, 8, doubleTowxString(img.getY(), m_distDigits));
    SetItem(imgNr, 9, doubleTowxString(img.getZ(), m_distDigits));
    wxChar flags[] = wxT("--");
    if (pano->getOptions().optimizeReferenceImage == imgNr) {
        flags[0]='A';
    }
    if (pano->getOptions().colorReferenceImage == imgNr) {
        flags[1]='C';
    }
    SetItem(imgNr,10, wxString(flags, wxConvLocal));
    // urgh.. slow.. stupid.. traverse control point list for each image..
    const CPVector & cps = pano->getCtrlPoints();
    int nCP=0;
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it) {
        if ((*it).image1Nr == imgNr || (*it).image2Nr == imgNr) {
            nCP++;
        }
    }
    SetItem(imgNr, 11, wxString::Format(wxT("%d"), nCP));
}

void ImagesListImage::UpdatePartNumbersForItem(unsigned int imgNr)
{
    unsigned int stackNumber = variable_groups->getStacks().getPartNumber(imgNr);
    SetItem(imgNr, 12, wxString::Format(wxT("%d"), stackNumber));
}

IMPLEMENT_DYNAMIC_CLASS(ImagesListImage, ImagesList)

ImagesListImageXmlHandler::ImagesListImageXmlHandler()
    : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *ImagesListImageXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, ImagesListImage)

    cp->Create(m_parentAsWindow,
               GetID(),
               GetPosition(), GetSize(),
               GetStyle(wxT("style")),
               GetName());

    SetupWindow( cp);

    return cp;
}

bool ImagesListImageXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("ImagesListImage"));
}

IMPLEMENT_DYNAMIC_CLASS(ImagesListImageXmlHandler, wxXmlResourceHandler)

      
// ============================================================================
// ============================================================================
// Lens


ImagesListLens::ImagesListLens()
{

}


bool ImagesListLens::Create(wxWindow* parent, wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
{
    DEBUG_TRACE("ListLens");
    if (! ImagesList::Create(parent, id, pos, size, style, name)) {
        return false;
    }
    DEBUG_TRACE("ListLens: Add list headers");

    m_configClassName = wxT("/ImagesListLens");

    InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 200 );
    InsertColumn( 2, _("Lens no."), wxLIST_FORMAT_LEFT, 60);
    InsertColumn( 3, _("Lens type (f)"), wxLIST_FORMAT_LEFT, 100 );
    InsertColumn( 4, _("EV"), wxLIST_FORMAT_RIGHT, 50 );
    InsertColumn( 5, _("hfov (v)"), wxLIST_FORMAT_RIGHT, 80 );
    InsertColumn( 6, wxT("a"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 7, wxT("b"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 8, wxT("c"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 9, wxT("d"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 10, wxT("e"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 11, wxT("g"), wxLIST_FORMAT_RIGHT, 40 );
    InsertColumn( 12, wxT("t"), wxLIST_FORMAT_RIGHT, 40 );

    //get saved width
    for ( int j=0; j < GetColumnCount() ; j++ )
    {
        // -1 is auto
        int width = wxConfigBase::Get()->Read(wxString::Format(m_configClassName+wxT("/ColumnWidth%d"), j ), -1);
        if(width != -1)
            SetColumnWidth(j, width);
    }
    return true;
}

void ImagesListLens::Init(PT::Panorama * panorama)
{
    ImagesList::Init(panorama);
}

void ImagesListLens::UpdateItem(unsigned int imgNr)
{
    const SrcPanoImage & img = pano->getImage(imgNr);
    wxFileName fn(wxString (img.getFilename().c_str(), HUGIN_CONV_FILENAME));
    SetItem(imgNr, 1, fn.GetFullName() );
    SetItem(imgNr, 2, wxString::Format(wxT("%d"),variable_groups->getLenses().getPartNumber(imgNr)));

    VariableMap var = pano->getImageVariables(imgNr);
    wxString ps;
    switch ( img.getProjection() ) {
    case SrcPanoImage::RECTILINEAR:          ps << _("Normal (rectilinear)"); break;
    case SrcPanoImage::PANORAMIC:            ps << _("Panoramic (cylindrical)"); break;
    case SrcPanoImage::CIRCULAR_FISHEYE:     ps << _("Circular fisheye"); break;
    case SrcPanoImage::FULL_FRAME_FISHEYE:   ps << _("Full frame fisheye"); break;
    case SrcPanoImage::EQUIRECTANGULAR:      ps << _("Equirectangular"); break;
    case SrcPanoImage::FISHEYE_ORTHOGRAPHIC: ps << _("Orthographic"); break;
    case SrcPanoImage::FISHEYE_STEREOGRAPHIC:ps << _("Stereographic"); break;
    case SrcPanoImage::FISHEYE_EQUISOLID:    ps << _("Equisolid"); break;
    case SrcPanoImage::FISHEYE_THOBY:        ps << _("Fisheye Thoby"); break;
    }
    SetItem(imgNr, 3, ps);
    SetItem(imgNr, 4, doubleTowxString( map_get(var, "Eev").getValue(),1));
    SetItem(imgNr, 5, doubleTowxString( map_get(var, "v").getValue(),m_degDigits));
    SetItem(imgNr, 6, doubleTowxString( map_get(var, "a").getValue(),m_distDigits));
    SetItem(imgNr, 7, doubleTowxString( map_get(var, "b").getValue(),m_distDigits));
    SetItem(imgNr, 8, doubleTowxString( map_get(var, "c").getValue(),m_distDigits));
    SetItem(imgNr, 9, doubleTowxString( map_get(var, "d").getValue(),m_pixelDigits));
    SetItem(imgNr, 10, doubleTowxString( map_get(var, "e").getValue(),m_pixelDigits));
    SetItem(imgNr, 11, doubleTowxString( map_get(var, "g").getValue(),m_distDigits));
    SetItem(imgNr, 12, doubleTowxString( map_get(var, "t").getValue(),m_distDigits));
}

void ImagesListLens::UpdatePartNumbersForItem(unsigned int imgNr)
{
    // Update the part numbers.
    SetItem(imgNr, 2, wxString::Format(wxT("%d"),variable_groups->getLenses().getPartNumber(imgNr)));
}


IMPLEMENT_DYNAMIC_CLASS(ImagesListLens, ImagesList)

ImagesListLensXmlHandler::ImagesListLensXmlHandler()
    : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *ImagesListLensXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, ImagesListLens)

            cp->Create(m_parentAsWindow,
                       GetID(),
                       GetPosition(), GetSize(),
                       GetStyle(wxT("style")),
                       GetName());

    SetupWindow( cp);

    return cp;
}

bool ImagesListLensXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("ImagesListLens"));
}

IMPLEMENT_DYNAMIC_CLASS(ImagesListLensXmlHandler, wxXmlResourceHandler)

      
// ============================================================================
// ============================================================================
// Crop


ImagesListCrop::ImagesListCrop()
{

}


bool ImagesListCrop::Create(wxWindow* parent, wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style,
                             const wxString& name)
{
    DEBUG_TRACE("");
    if (!ImagesList::Create(parent, id, pos, size, style, name))
        return false;

    m_configClassName = wxT("/ImagesListCrop");

    InsertColumn(1, _("Crop"), wxLIST_FORMAT_RIGHT,120);

    //get saved width
    for ( int j=0; j < GetColumnCount() ; j++ )
    {
        // -1 is auto
        int width = wxConfigBase::Get()->Read(wxString::Format(m_configClassName+wxT("/ColumnWidth%d"), j ), -1);
        if(width != -1)
            SetColumnWidth(j, width);
    }
    return true;
}


void ImagesListCrop::Init(PT::Panorama * panorama)
{
    ImagesList::Init(panorama);
}

void ImagesListCrop::UpdateItem(unsigned int imgNr)
{
    const SrcPanoImage & img = pano->getImage(imgNr);
    wxFileName fn(wxString (img.getFilename().c_str(), HUGIN_CONV_FILENAME));

    wxString cropstr(wxT("-"));
    if ( img.getCropMode() != SrcPanoImage::NO_CROP ) {
        vigra::Rect2D c = img.getCropRect();
        cropstr.Printf(wxT("%d,%d,%d,%d"), c.left(), c.right(), c.top(), c.bottom());
    }
    SetItem(imgNr, 1, cropstr);
}

IMPLEMENT_DYNAMIC_CLASS(ImagesListCrop, ImagesList)

ImagesListCropXmlHandler::ImagesListCropXmlHandler()
    : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *ImagesListCropXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, ImagesListCrop)

    cp->Create(m_parentAsWindow,
               GetID(),
               GetPosition(), GetSize(),
               GetStyle(wxT("style")),
               GetName());

    SetupWindow( cp);

    return cp;
}

bool ImagesListCropXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("ImagesListCrop"));
}

IMPLEMENT_DYNAMIC_CLASS(ImagesListCropXmlHandler, wxXmlResourceHandler)


// ============================================================================
// ============================================================================
// Mask

ImagesListMask::ImagesListMask()
{
}


bool ImagesListMask::Create(wxWindow* parent, wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style,
                             const wxString& name)
{
    DEBUG_TRACE("");
    if (!ImagesList::Create(parent, id, pos, size, wxLC_SINGLE_SEL | style, name))
        return false;

    m_configClassName = wxT("/ImagesListMask");

    InsertColumn(1, _("Number of masks"), wxLIST_FORMAT_RIGHT,120);

    //get saved width
    for ( int j=0; j < GetColumnCount() ; j++ )
    {
        // -1 is auto
        int width = wxConfigBase::Get()->Read(wxString::Format(m_configClassName+wxT("/ColumnWidth%d"), j ), -1);
        if(width != -1)
            SetColumnWidth(j, width);
    }
    return true;
}


void ImagesListMask::Init(PT::Panorama * panorama)
{
    ImagesList::Init(panorama);
}

void ImagesListMask::UpdateItem(unsigned int imgNr)
{
    wxString maskstr;
    if(pano->getImage(imgNr).hasMasks())
        maskstr=wxString::Format(wxT("%lu"), (unsigned long int) pano->getImage(imgNr).getMasks().size());
    else
        maskstr=wxT("-");
    SetItem(imgNr, 1, maskstr);
}

IMPLEMENT_DYNAMIC_CLASS(ImagesListMask, ImagesList)

ImagesListMaskXmlHandler::ImagesListMaskXmlHandler()
    : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *ImagesListMaskXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, ImagesListMask)

    cp->Create(m_parentAsWindow,
               GetID(),
               GetPosition(), GetSize(),
               GetStyle(wxT("style")),
               GetName());

    SetupWindow( cp);

    return cp;
}

bool ImagesListMaskXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("ImagesListMask"));
}

IMPLEMENT_DYNAMIC_CLASS(ImagesListMaskXmlHandler, wxXmlResourceHandler)

