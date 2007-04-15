// -*- c-basic-offset: 4 -*-
/** @file List.h
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
//#include "hugin/MainFrame.h"

using namespace PT;

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
    ImagesList( wxWindow * win, Panorama * pano);
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
    virtual void UpdateItem(unsigned int imgNr) = 0;

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

    /** get the currently selected images */
    const UIntSet & GetSelected() const;

protected:
    // the model
    Panorama &pano;

    // update selected map
    void OnItemSelected ( wxListEvent & e );
    // update selected map
    void OnItemDeselected ( wxListEvent & e );
    // save the column width when changed
    void OnColumnWidthChange( wxListEvent & e );

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

    //for saving column width
    wxString m_configClassName;
        
    DECLARE_EVENT_TABLE()
};

/** specialized to display image data (width, position)
 */
class ImagesListImage : public ImagesList
{
public:
    ImagesListImage(wxWindow * parent, Panorama * pano);

    /** update the information in an already existing list item
     */
    virtual void UpdateItem(unsigned int imgNr);


};


/** specialized to display the lens aspect of images
 */
class ImagesListLens : public ImagesList
{
public:
    ImagesListLens(wxWindow * parent, Panorama * pano);

    /** update the information in an already existing list item
     */
    virtual void UpdateItem(unsigned int imgNr);


};

/** specialized to display the crop aspect of images
 */
class ImagesListCrop : public ImagesList
{
public:
    ImagesListCrop(wxWindow * parent, Panorama * pano);

    /** update the information in an already existing list item
     */
    virtual void UpdateItem(unsigned int imgNr);


};


#endif // _LIST_H
