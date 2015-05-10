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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _OPTIMIZE_PANEL_H
#define _OPTIMIZE_PANEL_H

#include <panodata/Panorama.h>
#include <wx/xrc/xmlres.h>
#include "GuiLevel.h"

class ImagesTreeCtrl;
class wxCheckBox;

/** run the optimizer. this is tied into the wxWindows hugin
 *
 *  it will emit an event that
 */
class OptimizePanel : public wxPanel, public HuginBase::PanoramaObserver
{


public:
    OptimizePanel();

    /** Delayed creation */
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    virtual ~OptimizePanel();

    void Init(HuginBase::Panorama * pano);

    /** receives notification about panorama changes */
    virtual void panoramaChanged(HuginBase::Panorama & pano);
    /** receives notification about panorama changes */
    virtual void panoramaImagesChanged(HuginBase::Panorama &pano, const HuginBase::UIntSet & imgNr);

    /** run the optimizer */
    void OnOptimizeButton(wxCommandEvent & e);
    void SetGuiLevel(GuiLevel newGuiLevel);

protected:

    void OnClose(wxCloseEvent& e);
    void OnReset(wxCommandEvent& e);

    void runOptimizer(const HuginBase::UIntSet & img);

    bool AskApplyResult(wxWindow* activeWindow, const HuginBase::Panorama & pano);

    ImagesTreeCtrl* m_images_tree_list;
    ImagesTreeCtrl* m_lens_tree_list;

    wxCheckBox * m_only_active_images_cb;
    wxCheckBox * m_edit_cb;

    HuginBase::Panorama * m_pano;
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
