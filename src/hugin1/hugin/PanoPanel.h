// -*- c-basic-offset: 4 -*-
/** @file PanoPanel.h
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de>
 *
 *  Completely rewritten by Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _PANOPANEL_H
#define _PANOPANEL_H

#include "hugin/MainFrame.h"
#include "GuiLevel.h"

//using namespace PT;
class PanoDialog;
class wxSpinCtrl;
class wxTextCtrl;
class wxChoice;
class wxComboBox;

/** Define the pano edit panel
 *
 *  - it is for the settings of the final panorama.
 *    Maybe here goes the preview to.
 */
class PanoPanel: public wxPanel, public PT::PanoramaObserver
{
public:

    PanoPanel();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(PT::Panorama * pano);

    virtual ~PanoPanel(void) ;

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
    virtual void panoramaChanged(PT::Panorama &pano);
//    void panoramaImageChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** set the image */
// TODO remove
//    void previewSingleChanged(wxCommandEvent & e);


    /** stitching using hugin_stitch_project */
    void DoStitch();
    /** stitching with PTBatcherGUI */
    void DoSendToBatch();

    void SetGuiLevel(GuiLevel newGuiLevel);

 private:

    /// the supported stitching engines
    enum StitchingEngine { NONA=0, PTSTITCHER };

    /// the supported defaults
    enum StitchingPresets { PROFILE_CUSTOM =0 , PROFILE_JPEG=0,
                            PROFILE_DRAFT_JPEG, PROFILE_TIFF,
                            PROFILE_LAYER_TIFF, PROFILE_LAYER_PSD };


    // apply changes from the model
    bool StackCheck (PT::Panorama & pano);
    void UpdateDisplay(const PT::PanoramaOptions & opt, const bool hasStacks);
    void SetStitcher(PanoramaOptions::Remapper stitcher);

    // apply changes to the model. (gui values -> Panorama)
    void QuickModeChanged(wxCommandEvent & e);
    void ApplyQuickMode(int preset);

    void HFOVChanged(wxCommandEvent & e);
    void HFOVChangedSpin(wxSpinEvent & e);
    void VFOVChanged(wxCommandEvent & e );
    void VFOVChangedSpin(wxSpinEvent & e);
    void ProjectionChanged(wxCommandEvent & e);

    void OnOutputFilesChanged(wxCommandEvent & e);
    void RemapperChanged(wxCommandEvent & e);
    void OnRemapperOptions(wxCommandEvent & e);

    void FusionChanged(wxCommandEvent & e);
    void OnFusionOptions(wxCommandEvent & e);

    void HDRMergeChanged(wxCommandEvent & e);
    void OnHDRMergeOptions(wxCommandEvent & e);

    void BlenderChanged(wxCommandEvent & e);
    void OnBlenderOptions(wxCommandEvent & e);

    // File format options
    void FileFormatChanged(wxCommandEvent & e);
    void HDRFileFormatChanged(wxCommandEvent & e);
    void OnJPEGQualityText(wxCommandEvent & e);
    void OnNormalTIFFCompression(wxCommandEvent & e);
    void OnHDRTIFFCompression(wxCommandEvent & e);

    void OnHDRFileFormatOpts(wxCommandEvent & e);


    void WidthChanged(wxCommandEvent & e);
    void HeightChanged(wxCommandEvent & e);
    void ROIChanged(wxCommandEvent & e);

// TODO remove
//    void DoPreview(wxCommandEvent & e);
    void DoCalcFOV(wxCommandEvent & e);
    void OnDoStitch(wxCommandEvent & e);

    /** set the highest sensible width
     */
    void DoCalcOptimalWidth(wxCommandEvent & e);
    
    /** set the largest rectangle for crop ROI
     */
    void DoCalcOptimalROI(wxCommandEvent & e);    

    /** enable/disable control influenced by quick mode */
    void EnableControls(bool enable);
    
    /** Check the canvas size isn't overly huge, or the user knows what they are doing.
     * The canvas size can become too big if, for example, the field of view is
     * more than 180 degrees, then you select rectilinear proection, then press
     * "Calculate optimal size", or stitch a wide stereographic image using the
     * assistant tab.
     * If the canvas size is unreasonable, this function will display a warning.
     * The user has the option to continue.
     * @return true if the canvas size is reasonable, or the user presses
     * "continue anyway" on the warning, false if the stitch should be aborted 
     */
    bool CheckGoodSize();

    // the model
    Panorama * pano;
    //current gui level
    GuiLevel m_guiLevel;

    // don't listen to input on gui elements during
    // updating the gui from the model, to prevent recursion,
    // because the gui might report changes as well.
    bool updatesDisabled;
    PanoramaOptions m_oldOpt;
    double m_oldVFOV;

    bool m_keepViewOnResize;
    bool m_hasStacks;

    // controls of this frame
    wxChoice    * m_ProjectionChoice;
    wxTextCtrl  * m_HFOVText;
    wxTextCtrl  * m_VFOVText;

    wxTextCtrl  * m_WidthTxt;
    wxTextCtrl  * m_HeightTxt;
    wxTextCtrl  * m_ROILeftTxt;
    wxTextCtrl  * m_ROIRightTxt;
    wxTextCtrl  * m_ROITopTxt;
    wxTextCtrl  * m_ROIBottomTxt;
    wxChoice    * m_RemapperChoice;
    wxChoice    * m_FusionChoice;
    wxChoice    * m_HDRMergeChoice;
    wxChoice    * m_BlenderChoice;
    wxButton    * m_StitchButton;
    wxButton    * m_CalcHFOVButton;
    wxButton    * m_CalcOptWidthButton;
    wxButton    * m_CalcOptROIButton;

    wxChoice    * m_FileFormatChoice;
    wxStaticText* m_FileFormatOptionsLabel;
    wxTextCtrl  * m_FileFormatJPEGQualityText;
    wxChoice    * m_FileFormatTIFFCompChoice;

    wxChoice    * m_HDRFileFormatChoice;
    wxStaticText* m_HDRFileFormatLabelTIFFCompression;
    wxChoice    * m_FileFormatHDRTIFFCompChoice;

    wxScrolledWindow *m_pano_ctrls;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(PanoPanel)
};

/** xrc handler */
class PanoPanelXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(PanoPanelXmlHandler)

public:
    PanoPanelXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // _PANOPANEL_H
