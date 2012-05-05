// -*- c-basic-offset: 4 -*-
/** @file OptimizePanel.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _OPTIMIZE_PANEL_H
#define _OPTIMIZE_PANEL_H

#include <PT/Panorama.h>
#include <wx/xrc/xmlres.h>
#include "GuiLevel.h"

class ImagesTreeCtrl;
class wxCheckBox;

/** run the optimizer. this is tied into the wxWindows hugin
 *
 *  it will emit an event that
 */
class OptimizePanel : public wxPanel, public PT::PanoramaObserver
{


public:
    OptimizePanel();

    /** Delayed creation */
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    virtual ~OptimizePanel();

    void Init(PT::Panorama * pano);

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

    bool AskApplyResult(const PT::Panorama & pano);

    ImagesTreeCtrl* m_images_tree_list;
    ImagesTreeCtrl* m_lens_tree_list;

    wxCheckBox * m_only_active_images_cb;
    wxCheckBox * m_edit_cb;

    PT::Panorama * m_pano;
private:

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(OptimizePanel)
};


/** xrc handler */
class OptimizePanelXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(OptimizePanelXmlHandler)

public:
    OptimizePanelXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};


#endif // _WXPANOCOMMAND_H
