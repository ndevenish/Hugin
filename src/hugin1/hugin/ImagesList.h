// -*- c-basic-offset: 4 -*-
/** @file ImagesList.h
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de>
 *
 *  Rewritten by Pablo d'Angelo
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _IMAGESLIST_H
#define _IMAGESLIST_H


#include "PT/Panorama.h"
#include <panodata/StandardImageVariableGroups.h>
//#include "hugin/MainFrame.h"

using namespace PT;

//declare 2 event types to communicate with parent about adding/deleting images
#if wxCHECK_VERSION(2,9,0)
    wxDECLARE_EVENT(EVT_IMAGE_ADD,wxCommandEvent);
    wxDECLARE_EVENT(EVT_IMAGE_DEL,wxCommandEvent);
#else
#if _WINDOWS && defined Hugin_shared
    DECLARE_LOCAL_EVENT_TYPE(EVT_IMAGE_ADD,-1)
    DECLARE_LOCAL_EVENT_TYPE(EVT_IMAGE_DEL,-1)
#else
    DECLARE_EVENT_TYPE(EVT_IMAGE_ADD,-1)
    DECLARE_EVENT_TYPE(EVT_IMAGE_DEL,-1)
#endif
#endif

/** multi use list.
 *
 *  This list shall contain the overall handling of datas in lists,
 *  moving selecting, drag'n drop
 *
 *  the special layouts are realized with subclasses and not
 *  implemented in the base class. Subclasses can override CreateItem(),
 *  UpdateItem() and RemoveItem() to customize the layout.
 *
 *  They can also handle the list events, for example if they want
 *  to veto a selection.
 *
 *
 *  parent controls can use
 *     EVT_LIST_ITEM_SELECTED(id, func)
 *     EVT_LIST_ITEM_DESELECTED(id, func)
 *  to get notified when the selection changes. 
 *  WARNING: Do not read the event in the parent control, just
 *           use GetSelected()!
 *
 *  @todo   make the listcontents editable -> wxListCtrl->wxGrid?
 */
class ImagesList: public wxListCtrl, public PT::PanoramaObserver
{
public:
    ImagesList();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(PT::Panorama * pano);
   
    virtual ~ImagesList(void) ;

    /** receive the update signal and update display accordingly
     */
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** create an list item for imgNr
     *
     *  This creates the list item and calles UpdateItem(imgNr)
     *  to fill it with information
     *  usually, just UpdateItem needs to be overridden by base
     *  classes
     */
    virtual void CreateItem(unsigned int imgNr);

    /** update the information in an already existing list item
     */
    virtual void UpdateItem(unsigned int imgNr);
    
    /** Update the part numbers (e.g. Lens number) in an already existing list
     * item.
     * 
     * Needed as the part numbers can change without the rest of the image
     */
    virtual void UpdatePartNumbersForItem(unsigned int imgNr);
    
    /** remove an existing list item
     *
     *  just calles wxListCtrl::DeleteItem, but might be overriden
     *  to release ressources associated with the item
     */
    virtual void RemoveItem(unsigned int imgNr);

    /** Select an image
     *
     *  selects image @p imgNr, and deselects all other images
     */
    void SelectSingleImage(unsigned int imgNr);
     /** Select an image range
      *
      *  selects images between @p imgNs and @p imgNe, 
      *  and deselects all other images
      */
    void SelectImageRange(unsigned int imgStart,unsigned int imgEnd);

    /** selects the given images */
    void SelectImages(const HuginBase::UIntSet imgs);

    /** Select all images */
    void SelectAll();

    /** Deselects all images */
    void DeselectAll();

    /** get the currently selected images */
    const UIntSet & GetSelected() const;

protected:
    // the model
    Panorama * pano;
    
    // image variable group information
    HuginBase::StandardImageVariableGroups * variable_groups;

    // update selected map
    void OnItemSelected ( wxListEvent & e );
    // update selected map
    void OnItemDeselected ( wxListEvent & e );
    // save the column width when changed
    void OnColumnWidthChange( wxListEvent & e );
    /** event handler to capture special key code */
    void OnChar( wxKeyEvent & e);

    /** create icons for an image */
    //void createIcon(wxBitmap & bitmap, unsigned int imgNr, unsigned int size);

    UIntSet selectedItems;

    // image icons.
    wxImageList *m_smallIcons;
    int m_iconHeight;
    int m_degDigits;
    int m_distDigits;
    int m_pixelDigits;

    // propagate list selections/deselections to client.
    bool m_notifyParents;
    bool m_singleSelect;

    //for saving column width
    wxString m_configClassName;
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(ImagesList)
};

/** specialized to display the mask aspect of images
 */
class ImagesListMask : public ImagesList
{
public:
    ImagesListMask();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(PT::Panorama * pano);

    /** update the information in an already existing list item
     */
    virtual void UpdateItem(unsigned int imgNr);
    /** sets the listbox to single item select or multiply item select */
    void SetSingleSelect(bool isSingleSelect);

    DECLARE_DYNAMIC_CLASS(ImagesListMask)
};

/** xrc handler */
class ImagesListMaskXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(ImagesListMaskXmlHandler)

    public:
        ImagesListMaskXmlHandler();
        virtual wxObject *DoCreateResource();
        virtual bool CanHandle(wxXmlNode *node);
};


#endif // _LIST_H
