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
#include "hugin/huginApp.h"
#include "hugin/PreviewPanel.h"
#include "hugin/ImagesPanel.h"
#include "common/utils.h"
#include "common/stl_utils.h"

#include "hugin/CommandHistory.h"
#include "PT/PanoCommand.h"


// a random id, hope this doesn't break something..
#define ID_UPDATE_BUTTON 12333
#define ID_TOGGLE_BUT 12334

BEGIN_EVENT_TABLE(PreviewFrame, wxFrame)
    EVT_CLOSE(PreviewFrame::OnClose)
//    EVT_CHECKBOX(-1, PreviewFrame::OnAutoPreviewToggle)
    EVT_TOOL(XRCID("preview_center_tool"), PreviewFrame::OnCenterHorizontally)
    EVT_TOOL(XRCID("preview_fit_pano_tool"), PreviewFrame::OnFitPano)
    EVT_TOOL(XRCID("preview_auto_update_tool"), PreviewFrame::OnAutoPreviewToggle)
    EVT_TOOL(XRCID("preview_update_tool"), PreviewFrame::OnUpdateButton)
    EVT_TOOL(XRCID("preview_show_all_tool"), PreviewFrame::OnShowAll)
    EVT_TOOL(XRCID("preview_show_none_tool"), PreviewFrame::OnShowNone)
    EVT_TOGGLEBUTTON(-1, PreviewFrame::OnChangeDisplayedImgs)
    EVT_SCROLL_THUMBRELEASE(PreviewFrame::OnChangeFOV)
    EVT_SCROLL_ENDSCROLL(PreviewFrame::OnChangeFOV)
    EVT_SCROLL_THUMBTRACK(PreviewFrame::OnChangeFOV)
END_EVENT_TABLE()

PreviewFrame::PreviewFrame(wxFrame * frame, PT::Panorama &pano)
    : wxFrame(frame,-1, _("panorama preview"),
              wxDefaultPosition, wxDefaultSize),
    m_pano(pano)
{
//    SetTitle(_("panorama preview"));
    SetAutoLayout(TRUE);

    m_ToolBar = wxXmlResource::Get()->LoadToolBar(this, wxT("preview_toolbar"));
    DEBUG_ASSERT(m_ToolBar);
    // create tool bar
    SetToolBar(m_ToolBar);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

    m_ToggleButtonSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, -1, _("displayed images")),
        wxHORIZONTAL);

    topsizer->Add(m_ToggleButtonSizer, 0, wxEXPAND | wxALL, 1);

    wxFlexGridSizer * flexSizer = new wxFlexGridSizer(2,0,5,5);

    flexSizer->AddGrowableCol(0);
    flexSizer->AddGrowableRow(0);
    
    // create our preview panel
    m_PreviewPanel = new PreviewPanel(this, &pano);

    flexSizer->Add(m_PreviewPanel,
                  1,        // not vertically stretchable
                  wxEXPAND | // horizontally stretchable
                  wxALL,    // draw border all around
                  5);       // border width

    
    m_VFOVSlider = new wxSlider(this, -1, 0, 
                                0, 180,
                                wxDefaultPosition, wxDefaultSize,
                                wxSL_VERTICAL | wxSL_AUTOTICKS,
                                wxDefaultValidator,
                                "VFOV");
    m_VFOVSlider->SetToolTip(_("drag to change the vertical field of view"));
    
    flexSizer->Add(m_VFOVSlider, 0, wxEXPAND);

    m_HFOVSlider = new wxSlider(this, -1, 0, 
                                0, 360,
                                wxDefaultPosition, wxDefaultSize,
                                wxSL_HORIZONTAL | wxSL_AUTOTICKS,
                                wxDefaultValidator,
                                "HFOV");
    m_HFOVSlider->SetToolTip(_("drag to change the horizontal field of view"));
    
    flexSizer->Add(m_HFOVSlider, 0, wxEXPAND);
    

    topsizer->Add(flexSizer,
                  1,        // vertically stretchable
                  wxEXPAND | // horizontally stretchable
                  wxALL,    // draw border all around
                  5);       // border width


    // the initial size as calculated by the sizers
    topsizer->SetSizeHints( this );
    SetSizer( topsizer );

    // set the minimize icon
    SetIcon(wxIcon(MainFrame::Get()->GetXRCPath() + "/data/icon.xpm", wxBITMAP_TYPE_XPM));

    m_pano.addObserver(this);

    wxConfigBase * config = wxConfigBase::Get();
    long w = config->Read("/PreviewFrame/width",-1);
    long h = config->Read("/PreviewFrame/height",-1);
    if (w != -1) {
        SetClientSize(w,h);
    }

    long aup = config->Read("/PreviewFrame/autoUpdate",0l);
    m_PreviewPanel->SetAutoUpdate(aup != 0);

    m_ToolBar->ToggleTool(XRCID("preview_auto_update_tool"), aup !=0);
    
    // add a status bar
    CreateStatusBar(2);
    int widths[2] = {-1, 150};
    SetStatusWidths(2, widths);
}

PreviewFrame::~PreviewFrame()
{
    DEBUG_TRACE("dtor writing config");
    wxSize sz = GetClientSize();
    wxConfigBase * config = wxConfigBase::Get();
    config->Write("/PreviewFrame/width",sz.GetWidth());
    config->Write("/PreviewFrame/height",sz.GetHeight());
    bool checked = m_ToolBar->GetToolState(XRCID("preview_auto_update_tool"));
    config->Write("/PreviewFrame/autoUpdate", checked ? 1l: 0l);
    m_pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
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


void PreviewFrame::panoramaChanged(Panorama &pano) 
{
    m_PreviewPanel->panoramaChanged(pano);
    const PanoramaOptions & opts = pano.getOptions();
    
    wxString projection;
    switch (opts.projectionFormat) {
    case PanoramaOptions::RECTILINEAR:
        projection = _("rectilinear");
        break;
    case PanoramaOptions::CYLINDRICAL:
        projection = _("cylindrical");
        break;
    case PanoramaOptions::EQUIRECTANGULAR:
        projection = _("equirectangular");
        break;
    }
    SetStatusText(wxString::Format("%.1f° x %.1f°, %s", opts.HFOV, opts.VFOV,
                                   projection.c_str()),1);
    m_HFOVSlider->SetValue((int) round(opts.HFOV));
    m_VFOVSlider->SetValue((int) round(opts.VFOV));
    
}

void PreviewFrame::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    m_PreviewPanel->panoramaImagesChanged(pano,changed);

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

    // update existing itemsvoid PreviewFrame::OnFitPano(wxCommandEvent & e)

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

void PreviewFrame::OnCenterHorizontally(wxCommandEvent & e)
{
    VariableMapVector vars = m_pano.getVariables();
    VariableMapVector::iterator it;
    double min = 1000;
    double max = -1000;
    for(it = vars.begin(); it != vars.end(); it++) {
        double val = map_get(*it,"y").getValue();
        if (val < min) min = val;
        if (val > max) max = val;
    }
    double shift = min + (max-min)/2;
    for(it = vars.begin(); it != vars.end(); it++) {
        map_get(*it, "y").setValue( map_get(*it, "y").getValue() - shift);
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::UpdateVariablesCmd(m_pano, vars)
        );
    m_PreviewPanel->ForceUpdate();
}

void PreviewFrame::OnUpdateButton(wxCommandEvent& event)
{
    m_PreviewPanel->ForceUpdate();
}

void PreviewFrame::OnFitPano(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    PanoramaOptions opt = m_pano.getOptions();

    opt.HFOV = (int) m_pano.calcHFOV() + 1;
    opt.VFOV = (int) m_pano.calcVFOV() + 1;

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );

    DEBUG_INFO ( "new fov: [" << opt.HFOV << " "<< opt.VFOV << "] => height: " << opt.getHeight() );
    m_PreviewPanel->ForceUpdate();
}

void PreviewFrame::OnShowAll(wxCommandEvent & e)
{
    DEBUG_ASSERT(m_pano.getNrOfImages() == m_ToggleButtons.size());
    for (unsigned int i=0; i < m_pano.getNrOfImages(); i++) {
        m_displayedImgs.insert(i);
        m_ToggleButtons[i]->SetValue(true);
    }
    m_PreviewPanel->SetDisplayedImages(m_displayedImgs);
}

void PreviewFrame::OnShowNone(wxCommandEvent & e)
{
    DEBUG_ASSERT(m_pano.getNrOfImages() == m_ToggleButtons.size());
    for (unsigned int i=0; i < m_pano.getNrOfImages(); i++) {
        m_ToggleButtons[i]->SetValue(false);
    }
    m_displayedImgs.clear();
    m_PreviewPanel->SetDisplayedImages(m_displayedImgs);
}

void PreviewFrame::OnChangeFOV(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    
    PanoramaOptions opt = m_pano.getOptions();

    if (e.GetEventObject() == m_HFOVSlider) {
        DEBUG_DEBUG("HFOV changed (slider): " << e.GetInt() << " == " << m_HFOVSlider->GetValue());
        opt.HFOV = e.GetInt();
    } else if (e.GetEventObject() == m_VFOVSlider) {
        DEBUG_DEBUG("VFOV changed (slider): " << e.GetInt());
        opt.VFOV = e.GetInt();
    } else {
        DEBUG_FATAL("Slider event from unknown control received");
    }
    
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );
}
