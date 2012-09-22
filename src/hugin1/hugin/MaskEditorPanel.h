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
    /** sets the image, which is currently edited 
         @param imgNr the image which should be shown, use UINT_MAX for no image selected 
         @param updateListSelection if true, the selection of the images list is updated, 
                otherwise the selection of the list remains unchanged (e.g. when calling 
                 from the list selection changed event handler)
      */
    void setImage(unsigned int imgNr, bool updateListSelection=false);
    /** sets active mask number, set to UINT_MAX, if no mask is currently editing */
    void setMask(unsigned int maskNr);
    /** called when mask where changed in MaskImageCtrl */
    void UpdateMask();
    /** called when new mask added in MaskImageCtrl */
    void AddMask();
    /** selects the mask with index newMaskNr in the listbox */
    void SelectMask(unsigned int newMaskNr);

    /** updated the crop in the Panorama object with the current values from GUI 
      *  @param updateFromImgCtrl if true, the crop is updated from the image control, otherwise from the values inside
      *  MaskEditorPanel (so from wxTextCtrls)
      */
    void UpdateCrop(bool updateFromImgCtrl=false);
    /** updates the displayed crop in the text boxes (for dragging) */
    void UpdateCropFromImage();

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
    /** event handler for changing option if active masks should be drawn */
    void OnShowActiveMasks(wxCommandEvent &e);
    // reset crop area. 
    void OnResetButton(wxCommandEvent & e);
    void OnSetLeft(wxCommandEvent & e);
    void OnSetRight(wxCommandEvent & e);
    void OnSetTop(wxCommandEvent & e);
    void OnSetBottom(wxCommandEvent & e);
    void OnAutoCenter(wxCommandEvent & e);
    void OnModeChanged(wxNotebookEvent & e);

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
    /** copies the crop information from the Panorama object to GUI */
    void DisplayCrop(int imgNr);

    /** update GUI display */
    void UpdateCropDisplay();
    // ensure that the crop roi is centered
    void CenterCrop();

    size_t GetImgNr();

    // GUI controls
    MaskImageCtrl *m_editImg;
    ImagesListMask *m_imagesListMask;
    wxListCtrl *m_maskList;
    wxChoice *m_maskType;
    wxNotebook *m_maskCropCtrl;

    // my data
    PT::Panorama * m_pano;
    // current masks vector
    HuginBase::MaskPolygonVector m_currentMasks;
    HuginBase::MaskPolygon::MaskType m_defaultMaskType;
    // mask or crop mode
    bool m_maskMode;
    // the current images
    HuginBase::UIntSet m_selectedImages;
    // the current mask
    unsigned int m_MaskNr;
    // the filename of the current image
    std::string m_File;

    // controls for crop editing
    wxTextCtrl * m_left_textctrl;
    wxTextCtrl * m_right_textctrl;
    wxTextCtrl * m_top_textctrl;
    wxTextCtrl * m_bottom_textctrl;
    wxCheckBox * m_autocenter_cb;
    HuginBase::SrcPanoImage::CropMode m_cropMode;
    vigra::Rect2D m_cropRect;
    bool m_autoCenterCrop;
    vigra::Point2D m_cropCenter;

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
