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

// use toggle buttons or uncomment check boxes

#ifndef __WXMAC__
#define USE_TOGGLE_BUTTON 1
#endif

#include <config.h>

#include "panoinc_WX.h"

#include "panoinc.h"

#include "hugin/config_defaults.h"
#include "hugin/PreviewFrame.h"
#include "hugin/huginApp.h"
#include "hugin/PreviewPanel.h"
#include "hugin/ImagesPanel.h"
#include "hugin/CommandHistory.h"

using namespace utils;

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
    EVT_CHOICE(-1, PreviewFrame::OnBlendChoice)
#ifdef USE_TOGGLE_BUTTON
    EVT_TOGGLEBUTTON(-1, PreviewFrame::OnChangeDisplayedImgs)
#else
    EVT_CHECKBOX(-1, PreviewFrame::OnChangeDisplayedImgs)
#endif
    EVT_SCROLL_THUMBRELEASE(PreviewFrame::OnChangeFOV)
    EVT_SCROLL_ENDSCROLL(PreviewFrame::OnChangeFOV)
    EVT_SCROLL_THUMBTRACK(PreviewFrame::OnChangeFOV)
END_EVENT_TABLE()

PreviewFrame::PreviewFrame(wxFrame * frame, PT::Panorama &pano)
    : wxFrame(frame,-1, _("Panorama preview")),
      m_pano(pano)
{
	DEBUG_TRACE("");

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


    m_VFOVSlider = new wxSlider(this, -1, 1,
                                1, 180,
                                wxDefaultPosition, wxDefaultSize,
                                wxSL_VERTICAL | wxSL_AUTOTICKS,
                                wxDefaultValidator,
                                _("VFOV"));
    m_VFOVSlider->SetLineSize(2);
    m_VFOVSlider->SetPageSize(10);
    m_VFOVSlider->SetTickFreq(5,0);
    m_VFOVSlider->SetToolTip(_("drag to change the vertical field of view"));

    flexSizer->Add(m_VFOVSlider, 0, wxEXPAND);

    m_HFOVSlider = new wxSlider(this, -1, 1,
                                1, 360,
                                wxDefaultPosition, wxDefaultSize,
                                wxSL_HORIZONTAL | wxSL_AUTOTICKS,
                                wxDefaultValidator,
                                _("HFOV"));
    m_HFOVSlider->SetPageSize(10);
    m_HFOVSlider->SetLineSize(2);
    m_HFOVSlider->SetTickFreq(5,0);

    m_HFOVSlider->SetToolTip(_("drag to change the horizontal field of view"));

    flexSizer->Add(m_HFOVSlider, 0, wxEXPAND);

    topsizer->Add(flexSizer,
                  1,        // vertically stretchable
                  wxEXPAND | // horizontally stretchable
                  wxALL,    // draw border all around
                  5);       // border width

    wxStaticBoxSizer * blendModeSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, -1, _("Preview Options")),
        wxHORIZONTAL);

    blendModeSizer->Add(new wxStaticText(this, -1, _("Blend mode:")),
                        0,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width

    m_choices[0] = _("normal");
    m_choices[1] = _("difference");
    m_choices[2] = _("seams");

    int oldMode = wxConfigBase::Get()->Read(wxT("/PreviewFrame/blendMode"), 0l);
    DEBUG_ASSERT(oldMode >= 0 && oldMode < 3);
    m_BlendModeChoice = new wxChoice(this, -1,
                                     wxDefaultPosition, wxDefaultSize,
                                     3, m_choices);
    m_BlendModeChoice->SetSelection((PreviewPanel::BlendMode) oldMode);

    blendModeSizer->Add(m_BlendModeChoice,
                        0,
                        wxALL | wxALIGN_CENTER_VERTICAL,
                        5);

    topsizer->Add(blendModeSizer, 0, wxEXPAND | wxALL, 5);


    wxConfigBase * config = wxConfigBase::Get();
    long showDruid = config->Read(wxT("/PreviewFrame/showDruid"),HUGIN_PREVIEW_SHOW_DRUID);
    if (showDruid) {
        m_druid = new PanoDruid(this);
        topsizer->Add(m_druid, 0, wxEXPAND | wxALL | wxADJUST_MINSIZE, 5);
        m_druid->Update(m_pano);
    } else {
        m_druid = 0;
    }

    // add a status bar
    CreateStatusBar(2);
    int widths[2] = {-1, 150};
    SetStatusWidths(2, widths);
    SetStatusText(wxT(""),0);
    SetStatusText(wxT(""),1);

    // the initial size as calculated by the sizers
    SetSizer( topsizer );
    topsizer->SetSizeHints( this );

    // set the minimize icon
    SetIcon(wxIcon(MainFrame::Get()->
                   GetXRCPath() + wxT("/data/icon.png"), wxBITMAP_TYPE_PNG));

    m_pano.addObserver(this);

    bool maximized = config->Read(wxT("/PreviewFrame/maximized"), 0l) != 0;
    if (maximized) {
        this->Maximize();
    } else {
        //size
        int w = config->Read(wxT("/PreviewFrame/width"),-1l);
        int h = config->Read(wxT("/PreviewFrame/height"),-1l);
        if (w > 0) {
            SetClientSize(w,h);
        } else {
            Fit();
        }
        //position
        int x = config->Read(wxT("/PreviewFrame/positionX"),-1l);
        int y = config->Read(wxT("/PreviewFrame/positionY"),-1l);
        if ( y > 0) {
            Move(x, y);
        } else {
            Move(0, 44);
        }
    }
    long aup = config->Read(wxT("/PreviewFrame/autoUpdate"),0l);
    m_PreviewPanel->SetAutoUpdate(aup != 0);

    m_ToolBar->ToggleTool(XRCID("preview_auto_update_tool"), aup !=0);

    if (config->Read(wxT("/PreviewFrame/isShown"), 0l) != 0) {
        Show();
    }

}

PreviewFrame::~PreviewFrame()
{
    DEBUG_TRACE("dtor writing config");
    wxConfigBase * config = wxConfigBase::Get();
    wxSize sz = GetClientSize();
    if (! this->IsMaximized() ) {
        wxSize sz = GetClientSize();
        config->Write(wxT("/PreviewFrame/width"), sz.GetWidth());
        config->Write(wxT("/PreviewFrame/height"), sz.GetHeight());
        wxPoint ps = GetPosition();
        config->Write(wxT("/PreviewFrame/positionX"), ps.x);
        config->Write(wxT("/PreviewFrame/positionY"), ps.y);
        config->Write(wxT("/PreviewFrame/maximized"), 0);
    } else {
        config->Write(wxT("/PreviewFrame/maximized"), 1l);
    }

    if ( (!this->IsIconized()) && (! this->IsMaximized()) && this->IsShown()) {
        config->Write(wxT("/PreviewFrame/isShown"), 1l);
    } else {
        config->Write(wxT("/PreviewFrame/isShown"), 0l);
    }

    bool checked = m_ToolBar->GetToolState(XRCID("preview_auto_update_tool"));
    config->Write(wxT("/PreviewFrame/autoUpdate"), checked ? 1l: 0l);
    config->Write(wxT("/PreviewFrame/blendMode"), m_BlendModeChoice->GetSelection());
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
    SetStatusText(wxString::Format(wxT("%.1f x %.1f, %s"), opts.HFOV, opts.VFOV,
                                   projection.c_str()),1);
    m_HFOVSlider->SetValue(roundi(opts.HFOV));
    m_VFOVSlider->SetValue(roundi(opts.VFOV));
    if (m_druid) m_druid->Update(m_pano);
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

    // update existing items

    if ( nrImages >= nrButtons ) {
        for(UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it){
            if (*it >= nrButtons) {
                unsigned int imgNr = *it;
                // create new item.
//                wxImage * bmp = new wxImage(sz.GetWidth(), sz.GetHeight());
#ifdef USE_TOGGLE_BUTTON
                wxToggleButton * but = new wxToggleButton(this,
                                                          ID_TOGGLE_BUT + *it,
                                                          wxString::Format(wxT("%d"),*it));
#else
                wxCheckBox * but = new wxCheckBox(this,
                                                  ID_TOGGLE_BUT + *it,
                                                  wxString::Format(wxT("%d"),*it));
#endif
                wxSize sz = but->GetSize();
//                but->SetSize(res.GetWidth(),sz.GetHeight());
                // HACK.. set fixed width. that should work
                // better than all that stupid dialogunit stuff, that
                // breaks on wxWin 2.5
                but->SetSize(20, sz.GetHeight());
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
        if (m_druid) m_druid->Update(m_pano);
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
    if (e.IsChecked()) {
        m_PreviewPanel->ForceUpdate();
    }
}

#if 0
// need to add the wxChoice somewhere
void PreviewFrame::OnProjectionChanged()
{
    PanoramaOptions opt = m_pano.getOptions();
    int lt = m_ProjectionChoice->GetSelection();
    wxString Ip;
    switch ( lt ) {
    case PanoramaOptions::RECTILINEAR:       Ip = _("Rectilinear"); break;
    case PanoramaOptions::CYLINDRICAL:       Ip = _("Cylindrical"); break;
    case PanoramaOptions::EQUIRECTANGULAR:   Ip = _("Equirectangular"); break;
    }
    opt.projectionFormat = (PanoramaOptions::ProjectionFormat) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Projection changed: "  << lt << ":" << Ip )


}
#endif

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
    // fit pano afterwards
    OnFitPano(e);
}

void PreviewFrame::OnUpdateButton(wxCommandEvent& event)
{
    m_PreviewPanel->ForceUpdate();
    if (m_druid) m_druid->Update(m_pano);
}

void PreviewFrame::OnFitPano(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    PanoramaOptions opt = m_pano.getOptions();

    FDiff2D fov = m_pano.calcFOV();
    opt.HFOV = roundi(fov.x);
    opt.VFOV = roundi(fov.y);

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );

    DEBUG_INFO ( "new fov: [" << opt.HFOV << " "<< opt.VFOV << "] => height: " << opt.getHeight() );
    m_PreviewPanel->ForceUpdate();
    if (m_druid) m_druid->Update(m_pano);
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

void PreviewFrame::OnChangeFOV(wxScrollEvent & e)
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

void PreviewFrame::OnBlendChoice(wxCommandEvent & e)
{
    if (e.GetEventObject() == m_BlendModeChoice) {
        int sel = e.GetSelection();
        switch (sel) {
        case 0:
            m_PreviewPanel->SetBlendMode(PreviewPanel::BLEND_COPY);
            break;
        case 1:
            m_PreviewPanel->SetBlendMode(PreviewPanel::BLEND_DIFFERENCE);
            break;
        case 2:
            m_PreviewPanel->SetBlendMode(PreviewPanel::BLEND_SEAMING);
            break;
        default:
            DEBUG_WARN("Unknown blend mode selected");
        }
    } else {
        DEBUG_WARN("wxChoice event from unknown object received");
    }

}

//////////////////////////////////////////////////////////////////////////////

// The Panorama Druid is a set of tiered heuristics and advice on how to
// improve the current panorama.

struct advocation
{
	const wxChar* name;
	const wxChar* graphic;
	const wxChar* brief;
	const wxChar* text;
};

static struct advocation _advice[] =
{
	{ wxT("ERROR"), wxT("druid.images.128.png"), // "ERROR" must be at index 0
	  _("The druid has no advice at this time."), wxT("") },

	{ wxT("READY"), wxT("druid.stitch.128.png"),
	  _("The druid finds no problems with your panorama."),
	  _("Stitch your final image now, and then use an image editor\nsuch as the GNU Image Manipulation Program (the GIMP)\nto add any finishing touches.") },

	{ wxT("NO IMAGES"), wxT("druid.images.128.png"),
	  _("To get started, add some image files."),
	  _("You can add any number of images using the Images tab.") },

	{ wxT("ONE IMAGE"), wxT("druid.images.128.png"),
	  _("Add at least one more image."),
	  _("You should have at least two files listed in the Images tab.") },

	{ wxT("LOW HFOV"), wxT("druid.lenses.128.png"),
	  _("The Horizontal Field of View (HFOV) may be too low."),
	  _("Check that the focal lengths and/or hfov figures\nfor each image are correct for the camera settings.\nThen calculate the visible field of view again.\nHFOV is measured in degrees of arc, usually between\n5 and 120 degrees per image unless using specialized\nlenses.") },

	{ wxT("HUGE FINAL"), wxT("druid.stitch.128.png"),
	  _("Warning:  current stitch has huge dimensions."),
	  _("Very large pixel dimensions are currently entered.\nSome computers may take an excessively long time\nto render such a large final image.\nFor best results, use the automatic Calc button on\nthe Panorama Options tab to determine the\npixel dimensions which will give the best quality.") },

	{ wxT("UNSAVED"), wxT("druid.stitch.128.png"),
	  _("Warning:  you haven't saved the current project."),
	  _("While everything else seems to be ready to stitch,\ndon't forget to save your project file so you can\nexperiment or adjust the settings later.") },

	{ NULL, NULL, wxT("") }
};
