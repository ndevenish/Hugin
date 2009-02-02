// -*- c-basic-offset: 4 -*-

/** @file GLPreviewFrame.cpp
 *
 *  @brief implementation of GLPreviewFrame Class
 *
 *  @author James Legg and Pablo d'Angelo <pablo.dangelo@web.de>
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
//wxMac now has toggle buttons, but you can't overide their colours.

#include <config.h>

#include "panoinc_WX.h"

#include "panoinc.h"

#include "base_wx/platform.h"
#include "hugin/config_defaults.h"
#include "hugin/GLPreviewFrame.h"
#include "hugin/huginApp.h"
#include "hugin/ImagesPanel.h"
#include "hugin/CommandHistory.h"
#include "hugin/GLViewer.h"
// something messed up... temporary fix :-(
#include "hugin_utils/utils.h"
#define DEBUG_HEADER ""
/*#include <vigra_ext/ImageTransforms.h>
*/

extern "C" {
#ifdef HasPANO13
#include <pano13/queryfeature.h>
#else
#include <pano12/queryfeature.h>
#endif
}

#include "PreviewToolHelper.h"
#include "PreviewTool.h"
#include "PreviewCropTool.h"
#include "PreviewDragTool.h"
#include "PreviewIdentifyTool.h"
#include "PreviewDifferenceTool.h"
#include "PreviewPanoMaskTool.h"

using namespace utils;

// a random id, hope this doesn't break something..
enum {
    ID_PROJECTION_CHOICE = wxID_HIGHEST +11,
    ID_BLEND_CHOICE,
    ID_UPDATE_BUTTON,
    ID_OUTPUTMODE_CHOICE,
    ID_EXPOSURE_TEXT,
    ID_EXPOSURE_SPIN,
    ID_EXPOSURE_DEFAULT,
    ID_TOGGLE_BUT = wxID_HIGHEST+500,
    PROJ_PARAM_NAMES_ID = wxID_HIGHEST+1300,
    PROJ_PARAM_VAL_ID = wxID_HIGHEST+1400,
    PROJ_PARAM_SLIDER_ID = wxID_HIGHEST+1500,
    ID_TOGGLE_BUT_LEAVE = wxID_HIGHEST+1600
};

BEGIN_EVENT_TABLE(GLPreviewFrame, wxFrame)
    EVT_CLOSE(GLPreviewFrame::OnClose)
    EVT_TOOL(XRCID("preview_center_tool"), GLPreviewFrame::OnCenterHorizontally)
    EVT_TOOL(XRCID("preview_fit_pano_tool"), GLPreviewFrame::OnFitPano)
    EVT_TOOL(XRCID("preview_straighten_pano_tool"), GLPreviewFrame::OnStraighten)
    EVT_TOOL(XRCID("preview_num_transform"), GLPreviewFrame::OnNumTransform)
    EVT_TOOL(XRCID("preview_show_all_tool"), GLPreviewFrame::OnShowAll)
    EVT_TOOL(XRCID("preview_show_none_tool"), GLPreviewFrame::OnShowNone)
    EVT_TOOL(XRCID("preview_photometric_tool"), GLPreviewFrame::OnPhotometric)
    EVT_TOOL(XRCID("preview_crop_tool"), GLPreviewFrame::OnCrop)
    EVT_TOOL(XRCID("preview_drag_tool"), GLPreviewFrame::OnDrag)
    EVT_TOOL(XRCID("preview_identify_tool"), GLPreviewFrame::OnIdentify)
    
    EVT_TEXT_ENTER( -1 , GLPreviewFrame::OnTextCtrlChanged)

    EVT_BUTTON(ID_EXPOSURE_DEFAULT, GLPreviewFrame::OnDefaultExposure)
    EVT_SPIN_DOWN(ID_EXPOSURE_SPIN, GLPreviewFrame::OnDecreaseExposure)
    EVT_SPIN_UP(ID_EXPOSURE_SPIN, GLPreviewFrame::OnIncreaseExposure)
    EVT_CHOICE(ID_BLEND_CHOICE, GLPreviewFrame::OnBlendChoice)
    EVT_CHOICE(ID_PROJECTION_CHOICE, GLPreviewFrame::OnProjectionChoice)
#ifndef __WXMAC__
    // wxMac does not process these
    EVT_SCROLL_CHANGED(GLPreviewFrame::OnChangeFOV)
#else
    EVT_SCROLL_THUMBRELEASE(GLPreviewFrame::OnChangeFOV)
    EVT_SCROLL_ENDSCROLL(GLPreviewFrame::OnChangeFOV)
#endif
    EVT_SCROLL_THUMBTRACK(GLPreviewFrame::OnTrackChangeFOV)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(ImageToogleButtonEventHandler, wxEvtHandler)
    EVT_ENTER_WINDOW(ImageToogleButtonEventHandler::OnEnter)
    EVT_LEAVE_WINDOW(ImageToogleButtonEventHandler::OnLeave)
#ifdef USE_TOGGLE_BUTTON
    EVT_TOGGLEBUTTON(-1, ImageToogleButtonEventHandler::OnChange)
#else
    EVT_CHECKBOX(-1, ImageToogleButtonEventHandler::OnChange)
#endif    
END_EVENT_TABLE()

#define PF_STYLE (wxMAXIMIZE_BOX | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN)
#include <iostream>
GLPreviewFrame::GLPreviewFrame(wxFrame * frame, PT::Panorama &pano)
    : wxFrame(frame,-1, _("Fast Panorama preview"), wxDefaultPosition, wxDefaultSize,
              PF_STYLE),
      m_pano(pano)
{
	  DEBUG_TRACE("");

    m_oldProjFormat = -1;
    m_ToolBar = wxXmlResource::Get()->LoadToolBar(this, wxT("fast_preview_toolbar"));
    DEBUG_ASSERT(m_ToolBar);
    // create tool bar
    SetToolBar(m_ToolBar);
    drag_tool_id = wxXmlResource::Get()->GetXRCID(wxT("preview_drag_tool"));
    DEBUG_ASSERT(drag_tool_id != -2);
    crop_tool_id = wxXmlResource::Get()->GetXRCID(wxT("preview_crop_tool"));
    DEBUG_ASSERT(crop_tool_id != -2);
    identify_tool_id = wxXmlResource::Get()->GetXRCID(wxT("preview_identify_tool"));
    DEBUG_ASSERT(identify_tool_id != -2);
    

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

    m_topsizer->Add(m_ToggleButtonSizer, 0, wxEXPAND | wxADJUST_MINSIZE | wxBOTTOM, 5);

    wxFlexGridSizer * flexSizer = new wxFlexGridSizer(2,0,5,5);
    flexSizer->AddGrowableCol(0);
    flexSizer->AddGrowableRow(0);

    // create our Viewer
    int args[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0};
    m_GLViewer = new GLViewer(this, pano, args, this);

    flexSizer->Add(m_GLViewer,
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
#ifdef HasPANO13
    int nP = panoProjectionFormatCount();
    for(int n=0; n < nP; n++) {
        pano_projection_features proj;
        if (panoProjectionFeaturesQuery(n, &proj)) {
            wxString str2(proj.name, wxConvLocal);
            m_ProjectionChoice->Append(wxGetTranslation(str2));
        }
    }
#else
    bool ok = true;
    int n=0;
    while(ok) {
        char name[20];
        char str[255];
        sprintf(name,"PanoType%d",n);
        n++;
        int len = queryFeatureString(name,str,255);
        if (len > 0) {
            wxString str2(str, wxConvLocal);
            m_ProjectionChoice->Append(wxGetTranslation(str2));
        } else {
            ok = false;
        }
    }
#endif
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

    int oldMode = wxConfigBase::Get()->Read(wxT("/GLPreviewFrame/blendMode"), 0l);
    if (oldMode > 1) oldMode = 0;
    m_BlendModeChoice = new wxChoice(this, ID_BLEND_CHOICE,
                                     wxDefaultPosition, wxDefaultSize,
                                     2, m_choices);
    // TODO
    // m_BlendModeChoice->SetSelection((PreviewCanvas::BlendMode) oldMode);

    blendModeSizer->Add(m_BlendModeChoice,
                        0,
                        wxALL | wxALIGN_CENTER_VERTICAL,
                        5);

    // TODO implement hdr display in OpenGL, if possible?
    // Disabled until someone can figure out HDR display in OpenGL.
    /*
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
    */
    
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

    m_exposureTextCtrl = new wxTextCtrl(this, ID_EXPOSURE_TEXT, _("0"),
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

    
    
#ifdef HasPANO13
    m_projParamSizer = new wxStaticBoxSizer(
    new wxStaticBox(this, -1, _("Projection Parameters")),
     wxHORIZONTAL);

    m_projParamNamesLabel.resize(PANO_PROJECTION_MAX_PARMS);
    m_projParamTextCtrl.resize(PANO_PROJECTION_MAX_PARMS);
    m_projParamSlider.resize(PANO_PROJECTION_MAX_PARMS);

    for (int i=0; i < PANO_PROJECTION_MAX_PARMS; i++) {

        m_projParamNamesLabel[i] = new wxStaticText(this, PROJ_PARAM_NAMES_ID+i, _("param:"));
        m_projParamSizer->Add(m_projParamNamesLabel[i],
                        0,        // not vertically strechable
                        wxALL | wxALIGN_CENTER_VERTICAL, // draw border all around
                        5);       // border width
        m_projParamTextCtrl[i] = new wxTextCtrl(this, PROJ_PARAM_VAL_ID+i, _("0"),
                                    wxDefaultPosition,wxDefaultSize, wxTE_PROCESS_ENTER);
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

#endif

    wxConfigBase * config = wxConfigBase::Get();

    // add a status bar
    CreateStatusBar(3);
    int widths[3] = {-3, 150, 150};
    SetStatusWidths(3, widths);
    SetStatusText(wxT(""),1);
    SetStatusText(wxT(""),2);

    // the initial size as calculated by the sizers
    this->SetSizer( m_topsizer );
    m_topsizer->SetSizeHints( this );

    // set the minimize icon
#ifdef __WXMSW__
    wxIcon myIcon(MainFrame::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(MainFrame::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    m_pano.addObserver(this);

    RestoreFramePosition(this, wxT("GLPreviewFrame"));
    
    // TODO tell renderer
    // m_PreviewPanel->SetBlendMode((PreviewPanel::BlendMode)oldMode );

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows..
    this->SetBackgroundColour(m_GLViewer->GetBackgroundColour());
#endif

    if (config->Read(wxT("/GLPreviewFrame/isShown"), 0l) != 0) {
        Show();
    }
    crop_tool = 0;
}

GLPreviewFrame::~GLPreviewFrame()
{
    DEBUG_TRACE("dtor writing config");
    wxConfigBase * config = wxConfigBase::Get();
    wxSize sz = GetClientSize();

    StoreFramePosition(this, wxT("GLPreviewFrame"));

    if ( (!this->IsIconized()) && (! this->IsMaximized()) && this->IsShown()) {
        config->Write(wxT("/GLPreviewFrame/isShown"), 1l);
    } else {
        config->Write(wxT("/GLPreviewFrame/isShown"), 0l);
    }

    config->Write(wxT("/GLPreviewFrame/blendMode"), m_BlendModeChoice->GetSelection());
    
    // delete all of the tools. When the preview is never used we never get an
    // OpenGL context and therefore don't create the tools.
    if (crop_tool)
    {
        helper->DeactivateTool(crop_tool); delete crop_tool;
        helper->DeactivateTool(drag_tool); delete drag_tool;
        helper->DeactivateTool(identify_tool); delete identify_tool;
        helper->DeactivateTool(difference_tool); delete difference_tool;
        helper->DeactivateTool(pano_mask_tool); delete pano_mask_tool;
    }
    m_pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void GLPreviewFrame::panoramaChanged(Panorama &pano)
{
    const PanoramaOptions & opts = pano.getOptions();

    wxString projection;
    m_ProjectionChoice->SetSelection(opts.getProjection());
    m_VFOVSlider->Enable( opts.fovCalcSupported(opts.getProjection()) );
    
    // No HDR display yet.
    /*
    m_outputModeChoice->SetSelection(opts.outputMode);
    if (opts.outputMode == PanoramaOptions::OUTPUT_HDR) {
        m_exposureTextCtrl->Hide();
        m_defaultExposureBut->Hide();
        m_decExposureBut->Hide();
        m_incExposureBut->Hide();
    } else {
        m_exposureTextCtrl->Show();
        m_defaultExposureBut->Show();
        m_decExposureBut->Show();
        m_incExposureBut->Show();
    }*/
    m_exposureTextCtrl->SetValue(wxString(doubleToString(opts.outputExposureValue,2).c_str(), wxConvLocal));

    bool activeImgs = pano.getActiveImages().size() > 0;
    m_ToolBar->EnableTool(XRCID("preview_center_tool"), activeImgs);
    m_ToolBar->EnableTool(XRCID("preview_fit_pano_tool"), activeImgs);
    m_ToolBar->EnableTool(XRCID("preview_num_transform"), activeImgs);
    m_ToolBar->EnableTool(XRCID("preview_straighten_pano_tool"), pano.getNrOfImages() > 0);

    // TODO: enable display of parameters and set their limits, if projection has some.
#ifdef HasPANO13
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
                m_projParamSlider[i]->SetRange(utils::roundi(pp->minValue), utils::roundi(pp->maxValue));
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
            m_projParamTextCtrl[i]->SetValue(wxString(val.c_str(), wxConvLocal));
            m_projParamSlider[i]->SetValue(utils::roundi(params[i]));
        }
    }
    if (relayout) {
        m_topsizer->Layout();
    }
#endif
    SetStatusText(wxString::Format(wxT("%.1f x %.1f"), opts.getHFOV(), opts.getVFOV()),2);
    m_HFOVSlider->SetValue(roundi(opts.getHFOV()));
    m_VFOVSlider->SetValue(roundi(opts.getVFOV()));

    m_oldProjFormat = opts.getProjection();

}

void GLPreviewFrame::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
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
        delete toogle_button_event_handlers[i];
        toogle_button_event_handlers.pop_back();
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
				wxFileName tFilename(wxString (pano.getImage(imgNr).getFilename().c_str(), HUGIN_CONV_FILENAME));
				but->SetToolTip(tFilename.GetFullName());
                // for the identification tool to work, we need to find when the
                // mouse enters and exits the button. We use a custom event
                // handler, which will also toggle the images:
                ImageToogleButtonEventHandler * event_handler = new
                    ImageToogleButtonEventHandler(*it, &identify_tool,
                                                  identify_tool_id, m_ToolBar,
                                                  &m_pano);
                toogle_button_event_handlers.push_back(event_handler);
                but->PushEventHandler(event_handler);
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
    }

    if (dirty) {
		m_ButtonSizer->SetVirtualSizeHints(m_ButtonPanel);
		// Layout();
		DEBUG_INFO("New m_ButtonPanel width: " << (m_ButtonPanel->GetSize()).GetWidth());
		DEBUG_INFO("New m_ButtonPanel Height: " << (m_ButtonPanel->GetSize()).GetHeight());
    }
}


void GLPreviewFrame::OnClose(wxCloseEvent& event)
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

void GLPreviewFrame::OnCenterHorizontally(wxCommandEvent & e)
{
    if (m_pano.getActiveImages().size() == 0) return;

    GlobalCmdHist::getInstance().addCommand(
        new PT::CenterPanoCmd(m_pano)
        );
    // fit pano afterwards
    OnFitPano(e);
}

void GLPreviewFrame::OnStraighten(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    GlobalCmdHist::getInstance().addCommand(
        new PT::StraightenPanoCmd(m_pano)
        );
    if (m_pano.getOptions().getHFOV() > 359) {
        // adjust canvas size for 360 deg panos
        OnFitPano(e);
    } else {
        // also center non 360 deg panos
        OnCenterHorizontally(e);
    }
}

void GLPreviewFrame::OnFitPano(wxCommandEvent & e)
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
}

void GLPreviewFrame::OnShowAll(wxCommandEvent & e)
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
}

void GLPreviewFrame::OnShowNone(wxCommandEvent & e)
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
}

void GLPreviewFrame::OnNumTransform(wxCommandEvent & e)
{
    if (m_pano.getNrOfImages() == 0) return;

    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("dlg_numtrans"));
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
    } else {
        DEBUG_DEBUG("Numerical transform canceled");
    }
}

void GLPreviewFrame::OnTextCtrlChanged(wxCommandEvent & e)
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
#ifdef HasPANO13
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
#endif
    }
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opts )
                                           );
}

void GLPreviewFrame::OnChangeFOV(wxScrollEvent & e)
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
#ifdef HasPANO13
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
#endif
    }

    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( m_pano, opt )
        );    
}

void GLPreviewFrame::OnTrackChangeFOV(wxScrollEvent & e)
{
    DEBUG_TRACE("");

    PanoramaOptions opt = m_pano.getOptions();

    if (e.GetEventObject() == m_HFOVSlider) {
        opt.setHFOV(e.GetInt());
    } else if (e.GetEventObject() == m_VFOVSlider) {
        opt.setVFOV(e.GetInt());
    } else {
#ifdef HasPANO13
        int nParam = opt.m_projFeatures.numberOfParameters;
        std::vector<double> para = opt.getProjectionParameters();
        for (int i = 0; i < nParam; i++) {
            if (e.GetEventObject() == m_projParamSlider[i]) {
                // update
                para[i] = e.GetInt();
            }
        }
        opt.setProjectionParameters(para);
#endif
    }
    // we only actually update the panorama fully when the mouse is released.
    // As we are dragging it we don't want to create undo events, but we would
    // like to update the display, so we change the GLViewer's ViewState and
    // request a redraw.
    m_GLViewer->m_view_state->SetOptions(&opt);
    m_GLViewer->Refresh();
}

void GLPreviewFrame::OnBlendChoice(wxCommandEvent & e)
{
    if (e.GetEventObject() == m_BlendModeChoice) {
        int sel = e.GetSelection();
        switch (sel) {
        case 0:
            helper->DeactivateTool(difference_tool);
            break;
        case 1:
            helper->DeactivateTool(identify_tool);
            m_ToolBar->ToggleTool(identify_tool_id, false);
            helper->ActivateTool(difference_tool);
            CleanButtonColours();
            break;
        default:
            DEBUG_WARN("Unknown blend mode selected");
        }
    } else {
        // FIXME DEBUG_WARN("wxChoice event from unknown object received");
    }
}


void GLPreviewFrame::OnDefaultExposure( wxCommandEvent & e )
{
    if (m_pano.getNrOfImages() > 0) {
        PanoramaOptions opt = m_pano.getOptions();
        opt.outputExposureValue = calcMeanExposure(m_pano);
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( m_pano, opt )
                                               );
    }
}

void GLPreviewFrame::OnIncreaseExposure( wxSpinEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();
    opt.outputExposureValue = opt.outputExposureValue + 1.0/3;
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opt )
                                            );
}

void GLPreviewFrame::OnDecreaseExposure( wxSpinEvent & e )
{
    PanoramaOptions opt = m_pano.getOptions();
    opt.outputExposureValue = opt.outputExposureValue - 1.0/3;
    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( m_pano, opt )
                                           );
}

void GLPreviewFrame::OnProjectionChoice( wxCommandEvent & e )
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

    } else {
        // FIXME DEBUG_WARN("wxChoice event from unknown object received");
    }
}

/* We don't have an OpenGL hdr display yet
void GLPreviewFrame::OnOutputChoice( wxCommandEvent & e)
{
    if (e.GetEventObject() == m_outputModeChoice) {
        PanoramaOptions opt = m_pano.getOptions();
        int lt = m_outputModeChoice->GetSelection();
        wxString Ip;
        opt.outputMode = ( (PanoramaOptions::OutputMode) lt );
        GlobalCmdHist::getInstance().addCommand(
                new PT::SetPanoOptionsCmd( m_pano, opt )
                                               );

    } else {
        // FIXME DEBUG_WARN("wxChoice event from unknown object received");
    }
}
*/

/** update the display */
void GLPreviewFrame::updateProgressDisplay()
{
    wxString msg;
    // build the message:
    for (std::vector<ProgressTask>::iterator it = tasks.begin();
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

void GLPreviewFrame::SetStatusMessage(wxString message)
{
    SetStatusText(message, 0);
}


void GLPreviewFrame::OnPhotometric(wxCommandEvent & e)
{
    m_GLViewer->SetPhotometricCorrect(e.IsChecked());
}

void GLPreviewFrame::MakeTools(PreviewToolHelper *helper_in)
{
    // create the tool objects.
    // we delay this until we have an OpenGL context so that they are free to
    // create texture objects and display lists before they are used.
    helper = helper_in;
    crop_tool = new PreviewCropTool(helper);
    drag_tool = new PreviewDragTool(helper);
    identify_tool = new PreviewIdentifyTool(helper, this);
    difference_tool = new PreviewDifferenceTool(helper);
    pano_mask_tool = new PreviewPanoMaskTool(helper);
    
    // activate tools that are always active.
    helper->ActivateTool(pano_mask_tool);
}

void GLPreviewFrame::OnCrop(wxCommandEvent & e)
{
    // turn on or off the crop tool when its button is pressed.
    SetStatusText(wxT(""), 0); // blank status text as it refers to an old tool.
    if (e.IsChecked())
    {
        // ActivateTool returns pointers to the tools that were switched off to
        // enable the crop tool.
        TurnOffTools(helper->ActivateTool(crop_tool));
    } else {
        helper->DeactivateTool(crop_tool);
        // the tool draws some extra guides that need covering up
        m_GLViewer->Refresh();
    }
}

void GLPreviewFrame::OnDrag(wxCommandEvent & e)
{
    SetStatusText(wxT(""), 0); // blank status text as it refers to an old tool.
    if (e.IsChecked())
    {
        TurnOffTools(helper->ActivateTool(drag_tool));
    } else {
        helper->DeactivateTool(drag_tool);
    }
    // The plumb lines should only be shown when the tool is active:
    m_GLViewer->Refresh(); // ...draw them or draw over them.
}

void GLPreviewFrame::OnIdentify(wxCommandEvent & e)
{
    SetStatusText(wxT(""), 0); // blank status text as it refers to an old tool.
    if (e.IsChecked())
    {
        m_BlendModeChoice->SetSelection(0);
        helper->DeactivateTool(difference_tool);
        TurnOffTools(helper->ActivateTool(identify_tool));
    } else {
        helper->DeactivateTool(identify_tool);
        CleanButtonColours();
    }
    m_GLViewer->Refresh();
}

void GLPreviewFrame::TurnOffTools(std::set<PreviewTool*> tools)
{
    std::set<PreviewTool*>::iterator i;
    for (i = tools.begin(); i != tools.end(); i++)
    {
        if (*i == crop_tool)
        {
            // disabled the crop tool, toogle the button for it off
            m_ToolBar->ToggleTool(crop_tool_id, false);
            // cover up the guidelines
            m_GLViewer->Refresh();
        } else if (*i == drag_tool)
        {
            // disabled the drag tool, toggle its button off.
            m_ToolBar->ToggleTool(drag_tool_id, false);
            // cover up its boxes
            m_GLViewer->Refresh();
        } else if (*i == identify_tool)
        {
            // disabled the identify tool, toggle its button off.
            m_ToolBar->ToggleTool(identify_tool_id, false);
            // cover up its indicators and restore normal button colours.
            m_GLViewer->Refresh();
            CleanButtonColours();
        }
    }
}

void GLPreviewFrame::SetImageButtonColour(unsigned int image_nr,
                                          unsigned char red,
                                          unsigned char green,
                                          unsigned char blue)
{
    // 0, 0, 0 indicates we want to go back to the system colour.
    // TODO: Maybe we should test this better on different themes.
    // On OS X, the background colour is ignored on toggle buttons, but not
    // checkboxes.
    if (red || green || blue)
    {
        // the identify tool wants us to highlight an image button in the given
        // colour, to match up with the display in the preview.
        m_ToggleButtons[image_nr]->SetBackgroundStyle(wxBG_STYLE_COLOUR);
        m_ToggleButtons[image_nr]->SetBackgroundColour(
                                                    wxColour(red, green, blue));
        // black should be visible on the button's vibrant colours.
        m_ToggleButtons[image_nr]->SetForegroundColour(wxColour(0, 0, 0));
    } else {
        // return to the normal colour
        m_ToggleButtons[image_nr]->SetBackgroundStyle(wxBG_STYLE_SYSTEM);
        m_ToggleButtons[image_nr]->SetBackgroundColour(wxNullColour);
        m_ToggleButtons[image_nr]->SetForegroundColour(wxNullColour);
    }
    m_ToggleButtons[image_nr]->Refresh();
}

void GLPreviewFrame::CleanButtonColours()
{
    // when we turn off the identification tool, any buttons that were coloured
    // to match the image in the preview should be given back the system themed
    // colours.
    unsigned int nr_images = m_pano.getNrOfImages();
    for (unsigned image = 0; image < nr_images; image++)
    {
        m_ToggleButtons[image]->SetBackgroundStyle(wxBG_STYLE_SYSTEM);
        m_ToggleButtons[image]->SetBackgroundColour(wxNullColour);
        m_ToggleButtons[image]->SetForegroundColour(wxNullColour);
        m_ToggleButtons[image]->Refresh();
    }
}

ImageToogleButtonEventHandler::ImageToogleButtonEventHandler(
                                  unsigned int image_number_in,
                                  PreviewIdentifyTool **identify_tool_in,
                                  unsigned int identify_tool_id_in,
                                  wxToolBar *tool_bar_in,
                                  PT::Panorama * m_pano_in)
{
    image_number = image_number_in;
    identify_tool = identify_tool_in;
    identify_tool_id = identify_tool_id_in;
    tool_bar = tool_bar_in;
    m_pano = m_pano_in;
}

void ImageToogleButtonEventHandler::OnEnter(wxMouseEvent & e)
{
    // When using the identify tool, we want to identify image locations when
    // the user moves the mouse over the image buttons, but only if the image
    // is being shown.
    if (   tool_bar->GetToolState(identify_tool_id)
        && m_pano->getActiveImages().count(image_number))
    {
        (*identify_tool)->ShowImageNumber(image_number);
    }
    e.Skip();
}

void ImageToogleButtonEventHandler::OnLeave(wxMouseEvent & e)
{
    // if the mouse left one of the image toggle buttons with the identification
    // tool active, we should stop showing the image indicator for that button.
    if (tool_bar->GetToolState(identify_tool_id)
        && m_pano->getActiveImages().count(image_number))
    {
        (*identify_tool)->StopShowingImages();
    }
    e.Skip();
}

void ImageToogleButtonEventHandler::OnChange(wxCommandEvent & e)
{
    // the user is turning on or off an image using its button. We want to turn
    // the indicators on and off if appropriate correctly to. We use OnEnter
    // and OnLeave for the indicators, but these only work when the image is
    // showing, so we are carefull of the order:
    UIntSet activeImages = m_pano->getActiveImages();
    wxMouseEvent null_event;
    if (e.IsChecked()) {
        activeImages.insert(image_number);
      	GlobalCmdHist::getInstance().addCommand(
            new PT::SetActiveImagesCmd(*m_pano, activeImages)
        );
        OnEnter(null_event);
    } else {
        OnLeave(null_event);
        activeImages.erase(image_number);
      	GlobalCmdHist::getInstance().addCommand(
            new PT::SetActiveImagesCmd(*m_pano, activeImages)
        );
    }
}

