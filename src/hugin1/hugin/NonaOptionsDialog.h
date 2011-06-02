// -*- c-basic-offset: 4 -*-
/** @file NonaOptionsDialog.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: NonaOptionsDialog.h 2510 2007-10-28 22:24:11Z dangelo $
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

#ifndef _NONASTITCHERPANEL_H
#define _NONASTITCHERPANEL_H

//using namespace PT;
class PanoDialog;
class wxSpinCtrl;
class wxTextCtrl;
class wxChoice;
class wxComboBox;

/** Define the nona stitcher options panel
 *
 *  - This window will contain the stitcher specific options.
 */
class NonaOptionsDialog: public wxDialog, public PT::PanoramaObserver
{
public:
    NonaOptionsDialog(wxWindow *parent, PT::Panorama & pano);
    virtual ~NonaOptionsDialog();

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


 private:

    // apply changes from the model
    void UpdateDisplay(const PT::PanoramaOptions & opt);

    // apply changes to the model. (gui values -> Panorama)
    void InterpolatorChanged(wxCommandEvent & e);
    void OnSaveCropped(wxCommandEvent & e);

    // the model
    PT::Panorama &pano;

    // don't listen to input on gui elements during
    // updating the gui from the model, to prevent recursion,
    // because the gui might report changes as well.
    bool updatesDisabled;
    PT::PanoramaOptions m_oldOpt;

    wxChoice    * m_InterpolatorChoice;

    wxCheckBox  * m_SaveCroppedCB;

    DECLARE_EVENT_TABLE()
};

#endif // _NONASTITCHERPANEL_H
