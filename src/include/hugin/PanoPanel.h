// -*- c-basic-offset: 4 -*-
/** @file PanoPanel.h
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

#ifndef _PANOPANEL_H
#define _PANOPANEL_H


#include "wx/frame.h"
//#include "wx/dnd.h"
//#include "wx/listctrl.h"

#include "PT/Panorama.h"
#include "hugin/MainFrame.h"
//#include "hugin/List.h"

using namespace PT;
class PanoDialog;

/** Define the pano edit panel
 *
 *  - it is for the settings of the final panorama.
 *    Maybe here goes the preview to.
 */
class PanoPanel: public wxPanel, public PT::PanoramaObserver
{
 public:
    PanoPanel( wxWindow *parent, const wxPoint& pos, const wxSize& size,
                 Panorama * pano);
    ~PanoPanel(void) ;

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
//    virtual void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** function to update PanoramaOptions -> gui */
    bool auto_preview;
    bool panoviewer_enabled;
    bool panoviewer_precise;
    bool panoviewer_started;
    int previewWidth;
    /* initialize from gui values */
    void PanoChanged (wxCommandEvent & e);

    /** Start/Stop external dialog */
    void DoDialog (wxCommandEvent & e);
    /** Did we run DoDialog, to bring PanoPanel in dialog mode? */
    bool self_pano_dlg;
    /** Does PanoPanel run in dialog mode? */
    bool pano_dlg_run;
    /** follow the main window */
    void Resize(wxSizeEvent & e);

 private:
    void DoOptimization (wxCommandEvent & e);
    void Optimize (OptimizeVector & optvars, PanoramaOptions & output);

    void GammaChanged(wxCommandEvent & e);
    void HFOVChanged(wxCommandEvent & e);
    void InterpolatorChanged(wxCommandEvent & e);
    void ProjectionChanged(wxCommandEvent & e);

    void DoPreview(wxCommandEvent & e);
    void autoPreview(wxCommandEvent & e);
    void panoviewerEnabled(wxCommandEvent & e);
    void panoviewerPrecise(wxCommandEvent & e);
    void previewWidthChanged(wxCommandEvent & e);

    void FinalFormatChanged(wxCommandEvent & e);
    void WidthChanged(wxCommandEvent & e);
    void HeightChanged(wxCommandEvent & e);
    void JpegQChanged(wxCommandEvent & e);
    void JpegPChanged(wxCommandEvent & e);


    void Stitch(wxCommandEvent & e);

    void PanoOptionsChanged( PanoramaOptions & opt );
    bool changePano;

    // the model
    Panorama &pano;
    PanoramaOptions opt;

    // tearing off pano_panel
    PanoDialog * pano_dlg;

    int Width;
    int Height;

    wxDialog * preview_dlg;

    DECLARE_EVENT_TABLE()
};

/** Define an pano edit panel dialog box
 *
 *  A small helper class to show up an second dialog insted of tearing off.
 */
class PanoDialog: public wxDialog
{
 public:
    PanoDialog( wxWindow *parent, const wxPoint& pos, const wxSize& size,
                 Panorama * pano);
    ~PanoDialog(void) ;
    // event handlers
    void OnPaint (wxPaintEvent & e);

    // The panel containing the controls
    PanoPanel * pp;

 private:
    void DoPaint (wxPaintEvent & e);

    // the model
    Panorama &pano;

    DECLARE_EVENT_TABLE()
};



#endif // _PANOPANEL_H
