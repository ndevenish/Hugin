// -*- c-basic-offset: 4 -*-
/** @file MaskEditorPanel.h
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */

/*  This is free software; you can redistribute it and/or
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

#ifndef _MaskEditorPanel_H
#define _MaskEditorPanel_H


//-----------------------------------------------------------------------------
// Headers
//-----------------------------------------------------------------------------

#include <PT/Panorama.h>
#include <wx/clrpicker.h>
#include "MaskImageCtrl.h"
#include "ImagesList.h"

/** mask editor panel.
 *
 *  This panel is used to create/change/edit masks
 *
 */
class MaskEditorPanel : public wxPanel, public PT::PanoramaObserver
{
public:
    /** ctor.
     */
    MaskEditorPanel();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(PT::Panorama * pano);

    /** dtor.
     */
    virtual ~MaskEditorPanel();

    void SetPano(PT::Panorama * panorama)
        { m_pano = panorama; };
    /** sets the image, which is currently edited, set imgNr to UINT_MAX if no image is editing */
    void setImage(unsigned int imgNr);
    /** sets active mask number, set to UINT_MAX, if no mask is currently editing */
    void setMask(unsigned int maskNr);
    /** called when mask where changed in MaskImageCtrl */
    void UpdateMask();
    /** called when new mask added in MaskImageCtrl */
    void AddMask();
    /** selects the mask with index newMaskNr in the listbox */
    void SelectMask(unsigned int newMaskNr);

    /** called when the panorama changes and we should
     *  update our display
     */
    void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** called when user selected an other image */
    void OnImageSelect(wxListEvent &e);
    /** called when user selected an other mask */
    void OnMaskSelect(wxListEvent &e);
    /** handler when mask type was changed */
    void OnMaskTypeChange(wxCommandEvent &e);
    /** called when user wants to create new polygon */
    void OnMaskAdd(wxCommandEvent &e);
    /** called when user wants to save active mask */
    void OnMaskSave(wxCommandEvent &e);
    /** called when user wants to load a mask into the selected image */
    void OnMaskLoad(wxCommandEvent &e);
    /** called when user wants to copy a mask to clipboard */
    void OnMaskCopy(wxCommandEvent &e);
    /** called when user wants to paste a mask from clipboard */
    void OnMaskPaste(wxCommandEvent &e);
    /** called when user wants to delete active mask */
    void OnMaskDelete(wxCommandEvent &e);
    /** sets the actual zoom factor */
    void OnZoom(wxCommandEvent & e);
    /** event handler for changing colours */
    void OnColourChanged(wxColourPickerEvent &e);

private:

    /** updates the display after another image has been selected.
     *  updates mask list and editor panel 
     */
    void UpdateMaskList(bool restoreSelection=false);
    /** return index of currently selected masks, return UINT_MAX if no mask is selected */
    unsigned int GetSelectedMask();
    /** called, when column with of mask list box was changed */
    void OnColumnWidthChange( wxListEvent & e );
    /** load the mask from stream */
    void LoadMaskFromStream(std::istream& stream,vigra::Size2D& imageSize,HuginBase::MaskPolygonVector &newMasks);
    /** save the mask into stream */
    void SaveMaskToStream(std::ostream& stream);
    /** determines, if the image should be rotated for display */
    MaskImageCtrl::ImageRotation GetRot(const unsigned int imgNr);

    // GUI controls
    MaskImageCtrl *m_editImg;
    ImagesListMask *m_imagesListMask;
    wxListCtrl *m_maskList;
    wxChoice *m_maskType;

    // my data
    PT::Panorama * m_pano;
    // current masks vector
    HuginBase::MaskPolygonVector m_currentMasks;
    // the current images
    unsigned int m_ImageNr;
    // the current mask
    unsigned int m_MaskNr;
    // the filename of the current image
    std::string m_File;

    DECLARE_EVENT_TABLE();
    DECLARE_DYNAMIC_CLASS(MaskEditorPanel)
};

/** xrc handler for handling mask editor panel */
class MaskEditorPanelXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(MaskEditorPanelXmlHandler)

public:
    MaskEditorPanelXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};


#endif // _MaskEditorPanel_H
