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
#define USE_TOGGLE_BUTTON 1

#include "panoinc_WX.h"

#include "panoinc.h"

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
    : wxFrame(frame,-1, _("panorama preview"),
      wxDefaultPosition, wxDefaultSize),
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


    m_VFOVSlider = new wxSlider(this, -1, 0,
                                1, 180,
                                wxDefaultPosition, wxDefaultSize,
                                wxSL_VERTICAL | wxSL_AUTOTICKS,
                                wxDefaultValidator,
                                "VFOV");
    m_VFOVSlider->SetToolTip(_("drag to change the vertical field of view"));

    flexSizer->Add(m_VFOVSlider, 0, wxEXPAND);

    m_HFOVSlider = new wxSlider(this, -1, 0,
                                1, 360,
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

    wxConfigBase * config = wxConfigBase::Get();
    long showDruid = config->Read("/PreviewFrame/showDruid",1l);
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
    SetStatusText("",0);
    SetStatusText("",1);

    // the initial size as calculated by the sizers
    SetSizer( topsizer );
    topsizer->SetSizeHints( this );

    // set the minimize icon
    SetIcon(wxIcon(MainFrame::Get()->
                   GetXRCPath() + "/data/icon.png", wxBITMAP_TYPE_PNG));

    m_pano.addObserver(this);

    long w = config->Read("/PreviewFrame/width",-1);
    long h = config->Read("/PreviewFrame/height",-1);
    if (w != -1) {
        SetClientSize(w,h);
    }

    long aup = config->Read("/PreviewFrame/autoUpdate",0l);
    m_PreviewPanel->SetAutoUpdate(aup != 0);

    m_ToolBar->ToggleTool(XRCID("preview_auto_update_tool"), aup !=0);

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
    SetStatusText(wxString::Format("%.1f x %.1f, %s", opts.HFOV, opts.VFOV,
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
                                                          wxString::Format("%d",*it));
#else
                wxCheckBox * but = new wxToggleButton(this,
                                                      ID_TOGGLE_BUT + *it,
                                                      wxString::Format("%d",*it));
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
	{ "ERROR", "druid.images.128.png", // "ERROR" must be at index 0
	  _("The druid has no advice at this time."), _("") },

	{ "READY", "druid.stitch.128.png",
	  _("The druid finds no problems with your panorama."),
	  _("Stitch your final image now, and then use an image editor\n"
		"such as the GNU Image Manipulation Program (the GIMP)\n"
		"to add any finishing touches.") },

	{ "NO IMAGES", "druid.images.128.png",
	  _("To get started, add some image files."),
	  _("You can add any number of images using the Images tab.") },

	{ "ONE IMAGE", "druid.images.128.png",
	  _("Add at least one more image."),
	  _("You should have at least two files listed in the Images tab.") },

	{ "LOW HFOV", "druid.lenses.128.png",
	  _("The Horizontal Field of View (HFOV) may be too low."),
	  _("Check that the focal lengths and/or hfov figures\n"
		"for each image are correct for the camera settings.\n"
		"Then calculate the visible field of view again.\n"
		"HFOV is measured in degrees of arc, usually between\n"
		"5 and 120 degrees per image unless using specialized\n"
		"lenses.") },

	{ "HUGE FINAL", "druid.stitch.128.png",
	  _("Warning:  current stitch has huge dimensions."),
	  _("Very large pixel dimensions are currently entered.\n"
		"Some computers may take an excessively long time\n"
		"to render such a large final image.\n"
		"For best results, use the automatic Calc button on\n"
		"the Panorama Options tab to determine the\n"
		"pixel dimensions which will give the best quality.") },

	{ "UNSAVED", "druid.stitch.128.png",
	  _("Warning:  you haven't saved the current project."),
	  _("While everything else seems to be ready to stitch,\n"
		"don't forget to save your project file so you can\n"
		"experiment or adjust the settings later.") },

	{ NULL, NULL, _("") }
};
