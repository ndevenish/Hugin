// -*- c-basic-offset: 4 -*-

/** @file PreviewFrame.cpp
 *
 *  @brief implementation of PreviewFrame Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/tglbtn.h>
#include <wx/config.h>

#include "hugin/PreviewFrame.h"

#include "hugin/PreviewPanel.h"
#include "hugin/ImagesPanel.h"
#include "common/utils.h"
#include "common/stl_utils.h"


// a random id, hope this doesn't break something..
#define ID_TOGGLE_BUT 12334

BEGIN_EVENT_TABLE(PreviewFrame, wxFrame)
    EVT_CLOSE(PreviewFrame::OnClose)
    EVT_CHECKBOX(-1, PreviewFrame::OnAutoPreviewToggle)
    EVT_BUTTON(-1, PreviewFrame::OnUpdateButton)
    EVT_TOGGLEBUTTON(-1,PreviewFrame::OnChangeDisplayedImgs)
END_EVENT_TABLE()

PreviewFrame::PreviewFrame(wxFrame * frame, PT::Panorama &pano)
    : wxFrame(frame,-1, _("panorama preview"),
              wxDefaultPosition, wxDefaultSize),
    m_pano(pano)
{
//    SetTitle(_("panorama preview"));
    SetAutoLayout(TRUE);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

    m_ToggleButtonSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, -1, _("displayed images")),
        wxHORIZONTAL);

    topsizer->Add(m_ToggleButtonSizer, 0, wxEXPAND | wxALL, 1);

    // create our preview panel
    m_PreviewPanel = new PreviewPanel(this, &pano);

    topsizer->Add(m_PreviewPanel,
                  1,        // not vertically stretchable
                  wxEXPAND | // horizontally stretchable
                  wxALL,    // draw border all around
                  5);       // border width

    wxBoxSizer *ctrlSizer = new wxBoxSizer( wxHORIZONTAL );

    m_autoCB = new wxCheckBox(this, -1, _("auto update"));
    ctrlSizer->Add(m_autoCB,
                  0,        // not horizontally stretchable
                  wxCENTER |
                  wxALL,    // draw border all around
                  5);       // border width

    // create the update button
    wxButton *m_updatePreview = new wxButton(this,-1, _("Update Preview"),wxDefaultPosition);

    ctrlSizer->Add(m_updatePreview,
                  0,        // not horizontally stretchable
                  wxCENTER |
                  wxALL,    // draw border all around
                  5);       // border width

    // don't allow frame to get smaller than what the sizers tell it and also set
    topsizer->Add(ctrlSizer, 0, wxEXPAND, 0);


    // the initial size as calculated by the sizers
    topsizer->SetSizeHints( this );
    SetSizer( topsizer );

    m_pano.addObserver(this);

    wxConfigBase * config = wxConfigBase::Get();
    long w = config->Read("/PreviewFrame/width",-1);
    long h = config->Read("/PreviewFrame/height",-1);
    if (w != -1) {
        SetClientSize(w,h);
    }

    long aup = config->Read("/PreviewFrame/autoUpdate",0l);
    m_PreviewPanel->SetAutoUpdate(aup != 0);
    m_autoCB->SetValue(aup != 0);
}

PreviewFrame::~PreviewFrame()
{
    DEBUG_TRACE("dtor writing config");
    wxSize sz = GetClientSize();
    wxConfigBase * config = wxConfigBase::Get();
    config->Write("/PreviewFrame/width",sz.GetWidth());
    config->Write("/PreviewFrame/height",sz.GetHeight());
    config->Write("/PreviewFrame/autoUpdate", (m_autoCB->IsChecked() ? 1l: 0l));
    m_pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void PreviewFrame::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    bool dirty = false;

    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrButtons = m_ToggleButtons.size();

    // remove items for nonexisting images
    for (int i=nrButtons-1; i>=(int)nrImages; i--)
    {
        m_ToggleButtonSizer->Remove(m_ToggleButtons[i]);
        delete m_ToggleButtons[i];
        m_ToggleButtons.pop_back();
        m_displayedImgs.erase(i);
        dirty = true;
    }

    // update existing items
    if ( nrImages >= nrButtons ) {
        for(UIntSet::iterator it = changed.begin(); it != changed.end(); ++it){
            if (*it >= nrButtons) {
                unsigned int imgNr = *it;
                // create new item.
//                wxImage * bmp = new wxImage(sz.GetWidth(), sz.GetHeight());
                wxToggleButton * but = new wxToggleButton(this,
                                                          ID_TOGGLE_BUT + *it,
                                                          wxString::Format("%d",*it));
                wxSize sz(9,8);
                wxSize res = ConvertDialogToPixels(sz);
                sz = but->GetSize();
                but->SetSize(res.GetWidth(),sz.GetHeight());
                but->SetValue(true);
                m_ToggleButtonSizer->Add(but,
                                         0,
                                         wxLEFT,
                                         5);

                m_ToggleButtons.push_back(but);
                m_displayedImgs.insert(imgNr);
                dirty = true;
            }
        }
    }
    if (dirty) {
        m_ToggleButtonSizer->Layout();
        DEBUG_DEBUG("ndisplayed: " << m_displayedImgs.size());
        UIntSet copy = m_displayedImgs;
        m_PreviewPanel->SetDisplayedImages(copy);
    }
}

void PreviewFrame::OnChangeDisplayedImgs(wxCommandEvent & e)
{
    int id = e.GetId() - ID_TOGGLE_BUT;
    if (id >= 0 && id < (int) m_ToggleButtons.size()) {
        if (e.IsChecked()) {
            m_displayedImgs.insert(id);
        } else {
            m_displayedImgs.erase(id);
        }
        m_PreviewPanel->SetDisplayedImages(m_displayedImgs);
    } else {
        DEBUG_ERROR("invalid Togglebutton ID");
    }
}

void PreviewFrame::OnClose(wxCloseEvent& event)
{
    DEBUG_TRACE("OnClose")
    // do not close, just hide if we're not forced
    if (event.CanVeto()) {
        event.Veto();
        Hide();
        DEBUG_DEBUG("hiding");
    } else {
        DEBUG_DEBUG("closing");
        this->Destroy();
    }
}

void PreviewFrame::OnAutoPreviewToggle(wxCommandEvent & e)
{
    m_PreviewPanel->SetAutoUpdate(e.IsChecked());
}


void PreviewFrame::OnUpdateButton(wxCommandEvent& event)
{
    m_PreviewPanel->ForceUpdate();
}
