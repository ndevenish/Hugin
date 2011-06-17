// -*- c-basic-offset: 4 -*-
/** @file LensPanel.h
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de>
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

#ifndef _LENSPANEL_H
#define _LENSPANEL_H

#include "PT/Panorama.h"
#include "hugin/MainFrame.h"

#include <panodata/StandardImageVariableGroups.h>

class ImagesListLens;

/** Define the second the Lens panel
 *
 *  - the second for lens selection to images
 *
 *  @note currently only one lens per panorama is supported.
 *
 *  @todo add support multiple lenses
 *
 */
class LensPanel: public wxPanel, public PT::PanoramaObserver
{
 public:
    LensPanel( );
    
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(PT::Panorama * pano);
   
    virtual ~LensPanel(void) ;

    /// hack to restore the layout on next resize
    void RestoreLayoutOnNextResize()
    {
        m_restoreLayoutOnResize = true;
    }

    /** this is called whenever the panorama has changed.
     *
     *  This function must now update all the gui representations
     *  of the panorama to display the new state.
     *
     *  Functions that change the panororama must not update
     *  the GUI directly. The GUI should always be updated
     *  to reflect the current panorama state in this function.
     *
     *  This avoids unnessecary close coupling between the
     *  controller and the view (even if they sometimes
     *  are in the same object). See model view controller
     *  pattern.
     *
     *  @todo   react on different update signals more special
     */
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** update the edit Lens values with values from the
     *  selected image.
     */
    void UpdateLensDisplay ();

    /** enabled or disable the controls depending on selected images */
    void EnableInputs();

    void EditVigCorr (wxCommandEvent & e);

 private:
    // a window event
    //void OnSize(wxSizeEvent & e);

    // event handlers
    /**  changes lens type */
    void LensTypeChanged (wxCommandEvent & e);
    /**  computes & updates HFOV */
    void focalLengthChanged(wxCommandEvent & e);
    void focalLengthFactorChanged(wxCommandEvent & e);

    void ResponseTypeChanged (wxCommandEvent & e);

    /**  handlers for all other variables */
    void OnVarChanged(wxCommandEvent & e);
    /**  for all other variables */
    void OnVarInheritChanged(wxCommandEvent & e);

    /** try to read exif data from a file */
    void OnReadExif(wxCommandEvent & e);

    /** save the current lens parameters to a simple ini file */
    void OnSaveLensParameters(wxCommandEvent & e);

    /** load the lens parameters for a file */
    void OnLoadLensParameters(wxCommandEvent & e);

    /** change lens number of image */
    void OnChangeLens(wxCommandEvent & e);

    /** create a new lens and assign it to the
     *  selected images
     */
    void OnNewLens(wxCommandEvent & e);

    /** reset lens parameters
     */
    void OnReset(wxCommandEvent & e);

    /** catches changes to the list selection */
    void ListSelectionChanged(wxListEvent& e);


    // the model
    Panorama * pano;
    
    HuginBase::StandardImageVariableGroups * variable_groups;

    ImagesListLens * images_list;

    // currently selected images and lenses
    UIntSet m_selectedImages;
    UIntSet m_selectedLenses;
//    unsigned int m_editImageNr;
//    unsigned int m_editLensNr;

    void updateHFOV(void);
    int m_degDigits;
    int m_distDigitsEdit;
    int m_pixelDigits;

    wxPanel *m_lens_ctrls;

    bool m_restoreLayoutOnResize;

    static const char *m_varNames[];

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(LensPanel)

};

/** xrc handler */
class LensPanelXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(LensPanelXmlHandler)

public:
    LensPanelXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // _LENSPANEL_H
