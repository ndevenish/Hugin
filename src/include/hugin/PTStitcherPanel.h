// -*- c-basic-offset: 4 -*-
/** @file PTStitcherPanel.h
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

#ifndef _PTSTITCHERPANEL_H
#define _PTSTITCHERPANEL_H

#include "hugin/StitcherPanel.h"
//#include "hugin/MainFrame.h"

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
class PTStitcherPanel: public StitcherPanel, public PT::PanoramaObserver
{
public:
    PTStitcherPanel(wxWindow *parent, PT::Panorama & pano);
    virtual ~PTStitcherPanel(void) ;

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

 private:

    // resize if the notebook page changes size
    void FitParent(wxSizeEvent & e);

    // apply changes from the model
    void UpdateDisplay(const PT::PanoramaOptions & opt);

    // apply changes to the model. (gui values -> Panorama)
    void ColourModeChanged(wxCommandEvent & e);
    void ColourModeChangedSpin(wxSpinEvent & e);
    void GammaChanged(wxCommandEvent & e);
    void InterpolatorChanged(wxCommandEvent & e);
    void OnSetQuality(wxSpinEvent & e);
    void OnFeatherWidthChanged(wxSpinEvent & e);
    void OnSpeedupChanged(wxCommandEvent & e);

    void FileFormatChanged(wxCommandEvent & e);

    // actions
    virtual void Stitch(const PT::Panorama & pano,
                        const PT::PanoramaOptions & opts);

    // the model
    PT::Panorama &pano;

    // don't listen to input on gui elements during
    // updating the gui from the model, to prevent recursion,
    // because the gui might report changes as well.
    bool updatesDisabled;
    PT::PanoramaOptions m_oldOpt;

    // control of this frame

    wxChoice    * m_InterpolatorChoice;
    wxTextCtrl  * m_GammaText;
    wxChoice    * m_ColorCorrModeChoice;
    wxSpinCtrl  * m_ColorCorrRefSpin;
    wxSpinCtrl  * m_FeatherWidthSpin;

    wxChoice    * m_FormatChoice;
    wxSpinCtrl  * m_JPEGQualitySpin;
    wxChoice    * m_SpeedupChoice;
    wxCheckBox  * m_editScriptCB;

    DECLARE_EVENT_TABLE()
};

#endif // _PTSTITCHERPANEL_H
