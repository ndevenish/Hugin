// -*- c-basic-offset: 4 -*-
/** @file OptimizePhotometricPanel.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _OPTIMIZE_PHOTO_PANEL_H
#define _OPTIMIZE_PHOTO_PANEL_H

#include <PT/Panorama.h>
#include <panodata/StandardImageVariableGroups.h>
#include "GuiLevel.h"

class ImagesTreeCtrl;

/** run the optimizer. this is tied into the wxWindows hugin
 *
 *  it will emit an event that
 */
class OptimizePhotometricPanel : public wxPanel, public PT::PanoramaObserver
{


public:
    OptimizePhotometricPanel();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(PT::Panorama * pano);

    virtual ~OptimizePhotometricPanel();

    /** receives notification about panorama changes */
    virtual void panoramaChanged(PT::Panorama & pano);
    /** receives notification about panorama changes */
    virtual void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** run the optimizer */
    void OnOptimizeButton(wxCommandEvent & e);
    void SetGuiLevel(GuiLevel newGuiLevel);

protected:

    void OnClose(wxCloseEvent& e);
    void OnReset(wxCommandEvent& e);

    void runOptimizer(const PT::UIntSet & img);

    wxCheckBox * m_only_active_images_cb;

    ImagesTreeCtrl* m_images_tree;
    ImagesTreeCtrl* m_lens_tree;

    PT::Panorama * m_pano;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(OptimizePhotometricPanel)
};

/** xrc handler */
class OptimizePhotometricPanelXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(OptimizePhotometricPanelXmlHandler)

public:
    OptimizePhotometricPanelXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // _WXPANOCOMMAND_H
