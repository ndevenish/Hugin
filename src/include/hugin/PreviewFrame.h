// -*- c-basic-offset: 4 -*-
/** @file PreviewFrame.h
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

#ifndef _PREVIEWFRAME_H
#define _PREVIEWFRAME_H

#include <wx/frame.h>

#include "PT/Panorama.h"

class PreviewPanel;
class wxButton;
class wxToggleButton;

/** The image preview frame
 *
 *  Contains the ImagePreviewPanel and various controls for it.
 *
 *  it is not created with XRC, because it is highly dynamic, buttons
 *  have to be added etc.
 */
class PreviewFrame : public wxFrame, public PT::PanoramaObserver
{
public:

    /** ctor.
     */
    PreviewFrame(wxFrame * frame, PT::Panorama &pano);

    /** dtor.
     */
    virtual ~PreviewFrame();

    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet &changed);

protected:
    void OnClose(wxCloseEvent& e);

    void OnChangeDisplayedImgs(wxCommandEvent & e);
    void OnAutoPreviewToggle(wxCommandEvent & e);
    void OnUpdateButton(wxCommandEvent& event);
    
private:

    PT::Panorama & m_pano;
    
    PreviewPanel * m_PreviewPanel;
    wxButton * m_updatePreview;
    
    wxStaticBoxSizer * m_ToggleButtonSizer;
    
    std::vector<wxToggleButton *> m_ToggleButtons;
    
    PT::UIntSet m_displayedImgs;
    
    DECLARE_EVENT_TABLE()
};



#endif // _PREVIEWFRAME_H
