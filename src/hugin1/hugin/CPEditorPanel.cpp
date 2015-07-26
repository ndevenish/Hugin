// -*- c-basic-offset: 4 -*-

/** @file CPEditorPanel.cpp
 *
 *  @brief implementation of CPEditorPanel Class
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

// often necessary before panoinc.h
#ifdef __APPLE__
#include "panoinc_WX.h"
#endif
// standard hugin include
#include "panoinc.h"
// both includes above need to come before other wx includes on OSX

// hugin's
#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "base_wx/CommandHistory.h"
#include "base_wx/wxImageCache.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/CPEditorPanel.h"
#include "base_wx/wxPanoCommand.h"
#include "base_wx/MyProgressDialog.h"
#include "algorithms/optimizer/PTOptimizer.h"
#include "algorithms/basic/CalculateOptimalScale.h"
#include "base_wx/PTWXDlg.h"
#include "base_wx/wxPlatform.h"

// more standard includes if needed
#include <algorithm>
#include <float.h>
#include <vector>

// more vigra include if needed
#include "vigra/cornerdetection.hxx"
#include "vigra/localminmax.hxx"
#include "vigra_ext/openmp_vigra.h"
#include "vigra_ext/Correlation.h"

// Celeste header
#include "Celeste.h"

using namespace std;
using namespace HuginBase;
using namespace vigra;
using namespace vigra_ext;
using namespace vigra::functor;
using namespace hugin_utils;

BEGIN_EVENT_TABLE(CPEditorPanel, wxPanel)
    EVT_CPEVENT(CPEditorPanel::OnCPEvent)
    EVT_COMBOBOX(XRCID("cp_editor_left_choice"), CPEditorPanel::OnLeftChoiceChange )
    EVT_COMBOBOX(XRCID("cp_editor_right_choice"), CPEditorPanel::OnRightChoiceChange )
    EVT_LIST_ITEM_SELECTED(XRCID("cp_editor_cp_list"), CPEditorPanel::OnCPListSelect)
    EVT_LIST_ITEM_DESELECTED(XRCID("cp_editor_cp_list"), CPEditorPanel::OnCPListDeselect)
    EVT_LIST_COL_END_DRAG(XRCID("cp_editor_cp_list"), CPEditorPanel::OnColumnWidthChange)
    EVT_CHOICE(XRCID("cp_editor_choice_zoom"), CPEditorPanel::OnZoom)
    EVT_TEXT_ENTER(XRCID("cp_editor_x1"), CPEditorPanel::OnTextPointChange )
    EVT_TEXT_ENTER(XRCID("cp_editor_y1"), CPEditorPanel::OnTextPointChange )
    EVT_TEXT_ENTER(XRCID("cp_editor_x2"), CPEditorPanel::OnTextPointChange )
    EVT_TEXT_ENTER(XRCID("cp_editor_y2"), CPEditorPanel::OnTextPointChange )
    EVT_CHOICE(XRCID("cp_editor_mode"), CPEditorPanel::OnTextPointChange )
    EVT_CHAR(CPEditorPanel::OnKey)
    EVT_BUTTON(XRCID("cp_editor_delete"), CPEditorPanel::OnDeleteButton)
    EVT_BUTTON(XRCID("cp_editor_add"), CPEditorPanel::OnAddButton)
    EVT_BUTTON(XRCID("cp_editor_previous_img"), CPEditorPanel::OnPrevImg)
    EVT_BUTTON(XRCID("cp_editor_next_img"), CPEditorPanel::OnNextImg)
    EVT_BUTTON(XRCID("cp_editor_finetune_button"), CPEditorPanel::OnFineTuneButton)
    EVT_BUTTON(XRCID("cp_editor_action_button"), CPEditorPanel::OnActionButton)
    EVT_MENU(XRCID("cp_menu_create_cp"), CPEditorPanel::OnActionSelectCreate)
    EVT_MENU(XRCID("cp_menu_celeste"), CPEditorPanel::OnActionSelectCeleste)
    EVT_MENU(XRCID("cp_menu_clean_cp"), CPEditorPanel::OnActionSelectCleanCP)
END_EVENT_TABLE()

CPEditorPanel::CPEditorPanel()
{
    DEBUG_TRACE("**********************");
    m_pano = 0;
    m_countCP = 0;
}

bool CPEditorPanel::Create(wxWindow* parent, wxWindowID id,
                    const wxPoint& pos,
                    const wxSize& size,
                    long style,
                    const wxString& name)
{
    DEBUG_TRACE(" Create called *************");
    if (! wxPanel::Create(parent, id, pos, size, style, name) ) {
        return false;
    }

    cpCreationState = NO_POINT;
    m_leftImageNr=UINT_MAX;
    m_rightImageNr=UINT_MAX;
    m_listenToPageChange=true;
    m_detailZoomFactor=1;
    m_selectedPoint=UINT_MAX;
    m_leftRot=CPImageCtrl::ROT0;
    m_rightRot=CPImageCtrl::ROT0;

    DEBUG_TRACE("");
    wxXmlResource::Get()->LoadPanel(this, wxT("cp_editor_panel"));
    wxPanel * panel = XRCCTRL(*this, "cp_editor_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);

    m_leftChoice = XRCCTRL(*this, "cp_editor_left_choice", CPImagesComboBox); 
    m_leftImg = XRCCTRL(*this, "cp_editor_left_img", CPImageCtrl);
    assert(m_leftImg);
    m_leftImg->Init(this);
    m_leftImg->setTransforms(&m_leftTransform, &m_leftInvTransform, &m_rightInvTransform);

    // right image
    m_rightChoice = XRCCTRL(*this, "cp_editor_right_choice", CPImagesComboBox);
    m_rightImg = XRCCTRL(*this, "cp_editor_right_img", CPImageCtrl);
    assert(m_rightImg);
    m_rightImg->Init(this);
    m_rightImg->setTransforms(&m_rightTransform, &m_rightInvTransform, &m_leftInvTransform);

    // setup list view
    m_cpList = XRCCTRL(*this, "cp_editor_cp_list", wxListCtrl);
    m_cpList->Connect(wxEVT_CHAR,wxKeyEventHandler(CPEditorPanel::OnKey),NULL,this);
    m_cpList->InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 35);
    m_cpList->InsertColumn( 1, _("left x"), wxLIST_FORMAT_RIGHT, 65);
    m_cpList->InsertColumn( 2, _("left y"), wxLIST_FORMAT_RIGHT, 65);
    m_cpList->InsertColumn( 3, _("right x"), wxLIST_FORMAT_RIGHT, 65);
    m_cpList->InsertColumn( 4, _("right y"), wxLIST_FORMAT_RIGHT, 65);
    m_cpList->InsertColumn( 5, _("Alignment"), wxLIST_FORMAT_LEFT,110 );
    m_cpList->InsertColumn( 6, _("Distance"), wxLIST_FORMAT_RIGHT, 110);

    //get saved width
    wxConfigBase* config = wxConfig::Get();
    for ( int j=0; j < m_cpList->GetColumnCount() ; j++ )
    {
        // -1 is auto
        int width = config->Read(wxString::Format( wxT("/CPEditorPanel/ColumnWidth%d"), j ), -1);
        if(width != -1)
            m_cpList->SetColumnWidth(j, width);
    }

    // other controls
    m_x1Text = XRCCTRL(*this,"cp_editor_x1", wxTextCtrl);
    m_x1Text->PushEventHandler(new TextKillFocusHandler(this));
    m_y1Text = XRCCTRL(*this,"cp_editor_y1", wxTextCtrl);
    m_y1Text->PushEventHandler(new TextKillFocusHandler(this));
    m_x2Text = XRCCTRL(*this,"cp_editor_x2", wxTextCtrl);
    m_x2Text->PushEventHandler(new TextKillFocusHandler(this));
    m_y2Text = XRCCTRL(*this,"cp_editor_y2", wxTextCtrl);
    m_y2Text->PushEventHandler(new TextKillFocusHandler(this));

    m_cpModeChoice = XRCCTRL(*this, "cp_editor_mode", wxChoice);
    m_addButton = XRCCTRL(*this, "cp_editor_add", wxButton);
    m_delButton = XRCCTRL(*this, "cp_editor_delete", wxButton);

    m_autoAddCB = XRCCTRL(*this,"cp_editor_auto_add", wxCheckBox);
    DEBUG_ASSERT(m_autoAddCB);
    m_fineTuneCB = XRCCTRL(*this,"cp_editor_fine_tune_check",wxCheckBox);
    DEBUG_ASSERT(m_fineTuneCB);

    m_estimateCB = XRCCTRL(*this,"cp_editor_auto_estimate", wxCheckBox);
    DEBUG_ASSERT(m_estimateCB);

    m_actionButton = XRCCTRL(*this, "cp_editor_action_button", wxButton);
    m_actionButton->Connect(wxEVT_CONTEXT_MENU, wxContextMenuEventHandler(CPEditorPanel::OnActionContextMenu), NULL, this);
    m_cpActionContextMenu = wxXmlResource::Get()->LoadMenu(wxT("cp_menu_action"));
    // setup scroll window for the controls under the images
    m_cp_ctrls = XRCCTRL(*this, "cp_controls_panel", wxPanel);
    DEBUG_ASSERT(m_cp_ctrls);

    m_autoAddCB->SetValue(config->Read(wxT("/CPEditorPanel/autoAdd"),0l) != 0 );
    m_fineTuneCB->SetValue(config->Read(wxT("/CPEditorPanel/autoFineTune"),1l) != 0 );
    m_estimateCB->SetValue(config->Read(wxT("/CPEditorPanel/autoEstimate"),1l) != 0 );

    // disable controls by default
    m_cpModeChoice->Disable();
    m_addButton->Disable();
    m_delButton->Disable();
    m_autoAddCB->Disable();
    m_fineTuneCB->Disable();
    m_estimateCB->Disable();
    XRCCTRL(*this, "cp_editor_finetune_button", wxButton)->Disable();
    m_actionButton->Disable();
    XRCCTRL(*this, "cp_editor_choice_zoom", wxChoice)->Disable();
    XRCCTRL(*this, "cp_editor_previous_img", wxButton)->Disable();
    XRCCTRL(*this, "cp_editor_next_img", wxButton)->Disable();
    m_leftChoice->Disable();
    m_rightChoice->Disable();

    // apply zoom specified in xrc file
    wxCommandEvent dummy;
    dummy.SetInt(XRCCTRL(*this,"cp_editor_choice_zoom",wxChoice)->GetSelection());
    OnZoom(dummy);

    SetSizer( topsizer );
    // read last used action setting
    m_cpActionButtonMode = static_cast<CPTabActionButtonMode>(config->Read(wxT("/CPEditorPanel/ActionMode"), 1l));
    switch (m_cpActionButtonMode)
    {
        case CPTAB_ACTION_CREATE_CP:
            m_cpActionContextMenu->Check(XRCID("cp_menu_create_cp"), true);
            {
                wxCommandEvent e;
                OnActionSelectCreate(e);
            };
            break;
        case CPTAB_ACTION_CLEAN_CP:
            m_cpActionContextMenu->Check(XRCID("cp_menu_clean_cp"), true);
            {
                wxCommandEvent e;
                OnActionSelectCleanCP(e);
            };
            break;
        case CPTAB_ACTION_CELESTE:
        default:
            m_cpActionContextMenu->Check(XRCID("cp_menu_celeste"), true);
            {
                wxCommandEvent e;
                OnActionSelectCeleste(e);
            };
            break;
    };

    return true;
}

void CPEditorPanel::Init(HuginBase::Panorama * pano)
{
    m_pano=pano;
    // observe the panorama
    m_pano->addObserver(this);
}

CPEditorPanel::~CPEditorPanel()
{
    DEBUG_TRACE("dtor");

    m_x1Text->PopEventHandler(true);
    m_y1Text->PopEventHandler(true);
    m_x2Text->PopEventHandler(true);
    m_y2Text->PopEventHandler(true);

    wxConfigBase::Get()->Write(wxT("/CPEditorPanel/autoAdd"), m_autoAddCB->IsChecked() ? 1 : 0);
    wxConfigBase::Get()->Write(wxT("/CPEditorPanel/autoFineTune"), m_fineTuneCB->IsChecked() ? 1 : 0);
    wxConfigBase::Get()->Write(wxT("/CPEditorPanel/autoEstimate"), m_estimateCB->IsChecked() ? 1 : 0);

    m_pano->removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void CPEditorPanel::setLeftImage(unsigned int imgNr)
{
    DEBUG_TRACE("image " << imgNr);
    if (imgNr == UINT_MAX) {
        m_leftImg->setImage("", CPImageCtrl::ROT0);
        m_leftImageNr = imgNr;
        m_leftFile = "";
        changeState(NO_POINT);
        UpdateDisplay(true);
    } else if (m_leftImageNr != imgNr) {
        double yaw = const_map_get(m_pano->getImageVariables(imgNr),"y").getValue();
        double pitch = const_map_get(m_pano->getImageVariables(imgNr),"p").getValue();
        double roll = const_map_get(m_pano->getImageVariables(imgNr),"r").getValue();
        m_leftRot = GetRot(yaw, pitch, roll);
        m_leftImg->setImage(m_pano->getImage(imgNr).getFilename(), m_leftRot);
        m_leftImageNr = imgNr;
        if (m_leftChoice->GetSelection() != (int) imgNr) {
            m_leftChoice->SetSelection(imgNr);
        }
        m_rightChoice->SetRefImage(m_pano,m_leftImageNr);
        m_rightChoice->Refresh();
        m_leftFile = m_pano->getImage(imgNr).getFilename();
        changeState(NO_POINT);
        UpdateDisplay(true);
    }
    m_selectedPoint = UINT_MAX;
    // FIXME: lets hope that nobody holds references to these images..
    ImageCache::getInstance().softFlush();
    UpdateTransforms();
}


void CPEditorPanel::setRightImage(unsigned int imgNr)
{
    DEBUG_TRACE("image " << imgNr);
    if (imgNr == UINT_MAX) {
        m_rightImg->setImage("", CPImageCtrl::ROT0);
        m_rightImageNr = imgNr;
        m_rightFile = "";
        m_rightRot = CPImageCtrl::ROT0;
        changeState(NO_POINT);
        UpdateDisplay(true);
    } else if (m_rightImageNr != imgNr) {
        // set the new image
        double yaw = const_map_get(m_pano->getImageVariables(imgNr),"y").getValue();
        double pitch = const_map_get(m_pano->getImageVariables(imgNr),"p").getValue();
        double roll = const_map_get(m_pano->getImageVariables(imgNr),"r").getValue();
        m_rightRot = GetRot(yaw, pitch, roll);
        m_rightImg->setImage(m_pano->getImage(imgNr).getFilename(), m_rightRot);
        // select tab
        m_rightImageNr = imgNr;
        if (m_rightChoice->GetSelection() != (int) imgNr) {
            m_rightChoice->SetSelection(imgNr);
        }
        m_leftChoice->SetRefImage(m_pano,m_rightImageNr);
        m_leftChoice->Refresh();
        m_rightFile = m_pano->getImage(imgNr).getFilename();
        // update the rest of the display (new control points etc)
        changeState(NO_POINT);
        UpdateDisplay(true);
    }
    m_selectedPoint = UINT_MAX;

    // FIXME: lets hope that nobody holds references to these images..
    ImageCache::getInstance().softFlush();
    UpdateTransforms();
}

void CPEditorPanel::UpdateTransforms()
{
    if(m_leftImageNr<m_pano->getNrOfImages())
    {
        m_leftTransform.createTransform(m_pano->getImage(m_leftImageNr), m_pano->getOptions());
        m_leftInvTransform.createInvTransform(m_pano->getImage(m_leftImageNr), m_pano->getOptions());
    };
    if(m_rightImageNr<m_pano->getNrOfImages())
    {
        m_rightTransform.createTransform(m_pano->getImage(m_rightImageNr), m_pano->getOptions());
        m_rightInvTransform.createInvTransform(m_pano->getImage(m_rightImageNr), m_pano->getOptions());
    };
};

void CPEditorPanel::OnCPEvent( CPEvent&  ev)
{
    DEBUG_TRACE("");
    wxString text;
    unsigned int nr = ev.getPointNr();
    FDiff2D point = ev.getPoint();
    bool left (TRUE);
    if (ev.GetEventObject() == m_leftImg) {
        left = true;
    } else  if (ev.GetEventObject() == m_rightImg){
        left = false;
    } else {
        DEBUG_FATAL("UNKNOWN SOURCE OF CPEvent");
    }

    switch (ev.getMode()) {
    case CPEvent::NONE:
        text = wxT("NONE");
        break;
    case CPEvent::NEW_POINT_CHANGED:
        NewPointChange(ev.getPoint(),left);
        break;
    case CPEvent::POINT_SELECTED:
        // need to reset cpEditState
        DEBUG_DEBUG("selected point " << nr);
        SelectLocalPoint(nr);
        changeState(NO_POINT);
        break;
    case CPEvent::NEW_LINE_ADDED:
        {
            float vertBias = getVerticalCPBias();
            HuginBase::ControlPoint cp = ev.getControlPoint();
            cp.image1Nr=m_leftImageNr;
            cp.image2Nr=m_rightImageNr;
            bool  hor = abs(cp.x1 - cp.x2) > (abs(cp.y1 - cp.y2) * vertBias);
            switch (m_leftRot)
            {
                case CPImageCtrl::ROT0:
                case CPImageCtrl::ROT180:
                    if (hor)
                        cp.mode = HuginBase::ControlPoint::Y;
                    else
                        cp.mode = HuginBase::ControlPoint::X;
                    break;
                default:
                    if (hor)
                        cp.mode = HuginBase::ControlPoint::X;
                    else
                        cp.mode = HuginBase::ControlPoint::Y;
                    break;
            }
            changeState(NO_POINT);
            // create points
            PanoCommand::GlobalCmdHist::getInstance().addCommand(new PanoCommand::AddCtrlPointCmd(*m_pano, cp));
            // select new control Point
            unsigned int lPoint = m_pano->getNrOfCtrlPoints()-1;
            SelectGlobalPoint(lPoint);
            changeState(NO_POINT);
            MainFrame::Get()->SetStatusText(_("new control point added"));
            m_leftChoice->CalcCPDistance(m_pano);
            m_rightChoice->CalcCPDistance(m_pano);
            break;
        };
    case CPEvent::POINT_CHANGED:
        {
            DEBUG_DEBUG("move point("<< nr << ")");
            if (nr >= currentPoints.size()) {
                DEBUG_ERROR("invalid point number while moving point")
                return;
            }
            HuginBase::ControlPoint cp = ev.getControlPoint();
            changeState(NO_POINT);
            DEBUG_DEBUG("changing point to: " << cp.x1 << "," << cp.y1
                        << "  " << cp.x2 << "," << cp.y2);

            PanoCommand::GlobalCmdHist::getInstance().addCommand(
                new PanoCommand::ChangeCtrlPointCmd(*m_pano, currentPoints[nr].first, cp)
                );

            break;
        }
    case CPEvent::RIGHT_CLICK:
        {
            if (cpCreationState == BOTH_POINTS_SELECTED) {
                DEBUG_DEBUG("right click -> adding point");
                CreateNewPoint();
            } else {
                DEBUG_DEBUG("right click without two points..");
                changeState(NO_POINT);
            }
            break;
        }
    case CPEvent::SCROLLED:
        {
            wxPoint d(roundi(point.x), roundi(point.y));
            d = m_rightImg->MaxScrollDelta(d);
            d = m_leftImg->MaxScrollDelta(d);
            m_rightImg->ScrollDelta(d);
            m_leftImg->ScrollDelta(d);
        }
        break;
    case CPEvent::DELETE_REGION_SELECTED:
        {
            UIntSet cpToRemove;
            if(!currentPoints.empty())
            {
                wxRect rect=ev.getRect();
                for(unsigned int i=0;i<currentPoints.size();i++)
                {
                    HuginBase::ControlPoint cp = currentPoints[i].second;
                    if (cp.mode == HuginBase::ControlPoint::X_Y)
                    {
                        //checking only normal control points
                        if(left)
                        {
                            if(rect.Contains(roundi(cp.x1),roundi(cp.y1)))
                            {
                                cpToRemove.insert(localPNr2GlobalPNr(i));
                            };
                        }
                        else
                        {
                            if(rect.Contains(roundi(cp.x2),roundi(cp.y2)))
                            {
                                cpToRemove.insert(localPNr2GlobalPNr(i));
                            };
                        };
                    };
                };
            };
            changeState(NO_POINT);
            if(cpToRemove.size()>0)
            {
                PanoCommand::GlobalCmdHist::getInstance().addCommand(new PanoCommand::RemoveCtrlPointsCmd(*m_pano, cpToRemove));
            };
            break;
        }
    } //end switch
    m_leftImg->update();
    m_rightImg->update();
}


void CPEditorPanel::CreateNewPoint()
{
    DEBUG_TRACE("");
    FDiff2D p1 = m_leftImg->getNewPoint();
    FDiff2D p2 = m_rightImg->getNewPoint();
    HuginBase::ControlPoint point;
    point.image1Nr = m_leftImageNr;
    point.x1 = p1.x;
    point.y1 = p1.y;
    point.image2Nr = m_rightImageNr;
    point.x2 = p2.x;
    point.y2 = p2.y;
    if (point.image1Nr == point.image2Nr) {
        if (m_cpModeChoice->GetSelection()>=3) {
            // keep line until user chooses new mode
            point.mode = m_cpModeChoice->GetSelection();
        } else {
            // Most projections will have a bias to creating vertical
            // constraints.
            float vertBias = getVerticalCPBias();
            bool  hor = abs(p1.x - p2.x) > (abs(p1.y - p2.y) * vertBias);
            switch (m_leftRot) {
                case CPImageCtrl::ROT0:
                case CPImageCtrl::ROT180:
                    if (hor)
                        point.mode = HuginBase::ControlPoint::Y;
                    else
                        point.mode = HuginBase::ControlPoint::X;
                    break;
                default:
                    if (hor)
                        point.mode = HuginBase::ControlPoint::X;
                    else
                        point.mode = HuginBase::ControlPoint::Y;
                    break;
            }
        }
    } else {
        point.mode = HuginBase::ControlPoint::X_Y;
    }

    changeState(NO_POINT);

    // create points
    PanoCommand::GlobalCmdHist::getInstance().addCommand(
        new PanoCommand::AddCtrlPointCmd(*m_pano, point)
        );


    // select new control Point
    unsigned int lPoint = m_pano->getNrOfCtrlPoints() -1;
    SelectGlobalPoint(lPoint);
    changeState(NO_POINT);
    MainFrame::Get()->SetStatusText(_("new control point added"));
    m_leftChoice->CalcCPDistance(m_pano);
    m_rightChoice->CalcCPDistance(m_pano);
}


const float CPEditorPanel::getVerticalCPBias()
{
    HuginBase::PanoramaOptions opts = m_pano->getOptions();
    HuginBase::PanoramaOptions::ProjectionFormat projFormat = opts.getProjection();
    float bias;
    switch (projFormat)
    {
        case HuginBase::PanoramaOptions::RECTILINEAR:
            bias = 1.0;
            break;
        default:
            bias = 2.0;
            break;
    }
    return bias;
}


void CPEditorPanel::ClearSelection()
{
    if (m_selectedPoint == UINT_MAX) {
        // no point selected, no need to select one.
        return;
    }
    m_cpList->SetItemState(m_selectedPoint, 0, wxLIST_STATE_SELECTED);

    m_selectedPoint=UINT_MAX;
    changeState(NO_POINT);
    m_leftImg->deselect();
    m_rightImg->deselect();
    UpdateDisplay(false);
}

void CPEditorPanel::SelectLocalPoint(unsigned int LVpointNr)
{
    DEBUG_TRACE("selectLocalPoint(" << LVpointNr << ")");

    if ( m_selectedPoint == LVpointNr) {
        DEBUG_DEBUG("already selected");
        m_leftImg->selectPoint(LVpointNr);
        m_rightImg->selectPoint(LVpointNr);
        return;
    }
    m_selectedPoint = LVpointNr;

    const HuginBase::ControlPoint & p = currentPoints[LVpointNr].second;
    m_x1Text->SetValue(wxString::Format(wxT("%.2f"),p.x1));
    m_y1Text->SetValue(wxString::Format(wxT("%.2f"),p.y1));
    m_x2Text->SetValue(wxString::Format(wxT("%.2f"),p.x2));
    m_y2Text->SetValue(wxString::Format(wxT("%.2f"),p.y2));
    m_cpModeChoice->SetSelection(p.mode);
    m_leftImg->selectPoint(LVpointNr);
    m_rightImg->selectPoint(LVpointNr);
    m_cpList->SetItemState(LVpointNr, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    m_cpList->EnsureVisible(LVpointNr);

    EnablePointEdit(true);
}

void CPEditorPanel::SelectGlobalPoint(unsigned int globalNr)
{
    unsigned int localNr;
    if (globalPNr2LocalPNr(localNr,globalNr)) {
        DEBUG_DEBUG("CPEditor::setGlobalPoint(" << globalNr << ") found local point " << localNr);
        SelectLocalPoint(localNr);
    } else {
        DEBUG_ERROR("CPEditor::setGlobalPoint: point " << globalNr << " not found in currentPoints");
    }
}

bool CPEditorPanel::globalPNr2LocalPNr(unsigned int & localNr, unsigned int globalNr) const
{
    HuginBase::CPointVector::const_iterator it = currentPoints.begin();

    while(it != currentPoints.end() && (*it).first != globalNr) {
        ++it;
    }

    if (it != currentPoints.end()) {
        localNr = it - currentPoints.begin();
        return true;
    } else {
        return false;
    }
}

unsigned int CPEditorPanel::localPNr2GlobalPNr(unsigned int localNr) const
{
    assert(localNr < currentPoints.size());
    return currentPoints[localNr].first;
}


void CPEditorPanel::estimateAndAddOtherPoint(const FDiff2D & p,
                                             bool left,
                                             CPImageCtrl * thisImg,
                                             unsigned int thisImgNr,
                                             CPCreationState THIS_POINT,
                                             CPCreationState THIS_POINT_RETRY,
                                             CPImageCtrl * otherImg,
                                             unsigned int otherImgNr,
                                             CPCreationState OTHER_POINT,
                                             CPCreationState OTHER_POINT_RETRY)
{
    FDiff2D op;
    op = EstimatePoint(FDiff2D(p.x, p.y), left);
    // check if point is in image.
    const SrcPanoImage & pImg = m_pano->getImage(otherImgNr);
    if (op.x < (int) pImg.getSize().width() && op.x >= 0
        && op.y < (int) pImg.getSize().height() && op.y >= 0)
    {
        otherImg->setNewPoint(op);
        // if fine-tune is checked, run a fine-tune session as well.
        // hmm probably there should be another separate function for this..
        if (m_fineTuneCB->IsChecked()) {
            MainFrame::Get()->SetStatusText(_("searching similar points..."),0);
            FDiff2D newPoint = otherImg->getNewPoint();

            long templWidth = wxConfigBase::Get()->Read(wxT("/Finetune/TemplateSize"), HUGIN_FT_TEMPLATE_SIZE);
            const SrcPanoImage & img = m_pano->getImage(thisImgNr);
            double sAreaPercent = wxConfigBase::Get()->Read(wxT("/Finetune/SearchAreaPercent"),HUGIN_FT_SEARCH_AREA_PERCENT);
            int sWidth = std::min((int)(img.getWidth() * sAreaPercent / 100.0), 500);
            CorrelationResult corrPoint;
            bool corrOk=false;
            Diff2D roundp(p.toDiff2D());
            try {
                corrOk = PointFineTune(thisImgNr,
                                      roundp,
                                      templWidth,
                                      otherImgNr,
                                      newPoint,
                                      sWidth,
                                      corrPoint);
            } catch (std::exception & e) {
                wxMessageBox(wxString (e.what(), wxConvLocal), _("Error during Fine-tune"));
            }
            if (! corrOk) {
                // just set point, PointFineTune already complained
                if (corrPoint.corrPos.x >= 0 && corrPoint.corrPos.y >= 0 && corrPoint.maxpos.x > 0 && corrPoint.maxpos.y > 0)
                {
                    otherImg->setScale(m_detailZoomFactor);
                    otherImg->setNewPoint(corrPoint.maxpos);
                    thisImg->setNewPoint(corrPoint.corrPos);
                    changeState(BOTH_POINTS_SELECTED);
                };
            } else {
                // show point & zoom in if auto add is not set
                if (!m_autoAddCB->IsChecked()) {
                    otherImg->setScale(m_detailZoomFactor);
                    otherImg->setNewPoint(corrPoint.maxpos);
                    thisImg->setNewPoint(corrPoint.corrPos);
                    changeState(BOTH_POINTS_SELECTED);
                    wxString s1;
                    s1.Printf(_("Point fine-tuned, angle: %.0f deg, correlation coefficient: %0.3f, curvature: %0.3f %0.3f "),
                              corrPoint.maxAngle, corrPoint.maxi, corrPoint.curv.x, corrPoint.curv.y );
                    
                    wxString s2 = s1 + wxT(" -- ") + wxString(_("change points, or press right mouse button to add the pair"));
                    MainFrame::Get()->SetStatusText(s2,0);
                } else {
                    // add point
                    otherImg->setNewPoint(corrPoint.maxpos);
                    thisImg->setNewPoint(corrPoint.corrPos);
                    changeState(BOTH_POINTS_SELECTED);
                    CreateNewPoint();
                }
            }
        } else {
            // no fine-tune, set 100% scale and set both points to selected
            otherImg->setScale(m_detailZoomFactor);
            otherImg->showPosition(op);
            changeState(BOTH_POINTS_SELECTED);
        }

    } else {
        // estimate was outside of image
        // do nothing special
        wxBell();
        MainFrame::Get()->SetStatusText(_("Estimated point outside image"),0);
    }
}

void CPEditorPanel::NewPointChange(FDiff2D p, bool left)
{
    DEBUG_TRACE("");

    wxString corrMsg;

    CPImageCtrl * thisImg = m_leftImg;
    unsigned int thisImgNr = m_leftImageNr;
    CPImageCtrl * otherImg = m_rightImg;
    unsigned int otherImgNr = m_rightImageNr;
    CPCreationState THIS_POINT = LEFT_POINT;
    CPCreationState THIS_POINT_RETRY = LEFT_POINT_RETRY;
    CPCreationState OTHER_POINT = RIGHT_POINT;
    CPCreationState OTHER_POINT_RETRY = RIGHT_POINT_RETRY;

    bool estimate = m_estimateCB->IsChecked();

    if (!left) {
        thisImg = m_rightImg;
        thisImgNr = m_rightImageNr;
        otherImg = m_leftImg;
        otherImgNr = m_leftImageNr;
        THIS_POINT = RIGHT_POINT;
        THIS_POINT_RETRY = RIGHT_POINT_RETRY;
        OTHER_POINT = LEFT_POINT;
        OTHER_POINT_RETRY = LEFT_POINT_RETRY;
    }


    if (cpCreationState == NO_POINT) {
        //case NO_POINT
        changeState(THIS_POINT);
        // zoom into our window
        if (thisImg->getScale() < 1) {
            thisImg->setScale(m_detailZoomFactor);
            thisImg->showPosition(p);
        } else {
            // run auto-estimate procedure?
            bool hasNormalCP = false;
            for (HuginBase::CPointVector::const_iterator it = currentPoints.begin(); it != currentPoints.end() && !hasNormalCP; ++it)
            {
                hasNormalCP = (it->second.mode == HuginBase::ControlPoint::X_Y);
            };
            if (estimate && (thisImgNr != otherImgNr) && hasNormalCP) {
                estimateAndAddOtherPoint(p, left,
                                         thisImg, thisImgNr, THIS_POINT, THIS_POINT_RETRY,
                                         otherImg, otherImgNr, OTHER_POINT, OTHER_POINT_RETRY);
            };
        }

    } else if (cpCreationState == OTHER_POINT_RETRY) {
        thisImg->showPosition(p);
    } else if (cpCreationState == THIS_POINT) {
        thisImg->showPosition(p);

        bool hasNormalCP = false;
        for (HuginBase::CPointVector::const_iterator it = currentPoints.begin(); it != currentPoints.end() && !hasNormalCP; ++it)
        {
            hasNormalCP = (it->second.mode == HuginBase::ControlPoint::X_Y);
        };
        if (estimate && (thisImgNr != otherImgNr) && hasNormalCP) {
            estimateAndAddOtherPoint(p, left,
                                     thisImg, thisImgNr, THIS_POINT, THIS_POINT_RETRY,
                                     otherImg, otherImgNr, OTHER_POINT, OTHER_POINT_RETRY);
        }
    } else if (cpCreationState == OTHER_POINT || cpCreationState == THIS_POINT_RETRY) {
        // the try for the second point.
        if (cpCreationState == OTHER_POINT) {
            // other point already selected, finalize point.

            // TODO: option to ignore the auto fine tune button when multiple images are selected.
            if (m_fineTuneCB->IsChecked() ) {
                CorrelationResult corrRes;

                FDiff2D newPoint = otherImg->getNewPoint();

                long templWidth = wxConfigBase::Get()->Read(wxT("/Finetune/TemplateSize"),HUGIN_FT_TEMPLATE_SIZE);
                const SrcPanoImage & img = m_pano->getImage(thisImgNr);
                double sAreaPercent = wxConfigBase::Get()->Read(wxT("/Finetune/SearchAreaPercent"),
                                                                HUGIN_FT_SEARCH_AREA_PERCENT);
                int sWidth = std::min((int) (img.getWidth() * sAreaPercent / 100.0), 500);
                bool corrOk = false;
                // corr point
                Diff2D newPoint_round = newPoint.toDiff2D();
                try {
                    corrOk = PointFineTune(otherImgNr,
                                           newPoint_round,
                                           templWidth,
                                           thisImgNr,
                                           p,
                                           sWidth,
                                           corrRes);
                } catch (std::exception & e) {
                    wxMessageBox(wxString (e.what(), wxConvLocal), _("Error during Fine-tune"));
                }

                if (! corrOk) {
                    // low xcorr
                    // zoom to 100 percent. & set second stage
                    // to abandon finetune this time.
                    if (corrRes.corrPos.x >= 0 && corrRes.corrPos.y >= 0 && corrRes.maxpos.x >= 0 && corrRes.maxpos.y >= 0)
                    {
                        thisImg->setScale(m_detailZoomFactor);
                        thisImg->setNewPoint(corrRes.maxpos);
                        thisImg->update();
                        otherImg->setNewPoint(corrRes.corrPos);
                        changeState(BOTH_POINTS_SELECTED);
                    };
                } else {
                    // show point & zoom in if auto add is not set
                    changeState(BOTH_POINTS_SELECTED);
                    if (!m_autoAddCB->IsChecked()) {
                        thisImg->setScale(m_detailZoomFactor);
                    }
                    thisImg->setNewPoint(corrRes.maxpos);
                    otherImg->setNewPoint(corrRes.corrPos);
                    wxString s1;
                    s1.Printf(_("Point fine-tuned, angle: %.0f deg, correlation coefficient: %0.3f, curvature: %0.3f %0.3f "),
                              corrRes.maxAngle, corrRes.maxi, corrRes.curv.x, corrRes.curv.y );
                    
                    corrMsg = s1 + wxT(" -- ") +  wxString(_("change points, or press right mouse button to add the pair"));
                    MainFrame::Get()->SetStatusText(corrMsg,0);
                    
                }
            } else {
                // no finetune. but zoom into picture, when we where zoomed out
                if (thisImg->getScale() < 1) {
                    // zoom to 100 percent. & set second stage
                    // to abandon finetune this time.
                    thisImg->setScale(m_detailZoomFactor);
                    thisImg->clearNewPoint();
                    thisImg->showPosition(p);
                    //thisImg->setNewPoint(p.x, p.y);
                    changeState(THIS_POINT_RETRY);
                    return;
                } else {
                    // point is already set. no need to move.
                    // setNewPoint(p);
                    changeState(BOTH_POINTS_SELECTED);
                }
            }
        } else {
            // selection retry
            // nothing special, no second stage fine-tune yet.
        }

        // ok, we have determined the other point.. apply if auto add is on
        if (m_autoAddCB->IsChecked()) {
            CreateNewPoint();
        } else {
            // keep both point floating around, until they are
            // added with a right mouse click or the add button
            changeState(BOTH_POINTS_SELECTED);
            if (corrMsg != wxT("")) {
                MainFrame::Get()->SetStatusText(corrMsg,0);
            }
        }

    } else if (cpCreationState == BOTH_POINTS_SELECTED) {
        // nothing to do.. maybe a special fine-tune with
        // a small search region

    } else {
        // should never reach this, else state machine is broken.
        DEBUG_ASSERT(0);
    }
}

// return a SrcPanoImage so that the given point is in the center
SrcPanoImage GetImageRotatedTo(const SrcPanoImage& img, const vigra::Diff2D& point, int testWidth, double& neededHFOV)
{
    // copy only necessary information into temporary SrcPanoImage
    SrcPanoImage imgMod;
    imgMod.setSize(img.getSize());
    imgMod.setProjection(img.getProjection());
    imgMod.setHFOV(img.getHFOV());
    // calculate, where the interest point lies
    HuginBase::PanoramaOptions opt;
    opt.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
    opt.setHFOV(360);
    opt.setWidth(360);
    opt.setHeight(180);

    HuginBase::PTools::Transform transform;
    transform.createInvTransform(imgMod, opt);
    double x1, y1;
    if (!transform.transformImgCoord(x1, y1, point.x, point.y))
    {
        neededHFOV = -1;
        return imgMod;
    }
    // equirect image coordinates -> equirectangular coordinates
    // transformImgCoord places the origin at the upper left corner and negate
    Matrix3 rotY;
    rotY.SetRotationPT(DEG_TO_RAD(180 - (x1 + 0.5)), 0, 0);
    Matrix3 rotP;
    rotP.SetRotationPT(0, DEG_TO_RAD((y1 + 0.5) - 90), 0);
    double y, p, r;
    // calculate the necessary rotation angles and remember
    Matrix3 rot = rotP * rotY;
    rot.GetRotationPT(y, p, r);
    imgMod.setYaw(RAD_TO_DEG(y));
    imgMod.setPitch(RAD_TO_DEG(p));
    imgMod.setRoll(RAD_TO_DEG(r));

    // new we calculate the needed HFOV for template/search area width
    double x2, y2;
    // check a point left from our interest point
    if (transform.transformImgCoord(x2, y2, point.x - testWidth / 2.0, point.y))
    {
        if (x2 < x1)
        {
            neededHFOV = 2.0 * (x1 - x2);
        }
        else
        {
            // we crossed the 360 deg border
            neededHFOV = 2.0 * (360 - x2 + x1);
        };
        // limit maximum HFOV for remapping to stereographic projection done as next step
        if (neededHFOV > 90)
        {
            neededHFOV = 90;
        };
        return imgMod;
    };
    // this goes wrong, maybe the tested point is outside the image area of a fisheye image
    // now test the right point
    if (transform.transformImgCoord(x2, y2, point.x + testWidth / 2.0, point.y))
    {
        if (x1 < x2)
        {
            neededHFOV = 2.0 * (x2 - x1);
        }
        else
        {
            // we crossed the 360 deg border
            neededHFOV = 2.0 * (360 + x2 - x1);
        };
        // limit maximum HFOV for remapping to stereographic projection done as next step
        if (neededHFOV > 90)
        {
            neededHFOV = 90;
        };
        return imgMod;
    };
    // we can't calculate the needed HFOV, return -1
    neededHFOV = -1;
    return imgMod;
};

CorrelationResult PointFineTuneProjectionAware(const SrcPanoImage& templ, const vigra::UInt8RGBImage& templImg,
    vigra::Diff2D templPos, int templSize,
    const SrcPanoImage& search, const vigra::UInt8RGBImage& searchImg,
    vigra::Diff2D searchPos, int sWidth)
{
    wxBusyCursor busy;
    // read settings
    wxConfigBase *cfg = wxConfigBase::Get();
    bool rotatingFinetune = cfg->Read(wxT("/Finetune/RotationSearch"), HUGIN_FT_ROTATION_SEARCH) == 1;
    double startAngle = HUGIN_FT_ROTATION_START_ANGLE;
    cfg->Read(wxT("/Finetune/RotationStartAngle"), &startAngle, HUGIN_FT_ROTATION_START_ANGLE);
    startAngle = DEG_TO_RAD(startAngle);
    double stopAngle = HUGIN_FT_ROTATION_STOP_ANGLE;
    cfg->Read(wxT("/Finetune/RotationStopAngle"), &stopAngle, HUGIN_FT_ROTATION_STOP_ANGLE);
    stopAngle = DEG_TO_RAD(stopAngle);
    int nSteps = cfg->Read(wxT("/Finetune/RotationSteps"), HUGIN_FT_ROTATION_STEPS);
    // if both images have the same projection and the angle does not differ to much use normal point fine-tune
    if (templ.getProjection() == search.getProjection()
        && templ.getHFOV() < 65 && search.getHFOV() < 65
        && fabs(templ.getHFOV() - search.getHFOV()) < 5)
    {
        CorrelationResult res;
        if (rotatingFinetune)
        {
            res = vigra_ext::PointFineTuneRotSearch(templImg, templPos, templSize,
                searchImg, searchPos, sWidth, startAngle, stopAngle, nSteps);
        }
        else
        {
            res = vigra_ext::PointFineTune(templImg, vigra::RGBToGrayAccessor<RGBValue<UInt8> >(), templPos, templSize,
                searchImg, vigra::RGBToGrayAccessor<RGBValue<UInt8> >(), searchPos, sWidth);
        };
        res.corrPos = templPos;
        return res;
    };
    // images have different projections or the HFOV is different
    // so we reproject the image to stereographic projection and fine tune point there
    // rotate image so that interest point is in the center
    double templHFOV = 0;
    double searchHFOV = 0;
    SrcPanoImage templMod = GetImageRotatedTo(templ, templPos, templSize, templHFOV);
    SrcPanoImage searchMod = GetImageRotatedTo(search, searchPos, sWidth + templSize + 5, searchHFOV);
    CorrelationResult res;
    res.maxpos = FDiff2D(-1, -1);
    res.corrPos = FDiff2D(-1, -1);
    if (templHFOV < 0 || searchHFOV < 0)
    {
        //something went wrong, e.g. image outside of projection circle for fisheye lenses
        return res;
    }
    // populate PanoramaOptions
    HuginBase::PanoramaOptions opts;
    opts.setProjection(HuginBase::PanoramaOptions::STEREOGRAPHIC);
    opts.setHFOV(std::max(templHFOV, searchHFOV));
    // calculate a sensible scale factor
    double scaleTempl = HuginBase::CalculateOptimalScale::calcOptimalPanoScale(templMod, opts);
    double scaleSearch = HuginBase::CalculateOptimalScale::calcOptimalPanoScale(searchMod, opts);
    opts.setWidth(std::max<unsigned int>(opts.getWidth()*std::min(scaleTempl, scaleSearch), 3 * templSize));
    opts.setHeight(opts.getWidth());
    // transform coordinates to transform system
    HuginBase::PTools::Transform transform;
    transform.createInvTransform(templMod, opts);
    double templX, templY, searchX, searchY;
    transform.transformImgCoord(templX, templY, templPos.x, templPos.y);
    transform.createInvTransform(searchMod, opts);
    transform.transformImgCoord(searchX, searchY, searchPos.x, searchPos.y);
    // now transform the images
    vigra_ext::PassThroughFunctor<vigra::UInt8> ptf;
    AppBase::DummyProgressDisplay dummy;
    transform.createTransform(searchMod, opts);
    vigra::UInt8RGBImage searchImgMod(opts.getSize());
    vigra::BImage alpha(opts.getSize());
    vigra_ext::transformImage(srcImageRange(searchImg), destImageRange(searchImgMod), destImage(alpha),
        vigra::Diff2D(0, 0), transform, ptf, false, vigra_ext::INTERP_CUBIC, &dummy);
    // now remap template, we need to remap a little bigger area to have enough information when the template
    // is rotated in PointFineTuneRotSearch
    Diff2D templPointInt(hugin_utils::roundi(templX), hugin_utils::roundi(templY));
    vigra::Rect2D rect(templPointInt.x - templSize - 2, templPointInt.y - templSize - 2,
        templPointInt.x + templSize + 2, templPointInt.y + templSize + 2);
    rect &= vigra::Rect2D(opts.getSize());
    transform.createTransform(templMod, opts);
    vigra::UInt8RGBImage templImgMod(opts.getSize());
    vigra_ext::transformImage(srcImageRange(templImg), destImageRange(templImgMod, rect), destImage(alpha),
        vigra::Diff2D(rect.left(), rect.top()), transform, ptf, false, vigra_ext::INTERP_CUBIC, &dummy);
#if defined DEBUG_EXPORT_FINE_TUNE_REMAPPING
    {
        vigra::ImageExportInfo templExport("template_remapped.tif");
        vigra::exportImage(srcImageRange(templImgMod), templExport.setPixelType("UINT8"));
        vigra::ImageExportInfo searchExport("search_remapped.tif");
        vigra::exportImage(srcImageRange(searchImgMod), searchExport.setPixelType("UINT8"));
    }
#endif
    // now we can finetune the point in stereographic projection
    // we are always using the rotate fine-tune algorithm, because for this case
    // often a rotation is involved
    res = vigra_ext::PointFineTuneRotSearch(templImgMod, templPointInt, templSize,
        searchImgMod, Diff2D(hugin_utils::roundi(searchX), hugin_utils::roundi(searchY)), sWidth, startAngle, stopAngle, nSteps);
    // we transfer also the new found template position back to the original image
    transform.createTransform(templMod, opts);
    transform.transformImgCoord(res.corrPos.x, res.corrPos.y, templPointInt.x + 0.00001, templPointInt.y + 0.00001);
    // we need to move the finetune point back to position in original image
    transform.createTransform(searchMod, opts);
    transform.transformImgCoord(res.maxpos.x, res.maxpos.y, res.maxpos.x, res.maxpos.y);
    return res;
};

bool CPEditorPanel::PointFineTune(unsigned int tmplImgNr,
                                  const Diff2D & tmplPoint,
                                  int templSize,
                                  unsigned int subjImgNr,
                                  const FDiff2D & o_subjPoint,
                                  int sWidth,
                                  CorrelationResult & res)
{
    DEBUG_TRACE("tmpl img nr: " << tmplImgNr << " corr src: "
                << subjImgNr);

    MainFrame::Get()->SetStatusText(_("searching similar points..."),0);

    double corrThresh=HUGIN_FT_CORR_THRESHOLD;
    wxConfigBase::Get()->Read(wxT("/Finetune/CorrThreshold"),&corrThresh,
                              HUGIN_FT_CORR_THRESHOLD);

    double curvThresh = HUGIN_FT_CURV_THRESHOLD;
    wxConfigBase::Get()->Read(wxT("/Finetune/CurvThreshold"),&curvThresh,
                              HUGIN_FT_CURV_THRESHOLD);

    // fixme: just cutout suitable gray 
    ImageCache::ImageCacheRGB8Ptr subjImg = ImageCache::getInstance().getImage(m_pano->getImage(subjImgNr).getFilename())->get8BitImage();
    ImageCache::ImageCacheRGB8Ptr tmplImg = ImageCache::getInstance().getImage(m_pano->getImage(tmplImgNr).getFilename())->get8BitImage();

    res = PointFineTuneProjectionAware(m_pano->getImage(tmplImgNr), *(tmplImg), tmplPoint, templSize,
        m_pano->getImage(subjImgNr), *(subjImg), o_subjPoint.toDiff2D(), sWidth);

    // invert curvature. we always assume its a maxima, the curvature there is negative
    // however, we allow the user to specify a positive threshold, so we need to
    // invert it
    res.curv.x = - res.curv.x;
    res.curv.y = - res.curv.y;

    MainFrame::Get()->SetStatusText(wxString::Format(_("Point fine-tuned, angle: %.0f deg, correlation coefficient: %0.3f, curvature: %0.3f %0.3f "),
                                    res.maxAngle, res.maxi, res.curv.x, res.curv.y ),0);
    if (res.corrPos.x < 0 || res.corrPos.y < 0 || res.maxpos.x < 0 || res.maxpos.y < 0)
    {
        // invalid transformation in fine tune
#if wxCHECK_VERSION(2, 9, 0)
        wxMessageDialog dlg(this,
            _("No similar point found."),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxICON_ERROR | wxOK);
        dlg.SetExtendedMessage(_("An internal transformation went wrong.\nCheck that the point is inside the image."));
        dlg.ShowModal();
#else
        wxMessageBox(_("An internal transformation went wrong.\nCheck that the point is inside the image."),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxICON_ERROR | wxOK, this);
#endif
        return false;
    }
    if (res.maxi < corrThresh || res.curv.x < curvThresh || res.curv.y < curvThresh )
    {
        // Bad correlation result.
#if wxCHECK_VERSION(2, 9, 0)
        wxMessageDialog dlg(this,
            _("No similar point found."),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxICON_ERROR | wxOK);
        dlg.SetExtendedMessage(wxString::Format(_("Check the similarity visually.\nCorrelation coefficient (%.3f) is lower than the threshold set in the preferences."),
                             res.maxi));
        dlg.ShowModal();
#else
        wxMessageBox(
            wxString::Format(_("No similar point found. Check the similarity visually.\nCorrelation coefficient (%.3f) is lower than the threshold set in the preferences."),
                             res.maxi),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxICON_ERROR | wxOK, this);
#endif
        return false;
    }

    return true;
}

void CPEditorPanel::panoramaChanged(Panorama &pano)
{
    int nGui = m_cpModeChoice->GetCount();
    int nPano = pano.getNextCPTypeLineNumber()+1;
    DEBUG_DEBUG("mode choice: " << nGui << " entries, required: " << nPano);

    if (nGui > nPano)
    {
        m_cpModeChoice->Freeze();
        // remove some items.
        for (int i = nGui-1; i >=nPano-1; --i) {
            m_cpModeChoice->Delete(i);
        }
        if (nPano > 3) {
            m_cpModeChoice->SetString(nPano-1, _("Add new Line"));
        }
        m_cpModeChoice->Thaw();
    } else if (nGui < nPano) {
        m_cpModeChoice->Freeze();
        if (nGui > 3) {
            m_cpModeChoice->SetString(nGui-1, wxString::Format(_("Line %d"), nGui-1));
        }
        for (int i = nGui; i < nPano-1; i++) {
            m_cpModeChoice->Append(wxString::Format(_("Line %d"), i));
        }
        m_cpModeChoice->Append(_("Add new Line"));
        m_cpModeChoice->Thaw();
    }
    UpdateTransforms();
    // check if number of control points has changed, if so we need to update our variables
    if (pano.getNrOfCtrlPoints() != m_countCP)
    {
        m_countCP = pano.getNrOfCtrlPoints();
        UpdateDisplay(false);
    };
    DEBUG_TRACE("");
}

void CPEditorPanel::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrTabs = m_leftChoice->GetCount();
    DEBUG_TRACE("nrImages:" << nrImages << " nrTabs:" << nrTabs);

#ifdef __WXMSW__
    int oldLeftSelection = m_leftChoice->GetSelection();
    int oldRightSelection = m_rightChoice->GetSelection();
#endif

    if (nrImages == 0)
    {
        // disable controls
        m_cpModeChoice->Disable();
        m_addButton->Disable();
        m_delButton->Disable();
        m_autoAddCB->Disable();
        m_fineTuneCB->Disable();
        m_estimateCB->Disable();
        XRCCTRL(*this, "cp_editor_finetune_button", wxButton)->Disable();
        m_actionButton->Disable();
        XRCCTRL(*this, "cp_editor_choice_zoom", wxChoice)->Disable();
        XRCCTRL(*this, "cp_editor_previous_img", wxButton)->Disable();
        XRCCTRL(*this, "cp_editor_next_img", wxButton)->Disable();
        m_leftChoice->Disable();
        m_rightChoice->Disable();
    }
    else
    {
        // enable controls
        m_cpModeChoice->Enable();
        m_autoAddCB->Enable();
        m_fineTuneCB->Enable();
        m_estimateCB->Enable();
        XRCCTRL(*this, "cp_editor_finetune_button", wxButton)->Enable();
        m_actionButton->Enable();
        XRCCTRL(*this, "cp_editor_choice_zoom", wxChoice)->Enable();
        XRCCTRL(*this, "cp_editor_previous_img", wxButton)->Enable();
        XRCCTRL(*this, "cp_editor_next_img", wxButton)->Enable();
        m_leftChoice->Enable();
        m_rightChoice->Enable();

        ImageCache::getInstance().softFlush();

        for (unsigned int i=0; i < ((nrTabs < nrImages)? nrTabs: nrImages); i++) {
            wxFileName fileName(wxString (pano.getImage(i).getFilename().c_str(), HUGIN_CONV_FILENAME));
            m_leftChoice->SetString(i, wxString::Format(wxT("%d"), i) + wxT(". - ") + fileName.GetFullName());
            m_rightChoice->SetString(i, wxString::Format(wxT("%d"), i) + wxT(". - ") + fileName.GetFullName());
        }
        // wxChoice on windows looses the selection when setting new labels. Restore selection
#ifdef __WXMSW__
        m_leftChoice->SetSelection(oldLeftSelection);
        m_rightChoice->SetSelection(oldRightSelection);
#endif
        // add tab bar entries, if needed
        if (nrTabs < nrImages)
        {
            for (unsigned int i=nrTabs; i < nrImages; i++)
            {
                wxFileName fileName(wxString (pano.getImage(i).getFilename().c_str(), HUGIN_CONV_FILENAME));
                m_leftChoice->Append(wxString::Format(wxT("%d"), i) + wxT(". - ") + fileName.GetFullName());
                m_rightChoice->Append(wxString::Format(wxT("%d"), i) + wxT(". - ") + fileName.GetFullName());
            }
        }
    }
    if (nrTabs > nrImages)
    {
        // remove tab bar entries if needed
        // we have to disable listening to notebook selection events,
        // else we might update to a noexisting image
        m_listenToPageChange = false;
        for (int i=nrTabs-1; i >= (int)nrImages; i--) {
            m_leftChoice->Delete(i);
            m_rightChoice->Delete(i);
        }
        m_listenToPageChange = true;
        if (nrImages > 0) {
            // select some other image if we deleted the current image
            if (m_leftImageNr >= nrImages) {
                setLeftImage(nrImages -1);
            }
            if (m_rightImageNr >= nrImages) {
                setRightImage(nrImages -1);
            }
        } else {
            DEBUG_DEBUG("setting no images");
            m_leftImageNr = UINT_MAX;
            m_leftFile = "";
            m_rightImageNr = UINT_MAX;
            m_rightFile = "";
            // no image anymore..
            m_leftImg->setImage(m_leftFile, CPImageCtrl::ROT0);
            m_rightImg->setImage(m_rightFile, CPImageCtrl::ROT0);
        }
    }

    // update changed images
    bool update(false);
    for(UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it) {
        unsigned int imgNr = *it;
        // we only need to update the view if the currently
        // selected images were changed.
        // changing the images via the tabbar will always
        // take the current state directly from the pano
        // object
        DEBUG_DEBUG("image changed "<< imgNr);
        double yaw = const_map_get(m_pano->getImageVariables(imgNr), "y").getValue();
        double pitch = const_map_get(m_pano->getImageVariables(imgNr), "p").getValue();
        double roll = const_map_get(m_pano->getImageVariables(imgNr), "r").getValue();
        CPImageCtrl::ImageRotation rot = GetRot(yaw, pitch, roll);
        if (m_leftImageNr == imgNr) {
            DEBUG_DEBUG("left image dirty "<< imgNr);
            if (m_leftFile != pano.getImage(imgNr).getFilename()
                || m_leftRot != rot ) 
            {
                m_leftRot = rot;
                m_leftFile = pano.getImage(imgNr).getFilename();
                m_leftImg->setImage(m_leftFile, m_leftRot);
            }
            update=true;
        }

        if (m_rightImageNr == imgNr) {
            DEBUG_DEBUG("right image dirty "<< imgNr);
            if (m_rightFile != pano.getImage(imgNr).getFilename()
                 || m_rightRot != rot ) 
            {
                m_rightRot = rot;
                m_rightFile = pano.getImage(imgNr).getFilename();
                m_rightImg->setImage(m_rightFile, m_rightRot);
            }
            update=true;
        }
    }
    // check if number of control points has changed, if so we need to update our variables
    if (pano.getNrOfCtrlPoints() != m_countCP)
    {
        m_countCP = pano.getNrOfCtrlPoints();
        update = true;
    };

    // if there is no selection, select the first one.
    if (m_rightImageNr == UINT_MAX && nrImages > 0) {
        setRightImage(0);
    }
    if (m_leftImageNr == UINT_MAX && nrImages > 0) {
        setLeftImage(0);
    }

    if (update || nrImages == 0) {
        UpdateDisplay(false);
    }
    m_leftChoice->CalcCPDistance(m_pano);
    m_rightChoice->CalcCPDistance(m_pano);
}

void CPEditorPanel::UpdateDisplay(bool newPair)
{
    DEBUG_DEBUG("")
    int fI = m_leftChoice->GetSelection();
    int sI = m_rightChoice->GetSelection();

    // valid selection and already set left image
    if (fI >= 0 && m_leftImageNr != UINT_MAX)
    {
        // set image number to selection
        m_leftImageNr = (unsigned int) fI;
    }
    // valid selection and already set right image
    if (sI >= 0 && m_rightImageNr != UINT_MAX)
    {
        // set image number to selection
        m_rightImageNr = (unsigned int) sI;
    }
    // reset selection
    m_x1Text->Clear();
    m_y1Text->Clear();
    m_x2Text->Clear();
    m_y2Text->Clear();
    if (m_cpModeChoice->GetSelection() < 3) {
        m_cpModeChoice->SetSelection(0);
    }

    m_leftImg->setSameImage(m_leftImageNr==m_rightImageNr);
    m_rightImg->setSameImage(m_leftImageNr==m_rightImageNr);

    // update control points
    const HuginBase::CPVector & controlPoints = m_pano->getCtrlPoints();
    currentPoints.clear();
    mirroredPoints.clear();

    // create a list of all control points
    HuginBase::CPVector::size_type i = 0;
    m_leftImg->clearCtrlPointList();
    m_rightImg->clearCtrlPointList();
    for (HuginBase::CPVector::size_type index = 0; index < controlPoints.size(); ++index)
    {
        HuginBase::ControlPoint point(controlPoints[index]);
        if ((point.image1Nr == m_leftImageNr) && (point.image2Nr == m_rightImageNr)){
            m_leftImg->setCtrlPoint(point, false);
            m_rightImg->setCtrlPoint(point, true);
            currentPoints.push_back(make_pair(index, point));
            i++;
        } else if ((point.image2Nr == m_leftImageNr) && (point.image1Nr == m_rightImageNr)){
            m_leftImg->setCtrlPoint(point, true);
            m_rightImg->setCtrlPoint(point, false);
            point.mirror();
            mirroredPoints.insert(i);
            currentPoints.push_back(std::make_pair(index, point));
            i++;
        }
    }
    m_leftImg->update();
    m_rightImg->update();

    // put these control points into our listview.
    unsigned int selectedCP = UINT_MAX;
    for ( int i=0; i < m_cpList->GetItemCount() ; i++ ) {
      if ( m_cpList->GetItemState( i, wxLIST_STATE_SELECTED ) ) {
        selectedCP = i;            // remembers the old selection
      }
    }
    m_cpList->Freeze();
    m_cpList->DeleteAllItems();

    for (unsigned int i=0; i < currentPoints.size(); ++i) {
        const HuginBase::ControlPoint & p(currentPoints[i].second);
        DEBUG_DEBUG("inserting LVItem " << i);
        m_cpList->InsertItem(i,wxString::Format(wxT("%d"),i));
        m_cpList->SetItem(i,1,wxString::Format(wxT("%.2f"),p.x1));
        m_cpList->SetItem(i,2,wxString::Format(wxT("%.2f"),p.y1));
        m_cpList->SetItem(i,3,wxString::Format(wxT("%.2f"),p.x2));
        m_cpList->SetItem(i,4,wxString::Format(wxT("%.2f"),p.y2));
        wxString mode;
        switch (p.mode) {
        case HuginBase::ControlPoint::X_Y:
            mode = _("normal");
            break;
        case HuginBase::ControlPoint::X:
            mode = _("vert. Line");
            break;
        case HuginBase::ControlPoint::Y:
            mode = _("horiz. Line");
            break;
        default:
            mode = wxString::Format(_("Line %d"), p.mode);
            break;
        }
        m_cpList->SetItem(i,5,mode);
        m_cpList->SetItem(i,6,wxString::Format(wxT("%.2f"),p.error));
    }

    if ( selectedCP < (unsigned int) m_cpList->GetItemCount() && ! newPair) {
        // sets an old selection again, only if the images have not changed
        m_cpList->SetItemState( selectedCP,
                                wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
        m_cpList->EnsureVisible(selectedCP);
        m_selectedPoint = selectedCP;
        EnablePointEdit(true);

        const HuginBase::ControlPoint & p = currentPoints[m_selectedPoint].second;
        m_x1Text->SetValue(wxString::Format(wxT("%.2f"),p.x1));
        m_y1Text->SetValue(wxString::Format(wxT("%.2f"),p.y1));
        m_x2Text->SetValue(wxString::Format(wxT("%.2f"),p.x2));
        m_y2Text->SetValue(wxString::Format(wxT("%.2f"),p.y2));
        m_cpModeChoice->SetSelection(p.mode);
        m_leftImg->selectPoint(m_selectedPoint);
        m_rightImg->selectPoint(m_selectedPoint);

    } else {
        m_selectedPoint = UINT_MAX;
        EnablePointEdit(false);
    }

    for ( int j=0; j < m_cpList->GetColumnCount() ; j++ )
    {
        //get saved width
        // -1 is auto
        int width = wxConfigBase::Get()->Read(wxString::Format( wxT("/CPEditorPanel/ColumnWidth%d"), j ), -1);
        if(width != -1)
            m_cpList->SetColumnWidth(j, width);
    }

    m_cpList->Thaw();
}

void CPEditorPanel::EnablePointEdit(bool state)
{
    m_delButton->Enable(state);
    XRCCTRL(*this, "cp_editor_finetune_button", wxButton)->Enable(state);
    m_x1Text->Enable(state);
    m_y1Text->Enable(state);
    m_x2Text->Enable(state);
    m_y2Text->Enable(state);
    m_cpModeChoice->Enable(state);
}

void CPEditorPanel::OnTextPointChange(wxCommandEvent &e)
{
    DEBUG_TRACE("");
    // find selected point
    long item = -1;
    item = m_cpList->GetNextItem(item,
                                 wxLIST_NEXT_ALL,
                                 wxLIST_STATE_SELECTED);
    // no selected item.
    if (item == -1) {
        return;
    }
    unsigned int nr = (unsigned int) item;
    assert(nr < currentPoints.size());
    HuginBase::ControlPoint cp = currentPoints[nr].second;

    // update point state
    double oldValue=cp.x1;
    bool valid_input=str2double(m_x1Text->GetValue(), cp.x1);
    if(valid_input)
        valid_input=(cp.x1>=0) && (cp.x1<=m_pano->getSrcImage(cp.image1Nr).getWidth());
    if (!valid_input) {
        m_x1Text->Clear();
        *m_x1Text << oldValue;
        return;
    }
    oldValue=cp.y1;
    valid_input=str2double(m_y1Text->GetValue(), cp.y1);
    if(valid_input)
        valid_input=(cp.y1>=0) && (cp.y1<=m_pano->getSrcImage(cp.image1Nr).getHeight());
    if (!valid_input) {
        m_y1Text->Clear();
        *m_y1Text << oldValue;
        return;
    }
    oldValue=cp.x2;
    valid_input=str2double(m_x2Text->GetValue(), cp.x2);
    if(valid_input)
        valid_input=(cp.x2>=0) && (cp.x2<=m_pano->getSrcImage(cp.image2Nr).getWidth());
    if (!valid_input) {
        m_x2Text->Clear();
        *m_x2Text << oldValue;
        return;
    }
    oldValue=cp.y2;
    valid_input=str2double(m_y2Text->GetValue(), cp.y2);
    if(valid_input)
        valid_input=(cp.y2>=0) && (cp.y2<=m_pano->getSrcImage(cp.image1Nr).getHeight());
    if (!valid_input) {
        m_y2Text->Clear();
        *m_y2Text << oldValue;
        return;
    }

    cp.mode = m_cpModeChoice->GetSelection();
    // if point was mirrored, reverse before setting it.
    if (set_contains(mirroredPoints, nr)) {
        cp.mirror();
    }
    PanoCommand::GlobalCmdHist::getInstance().addCommand(
        new PanoCommand::ChangeCtrlPointCmd(*m_pano, currentPoints[nr].first, cp)
        );

}

void CPEditorPanel::OnLeftChoiceChange(wxCommandEvent & e)
{
    DEBUG_TRACE("OnLeftChoiceChange() to " << e.GetSelection());
    if (m_listenToPageChange && e.GetSelection() >= 0) {
        setLeftImage((unsigned int) e.GetSelection());
    }
}

void CPEditorPanel::OnRightChoiceChange(wxCommandEvent & e)
{
    DEBUG_TRACE("OnRightChoiceChange() to " << e.GetSelection());
    if (m_listenToPageChange && e.GetSelection() >= 0) {
        setRightImage((unsigned int) e.GetSelection());
    }
}
void CPEditorPanel::OnCPListSelect(wxListEvent & ev)
{
    int t = ev.GetIndex();
    DEBUG_TRACE("selected: " << t);
    if (t >=0) {
        SelectLocalPoint((unsigned int) t);
        changeState(NO_POINT);
    }
    EnablePointEdit(true);
}

void CPEditorPanel::OnCPListDeselect(wxListEvent & ev)
{
    // disable controls
    // when doing changes to this procedure do also check
    // interaction with control point table
    // e.g. m_selectedPoint=UINT_MAX will result in a endless loop and crash
    changeState(NO_POINT);
    EnablePointEdit(false);
    m_leftImg->deselect();
    m_rightImg->deselect();
}

void CPEditorPanel::OnZoom(wxCommandEvent & e)
{
    double factor;
    switch (e.GetSelection()) {
    case 0:
        factor = 1;
        m_detailZoomFactor = factor;
        break;
    case 1:
        // fit to window
        factor = 0;
        break;
    case 2:
        factor = 2;
        m_detailZoomFactor = factor;
        break;
    case 3:
        factor = 1.5;
        m_detailZoomFactor = factor;
        break;
    case 4:
        factor = 0.75;
        break;
    case 5:
        factor = 0.5;
        break;
    case 6:
        factor = 0.25;
        break;
    default:
        DEBUG_ERROR("unknown scale factor");
        factor = 1;
    }
    m_leftImg->setScale(factor);
    m_rightImg->setScale(factor);
    // if a point is selected, keep it in view
    if (m_selectedPoint < UINT_MAX) {
        SelectLocalPoint(m_selectedPoint);
    }
}

void CPEditorPanel::OnKey(wxKeyEvent & e)
{
    DEBUG_DEBUG("key " << e.GetKeyCode()
                << " origin: id:" << e.GetId() << " obj: "
                << e.GetEventObject());

    if (e.m_keyCode == WXK_DELETE){
        DEBUG_DEBUG("Delete pressed");
        // remove working points..
        if (cpCreationState != NO_POINT) {
            changeState(NO_POINT);
        } else {
            // remove selected point
            // find selected point
            long item = -1;
            item = m_cpList->GetNextItem(item,
                                         wxLIST_NEXT_ALL,
                                         wxLIST_STATE_SELECTED);
            // no selected item.
            if (item == -1) {
                wxBell();
                return;
            }
            unsigned int pNr = localPNr2GlobalPNr((unsigned int) item);
            DEBUG_DEBUG("about to delete point " << pNr);
            PanoCommand::GlobalCmdHist::getInstance().addCommand(
                new PanoCommand::RemoveCtrlPointCmd(*m_pano,pNr)
                );
        }
    } else if (e.m_keyCode == '0') {
        wxCommandEvent dummy;
        dummy.SetInt(1);
        OnZoom(dummy);
        XRCCTRL(*this,"cp_editor_choice_zoom",wxChoice)->SetSelection(1);
    } else if (e.m_keyCode == '1') {
        wxCommandEvent dummy;
        dummy.SetInt(0);
        OnZoom(dummy);
        XRCCTRL(*this,"cp_editor_choice_zoom",wxChoice)->SetSelection(0);
    } else if (e.m_keyCode == '2') {
        wxCommandEvent dummy;
        dummy.SetInt(2);
        OnZoom(dummy);
        XRCCTRL(*this,"cp_editor_choice_zoom",wxChoice)->SetSelection(2);
    } else if (e.CmdDown() && e.GetKeyCode() == WXK_LEFT) {
        // move to previous
        wxCommandEvent dummy;
        OnPrevImg(dummy);
    } else if (e.CmdDown() && e.GetKeyCode() == WXK_RIGHT) {
        // move to next
        wxCommandEvent dummy;
        OnNextImg(dummy);
    } else if (e.GetKeyCode() == 'f') {
        bool left =  e.GetEventObject() == m_leftImg;
        if (cpCreationState == NO_POINT) {
            FineTuneSelectedPoint(left);
        } else if (cpCreationState == BOTH_POINTS_SELECTED) { 
            FineTuneNewPoint(left);
        }
    } else if (e.GetKeyCode() == 'g') {
        // generate keypoints
        long th = wxGetNumberFromUser(_("Create control points.\nTo create less points,\nenter a higher number."), _("Corner Detection threshold"), _("Create control points"), 400, 0, 32000);
        if (th == -1) {
            return;
        }
        long scale = wxGetNumberFromUser(_("Create control points"), _("Corner Detection scale"), _("Create control points"), 2);
        if (scale == -1) {
            return;
        }

        try {
            wxBusyCursor busy;
            DEBUG_DEBUG("corner threshold: " << th << "  scale: " << scale);
            PanoCommand::GlobalCmdHist::getInstance().addCommand(
                new PanoCommand::wxAddCtrlPointGridCmd(*m_pano, m_leftImageNr, m_rightImageNr, scale, th)
                            );
        } catch (std::exception & e) {
            wxLogError(_("Error during control point creation:\n") + wxString(e.what(), wxConvLocal));
        }
    } else {
        e.Skip();
    }
}

void CPEditorPanel::OnAddButton(wxCommandEvent & e)
{
    // check if the point can be created..
    if (cpCreationState == BOTH_POINTS_SELECTED) {
        CreateNewPoint();
    }
}

void CPEditorPanel::OnDeleteButton(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // check if a point has been selected, but not added.
    if (cpCreationState != NO_POINT) {
        changeState(NO_POINT);
    } else {
        // find selected point
        long item = -1;
        item = m_cpList->GetNextItem(item,
                                     wxLIST_NEXT_ALL,
                                     wxLIST_STATE_SELECTED);
        // no selected item.
        if (item == -1) {
            wxBell();
            return;
        }
        // get the global point number
        unsigned int pNr = localPNr2GlobalPNr((unsigned int) item);

        PanoCommand::GlobalCmdHist::getInstance().addCommand(
            new PanoCommand::RemoveCtrlPointCmd(*m_pano,pNr )
            );
        m_leftChoice->CalcCPDistance(m_pano);
        m_rightChoice->CalcCPDistance(m_pano);
    }
}

// show a global control point
void CPEditorPanel::ShowControlPoint(unsigned int cpNr)
{
    const HuginBase::ControlPoint & p = m_pano->getCtrlPoint(cpNr);
    setLeftImage(p.image1Nr);
    setRightImage(p.image2Nr);
    // FIXME reset display state
    changeState(NO_POINT);

    SelectGlobalPoint(cpNr);
}

void CPEditorPanel::changeState(CPCreationState newState)
{
    DEBUG_TRACE(cpCreationState << " --> " << newState);
    // handle global state changes.
    bool fineTune = m_fineTuneCB->IsChecked() && (m_leftImageNr != m_rightImageNr);
    switch(newState) {
    case NO_POINT:
        // disable all drawing search boxes.
        m_leftImg->showSearchArea(false);
        m_rightImg->showSearchArea(false);
        // but draw template size, if fine-tune enabled
        m_leftImg->showTemplateArea(fineTune);
        m_rightImg->showTemplateArea(fineTune);
        m_addButton->Enable(false);
        if (m_selectedPoint < UINT_MAX) {
            m_delButton->Enable(true);
        } else {
            m_delButton->Enable(false);
        }
        if (cpCreationState != NO_POINT) {
            // reset zoom to previous setting
            wxCommandEvent tmpEvt;
            tmpEvt.SetInt(XRCCTRL(*this,"cp_editor_choice_zoom",wxChoice)->GetSelection());
            OnZoom(tmpEvt);
            m_leftImg->clearNewPoint();
            m_rightImg->clearNewPoint();
        }
        break;
    case LEFT_POINT:
        // disable search area on left window
        m_leftImg->showSearchArea(false);
        // show search area on right window
        m_rightImg->showSearchArea(fineTune);

        // show template area
        m_leftImg->showTemplateArea(fineTune);
        m_rightImg->showTemplateArea(false);

        // unselect point
        ClearSelection();
        m_addButton->Enable(false);
        m_delButton->Enable(false);
        MainFrame::Get()->SetStatusText(_("Select point in right image"),0);
        break;
    case RIGHT_POINT:
        m_leftImg->showSearchArea(fineTune);
        m_rightImg->showSearchArea(false);

        m_leftImg->showTemplateArea(false);
        m_rightImg->showTemplateArea(fineTune);

        ClearSelection();
        m_addButton->Enable(false);
        m_delButton->Enable(false);
        MainFrame::Get()->SetStatusText(_("Select point in left image"),0);
        break;
    case LEFT_POINT_RETRY:
    case RIGHT_POINT_RETRY:
        m_leftImg->showSearchArea(false);
        m_rightImg->showSearchArea(false);
        // but draw template size, if fine-tune enabled
        m_leftImg->showTemplateArea(false);
        m_rightImg->showTemplateArea(false);
        m_addButton->Enable(false);
        m_delButton->Enable(false);
        break;
    case BOTH_POINTS_SELECTED:
        m_leftImg->showTemplateArea(false);
        m_rightImg->showTemplateArea(false);
        m_leftImg->showSearchArea(false);
        m_rightImg->showSearchArea(false);
        m_addButton->Enable(true);
        m_delButton->Enable(false);
    }
    // apply the change
    cpCreationState = newState;
}

void CPEditorPanel::OnPrevImg(wxCommandEvent & e)
{
    if (m_pano->getNrOfImages() < 2) return;
    int nImgs = m_pano->getNrOfImages();
    int left = m_leftImageNr -1;
    int right = m_rightImageNr -1;
    if (left < 0) {
        left += nImgs;
    } else if (left >= nImgs) {
        left -= nImgs;
    }

    if (right < 0) {
        right += nImgs;
    } else if (right >= nImgs) {
        right -= nImgs;
    }
    setLeftImage((unsigned int) left);
    setRightImage((unsigned int) right);
}

void CPEditorPanel::OnNextImg(wxCommandEvent & e)
{
    if (m_pano->getNrOfImages() < 2) return;
    int nImgs = m_pano->getNrOfImages();
    int left = m_leftImageNr + 1;
    int right = m_rightImageNr + 1;
    if (left < 0) {
        left += nImgs;
    } else if (left >= nImgs) {
        left -= nImgs;
    }

    if (right < 0) {
        right += nImgs;
    } else if (right >= nImgs) {
        right -= nImgs;
    }
    setLeftImage((unsigned int) left);
    setRightImage((unsigned int) right);
}

void CPEditorPanel::OnFineTuneButton(wxCommandEvent & e)
{
    if (cpCreationState == NO_POINT) {
        FineTuneSelectedPoint(false);
    } else if (cpCreationState == BOTH_POINTS_SELECTED) {
        FineTuneNewPoint(false);
    }
}

void CPEditorPanel::OnActionContextMenu(wxContextMenuEvent& e)
{
    m_cpActionContextMenu->SetLabel(XRCID("cp_menu_create_cp"), wxString::Format(_("Create cp (Current setting: %s)"), MainFrame::Get()->GetSelectedCPGenerator().c_str()));
    PopupMenu(m_cpActionContextMenu);
};

void CPEditorPanel::OnActionButton(wxCommandEvent& e)
{
    switch (m_cpActionButtonMode)
    {
        case CPTAB_ACTION_CREATE_CP:
            OnCreateCPButton(e);
            break;
        case CPTAB_ACTION_CLEAN_CP:
            OnCleanCPButton(e);
            break;
        case CPTAB_ACTION_CELESTE:
        default:
            OnCelesteButton(e);
            break;
    };
};

void CPEditorPanel::OnCreateCPButton(wxCommandEvent& e)
{
    if (m_leftImageNr == m_rightImageNr)
    {
        // when the same image is selected left and right we are running linefind 
        // with default parameters
        CPDetectorSetting linefindSetting;
#ifdef __WXMSW__
        linefindSetting.SetProg(wxT("linefind.exe"));
#else
        linefindSetting.SetProg(wxT("linefind"));
#endif
        linefindSetting.SetArgs(wxT("-o %o %s"));
        HuginBase::UIntSet imgs;
        imgs.insert(m_leftImageNr);
        MainFrame::Get()->RunCPGenerator(linefindSetting, imgs);
    }
    else
    {
        HuginBase::UIntSet imgs;
        imgs.insert(m_leftImageNr);
        imgs.insert(m_rightImageNr);
        MainFrame::Get()->RunCPGenerator(imgs);
    };
};

void CPEditorPanel::OnCelesteButton(wxCommandEvent & e)
{
    if (currentPoints.empty())
    {
        wxMessageBox(_("Cannot run celeste without at least one control point connecting the two images"),_("Error"));
        cout << "Cannot run celeste without at least one control point connecting the two images" << endl;
    }
    else
    {
        ProgressReporterDialog progress(4, _("Running Celeste"), _("Running Celeste"), this);
        progress.updateDisplayValue(_("Loading model file"));

        struct celeste::svm_model* model=MainFrame::Get()->GetSVMModel();
        if(model==NULL)
        {
            return;
        };

        // Get Celeste parameters
        wxConfigBase *cfg = wxConfigBase::Get();
        // SVM threshold
        double threshold = HUGIN_CELESTE_THRESHOLD;
        cfg->Read(wxT("/Celeste/Threshold"), &threshold, HUGIN_CELESTE_THRESHOLD);

        // Mask resolution - 1 sets it to fine
        bool t = (cfg->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER) == 0);
        int radius=(t)?10:20;
        DEBUG_TRACE("Running Celeste");

        if (!progress.updateDisplayValue(_("Running Celeste")))
        {
            return;
        }
        // Image to analyse
        ImageCache::EntryPtr img=ImageCache::getInstance().getImage(m_pano->getImage(m_leftImageNr).getFilename());
        vigra::UInt16RGBImage in;
        if(img->image16->width()>0)
        {
            in.resize(img->image16->size());
            vigra::omp::copyImage(srcImageRange(*(img->image16)),destImage(in));
        }
        else
        {
            ImageCache::ImageCacheRGB8Ptr im8=img->get8BitImage();
            in.resize(im8->size());
            vigra::omp::transformImage(srcImageRange(*im8),destImage(in),vigra::functor::Arg1()*vigra::functor::Param(65535/255));
        };
        if (!progress.updateDisplayValue())
        {
            return;
        };
        UIntSet cloudCP=celeste::getCelesteControlPoints(model,in,currentPoints,radius,threshold,800);
        in.resize(0,0);
        if (!progress.updateDisplay())
        {
            return;
        }

        if(cloudCP.size()>0)
        {
            PanoCommand::GlobalCmdHist::getInstance().addCommand(
                new PanoCommand::RemoveCtrlPointsCmd(*m_pano,cloudCP)
                );
        };

        progress.updateDisplayValue();
        wxMessageBox(wxString::Format(_("Removed %lu control points"), static_cast<unsigned long int>(cloudCP.size())), _("Celeste result"), wxOK | wxICON_INFORMATION, this);
        DEBUG_TRACE("Finished running Celeste");
    }
}

void CPEditorPanel::OnCleanCPButton(wxCommandEvent& e)
{
    if (currentPoints.size() < 2)
    {
        wxBell();
        return;
    };
    // calculate mean and variance only for currently active cp
    double mean = 0;
    double var = 0;
    size_t n = 0;
    for (HuginBase::CPointVector::const_iterator it = currentPoints.begin(); it != currentPoints.end(); ++it)
    {
        n++;
        double x = it->second.error;
        double delta = x - mean;
        mean += delta / n;
        var += delta*(x - mean);
    }
    var = var / (n - 1);
    const double limit = (sqrt(var) > mean) ? mean : (mean + sqrt(var));
    HuginBase::UIntSet removedCPs;
    for (HuginBase::CPointVector::const_iterator it = currentPoints.begin(); it != currentPoints.end(); ++it)
    {
        if (it->second.error > limit)
        {
            removedCPs.insert(it->first);
        };
    };
    if (!removedCPs.empty())
    {
        wxMessageBox(wxString::Format(_("Removed %lu control points"), (unsigned long int)removedCPs.size()), _("Cleaning"), wxOK | wxICON_INFORMATION, this);
        PanoCommand::GlobalCmdHist::getInstance().addCommand(new PanoCommand::RemoveCtrlPointsCmd(*m_pano, removedCPs));
    }
    else
    {
        wxBell();
    }
};

void CPEditorPanel::OnActionSelectCreate(wxCommandEvent& e)
{
    m_cpActionButtonMode = CPTAB_ACTION_CREATE_CP;
    m_actionButton->SetLabel(_("Create cp"));
    m_actionButton->SetToolTip(_("Create control points for image pair with currently selected control point detector on photos tab."));
    Layout();
    wxConfig::Get()->Write(wxT("/CPEditorPanel/ActionMode"), static_cast<long>(m_cpActionButtonMode));
};

void CPEditorPanel::OnActionSelectCeleste(wxCommandEvent& e)
{
    m_cpActionButtonMode = CPTAB_ACTION_CELESTE;
    m_actionButton->SetLabel(_("Celeste"));
    m_actionButton->SetToolTip(_("Tries to remove control points from clouds"));
    Layout();
    wxConfig::Get()->Write(wxT("/CPEditorPanel/ActionMode"), static_cast<long>(m_cpActionButtonMode));
};

void CPEditorPanel::OnActionSelectCleanCP(wxCommandEvent& e)
{
    m_cpActionButtonMode = CPTAB_ACTION_CLEAN_CP;
    m_actionButton->SetLabel(_("Clean cp"));
    m_actionButton->SetToolTip(_("Remove outlying control points by statistical method"));
    Layout();
    wxConfig::Get()->Write(wxT("/CPEditorPanel/ActionMode"), static_cast<long>(m_cpActionButtonMode));
};

FDiff2D CPEditorPanel::LocalFineTunePoint(unsigned int srcNr,
                                          const Diff2D & srcPnt,
                                          hugin_utils::FDiff2D & movedSrcPnt,
                                          unsigned int moveNr,
                                          const FDiff2D & movePnt)
{
    long templWidth = wxConfigBase::Get()->Read(wxT("/Finetune/TemplateSize"),HUGIN_FT_TEMPLATE_SIZE);
    long sWidth = templWidth + wxConfigBase::Get()->Read(wxT("/Finetune/LocalSearchWidth"),HUGIN_FT_LOCAL_SEARCH_WIDTH);
    CorrelationResult result;
    PointFineTune(srcNr,
                  srcPnt,
                  templWidth,
                  moveNr,
                  movePnt,
                  sWidth,
                  result);
    movedSrcPnt = result.corrPos;
    if (result.corrPos.x < 0 || result.corrPos.y < 0 || result.maxpos.x < 0 || result.maxpos.y < 0)
    {
        return FDiff2D(-1, -1);
    }
    return result.maxpos;
}

void CPEditorPanel::FineTuneSelectedPoint(bool left)
{
    DEBUG_DEBUG(" selected Point: " << m_selectedPoint);
    if (m_selectedPoint == UINT_MAX) return;
    DEBUG_ASSERT(m_selectedPoint < currentPoints.size());

    HuginBase::ControlPoint cp = currentPoints[m_selectedPoint].second;

    unsigned int srcNr = cp.image1Nr;
    unsigned int moveNr = cp.image2Nr;
    Diff2D srcPnt(roundi(cp.x1), roundi(cp.y1));
    Diff2D movePnt(roundi(cp.x2), roundi(cp.y2));
    if (left) {
        srcNr = cp.image2Nr;
        moveNr = cp.image1Nr;
        srcPnt = Diff2D(roundi(cp.x2), roundi(cp.y2));
        movePnt = Diff2D(roundi(cp.x1), roundi(cp.y1));
    }

    FDiff2D movedSrcPnt;
    FDiff2D result = LocalFineTunePoint(srcNr, srcPnt, movedSrcPnt, moveNr, movePnt);

    if (result.x < 0 || result.y < 0)
    {
        wxBell();
        return;
    };
    
    if (left) {
       cp.x1 = result.x;
       cp.y1 = result.y;
       cp.x2 = movedSrcPnt.x;
       cp.y2 = movedSrcPnt.y;
    } else {
       cp.x2 = result.x;
       cp.y2 = result.y;
       cp.x1 = movedSrcPnt.x;
       cp.y1 = movedSrcPnt.y;
    }

    // if point was mirrored, reverse before setting it.
    if (set_contains(mirroredPoints, m_selectedPoint)) {
        cp.mirror();
    }
    PanoCommand::GlobalCmdHist::getInstance().addCommand(
        new PanoCommand::ChangeCtrlPointCmd(*m_pano, currentPoints[m_selectedPoint].first, cp)
        );
}


void CPEditorPanel::FineTuneNewPoint(bool left)
{
    if (!(cpCreationState == RIGHT_POINT_RETRY ||
          cpCreationState == LEFT_POINT_RETRY ||
          cpCreationState == BOTH_POINTS_SELECTED))
    {
        return;
    }

    FDiff2D leftP = m_leftImg->getNewPoint();
    FDiff2D rightP = m_rightImg->getNewPoint();

    unsigned int srcNr = m_leftImageNr;
    Diff2D srcPnt(leftP.toDiff2D());
    unsigned int moveNr = m_rightImageNr;
    Diff2D movePnt(rightP.toDiff2D());
    if (left) {
        srcNr = m_rightImageNr;
        srcPnt = rightP.toDiff2D();
        moveNr = m_leftImageNr;
        movePnt = leftP.toDiff2D();
    }

    FDiff2D movedSrcPnt;
    FDiff2D result = LocalFineTunePoint(srcNr, srcPnt, movedSrcPnt, moveNr, movePnt);

    if (result.x < 0 || result.y < 0)
    {
        wxBell();
        return;
    };
    if (left) {
        m_leftImg->setNewPoint(result);
        m_leftImg->update();
        m_rightImg->setNewPoint(movedSrcPnt);
        m_rightImg->update();

    } else {
        m_rightImg->setNewPoint(result);
        m_rightImg->update();
        m_leftImg->setNewPoint(movedSrcPnt);
        m_leftImg->update();
    }
}

FDiff2D CPEditorPanel::EstimatePoint(const FDiff2D & p, bool left)
{
    size_t nrNormalCp = 0;
    for (HuginBase::CPointVector::const_iterator it = currentPoints.begin(); it != currentPoints.end(); ++it)
    {
        if (it->second.mode == HuginBase::ControlPoint::X_Y)
        {
            ++nrNormalCp;
        };
    };
    if (nrNormalCp==0)
    {
        DEBUG_WARN("Cannot estimate position without at least one point");
        return FDiff2D(0,0);
    }

    // get copy of SrcPanoImage and reset position
    SrcPanoImage leftImg = m_pano->getSrcImage(left ? m_leftImageNr : m_rightImageNr);
    leftImg.setYaw(0);
    leftImg.setPitch(0);
    leftImg.setRoll(0);
    leftImg.setX(0);
    leftImg.setY(0);
    leftImg.setZ(0);
    SrcPanoImage rightImg = m_pano->getSrcImage(left ? m_rightImageNr : m_leftImageNr);
    rightImg.setYaw(0);
    rightImg.setPitch(0);
    rightImg.setRoll(0);
    rightImg.setX(0);
    rightImg.setY(0);
    rightImg.setZ(0);
    // generate a temporary pano
    Panorama optPano;
    optPano.addImage(leftImg);
    optPano.addImage(rightImg);
    // construct OptimizeVector
    HuginBase::OptimizeVector optVec;
    std::set<std::string> opt;
    optVec.push_back(opt);
    opt.insert("y");
    opt.insert("p");
    if (nrNormalCp > 1)
    {
        opt.insert("r");
    };
    optVec.push_back(opt);
    optPano.setOptimizeVector(optVec);
    // now add control points, need to check image numbers
    HuginBase::CPVector cps;
    for (HuginBase::CPointVector::const_iterator it = currentPoints.begin(); it != currentPoints.end(); ++it)
    {
        HuginBase::ControlPoint cp(it->second);
        if (cp.mode == HuginBase::ControlPoint::X_Y)
        {
            cp.image1Nr = left ? 0 : 1;
            cp.image2Nr = left ? 1 : 0;
            cps.push_back(cp);
        };
    };
    optPano.setCtrlPoints(cps);
    deregisterPTWXDlgFcn();
    HuginBase::PTools::optimize(optPano);
    registerPTWXDlgFcn();

    // now transform the wanted point p to other image
    HuginBase::PTools::Transform transformBackward;
    transformBackward.createInvTransform(optPano.getImage(0), optPano.getOptions());
    HuginBase::PTools::Transform transformForward;
    transformForward.createTransform(optPano.getImage(1), optPano.getOptions());
    FDiff2D t;
    if (transformBackward.transformImgCoord(t, p))
    {
        if (transformForward.transformImgCoord(t, t))
        {
            // clip to fit to
            if (t.x < 0) t.x = 0;
            if (t.y < 0) t.y = 0;
            if (t.x > optPano.getImage(1).getWidth()) t.x = optPano.getImage(1).getWidth();
            if (t.y > optPano.getImage(1).getHeight()) t.y = optPano.getImage(1).getHeight();
            DEBUG_DEBUG("estimated point " << t.x << "," << t.y);
            return t;
        };
    };
    wxBell();
    return FDiff2D(0, 0);
}

void CPEditorPanel::OnColumnWidthChange( wxListEvent & e )
{
    int colNum = e.GetColumn();
    wxConfigBase::Get()->Write( wxString::Format(wxT("/CPEditorPanel/ColumnWidth%d"),colNum), m_cpList->GetColumnWidth(colNum) );
}

CPImageCtrl::ImageRotation CPEditorPanel::GetRot(double yaw, double pitch, double roll)
{
    CPImageCtrl::ImageRotation rot = CPImageCtrl::ROT0;
    // normalize roll angle
    while (roll > 360) roll-= 360;
    while (roll < 0) roll += 360;

    while (pitch > 180) pitch -= 360;
    while (pitch < -180) pitch += 360;
    bool headOver = (pitch > 90 || pitch < -90);

    if (wxConfig::Get()->Read(wxT("/CPEditorPanel/AutoRot"),1L)) {
        if (roll >= 315 || roll < 45) {
            rot = headOver ? CPImageCtrl::ROT180 : CPImageCtrl::ROT0;
        } else if (roll >= 45 && roll < 135) {
            rot = headOver ? CPImageCtrl::ROT270 : CPImageCtrl::ROT90;
        } else if (roll >= 135 && roll < 225) {
            rot = headOver ? CPImageCtrl::ROT0 : CPImageCtrl::ROT180;
        } else {
            rot = headOver ? CPImageCtrl::ROT90 : CPImageCtrl::ROT270;
        }
    }
    return rot;
}

IMPLEMENT_DYNAMIC_CLASS(CPEditorPanel, wxPanel)

CPEditorPanelXmlHandler::CPEditorPanelXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *CPEditorPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, CPEditorPanel)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow( cp);

    return cp;
}

bool CPEditorPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("CPEditorPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(CPEditorPanelXmlHandler, wxXmlResourceHandler)

