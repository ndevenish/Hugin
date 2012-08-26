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

//#ifndef __WXMAC__
#define USE_TOGGLE_BUTTON 1
//#endif
//wxMac now has toggle buttons.

#include <config.h>

#include "panoinc_WX.h"

#include "panoinc.h"

#include "base_wx/platform.h"
#include "hugin/config_defaults.h"
#include "hugin/PreviewFrame.h"
#include "hugin/huginApp.h"
#include "hugin/PreviewPanel.h"
#include "hugin/ImagesPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/TextKillFocusHandler.h"

#include <vigra_ext/ImageTransforms.h>

extern "C" {
#include <pano13/queryfeature.h>
}

using namespace hugin_utils;

// a random id, hope this doesn't break something..
enum {
    ID_PROJECTION_CHOICE = wxID_HIGHEST +1,
    ID_BLEND_CHOICE,
    ID_UPDATE_BUTTON,
    ID_OUTPUTMODE_CHOICE,
    ID_EXPOSURE_TEXT,
    ID_EXPOSURE_SPIN,
    ID_EXPOSURE_DEFAULT,
    ID_TOGGLE_BUT = wxID_HIGHEST+100,
    PROJ_PARAM_NAMES_ID = wxID_HIGHEST+1000,
    PROJ_PARAM_VAL_ID = wxID_HIGHEST+1100,
    PROJ_PARAM_SLIDER_ID = wxID_HIGHEST+1200,
    PROJ_PARAM_RESET_ID = wxID_HIGHEST+1250,
    ID_FULL_SCREEN = wxID_HIGHEST+1700,
    ID_UNDO = wxID_HIGHEST+1701,
    ID_REDO = wxID_HIGHEST+1702
};

BEGIN_EVENT_TABLE(PreviewFrame, wxFrame)
    EVT_CLOSE(PreviewFrame::OnClose)
//    EVT_CHECKBOX(-1, PreviewFrame::OnAutoPreviewToggle)
    EVT_TOOL(XRCID("preview_center_tool"), PreviewFrame::OnCenterHorizontally)
    EVT_TOOL(XRCID("preview_fit_pano_tool"), PreviewFrame::OnFitPano)
    EVT_TOOL(XRCID("preview_straighten_pano_tool"), PreviewFrame::OnStraighten)
    EVT_TOOL(XRCID("preview_auto_update_tool"), PreviewFrame::OnAutoPreviewToggle)
    EVT_TOOL(XRCID("preview_update_tool"), PreviewFrame::OnUpdate)
    EVT_TOOL(XRCID("preview_show_all_tool"), PreviewFrame::OnShowAll)
    EVT_TOOL(XRCID("preview_show_none_tool"), PreviewFrame::OnShowNone)
    EVT_TOOL(XRCID("preview_num_transform"), PreviewFrame::OnNumTransform)
    EVT_TEXT_ENTER( -1 , PreviewFrame::OnTextCtrlChanged)

    EVT_BUTTON(ID_EXPOSURE_DEFAULT, PreviewFrame::OnDefaultExposure)
    EVT_SPIN_DOWN(ID_EXPOSURE_SPIN, PreviewFrame::OnDecreaseExposure)
    EVT_SPIN_UP(ID_EXPOSURE_SPIN, PreviewFrame::OnIncreaseExposure)
    EVT_CHOICE(ID_BLEND_CHOICE, PreviewFrame::OnBlendChoice)
    EVT_CHOICE(ID_PROJECTION_CHOICE, PreviewFrame::OnProjectionChoice)
    EVT_CHOICE(ID_OUTPUTMODE_CHOICE, PreviewFrame::OnOutputChoice)
#ifdef USE_TOGGLE_BUTTON
    EVT_TOGGLEBUTTON(-1, PreviewFrame::OnChangeDisplayedImgs)
#else
    EVT_CHECKBOX(-1, PreviewFrame::OnChangeDisplayedImgs)
#endif
#ifndef __WXMAC__
	EVT_SCROLL_CHANGED(PreviewFrame::OnChangeFOV)
#else
 #if wxCHECK_VERSION(2,9,0)
	EVT_SCROLL_CHANGED(PreviewFrame::OnChangeFOV)
 #else
	EVT_SCROLL_THUMBRELEASE(PreviewFrame::OnChangeFOV)
	EVT_SCROLL_ENDSCROLL(PreviewFrame::OnChangeFOV)
	EVT_SCROLL_THUMBTRACK(PreviewFrame::OnChangeFOV)
 #endif
#endif
	EVT_TOOL(ID_FULL_SCREEN, PreviewFrame::OnFullScreen)
    EVT_TOOL(ID_UNDO, PreviewFrame::OnUndo)
    EVT_TOOL(ID_REDO, PreviewFrame::OnRedo)
    EVT_BUTTON(PROJ_PARAM_RESET_ID, PreviewFrame::OnProjParameterReset)
END_EVENT_TABLE()

#define PF_STYLE (wxMAXIMIZE_BOX | wxMINIMIZE_BOX | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN)

PreviewFrame::PreviewFrame(wxFrame * frame, PT::Panorama &pano)
    : wxFrame(frame,-1, _("Panorama preview"), wxDefaultPosition, wxDefaultSize,
              PF_STYLE),
      m_pano(pano)
{
	DEBUG_TRACE("");

    m_oldProjFormat = -1;
    m_ToolBar = wxXmlResource::Get()->LoadToolBar(this, wxT("preview_toolbar"));
    DEBUG_ASSERT(m_ToolBar);
    // create tool bar
    SetToolBar(m_ToolBar);

    m_topsizer = new wxBoxSizer( wxVERTICAL );

    m_ToggleButtonSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, -1, _("displayed images")),
    wxHORIZONTAL );

	m_ButtonPanel = new wxScrolledWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	// Set min height big enough to display scrollbars as well
    m_ButtonPanel->SetSizeHints(20, 42);
	//Horizontal scroll bars only
	m_ButtonPanel->SetScrollRate(10, 0);
    m_ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_ButtonPanel->SetAutoLayout(true);
	m_ButtonPanel->SetSizer(m_ButtonSizer);
						
	m_ToggleButtonSizer->Add(m_ButtonPanel, 1, wxEXPAND | wxADJUST_MINSIZE, 0);

    m_topsizer->Add(m_ToggleButtonSizer, 0, wxEXPAND | wxADJUST_MINSIZE | wxALL, 5);

    wxFlexGridSizer * flexSizer = new wxFlexGridSizer(2,0,5,5);
    flexSizer->AddGrowableCol(0);
    flexSizer->AddGrowableRow(0);

    // create our preview panel
    m_PreviewPanel = new PreviewPanel();
    m_PreviewPanel->Create(this);
    m_PreviewPanel->Init(this, &pano);

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

    m_topsizer->Add(flexSizer,
                  1,        // vertically stretchable
                  wxEXPAND | // horizontally stretchable
                  wxALL,    // draw border all around
                  5);       // border width

    wxStaticBoxSizer * blendModeSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, -1, _("Preview Options")),
        wxHORIZONTAL);

    blendModeSizer->Add(new wxStaticText(this, -1, _("projection (f):")),
                        0,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width
    m_ProjectionChoice = new wxChoice(this, ID_PROJECTION_CHOICE,
                                      wxDefaultPosition, wxDefaultSize);

    /* populate with all available projection types */
    int nP = panoProjectionFormatCount();
    for(int n=0; n < nP; n++) {
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(n, &proj)) {
            wxString str2(proj.name, wxConvLocal);
            m_ProjectionChoice->Append(wxGetTranslation(str2));
        }
    }
    m_ProjectionChoice->SetSelection(2);

    blendModeSizer->Add(m_ProjectionChoice,
                        0,
                        wxALL | wxALIGN_CENTER_VERTICAL,
                        5);

    //////////////////////////////////////////////////////
    // Blend mode
    blendModeSizer->Add(new wxStaticText(this, -1, _("Blend mode:")),
                        0,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width

    m_choices[0] = _("normal");
    m_choices[1] = _("difference");

    int oldMode = wxConfigBase::Get()->Read(wxT("/PreviewFrame/blendMode"), 0l);
    if (oldMode > 1) oldMode = 0;
    m_BlendModeChoice = new wxChoice(this, ID_BLEND_CHOICE,
                                     wxDefaultPosition, wxDefaultSize,
                                     2, m_choices);
    m_BlendModeChoice->SetSelection((PreviewPanel::BlendMode) oldMode);

    blendModeSizer->Add(m_BlendModeChoice,
                        0,
                        wxALL | wxALIGN_CENTER_VERTICAL,
                        5);

    //////////////////////////////////////////////////////
    // LDR, HDR
    blendModeSizer->Add(new wxStaticText(this, -1, _("Output:")),
                        0,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width

    m_choices[0] = _("LDR");
    m_choices[1] = _("HDR");
    m_outputModeChoice = new wxChoice(this, ID_OUTPUTMODE_CHOICE,
                                      wxDefaultPosition, wxDefaultSize,
                                      2, m_choices);
    m_outputModeChoice->SetSelection(0);
    blendModeSizer->Add(m_outputModeChoice,
                        0,
                        wxALL | wxALIGN_CENTER_VERTICAL,
                        5);

    /////////////////////////////////////////////////////
    // exposure
    blendModeSizer->Add(new wxStaticText(this, -1, _("EV:")),
                          0,        // not vertically strechable
                          wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                          5);       // border width
    
    m_defaultExposureBut = new wxBitmapButton(this, ID_EXPOSURE_DEFAULT,
                                              wxArtProvider::GetBitmap(wxART_REDO));
    blendModeSizer->Add(m_defaultExposureBut, 0, wxLEFT | wxRIGHT, 5);

//    m_decExposureBut = new wxBitmapButton(this, ID_EXPOSURE_DECREASE,
//                                          wxArtProvider::GetBitmap(wxART_GO_BACK));
//    blendModeSizer->Add(m_decExposureBut);

    m_exposureTextCtrl = new wxTextCtrl(this, ID_EXPOSURE_TEXT, wxT("0"),
                                        wxDefaultPosition,wxSize(50,-1), wxTE_PROCESS_ENTER);
    blendModeSizer->Add(m_exposureTextCtrl,
                          0,        // not vertically strechable
                          wxLEFT | wxTOP | wxBOTTOM  | wxALIGN_CENTER_VERTICAL, // draw border all around
                          5);       // border width
//    m_incExposureBut = new wxBitmapButton(this, ID_EXPOSURE_INCREASE,
//                                          wxArtProvider::GetBitmap(wxART_GO_FORWARD));
    m_exposureSpinBut = new wxSpinButton(this, ID_EXPOSURE_SPIN, wxDefaultPosition,
                                         wxDefaultSize, wxSP_VERTICAL);
    m_exposureSpinBut->SetRange(-0x8000, 0x7fff);
    m_exposureSpinBut->SetValue(0);
    blendModeSizer->Add(m_exposureSpinBut, 0, wxALIGN_CENTER_VERTICAL);

    m_topsizer->Add(blendModeSizer, 0, wxEXPAND | wxALL, 5);

    m_projParamSizer = new wxStaticBoxSizer(
    new wxStaticBox(this, -1, _("Projection Parameters")),
     wxHORIZONTAL);

    wxBitmapButton * resetProjButton=new wxBitmapButton(this, PROJ_PARAM_RESET_ID, 
        wxArtProvider::GetBitmap(wxART_REDO));
    resetProjButton->SetToolTip(_("Resets the projection's parameters to their default values."));
    m_projParamSizer->Add(resetProjButton, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

    m_projParamNamesLabel.resize(PANO_PROJECTION_MAX_PARMS);
    m_projParamTextCtrl.resize(PANO_PROJECTION_MAX_PARMS);
    m_projParamSlider.resize(PANO_PROJECTION_MAX_PARMS);

    for (int i=0; i < PANO_PROJECTION_MAX_PARMS; i++) {

        m_projParamNamesLabel[i] = new wxStaticText(this, PROJ_PARAM_NAMES_ID+i, _("param:"));
        m_projParamSizer->Add(m_projParamNamesLabel[i],
                        0,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width
        m_projParamTextCtrl[i] = new wxTextCtrl(this, PROJ_PARAM_VAL_ID+i, wxT("0"),
                                    wxDefaultPosition, wxSize(35,-1), wxTE_PROCESS_ENTER);
        m_projParamTextCtrl[i]->PushEventHandler(new TextKillFocusHandler(this));
        m_projParamSizer->Add(m_projParamTextCtrl[i],
                        0,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width

        m_projParamSlider[i] = new wxSlider(this, PROJ_PARAM_SLIDER_ID+i, 0, -90, 90);
        m_projParamSizer->Add(m_projParamSlider[i],
                        1,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL | wxEXPAND, // draw border all around
                        5);       // border width
    }

    m_topsizer->Add(m_projParamSizer, 0, wxEXPAND | wxALL, 5);

    // do not show projection param sizer
    m_topsizer->Show(m_projParamSizer, false, true);

    wxConfigBase * config = wxConfigBase::Get();

    // add a status bar
    CreateStatusBar(3);
    int widths[3] = {-3, 150, 150};
    SetStatusWidths(3, widths);
    SetStatusText(_("Left click to define new center point, right click to move point to horizon."),0);
    SetStatusText(wxT(""),1);
    SetStatusText(wxT(""),2);

    // the initial size as calculated by the sizers
    this->SetSizer( m_topsizer );
    m_topsizer->SetSizeHints( this );

    // set the minimize icon
#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    m_pano.addObserver(this);

    RestoreFramePosition(this, wxT("PreviewFrame"));
    
    m_PreviewPanel->SetBlendMode((PreviewPanel::BlendMode)oldMode );

    long aup = config->Read(wxT("/PreviewFrame/autoUpdate"),0l);
    m_PreviewPanel->SetAutoUpdate(aup != 0);

    m_ToolBar->ToggleTool(XRCID("preview_auto_update_tool"), aup !=0);

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows..
    this->SetBackgroundColour(m_PreviewPanel->GetBackgroundColour());
#endif

    if (config->Read(wxT("/PreviewFrame/isShown"), 0l) != 0) {
        Show();
    }
    SetStatusText(_("Center panorama with left mouse button, set horizon with right button"),0);
    wxAcceleratorEntry entries[3];
#ifdef __WXMAC
    entries[0].Set(wxACCEL_CMD,(int)'F',ID_FULL_SCREEN);
#else
    entries[0].Set(wxACCEL_NORMAL,WXK_F11,ID_FULL_SCREEN);
#endif
    entries[1].Set(wxACCEL_CMD,(int)'Z',ID_UNDO);
    entries[2].Set(wxACCEL_CMD,(int)'R',ID_REDO);
    wxAcceleratorTable accel(3, entries);
    SetAcceleratorTable(accel);
#ifdef __WXGTK__
    // set explicit focus to button panel, otherwise the hotkey F11 is not right processed
    m_ButtonPanel->SetFocus();
#endif
}

PreviewFrame::~PreviewFrame()
{
    DEBUG_TRACE("dtor writing config");
    wxConfigBase * config = wxConfigBase::Get();
    wxSize sz = GetClientSize();

    StoreFramePosition(this, wxT("PreviewFrame"));

    if ( (!this->IsIconized()) && (! this->IsMaximized()) && this->IsShown()) {
        config->Write(wxT("/PreviewFrame/isShown"), 1l);
    } else {
        config->Write(wxT("/PreviewFrame/isShown"), 0l);
    }

    bool checked = m_ToolBar->GetToolState(XRCID("preview_auto_update_tool"));
    config->Write(wxT("/PreviewFrame/autoUpdate"), checked ? 1l: 0l);
    config->Write(wxT("/PreviewFrame/blendMode"), m_BlendModeChoice->GetSelection());
    for (int i=0; i < PANO_PROJECTION_MAX_PARMS; i++)
    {
        m_projParamTextCtrl[i]->PopEventHandler(true);
    };
    m_pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void PreviewFrame::OnChangeDisplayedImgs(wxCommandEvent & e)
{
    int id = e.GetId() - ID_TOGGLE_BUT;
    int nImg = m_pano.getNrOfImages();
    UIntSet activeImages = m_pano.getActiveImages();
    DEBUG_DEBUG("toggle_button_id: " << id << " nImg:" << nImg << "  m_ToggleButtons.size(): " << m_ToggleButtons.size());
    if (id >= 0 && id < nImg) {
        if (e.IsChecked()) {
            activeImages.insert(id);
        } else {
            activeImages.erase(id);
        }
//        m_PreviewPanel->SetDisplayedImages(m_displayedImgs);
    	GlobalCmdHist::getInstance().addCommand(
            new PT::SetActiveImagesCmd(m_pano, activeImages)
        );
    } else {
        DEBUG_ERROR("invalid Togglebutton ID");
    }
}

void PreviewFrame::panoramaChanged(Panorama &pano)
{
    const PanoramaOptions & opts = pano.getOptions();

    wxString projection;
    m_ProjectionChoice->SetSelection(opts.getProjection());
    m_VFOVSlider->Enable( opts.fovCalcSupported(opts.getProjection()) );

    m_outputModeChoice->SetSelection(opts.outputMode);
    if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
        /*
        m_exposureTextCtrl->Hide();
        m_defaultExposureBut->Hide();
        m_decExposureBut->Hide();
        m_incExposureBut->Hide();
        */
    } else {
        /*
        m_exposureTextCtrl->Show();
        m_defaultExposureBut->Show();
        m_decExposureBut->Show();
        m_incExposureBut->Show();
        */
    }
    m_exposureTextCtrl->SetValue(wxString(doubleToString(opts.outputExposureValue,2).c_str(), wxConvLocal));

    bool activeImgs = pano.getActiveImages().size() > 0;
    m_ToolBar->EnableTool(XRCID("preview_center_tool"), activeImgs);
    m_ToolBar->EnableTool(XRCID("preview_fit_pano_tool"), activeImgs);
    m_ToolBar->EnableTool(XRCID("preview_update_tool"), activeImgs);
    m_ToolBar->EnableTool(XRCID("preview_num_transform"), activeImgs);
    m_ToolBar->EnableTool(XRCID("preview_straighten_pano_tool"), pano.getNrOfImages() > 0);

    // TODO: enable display of parameters and set their limits, if projection has some.
    int nParam = opts.m_projFeatures.numberOfParameters;
    bool relayout = false;
    // if the projection format has changed
    if (opts.getProjection() != m_oldProjFormat) {
        DEBUG_DEBUG("Projection format changed");
        if (nParam) {
            // show parameters and update labels.
            m_topsizer->Show(m_projParamSizer, true, true);
            int i;
            for (i=0; i < nParam; i++) {
                const pano_projection_parameter * pp = & (opts.m_projFeatures.parm[i]);
                wxString str2(pp->name, wxConvLocal);
                str2 = wxGetTranslation(str2);
                m_projParamNamesLabel[i]->SetLabel(str2);
                m_projParamSlider[i]->SetRange(roundi(pp->minValue), roundi(pp->maxValue));
            }
            for(;i < PANO_PROJECTION_MAX_PARMS; i++) {
                m_projParamNamesLabel[i]->Hide();
                m_projParamSlider[i]->Hide();
                m_projParamTextCtrl[i]->Hide();
            }
            relayout = true;
        } else {
            m_topsizer->Show(m_projParamSizer, false, true);
            relayout = true;
        }
    }
    if (nParam) {
        // display new values
        std::vector<double> params = opts.getProjectionParameters();
        assert((int) params.size() == nParam);
        for (int i=0; i < nParam; i++) {
            wxString val = wxString(doubleToString(params[i],1).c_str(), wxConvLocal);
            m_projParamTextCtrl[i]->SetValue(wxString(val.wc_str(), wxConvLocal));
            m_projParamSlider[i]->SetValue(roundi(params[i]));
        }
    }
    if (relayout) {
        m_topsizer->Layout();
    }
    SetStatusText(_("Center panorama with left mouse button, set horizon with right button"),0);
    SetStatusText(wxString::Format(wxT("%.1f x %.1f"), opts.getHFOV(), opts.getVFOV()),2);
    m_HFOVSlider->SetValue(roundi(opts.getHFOV()));
    m_VFOVSlider->SetValue(roundi(opts.getVFOV()));

    m_oldProjFormat = opts.getProjection();
    updatePano();
}

void PreviewFrame::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    bool dirty = false;

    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrButtons = m_ToggleButtons.size();

//    m_displayedImgs.clear();

    // remove items for nonexisting images
    for (int i=nrButtons-1; i>=(int)nrImages; i--)
    {
        m_ButtonSizer->Detach(m_ToggleButtons[i]);
        delete m_ToggleButtons[i];
        m_ToggleButtons.pop_back();
        dirty = true;
    }

    // add buttons
    if ( nrImages >= nrButtons ) {
        for(UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it){
            if (*it >= nrButtons) {
                unsigned int imgNr = *it;
                // create new item.
//                wxImage * bmp = new wxImage(sz.GetWidth(), sz.GetHeight());
#ifdef USE_TOGGLE_BUTTON
                wxToggleButton * but = new wxToggleButton(m_ButtonPanel,
                                                          ID_TOGGLE_BUT + *it,
                                                          wxString::Format(wxT("%d"),*it),
                                                          wxDefaultPosition, wxDefaultSize,
                                                          wxBU_EXACTFIT);
#else
                wxCheckBox * but = new wxCheckBox(m_ButtonPanel,
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
                m_ButtonSizer->Add(but,
                                   0,
                                   wxLEFT | wxTOP | wxADJUST_MINSIZE,
                                   5);
                m_ToggleButtons.push_back(but);
                dirty = true;
            }
        }
    }

    // update existing items
    UIntSet displayedImages = m_pano.getActiveImages();
    for (unsigned i=0; i < nrImages; i++) {
        m_ToggleButtons[i]->SetValue(set_contains(displayedImages, i));
        wxFileName tFilename(wxString (pano.getImage(i).getFilename().c_str(), HUGIN_CONV_FILENAME));
        m_ToggleButtons[i]->SetToolTip(tFilename.GetFullName());
    }

    if (dirty) {
		m_ButtonSizer->SetVirtualSizeHints(m_ButtonPanel);
		// Layout();
		DEBUG_INFO("New m_ButtonPanel width: " << (m_ButtonPanel->GetSize()).GetWidth());
		DEBUG_INFO("New m_ButtonPanel Height: " << (m_ButtonPanel->GetSize()).GetHeight());
    }
    updatePano();
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
    if (m_pano.getActiveImages().size() == 0) return;

    GlobalCmdHist::getInstance().addCommand(
        new PT::CenterPanoCmd(m_pano)
        );
}

void PreviewFrame::OnStraighten(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    GlobalCmdHist::getInstance().addCommand(
        new PT::StraightenPanoCmd(m_pano)
        );
}

void PreviewFrame::OnUpdate(wxCommandEvent& event)
{
    m_PreviewPanel->ForceUpdate();
}

void PreviewFrame::updatePano()
{
    if (m_ToolBar->GetToolState(XRCID("preview_auto_update_tool"))) {
        m_PreviewPanel->ForceUpdate();
    }
}

void PreviewFrame::OnFitPano(wxCommandEvent & e)
{
    if (m_pano.getActiveImages().size() == 0) return;

    DEBUG_TRACE("");
    PanoramaOptions opt = m_pano.getOptions();

    double hfov, height;
    m_pano.fitPano(hfov, height);
    opt.setHFOV(hfov);
    opt.setHeight(roundi(height));

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );

    DEBUG_INFO ( "new fov: [" << opt.getHFOV() << " "<< opt.getVFOV() << "] => height: " << opt.getHeight() );
    updatePano();
}

void PreviewFrame::OnShowAll(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    DEBUG_ASSERT(m_pano.getNrOfImages() == m_ToggleButtons.size());
    UIntSet displayedImgs;
    for (unsigned int i=0; i < m_pano.getNrOfImages(); i++) {
        displayedImgs.insert(i);
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetActiveImagesCmd(m_pano, displayedImgs)
        );
    updatePano();
}

void PreviewFrame::OnShowNone(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    DEBUG_ASSERT(m_pano.getNrOfImages() == m_ToggleButtons.size());
    for (unsigned int i=0; i < m_pano.getNrOfImages(); i++) {
        m_ToggleButtons[i]->SetValue(false);
    }
    UIntSet displayedImgs;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetActiveImagesCmd(m_pano, displayedImgs)
        );
    updatePano();
}

void PreviewFrame::OnNumTransform(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("dlg_numtrans"));
    dlg.CentreOnParent();
    if (dlg.ShowModal() == wxID_OK ) {
        wxString text = XRCCTRL(dlg, "numtrans_yaw", wxTextCtrl)->GetValue();
        double y;
        if (!str2double(text, y)) {
            wxLogError(_("Yaw value must be numeric."));
            return;
        }
        text = XRCCTRL(dlg, "numtrans_pitch", wxTextCtrl)->GetValue();
        double p;
        if (!str2double(text, p)) {
            wxLogError(_("Pitch value must be numeric."));
            return;
        }
        text = XRCCTRL(dlg, "numtrans_roll", wxTextCtrl)->GetValue();
        double r;
        if (!str2double(text, r)) {
            wxLogError(_("Roll value must be numeric."));
            return;
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::RotatePanoCmd(m_pano, y, p, r)
            );
        // update preview panel
        updatePano();
    } else {
        DEBUG_DEBUG("Numerical transform canceled");
    }
}

void PreviewFrame::OnTextCtrlChanged(wxCommandEvent & e)
{
    PanoramaOptions opts = m_pano.getOptions();
    if (e.GetEventObject() == m_exposureTextCtrl) {
        // exposure
        wxString text = m_exposureTextCtrl->GetValue();
        DEBUG_INFO ("target exposure = " << text.mb_str(wxConvLocal) );
        double p = 0;
        if (text != wxT("")) {
            if (!str2double(text, p)) {
                wxLogError(_("Value must be numeric."));
                return;
            }
        }
        opts.outputExposureValue = p;
    } else {
        int nParam = opts.m_projFeatures.numberOfParameters;
        std::vector<double> para = opts.getProjectionParameters();
        for (int i = 0; i < nParam; i++) {
            if (e.GetEventObject() == m_projParamTextCtrl[i]) {
                wxString text = m_projParamTextCtrl[i]->GetValue();
                DEBUG_INFO ("param " << i << ":  = " << text.mb_str(wxConvLocal) );
                double p = 0;
                if (text != wxT("")) {
                    if (!str2double(text, p)) {
                        wxLogError(_("Value must be numeric."));
                        return;
                    }
                }
                para[i] = p;
            }
        }
        opts.setProjectionParameters(para);
    }
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opts )
                                           );
    // update preview panel
    updatePano();

}

void PreviewFrame::OnProjParameterReset(wxCommandEvent &e)
{
    PanoramaOptions opts=m_pano.getOptions();
    opts.resetProjectionParameters();
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd(m_pano, opts)
        );
};

void PreviewFrame::OnChangeFOV(wxScrollEvent & e)
{
    DEBUG_TRACE("");

    PanoramaOptions opt = m_pano.getOptions();

    if (e.GetEventObject() == m_HFOVSlider) {
        DEBUG_DEBUG("HFOV changed (slider): " << e.GetInt() << " == " << m_HFOVSlider->GetValue());
        opt.setHFOV(e.GetInt());
    } else if (e.GetEventObject() == m_VFOVSlider) {
        DEBUG_DEBUG("VFOV changed (slider): " << e.GetInt());
        opt.setVFOV(e.GetInt());
    } else {
        int nParam = opt.m_projFeatures.numberOfParameters;
        std::vector<double> para = opt.getProjectionParameters();
        for (int i = 0; i < nParam; i++) {
            if (e.GetEventObject() == m_projParamSlider[i]) {
                // update
                para[i] = e.GetInt();
            }
        }
        opt.setProjectionParameters(para);
		opt.setHFOV(m_HFOVSlider->GetValue());
		opt.setVFOV(m_VFOVSlider->GetValue());
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
        default:
            DEBUG_WARN("Unknown blend mode selected");
        }
    } else {
        DEBUG_WARN("wxChoice event from unknown object received");
    }
}


void PreviewFrame::OnDefaultExposure( wxCommandEvent & e )
{
    if (m_pano.getNrOfImages() > 0) {
        PanoramaOptions opt = m_pano.getOptions();
        opt.outputExposureValue = calcMeanExposure(m_pano);
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( m_pano, opt )
                                               );
        updatePano();
    }
}

void PreviewFrame::OnIncreaseExposure( wxSpinEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();
    opt.outputExposureValue = opt.outputExposureValue + 1.0/3;
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opt )
                                            );
    updatePano();
}

void PreviewFrame::OnDecreaseExposure( wxSpinEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();
    opt.outputExposureValue = opt.outputExposureValue - 1.0/3;
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opt )
                                           );
    updatePano();
}

void PreviewFrame::OnProjectionChoice( wxCommandEvent & e )
{
    if (e.GetEventObject() == m_ProjectionChoice) {
        PanoramaOptions opt = m_pano.getOptions();
        int lt = m_ProjectionChoice->GetSelection();
        wxString Ip;
        opt.setProjection( (PanoramaOptions::ProjectionFormat) lt );
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( m_pano, opt )
                                            );
        DEBUG_DEBUG ("Projection changed: "  << lt);
        updatePano();

    } else {
        DEBUG_WARN("wxChoice event from unknown object received");
    }
}

void PreviewFrame::OnOutputChoice( wxCommandEvent & e)
{
    if (e.GetEventObject() == m_outputModeChoice) {
        PanoramaOptions opt = m_pano.getOptions();
        int lt = m_outputModeChoice->GetSelection();
        wxString Ip;
        opt.outputMode = ( (PanoramaOptions::OutputMode) lt );
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( m_pano, opt )
                                               );
        updatePano();

    } else {
        DEBUG_WARN("wxChoice event from unknown object received");
    }
}

/** update the display */
void PreviewFrame::updateProgressDisplay()
{
    wxString msg;
    // build the message:
    for (std::vector<AppBase::ProgressTask>::iterator it = tasks.begin();
         it != tasks.end(); ++it)
    {
        wxString cMsg;
        if (it->getProgress() > 0) {
            cMsg.Printf(wxT("%s [%3.0f%%]: %s "),
                        wxString(it->getShortMessage().c_str(), wxConvLocal).c_str(),
                        100 * it->getProgress(),
                        wxString(it->getMessage().c_str(), wxConvLocal).c_str());
        } else {
            cMsg.Printf(wxT("%s %s"),wxString(it->getShortMessage().c_str(), wxConvLocal).c_str(),
                        wxString(it->getMessage().c_str(), wxConvLocal).c_str());
        }
        // append to main message
        if (it == tasks.begin()) {
            msg = cMsg;
        } else {
            msg.Append(wxT(" | "));
            msg.Append(cMsg);
        }
    }
//    wxStatusBar *m_statbar = GetStatusBar();
    //DEBUG_TRACE("Statusmb : " << msg.mb_str(wxConvLocal));
    //m_statbar->SetStatusText(msg,0);

#ifdef __WXMSW__
    UpdateWindow(NULL);
#else
    // This is a bad call.. we just want to repaint the window, instead we will
    // process user events as well :( Unfortunately, there is not portable workaround...
//    wxYield();
#endif
}

void PreviewFrame::OnFullScreen(wxCommandEvent & e)
{
    ShowFullScreen(!IsFullScreen(), wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
#ifdef __WXGTK__
    //workaround a wxGTK bug that also the toolbar is hidden, but not requested to hide
    GetToolBar()->Show(true);
#endif
    m_PreviewPanel->ForceUpdate();
};

void PreviewFrame::OnUndo(wxCommandEvent &e)
{
    if(GlobalCmdHist::getInstance().canUndo())
    {
        wxCommandEvent dummy(wxEVT_COMMAND_MENU_SELECTED, XRCID("ID_EDITUNDO"));
        m_parent->GetEventHandler()->AddPendingEvent(dummy);
    }
    else
    {
        wxBell();
    };
};

void PreviewFrame::OnRedo(wxCommandEvent &e)
{
    if(GlobalCmdHist::getInstance().canRedo())
    {
        wxCommandEvent dummy(wxEVT_COMMAND_MENU_SELECTED, XRCID("ID_EDITREDO"));
        m_parent->GetEventHandler()->AddPendingEvent(dummy);
    }
    else
    {
        wxBell();
    };
};

