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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>

#include "panoinc_WX.h"
// hugin's
#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/TextKillFocusHandler.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/CPFineTuneFrame.h"


// more standard includes if needed
#include <algorithm>
#include <float.h>

// standard hugin include
#include "panoinc.h"

// more vigra include if needed
#include "vigra/cornerdetection.hxx"
#include "vigra/localminmax.hxx"
#include "vigra_ext/Correlation.h"

using namespace std;
using namespace PT;
using namespace vigra;
using namespace vigra_ext;
using namespace vigra::functor;
using namespace utils;

/*
void ToGray(wxImageIterator sy, wxImageIterator send, vigra::BImage::Iterator dy)
{
    // iterate down the first column of the images
    for(; sy.y != send.y; ++sy.y, ++dy.y)
    {
        // create image iterator that points to the first
        // pixel of the current row of the source image
        wxImageIterator sx = sy;

        // create image iterator that points to the first
        // pixel of the current row of the destination image
        vigra::BImage::Iterator dx = dy;

        // iterate across current row
        for(; sx.x != send.x; ++sx.x, ++dx.x)
        {
            // calculate negative gray value
            *dx = (unsigned char) ( (*sx).red()*0.3 + (*sx).green()*0.59
                                    + (*sx).blue()*0.11);
        }
    }
}
*/

BEGIN_EVENT_TABLE(CPEditorPanel, wxPanel)
    EVT_CPEVENT(CPEditorPanel::OnCPEvent)
    EVT_NOTEBOOK_PAGE_CHANGED ( XRCID("cp_editor_left_tab"),CPEditorPanel::OnLeftImgChange )
    EVT_NOTEBOOK_PAGE_CHANGED ( XRCID("cp_editor_right_tab"),CPEditorPanel::OnRightImgChange )
    EVT_LIST_ITEM_SELECTED(XRCID("cp_editor_cp_list"), CPEditorPanel::OnCPListSelect)
    EVT_COMBOBOX(XRCID("cp_editor_zoom_box"), CPEditorPanel::OnZoom)
    EVT_TEXT_ENTER(XRCID("cp_editor_x1"), CPEditorPanel::OnTextPointChange )
    EVT_TEXT_ENTER(XRCID("cp_editor_y1"), CPEditorPanel::OnTextPointChange )
    EVT_TEXT_ENTER(XRCID("cp_editor_x2"), CPEditorPanel::OnTextPointChange )
    EVT_TEXT_ENTER(XRCID("cp_editor_y2"), CPEditorPanel::OnTextPointChange )
    EVT_CHOICE(XRCID("cp_editor_mode"), CPEditorPanel::OnTextPointChange )
    EVT_CHAR(CPEditorPanel::OnKey)
    EVT_KEY_UP(CPEditorPanel::OnKeyUp)
    EVT_KEY_DOWN(CPEditorPanel::OnKeyDown)
    EVT_BUTTON(XRCID("cp_editor_delete"), CPEditorPanel::OnDeleteButton)
    EVT_BUTTON(XRCID("cp_editor_add"), CPEditorPanel::OnAddButton)
    EVT_CHECKBOX(XRCID("cp_editor_auto_add_cb"), CPEditorPanel::OnAutoAddCB)
    EVT_BUTTON(XRCID("cp_editor_previous_img"), CPEditorPanel::OnPrevImg)
    EVT_BUTTON(XRCID("cp_editor_next_img"), CPEditorPanel::OnNextImg)
    EVT_BUTTON(XRCID("cp_editor_finetune_button"), CPEditorPanel::OnFineTuneButton)
END_EVENT_TABLE()

CPEditorPanel::CPEditorPanel(wxWindow * parent, PT::Panorama * pano)
    : cpCreationState(NO_POINT), m_pano(pano), m_leftImageNr(UINT_MAX),
      m_rightImageNr(UINT_MAX), m_listenToPageChange(true), m_detailZoomFactor(1),
      m_selectedPoint(UINT_MAX)

{
    DEBUG_TRACE("");
    wxXmlResource::Get()->LoadPanel(this, parent, wxT("cp_editor_panel"));

    wxPoint tabsz(1,14);
    tabsz = ConvertDialogToPixels(tabsz);
    int tabH = tabsz.y;
    // left image
    m_leftTabs = XRCCTRL(*this, "cp_editor_left_tab", wxNotebook);
    m_leftTabs->SetSizeHints(1,tabH,1000,tabH,-1,-1);
    m_leftImg = new CPImageCtrl(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_left_img"),
                                               m_leftImg);

    // right image
    m_rightTabs = XRCCTRL(*this, "cp_editor_right_tab", wxNotebook);
    m_rightTabs->SetSizeHints(1,tabH,1000,tabH,-1,-1);
    m_rightImg = new CPImageCtrl(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_right_img"),
                                               m_rightImg);
#ifdef USE_FINETUNEFRAME
    // setup finetune frame
    m_fineTuneFrame = new CPFineTuneFrame(this, *pano);
    m_fineTuneFrame->Show();
    // connect to image displays for editing (urgh.. not really that nice,
    // from a software design viewpoint)
    m_leftImg->SetZoomView(m_fineTuneFrame->GetLeftImg());
    m_rightImg->SetZoomView(m_fineTuneFrame->GetRightImg());
#else
    m_fineTuneFrame=0;
#endif

    // setup list view
    m_cpList = XRCCTRL(*this, "cp_editor_cp_list", wxListCtrl);
    m_cpList->InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25);
    m_cpList->InsertColumn( 1, _("left x"), wxLIST_FORMAT_RIGHT, 65);
    m_cpList->InsertColumn( 2, _("left y"), wxLIST_FORMAT_RIGHT, 65);
    m_cpList->InsertColumn( 3, _("right x"), wxLIST_FORMAT_RIGHT, 65);
    m_cpList->InsertColumn( 4, _("right y"), wxLIST_FORMAT_RIGHT, 65);
    m_cpList->InsertColumn( 5, _("Alignment"), wxLIST_FORMAT_LEFT,110 );
    m_cpList->InsertColumn( 6, _("Distance"), wxLIST_FORMAT_RIGHT, 110);


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

#ifdef USE_WX25x
    m_cp_ctrls = XRCCTRL(*this, "cp_controls_panel", wxScrolledWindow);
	DEBUG_ASSERT(m_cp_ctrls);
    m_cp_splitter = XRCCTRL(*this, "cp_editor_panel_splitter", wxSplitterWindow);
	DEBUG_ASSERT(m_cp_splitter);

	m_cp_ctrls->SetSizeHints(20, 20);
	m_cp_ctrls->FitInside();
	m_cp_ctrls->SetScrollRate(10, 10);
	// wx 2.5.4
	// m_cp_splitter->SetSashGravity(0.5);
	m_cp_splitter->SetSashPosition(wxConfigBase::Get()->Read(wxT("/CPEditorPanel/sashPos"),200));
	m_cp_splitter->SetMinimumPaneSize(20);
#endif

    // apply selection from xrc file
    wxCommandEvent dummy;
    dummy.m_commandInt = XRCCTRL(*this,"cp_editor_zoom_box",wxComboBox)->GetSelection();
    OnZoom(dummy);


    wxConfigBase *config = wxConfigBase::Get();

    m_autoAddCB->SetValue(config->Read(wxT("/CPEditorPanel/autoAdd"),0l));
    m_fineTuneCB->SetValue(config->Read(wxT("/CPEditorPanel/fineTune"),1l));
    m_estimateCB->SetValue(config->Read(wxT("/CPEditorPanel/autoEstimate"),1l));

    // observe the panorama
    m_pano->addObserver(this);
}


CPEditorPanel::~CPEditorPanel()
{
    DEBUG_TRACE("dtor");

#ifdef USE_WX25x
    int sashPos;
	sashPos = m_cp_splitter->GetSashPosition();
	DEBUG_INFO("CP Editor panel sash pos: " << sashPos);
	wxConfigBase::Get()->Write(wxT("/CPEditorPanel/sashPos"), sashPos);
#endif
				
    // FIXME. why does this crash at exit?
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
        m_leftImg->setImage("");
        m_leftImageNr = imgNr;
        m_leftFile = "";
        if (m_fineTuneFrame) {
            m_fineTuneFrame->GetLeftImg()->Clear();
        }
        changeState(NO_POINT);
        UpdateDisplay();
    } else if (m_leftImageNr != imgNr) {
        m_leftImg->setImage(m_pano->getImage(imgNr).getFilename());
        if (m_leftTabs->GetSelection() != (int) imgNr) {
            m_leftTabs->SetSelection(imgNr);
        }
        m_leftImageNr = imgNr;
        m_leftFile = m_pano->getImage(imgNr).getFilename();
        changeState(NO_POINT);
        if (m_fineTuneFrame) {
            m_fineTuneFrame->GetLeftImg()->SetImage(imgNr);
        }
        UpdateDisplay();
    }
    m_selectedPoint = UINT_MAX;
}


void CPEditorPanel::setRightImage(unsigned int imgNr)
{
    DEBUG_TRACE("image " << imgNr);
    if (imgNr == UINT_MAX) {
        m_rightImg->setImage("");
        m_rightImageNr = imgNr;
        m_rightFile = "";
        if (m_fineTuneFrame) {
            m_fineTuneFrame->GetLeftImg()->Clear();
        }
        changeState(NO_POINT);
        UpdateDisplay();
    } else if (m_rightImageNr != imgNr) {
        // set the new image
        m_rightImg->setImage(m_pano->getImage(imgNr).getFilename());
        // select tab
        if (m_rightTabs->GetSelection() != (int) imgNr) {
            m_rightTabs->SetSelection(imgNr);
        }
        m_rightImageNr = imgNr;
        m_rightFile = m_pano->getImage(imgNr).getFilename();
        // update the rest of the display (new control points etc)
        changeState(NO_POINT);
        if (m_fineTuneFrame) {
            m_fineTuneFrame->GetRightImg()->SetImage(imgNr);
        }
        UpdateDisplay();
    }
    m_selectedPoint = UINT_MAX;

}


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
        DEBUG_FATAL("UNKOWN SOURCE OF CPEvent");
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

    case CPEvent::POINT_CHANGED:
    {
        DEBUG_DEBUG("move point("<< nr << ")");
        if (nr >= currentPoints.size()) {
            DEBUG_ERROR("invalid point number while moving point")
            return;
        }
        ControlPoint cp = currentPoints[nr].second;
        changeState(NO_POINT);

        if (left) {
            cp.x1 = point.x;
            cp.y1 = point.y;
        } else {
            cp.x2 = point.x;
            cp.y2 = point.y;
        }
        if (set_contains(mirroredPoints, nr)) {
            cp.mirror();
        }

        DEBUG_DEBUG("changing point to: " << cp.x1 << "," << cp.y1
                    << "  " << cp.x2 << "," << cp.y2);

        GlobalCmdHist::getInstance().addCommand(
            new PT::ChangeCtrlPointCmd(*m_pano, currentPoints[nr].first, cp)
            );

        break;
    }
    // currently not emitted by CPImageCtrl
    case CPEvent::REGION_SELECTED:
    {
        changeState(NO_POINT);
#if 0
	if (false) {
            text = "REGION_SELECTED";
            wxRect region = ev.getRect();
            int dx = region.GetWidth() / 2;
            int dy = region.GetHeight() / 2;
            vigra_ext::CorrelationResult pos;
            ControlPoint point;
            bool found(false);
            DEBUG_DEBUG("left img: " << m_leftImageNr
                        << "  right img: " << m_rightImageNr);
            if (left) {
                if (FindTemplate(m_leftImageNr, region, m_rightImageNr, pos)) {
                    point.image1Nr = m_leftImageNr;
                    point.x1 = region.GetLeft() + dx;
                    point.y1 = region.GetTop() + dy;
                    point.image2Nr = m_rightImageNr;
                    point.x2 = pos.maxpos.x + dx;
                    point.y2 = pos.maxpos.y + dy;
                    point.mode = PT::ControlPoint::X_Y;
                    found = true;
                } else {
                    DEBUG_DEBUG("No matching point found");
                }
            } else {
                if (FindTemplate(m_rightImageNr, region, m_leftImageNr, pos)) {
                    point.image1Nr = m_leftImageNr;
                    point.x1 = pos.maxpos.x + dx;
                    point.y1 = pos.maxpos.y + dy;
                    point.image2Nr = m_rightImageNr;
                    point.x2 = region.GetLeft() + dx;
                    point.y2 = region.GetTop() + dy;
                    point.mode = PT::ControlPoint::X_Y;
                    found = true;
                } else {
                    DEBUG_DEBUG("No matching point found");
                }
            }
            if (found) {
                GlobalCmdHist::getInstance().addCommand(
                    new PT::AddCtrlPointCmd(*m_pano, point)
                    );
                // select new control Point
                unsigned int lPoint = m_pano->getNrOfCtrlPoints() -1;
                SelectGlobalPoint(lPoint);
                changeState(NO_POINT);
            } else {
                wxLogError(_("No corresponding point found"));
            }
	    }
#endif
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
	if (left) {
	    m_rightImg->ScrollDelta(wxPoint(roundi(point.x), roundi(point.y)) );
	} else {
	    m_leftImg->ScrollDelta(wxPoint(roundi(point.x), roundi(point.y)));
	}
	break;

//    default:
//        text = "FATAL: unknown event mode";
    }
    m_leftImg->update();
    m_rightImg->update();
}


void CPEditorPanel::CreateNewPoint()
{
    DEBUG_TRACE("");
//    DEBUG_ASSERT(m_leftImg->GetState == NEW_POINT_SELECTED);
//    DEBUG_ASSERT(m_rightImg->GetState == NEW_POINT_SELECTED);
    FDiff2D p1 = m_leftImg->getNewPoint();
    FDiff2D p2 = m_rightImg->getNewPoint();
    ControlPoint point;
    point.image1Nr = m_leftImageNr;
    point.x1 = p1.x;
    point.y1 = p1.y;
    point.image2Nr = m_rightImageNr;
    point.x2 = p2.x;
    point.y2 = p2.y;
    if (point.image1Nr == point.image2Nr) {
        if ( abs(p1.x - p2.x) > abs(p1.y - p2.y)) {
            point.mode = PT::ControlPoint::Y;
        } else {
            point.mode = PT::ControlPoint::X;
        }
    } else {
        point.mode = PT::ControlPoint::X_Y;
    }

    changeState(NO_POINT);

    // create points
    GlobalCmdHist::getInstance().addCommand(
        new PT::AddCtrlPointCmd(*m_pano, point)
        );


    // select new control Point
    unsigned int lPoint = m_pano->getNrOfCtrlPoints() -1;
    SelectGlobalPoint(lPoint);
    changeState(NO_POINT);
    MainFrame::Get()->SetStatusText(_("new control point added"));
}

void CPEditorPanel::ClearSelection()
{
    if (m_selectedPoint == UINT_MAX) {
        DEBUG_ASSERT(m_cpList->GetSelectedItemCount() == 0);
        // no point selected, no need to select one.
        return;
    }
    m_cpList->SetItemState(m_selectedPoint, 0, wxLIST_STATE_SELECTED);

    m_selectedPoint=UINT_MAX;
    changeState(NO_POINT);
    m_leftImg->deselect();
    m_rightImg->deselect();
    UpdateDisplay();
}

void CPEditorPanel::SelectLocalPoint(unsigned int LVpointNr)
{
    DEBUG_TRACE("selectLocalPoint(" << LVpointNr << ")");

    if ( m_selectedPoint == LVpointNr) {
        DEBUG_DEBUG("already selected");
        return;
    }
    m_selectedPoint = LVpointNr;

    const ControlPoint & p = currentPoints[LVpointNr].second;
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
    vector<CPoint>::const_iterator it = currentPoints.begin();

    while(it != currentPoints.end() && (*it).first != globalNr) {
        it++;
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
//    DEBUG_DEBUG("automatically estimating point in other window");
    FDiff2D op;
    op = EstimatePoint(FDiff2D(p.x, p.y), left);
    // check if point is in image.
    const PanoImage & pImg = m_pano->getImage(otherImgNr);
    if (p.x < (int) pImg.getWidth() && p.x >= 0
        && p.y < (int) pImg.getHeight() && p.y >= 0)
    {
        otherImg->setNewPoint(op);
        // if fine-tune is checked, run a fine-tune session as well.
        // hmm probably there should be another separate function for this..
        if (m_fineTuneCB->IsChecked()) {
            MainFrame::Get()->SetStatusText(_("searching similar point..."),0);
            FDiff2D newPoint = otherImg->getNewPoint();

            long templWidth = wxConfigBase::Get()->Read(wxT("/Finetune/TemplateSize"), HUGIN_FT_TEMPLATE_SIZE);
            const PanoImage & img = m_pano->getImage(thisImgNr);
            double sAreaPercent = wxConfigBase::Get()->Read(wxT("/Finetune/SearchAreaPercent"),10);
            int sWidth = (int) (img.getWidth() * sAreaPercent / 100.0);
            CorrelationResult corrPoint;
            double corrOk=false;
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
                wxMessageBox(wxString (e.what(), *wxConvCurrent), _("Error during Fine-tune"));
            }
            if (! corrOk) {
                // just set point, PointFineTune already complained
                otherImg->setScale(m_detailZoomFactor);
                otherImg->setNewPoint(corrPoint.maxpos);
                changeState(BOTH_POINTS_SELECTED);
            } else {
                // show point & zoom in if auto add is not set
                if (!m_autoAddCB->IsChecked()) {
                    otherImg->setScale(m_detailZoomFactor);
                    otherImg->setNewPoint(corrPoint.maxpos);
                    changeState(BOTH_POINTS_SELECTED);
                } else {
                    // add point
                    otherImg->setNewPoint(corrPoint.maxpos);
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
            // run auto estimate procedure?
            if (estimate && currentPoints.size() > 0) {
                estimateAndAddOtherPoint(p, left,
                                         thisImg, thisImgNr, THIS_POINT, THIS_POINT_RETRY,
                                         otherImg, otherImgNr, OTHER_POINT, OTHER_POINT_RETRY);
            };
        }

    } else if (cpCreationState == OTHER_POINT_RETRY) {
        thisImg->showPosition(p);
    } else if (cpCreationState == THIS_POINT) {
        thisImg->showPosition(p);

        if (estimate && currentPoints.size() > 0) {
            estimateAndAddOtherPoint(p, left,
                                     thisImg, thisImgNr, THIS_POINT, THIS_POINT_RETRY,
                                     otherImg, otherImgNr, OTHER_POINT, OTHER_POINT_RETRY);
        }
    } else if (cpCreationState == OTHER_POINT || cpCreationState == THIS_POINT_RETRY) {
        // the try for the second point.
        if (cpCreationState == OTHER_POINT) {
            // other point already selected, finalize point.

            if (m_fineTuneCB->IsChecked()) {
                CorrelationResult corrRes;

                FDiff2D newPoint = otherImg->getNewPoint();

                long templWidth = wxConfigBase::Get()->Read(wxT("/Finetune/TemplateSize"),HUGIN_FT_TEMPLATE_SIZE);
                const PanoImage & img = m_pano->getImage(thisImgNr);
                double sAreaPercent = wxConfigBase::Get()->Read(wxT("/Finetune/SearchAreaPercent"),
                                                                HUGIN_FT_SEARCH_AREA_PERCENT);
                int sWidth = (int) (img.getWidth() * sAreaPercent / 100.0);
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
                    wxMessageBox(wxString (e.what(), *wxConvCurrent), _("Error during Fine-tune"));
                }

                if (! corrOk) {
                    // low xcorr
                    // zoom to 100 percent. & set second stage
                    // to abandon finetune this time.
                    thisImg->setScale(m_detailZoomFactor);
                    thisImg->setNewPoint(corrRes.maxpos);
                    thisImg->update();
                    otherImg->setNewPoint(FDiff2D(newPoint_round.x, newPoint_round.y));
                    changeState(BOTH_POINTS_SELECTED);
                } else {
                    // show point & zoom in if auto add is not set
                    changeState(BOTH_POINTS_SELECTED);
                    if (!m_autoAddCB->IsChecked()) {
                        thisImg->setScale(m_detailZoomFactor);
                    }
                    thisImg->setNewPoint(corrRes.maxpos);
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
        }

    } else if (cpCreationState == BOTH_POINTS_SELECTED) {
        // nothing to do.. maybe a special fine-tune with
        // a small search region

    } else {
        // should never reach this, else state machine is broken.
        DEBUG_ASSERT(0);
    }
}

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

    MainFrame::Get()->SetStatusText(_("searching similar point..."),0);

    double corrThresh=HUGIN_FT_CORR_THRESHOLD;
    wxConfigBase::Get()->Read(wxT("/Finetune/CorrThreshold"),&corrThresh,
                              HUGIN_FT_CORR_THRESHOLD);

    double curvThresh = HUGIN_FT_CURV_THRESHOLD;
    wxConfigBase::Get()->Read(wxT("/Finetune/CurvThreshold"),&curvThresh,
                              HUGIN_FT_CURV_THRESHOLD);

    const PanoImage & img = m_pano->getImage(subjImgNr);

    const BImage & subjImg = ImageCache::getInstance().getPyramidImage(
        img.getFilename(),0);
    const BImage & tmplImg = ImageCache::getInstance().getPyramidImage(
        m_pano->getImage(tmplImgNr).getFilename(),0);

    wxConfigBase *cfg = wxConfigBase::Get();
    bool rotatingFinetune = cfg->Read(wxT("/Finetune/RotationSearch"), HUGIN_FT_ROTATION_SEARCH) == 1;

    if (rotatingFinetune) {
        double startAngle=HUGIN_FT_ROTATION_START_ANGLE;
        cfg->Read(wxT("/Finetune/RotationStartAngle"),&startAngle,HUGIN_FT_ROTATION_START_ANGLE);
        startAngle=DEG_TO_RAD(startAngle);
        double stopAngle=HUGIN_FT_ROTATION_STOP_ANGLE;
        cfg->Read(wxT("/Finetune/RotationStopAngle"),&stopAngle,HUGIN_FT_ROTATION_STOP_ANGLE);
        stopAngle=DEG_TO_RAD(stopAngle);
        int nSteps = cfg->Read(wxT("/Finetune/RotationSteps"), HUGIN_FT_ROTATION_STEPS);
        {
            wxBusyCursor busy;
            res = vigra_ext::PointFineTuneRotSearch(tmplImg, tmplPoint, templSize,
                                                    subjImg, o_subjPoint.toDiff2D(),
                                                    sWidth,
                                                    startAngle, stopAngle, nSteps);
        }
    } else {
        res = vigra_ext::PointFineTune(tmplImg, tmplPoint, templSize,
                                       subjImg, o_subjPoint.toDiff2D(),
                                       sWidth);
    }
    // invert curvature. we always assume its a maxima, the curvature there is negative
    // however, we allow the user to specify a positive threshold, so we need to
    // invert it
    res.curv.x = - res.curv.x;
    res.curv.y = - res.curv.y;

    MainFrame::Get()->SetStatusText(wxString::Format(_("Point finetuned, angle: %.0f deg, correlation coefficient: %0.3f, curvature: %0.3f %0.3f "),
                                    res.maxAngle, res.maxi, res.curv.x, res.curv.y ),0);
    if (res.maxi < corrThresh ||res.curv.x < curvThresh || res.curv.y < curvThresh  )
    {
        // Bad correlation result.
        wxMessageBox(
            wxString::Format(_("No similar point found.\ncorrelation coefficient: %.3f (should be > %.3f)\npeak curvature: (%.3f, %.3f) ( should be > %.3f)"),
                             res.maxi, corrThresh, res.curv.x, res.curv.y, curvThresh),
            _("No similar point found"),
            wxICON_HAND, this);
        return false;
    }

    return true;
}

#if 0
double CPEditorPanel::PointFineTune_old(unsigned int tmplImgNr,
                                    const Diff2D & tmplPoint,
                                    int templSize,
                                    unsigned int subjImgNr,
                                    const FDiff2D & o_subjPoint,
                                    int sWidth,
                                    FDiff2D & tunedPos)
{
    DEBUG_TRACE("tmpl img nr: " << tmplImgNr << " corr src: "
                << subjImgNr);

    const PanoImage & img = m_pano->getImage(subjImgNr);

    const BImage & subjImg = ImageCache::getInstance().getPyramidImage(
        img.getFilename(),0);

    int swidth = sWidth/2;
    DEBUG_DEBUG("search window half width/height: " << swidth << "x" << swidth);
    Diff2D subjPoint(o_subjPoint.toDiff2D());
    if (subjPoint.x < 0) subjPoint.x = 0;
    if (subjPoint.x > (int) img.getWidth()) subjPoint.x = img.getWidth()-1;
    if (subjPoint.y < 0) subjPoint.y = 0;
    if (subjPoint.y > (int) img.getHeight()) subjPoint.x = img.getHeight()-1;

    Diff2D searchUL(subjPoint.x - swidth, subjPoint.y - swidth);
    Diff2D searchLR(subjPoint.x + swidth, subjPoint.y + swidth);
    // clip search window
    if (searchUL.x < 0) searchUL.x = 0;
    if (searchUL.x > subjImg.width()) searchUL.x = subjImg.width();
    if (searchUL.y < 0) searchUL.y = 0;
    if (searchUL.y > subjImg.height()) searchUL.y = subjImg.height();
    if (searchLR.x > subjImg.width()) searchLR.x = subjImg.width();
    if (searchLR.x < 0) searchLR.x = 0;
    if (searchLR.y > subjImg.height()) searchLR.y = subjImg.height();
    if (searchLR.y < 0) searchLR.y = 0;
    DEBUG_DEBUG("search borders: " << searchLR.x << "," << searchLR.y);
    Diff2D searchSize = searchLR - searchUL;

    const BImage & tmplImg = ImageCache::getInstance().getPyramidImage(
        m_pano->getImage(tmplImgNr).getFilename(),0);

    // remap template into searchImage perspective
    // We have 3 coordinate systems:
    //
    // S - search image, centered at p,y = 0, we assume r = 0, too;
    // T - template image, centered at p,y = 0, we assume r = 0, too;
    // E - equirectangular world coordinate system
    //
    // and two points
    //
    // S
    //  Ps - point in S
    //
    // T
    //  Pt - point in T
    //                                           S
    // we need a transformation X that will move  Ps into T, so that they
    // coincide:
    //
    // T    T  S
    //  Ps = X* Ps
    //      S
    //
    // E   T    E   S
    //  X * Pt = X * Ps
    // T        S
    //
    // We can use X to remap the template from T into S.
    // (by sampling a template grid in S)
    //
    // We assume that r = 0 for all images
    //
    // Then X can be estimated with:
    //
    // T    T'    T
    //  X =   X *  X
    // S     E
    //
    // This can can be solved for the r,p,y, so we assume that T and S are
    // not rotated.
    //
    // assuming that r = 0 for all images (hmm should redo without that
    //                                     assumption!)
    //
    // We then shift these points.
    //
    // T2 - template image coordinate system, shifted, so that
    //      the coordinates of

    // 1. transf calc Ps in E
    //


    // make template size user configurable as well?
    int templWidth = templSize/2;
    Diff2D tmplUL(-templWidth, -templWidth);
    Diff2D tmplLR(templWidth, templWidth);
    // clip template
    if (tmplUL.x + tmplPoint.x < 0) tmplUL.x = -tmplPoint.x;
    if (tmplUL.y + tmplPoint.y < 0) tmplUL.y = -tmplPoint.y;
    if (tmplLR.x + tmplPoint.x> tmplImg.width())
        tmplLR.x = tmplImg.width() - tmplPoint.x;
    if (tmplLR.y + tmplPoint.y > tmplImg.height())
        tmplLR.y = tmplImg.height() - tmplPoint.y;

    FImage dest(searchSize);
    dest.init(1);
    DEBUG_DEBUG("starting fine-tune");
    // we could use the multiresolution version as well.
    // but usually the region is quite small.
    vigra_ext::CorrelationResult res;
    res = vigra_ext::correlateImage(subjImg.upperLeft() + searchUL,
                                    subjImg.upperLeft() + searchLR,
                                    subjImg.accessor(),
                                    dest.upperLeft(),
                                    dest.accessor(),
                                    tmplImg.upperLeft() + tmplPoint,
                                    tmplImg.accessor(),
                                    tmplUL, tmplLR, -1);
    res.maxpos = res.maxpos + searchUL;
    DEBUG_DEBUG("normal search finished, max:" << res.maxi
                << " at " << res.maxpos.x << "," << res.maxpos.y);

    tunedPos.x = res.maxpos.x;
    tunedPos.y = res.maxpos.y;
    return res.maxi;
}
#endif


void CPEditorPanel::panoramaChanged(PT::Panorama &pano)
{
    DEBUG_TRACE("");
}

void CPEditorPanel::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrTabs = m_leftTabs->GetPageCount();
    DEBUG_TRACE("nrImages:" << nrImages << " nrTabs:" << nrTabs);

    // FIXME: lets hope that nobody holds references to these images..
    ImageCache::getInstance().softFlush();

    // add tab bar entries, if needed
    if (nrTabs < nrImages) {
        for (unsigned int i=nrTabs; i < nrImages; i++) {
            wxWindow* t1= new wxWindow(m_leftTabs,-1,wxPoint(0,0),wxSize(0,0));
            t1->SetSize(0,0);
            t1->SetSizeHints(0,0,0,0);
            wxWindow* t2= new wxWindow(m_rightTabs,-1,wxPoint(0,0),wxSize(0,0));
            t2->SetSize(0,0);
            t2->SetSizeHints(0,0,0,0);
            // update tab buttons
            if (!m_leftTabs->AddPage(t1, wxString::Format(wxT("%d"),i))) {
                DEBUG_FATAL("could not add dummy window to left notebook");
            }
            if (!m_rightTabs->AddPage(t2, wxString::Format(wxT("%d"),i))){
                DEBUG_FATAL("could not add dummy window to right notebook");
            }
        }
    }
    if (nrTabs > nrImages) {
        // remove tab bar entries if needed
        // we have to disable listening to notebook selection events,
        // else we might update to a noexisting image
        int left = m_leftTabs->GetSelection();
        int right = m_rightTabs->GetSelection();
        m_listenToPageChange = false;
        for (int i=nrTabs-1; i >= (int)nrImages; i--) {
            DEBUG_DEBUG("removing tab " << i);
            m_leftTabs->DeletePage(i);
            m_rightTabs->DeletePage(i);
        }
        m_listenToPageChange = true;
        if (nrImages > 0) {
            // select some other image if we deleted the current image
            if (left >= (int) nrImages) {
                setLeftImage(nrImages -1);
                m_leftFile = pano.getImage(nrImages-1).getFilename();
                m_leftImg->setImage(m_leftFile);
            }
            if (right >= (int)nrImages) {
                setRightImage(nrImages -1);
                m_rightFile = pano.getImage(nrImages-1).getFilename();
                m_rightImg->setImage(m_rightFile);
            }
        } else {
            DEBUG_DEBUG("setting no images");
            m_leftImageNr = UINT_MAX;
            m_leftFile = "";
            m_rightImageNr = UINT_MAX;
            m_rightFile = "";
            // no image anymore..
            m_leftImg->setImage(m_leftFile);
            m_rightImg->setImage(m_rightFile);
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
        if (m_leftImageNr == imgNr) {
            DEBUG_DEBUG("left image dirty "<< imgNr);
            if (m_leftFile != pano.getImage(imgNr).getFilename()) {
                m_leftFile = pano.getImage(imgNr).getFilename();
                m_leftImg->setImage(m_leftFile);
            }
            update=true;
        }

        if (m_rightImageNr == imgNr) {
            DEBUG_DEBUG("right image dirty "<< imgNr);
            if (m_rightFile != pano.getImage(imgNr).getFilename()) {
                m_rightFile = pano.getImage(imgNr).getFilename();
                m_rightImg->setImage(m_rightFile);
            }
            update=true;
        }
    }

    // if there is no selection, select the first one.
    if (nrImages > 0 && nrTabs == 0) {
        setLeftImage(0);
        setRightImage(0);
    }

    if (update) {
        UpdateDisplay();
    }

}

void CPEditorPanel::UpdateDisplay()
{
    DEBUG_DEBUG("")
    int fI = m_leftTabs->GetSelection();
    int sI = m_rightTabs->GetSelection();
    if (fI < 0 || m_leftImageNr == UINT_MAX || m_rightImageNr == UINT_MAX) {
        return;
    }
    m_leftImageNr = (unsigned int) fI;
    m_rightImageNr = (unsigned int) sI;

    // reset selection
    m_x1Text->Clear();
    m_y1Text->Clear();
    m_x2Text->Clear();
    m_y2Text->Clear();
    m_cpModeChoice->SetSelection(0);

    // update control points
    const PT::CPVector & controlPoints = m_pano->getCtrlPoints();
    currentPoints.clear();
    mirroredPoints.clear();
    std::vector<FDiff2D> left;
    std::vector<FDiff2D> right;

    // create a list of all control points
    unsigned int i = 0;
    for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        PT::ControlPoint point = *it;
        if ((point.image1Nr == m_leftImageNr) && (point.image2Nr == m_rightImageNr)){
            left.push_back(FDiff2D(point.x1,point.y1));
            right.push_back(FDiff2D(point.x2, point.y2));
            currentPoints.push_back(make_pair(it - controlPoints.begin(), *it));
            i++;
        } else if ((point.image2Nr == m_leftImageNr) && (point.image1Nr == m_rightImageNr)){
            point.mirror();
            mirroredPoints.insert(i);
            left.push_back(FDiff2D(point.x1, point.y1));
            right.push_back(FDiff2D(point.x2, point.y2));
            currentPoints.push_back(std::make_pair(it - controlPoints.begin(), point));
            i++;
        }
    }
    m_leftImg->setCtrlPoints(left);
    m_rightImg->setCtrlPoints(right);

    // put these control points into our listview.
    unsigned int selectedCP = UINT_MAX;
    for ( int i=0; i < m_cpList->GetItemCount() ; i++ ) {
      if ( m_cpList->GetItemState( i, wxLIST_STATE_SELECTED ) ) {
        selectedCP = i;            // remembers the old selection
      }
    }
    m_cpList->Hide();
    m_cpList->DeleteAllItems();

    for (unsigned int i=0; i < currentPoints.size(); ++i) {
        const ControlPoint & p = currentPoints[i].second;
        DEBUG_DEBUG("inserting LVItem " << i);
        m_cpList->InsertItem(i,wxString::Format(wxT("%d"),currentPoints[i].first));
        m_cpList->SetItem(i,1,wxString::Format(wxT("%.2f"),p.x1));
        m_cpList->SetItem(i,2,wxString::Format(wxT("%.2f"),p.y1));
        m_cpList->SetItem(i,3,wxString::Format(wxT("%.2f"),p.x2));
        m_cpList->SetItem(i,4,wxString::Format(wxT("%.2f"),p.y2));
        wxString mode;
        switch (p.mode) {
        case ControlPoint::X_Y:
            mode = _("normal");
            break;
        case ControlPoint::X:
            mode = _("vert. Line");
            break;
        case ControlPoint::Y:
            mode = _("horiz. Line");
        }
        m_cpList->SetItem(i,5,mode);
        m_cpList->SetItem(i,6,wxString::Format(wxT("%.2f"),p.error));
    }
    if ( selectedCP < (unsigned int) m_cpList->GetItemCount() ) { // sets an old selection again
        m_cpList->SetItemState( selectedCP,
                                wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
        m_cpList->EnsureVisible(selectedCP);
        m_selectedPoint = selectedCP;
        EnablePointEdit(true);

        const ControlPoint & p = currentPoints[m_selectedPoint].second;
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

    // autosize all columns // not needed , set defaults on InsertColum Kai-Uwe
/*    for (int i=0; i<7; i++) {
        m_cpList->SetColumnWidth(i,wxLIST_AUTOSIZE);
    }*/
    m_cpList->Show();
    // clear selectedPoint marker
}

void CPEditorPanel::EnablePointEdit(bool state)
    {
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
    ControlPoint cp = currentPoints[nr].second;

    // update point state
    if (!str2double(m_x1Text->GetValue(), cp.x1)) {
        m_x1Text->Clear();
        *m_x1Text << cp.x1;
        return;
    }
    if (!str2double(m_y1Text->GetValue(), cp.y1)) {
        m_y1Text->Clear();
        *m_y1Text << cp.y1;
        return;
    }
    if (!str2double(m_x2Text->GetValue(),cp.x2)) {
        m_x2Text->Clear();
        *m_x2Text << cp.x2;
        return;
    }
    if (!str2double(m_y2Text->GetValue(),cp.y2)) {
        m_y2Text->Clear();
        *m_y2Text << cp.x2;
        return;
    }

    switch(m_cpModeChoice->GetSelection()) {
    case 0:
        cp.mode = ControlPoint::X_Y;
        break;
    case 1:
        cp.mode = ControlPoint::X;
        break;
    case 2:
        cp.mode = ControlPoint::Y;
        break;
    default:
        DEBUG_FATAL("unkown control point type selected");
        return;
        break;
    }

    // if point was mirrored, reverse before setting it.
    if (set_contains(mirroredPoints, nr)) {
        cp.mirror();
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeCtrlPointCmd(*m_pano, currentPoints[nr].first, cp)
        );

}

void CPEditorPanel::OnLeftImgChange(wxNotebookEvent & e)
{
    DEBUG_TRACE("OnLeftImgChange() to " << e.GetSelection());
    if (m_listenToPageChange && e.GetSelection() >= 0) {
        setLeftImage((unsigned int) e.GetSelection());
    }
}

void CPEditorPanel::OnRightImgChange(wxNotebookEvent & e)
{
    DEBUG_TRACE("OnRightImgChange() to " << e.GetSelection());
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
}

void CPEditorPanel::OnKey(wxKeyEvent & e)
{
    DEBUG_DEBUG("key " << e.m_keyCode
                << " origin: id:" << e.m_id << " obj: "
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
            GlobalCmdHist::getInstance().addCommand(
                new PT::RemoveCtrlPointCmd(*m_pano,pNr)
                );
        }
    } else if (e.m_keyCode == '0') {
        wxCommandEvent dummy;
        dummy.SetInt(1);
        OnZoom(dummy);
        XRCCTRL(*this,"cp_editor_zoom_box",wxComboBox)->SetSelection(1);
    } else if (e.m_keyCode == '1') {
        wxCommandEvent dummy;
        dummy.SetInt(0);
        OnZoom(dummy);
        XRCCTRL(*this,"cp_editor_zoom_box",wxComboBox)->SetSelection(0);

#if 0
    } else if (e.m_keyCode == 'p') {
        // only estimate when there are control points.
        if (currentPoints.size() > 0) {
            if (cpCreationState == LEFT_POINT) {
                // jump to right point
                FDiff2D lp = m_leftImg->getNewPoint();
                FDiff2D t = EstimatePoint(FDiff2D(lp.x, lp.y), true);
                m_rightImg->showPosition(roundi(t.x),
                                         roundi(t.y), true);
            } else if (cpCreationState == RIGHT_POINT) {
                // jump to left point
                FDiff2D rp = m_rightImg->getNewPoint();
                FDiff2D t = EstimatePoint(FDiff2D(rp.x, rp.y), false);
                m_leftImg->showPosition(roundi(t.x),
                                        roundi(t.y), true);
            } else {
                if (e.GetEventObject() == m_leftImg) {
                    DEBUG_DEBUG("p pressed in left img");
                    // wrap pointer to other image
                    if (cpCreationState == LEFT_POINT ||
                        cpCreationState == RIGHT_POINT_RETRY ||
                        cpCreationState == LEFT_POINT_RETRY ||
                        cpCreationState == BOTH_POINTS_SELECTED)
                    {
                        FDiff2D p = m_leftImg->getNewPoint();
                        FDiff2D t = EstimatePoint(FDiff2D(p.x, p.y), true);
                        m_rightImg->showPosition(roundi(t.x),
                                                 roundi(t.y), true);
                    }
                } else if (e.GetEventObject() == m_rightImg) {
                    DEBUG_DEBUG("p pressed in right img");
                    // wrap pointer to other image
                    if (cpCreationState == RIGHT_POINT ||
                        cpCreationState == RIGHT_POINT_RETRY ||
                        cpCreationState == LEFT_POINT_RETRY ||
                        cpCreationState == BOTH_POINTS_SELECTED)
                    {
                        FDiff2D p = m_rightImg->getNewPoint();
                        FDiff2D t = EstimatePoint(FDiff2D(p.x, p.y), true);
                        m_leftImg->showPosition(roundi(t.x),
                                                roundi(t.y), true);
                    }
                }
            }
        } else {
	    wxLogError(_("Cannot estimate image position without control points"));
	}
#endif

    } else if (e.ControlDown() && e.GetKeyCode() == WXK_LEFT) {
        // move to next
        wxCommandEvent dummy;
        OnPrevImg(dummy);
    } else if (e.ControlDown() && e.GetKeyCode() == WXK_RIGHT) {
        // move to next
        wxCommandEvent dummy;
        OnNextImg(dummy);
    } else if (e.GetKeyCode() == 'f') {
        bool left =  e.GetEventObject() == m_leftImg;
        if (cpCreationState == NO_POINT) {
            FineTuneSelectedPoint(left);
        } else {
            FineTuneNewPoint(left);
        }
    } else {
        e.Skip();
    }
}

void CPEditorPanel::OnKeyUp(wxKeyEvent & e)
{
    DEBUG_TRACE("key:" << e.m_keyCode);
    if (e.ShiftDown()) {
        DEBUG_DEBUG("shift down");
    } else {
        e.Skip();
    }

}

void CPEditorPanel::OnKeyDown(wxKeyEvent & e)
{
    DEBUG_TRACE("key:" << e.m_keyCode);
    if (e.ShiftDown()) {
        DEBUG_DEBUG("shift down");
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

        GlobalCmdHist::getInstance().addCommand(
            new PT::RemoveCtrlPointCmd(*m_pano,pNr )
            );
    }
}

// show a global control point
void CPEditorPanel::ShowControlPoint(unsigned int cpNr)
{
    const ControlPoint & p = m_pano->getCtrlPoint(cpNr);
    setLeftImage(p.image1Nr);
    setRightImage(p.image2Nr);
    // FIXME reset display state
    changeState(NO_POINT);

    SelectGlobalPoint(cpNr);
}


void CPEditorPanel::OnAutoCreateCP()
{
    DEBUG_DEBUG("corner detection software");


    const PanoImage & limg = m_pano->getImage(m_leftImageNr);
    // run both images through the harris corner detector
    BImage leftImg = ImageCache::getInstance().getPyramidImage(
        limg.getFilename(),0);

    BImage leftCorners(leftImg.size());
    FImage leftCornerResponse(leftImg.size());

    // empty corner image
    leftCorners.init(0);

    DEBUG_DEBUG("running corner detector");

    // find corner response at scale 1.0
    vigra::cornerResponseFunction(srcImageRange(leftImg),
                                  destImage(leftCornerResponse),
                                  1.0);

//    saveScaledImage(leftCornerResponse,"corner_response.png");
    DEBUG_DEBUG("finding local maxima");

    // find local maxima of corner response, mark with 1
    vigra::localMaxima(srcImageRange(leftCornerResponse), destImage(leftCorners));
//    saveScaledImage(leftCornerResponse,"corner_response_maxima.png");

    DEBUG_DEBUG("thresholding corner response");
    // threshold corner response to keep only strong corners (above 400.0)
    transformImage(srcImageRange(leftCornerResponse), destImage(leftCornerResponse),
                   vigra::Threshold<double, double>(
                       400.0, DBL_MAX, 0.0, 1.0));
//    saveScaledImage(leftCornerResponse,"corner_response_maxima_thresh.png");

    DEBUG_DEBUG("combining corners and response");

    // combine thresholding and local maxima
    vigra::combineTwoImages(srcImageRange(leftCorners), srcImage(leftCornerResponse),
                            destImage(leftCorners), std::multiplies<float>());

    // save image

//    saveScaledImage(leftCornerResponse,"corners.png");
}


void CPEditorPanel::OnAutoAddCB(wxCommandEvent & e)
{
    // toggle auto add button

}


void CPEditorPanel::changeState(CPCreationState newState)
{
    DEBUG_TRACE(cpCreationState << " --> " << newState);
    // handle global state changes.
    bool fineTune = m_fineTuneCB->IsChecked();
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
            tmpEvt.m_commandInt = XRCCTRL(*this,"cp_editor_zoom_box",wxComboBox)->GetSelection();
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
        MainFrame::Get()->SetStatusText(_("Select Point in right image"),0);
        break;
    case RIGHT_POINT:
        m_leftImg->showSearchArea(fineTune);
        m_rightImg->showSearchArea(false);

        m_leftImg->showTemplateArea(false);
        m_rightImg->showTemplateArea(fineTune);

        ClearSelection();
        m_addButton->Enable(false);
        m_delButton->Enable(false);
        MainFrame::Get()->SetStatusText(_("Select Point in left image"),0);
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
        MainFrame::Get()->SetStatusText(_("change points, or press right mouse button to add the pair"));
    }
    // apply the change
    cpCreationState = newState;
}

void CPEditorPanel::OnPrevImg(wxCommandEvent & e)
{
    if (m_pano->getNrOfImages() < 2) return;
    int nImgs = m_leftTabs->GetPageCount();
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
    int nImgs = m_leftTabs->GetPageCount();
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
    } else if (BOTH_POINTS_SELECTED) {
        FineTuneNewPoint(false);
    }
}


FDiff2D CPEditorPanel::LocalFineTunePoint(unsigned int srcNr,
                                          const Diff2D & srcPnt,
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
    return result.maxpos;
}

void CPEditorPanel::FineTuneSelectedPoint(bool left)
{
    DEBUG_DEBUG(" selected Point: " << m_selectedPoint);
    if (m_selectedPoint == UINT_MAX) return;
    DEBUG_ASSERT(m_selectedPoint < currentPoints.size());

    ControlPoint cp = currentPoints[m_selectedPoint].second;

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

    FDiff2D result = LocalFineTunePoint(srcNr, srcPnt, moveNr, movePnt);

    if (left) {
       cp.x1 = result.x;
       cp.y1 = result.y;
       cp.x2 = srcPnt.x;
       cp.y2 = srcPnt.y;
    } else {
       cp.x2 = result.x;
       cp.y2 = result.y;
       cp.x1 = srcPnt.x;
       cp.y1 = srcPnt.y;
    }

    // if point was mirrored, reverse before setting it.
    if (set_contains(mirroredPoints, m_selectedPoint)) {
        cp.mirror();
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::ChangeCtrlPointCmd(*m_pano, currentPoints[m_selectedPoint].first, cp)
        );
}


void CPEditorPanel::FineTuneNewPoint(bool left)
{
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

    FDiff2D result = LocalFineTunePoint(srcNr, srcPnt, moveNr, movePnt);

    if (left) {
        m_leftImg->setNewPoint(result);
        m_leftImg->update();
        m_rightImg->setNewPoint(srcPnt);
        m_rightImg->update();

    } else {
        m_rightImg->setNewPoint(result);
        m_rightImg->update();
        m_leftImg->setNewPoint(srcPnt);
        m_leftImg->update();
    }
}


FDiff2D CPEditorPanel::EstimatePoint(const FDiff2D & p, bool left)
{
    int imgNr = left? m_rightImageNr : m_leftImageNr;
    const PanoImage & img = m_pano->getImage(imgNr);
    FDiff2D t;
    if (currentPoints.size() == 0) {
        DEBUG_WARN("Cannot estimate position without at least one point");
        return FDiff2D(0,0);
    }

    for (vector<CPoint>::const_iterator it = currentPoints.begin(); it != currentPoints.end(); ++it) {
        t.x += it->second.x2 - it->second.x1;
        t.y += it->second.y2 - it->second.y1;
    }
    t.x /= currentPoints.size();
    t.y /= currentPoints.size();
    DEBUG_DEBUG("estimated translation: x: " << t.x << " y: " << t.y);

    if (left) {
        t.x += p.x;
        t.y += p.y;
    } else {
        t.x = p.x - t.x;
        t.y = p.y - t.y;
    }

    // clip to fit to
    if (t.x < 0) t.x=0;
    if (t.y < 0) t.y=0;
    if (t.x > img.getWidth()) t.x = img.getWidth();
    if (t.y > img.getHeight()) t.y = img.getHeight();
    DEBUG_DEBUG("estimated point " << t.x << "," << t.y);
    return t;
}
