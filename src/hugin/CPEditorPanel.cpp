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

//-----------------------------------------------------------------------------
// Standard wxWindows headers
//-----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/xrc/xmlres.h"              // XRC XML resouces
#include "wx/notebook.h"
#include "wx/listctrl.h"
#include "wx/config.h"

#include <algorithm>

#include <float.h>

#include "vigra/cornerdetection.hxx"
#include "vigra/localminmax.hxx"

#include "common/utils.h"
#include "common/stl_utils.h"
#include "PT/PanoCommand.h"

#include "hugin/huginApp.h"
#include "hugin/ImageProcessing.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/CPImageCtrl.h"

#include "hugin/CPEditorPanel.h"


using namespace std;
using namespace PT;
using namespace vigra;
using namespace vigra::functor;

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
    EVT_BUTTON(XRCID("cp_editor_add_button"), CPEditorPanel::OnAddButton)
    EVT_CHECKBOX(XRCID("cp_editor_auto_add_cb"), CPEditorPanel::OnAutoAddCB)
END_EVENT_TABLE()

CPEditorPanel::CPEditorPanel(wxWindow * parent, PT::Panorama * pano)
    : m_pano(pano), m_leftImageNr(UINT_MAX), m_rightImageNr(UINT_MAX),
      m_listenToPageChange(true), cpCreationState(NO_POINT),
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
    m_y1Text = XRCCTRL(*this,"cp_editor_y1", wxTextCtrl);
    m_x2Text = XRCCTRL(*this,"cp_editor_x2", wxTextCtrl);
    m_y2Text = XRCCTRL(*this,"cp_editor_y2", wxTextCtrl);
    m_cpModeChoice = XRCCTRL(*this, "cp_editor_mode", wxChoice);

    m_autoAddCB = XRCCTRL(*this,"cp_editor_auto_add", wxCheckBox);
    DEBUG_ASSERT(m_autoAddCB);

    wxConfigBase *config = wxConfigBase::Get();
    
    m_rightImg->showTemplateArea( config->Read("/CPEditorPanel/templateSize",14l));
    m_leftImg->showTemplateArea( config->Read("/CPEditorPanel/templateSize",14l));
    
    bool autoAdd = config->Read("/CPEditorPanel/autoAdd",1l);
    m_autoAddCB->SetValue(autoAdd);
    
    // observe the panorama
    m_pano->addObserver(this);
}


CPEditorPanel::~CPEditorPanel()
{
    DEBUG_TRACE("dtor");
    wxConfigBase::Get()->Write("/CPEditorPanel/autoAdd", m_autoAddCB->IsChecked() ? 1 : 0);
    m_pano->removeObserver(this);
    DEBUG_TRACE("dtor end");
}


void CPEditorPanel::setLeftImage(unsigned int imgNr)
{
    DEBUG_TRACE("image " << imgNr);
    if (m_leftImageNr != imgNr) {
        m_leftImg->setImage(m_pano->getImage(imgNr).getFilename());
        if (m_leftTabs->GetSelection() != (int) imgNr) {
            m_leftTabs->SetSelection(imgNr);
        }
        m_leftImageNr = imgNr;
        m_leftFile = m_pano->getImage(imgNr).getFilename();
        UpdateDisplay();
    }
}


void CPEditorPanel::setRightImage(unsigned int imgNr)
{
    DEBUG_TRACE("image " << imgNr);
    if (m_rightImageNr != imgNr) {
        // set the new image
        m_rightImg->setImage(m_pano->getImage(imgNr).getFilename());
        // select tab
        if (m_rightTabs->GetSelection() != (int) imgNr) {
            m_rightTabs->SetSelection(imgNr);
        }
        m_rightImageNr = imgNr;
        m_rightFile = m_pano->getImage(imgNr).getFilename();
        // update the rest of the display (new control points etc)
        UpdateDisplay();
    }
}


void CPEditorPanel::OnCPEvent( CPEvent&  ev)
{
    DEBUG_TRACE("");
    wxString text;
    unsigned int nr = ev.getPointNr();
    wxPoint point = ev.getPoint();
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
        text = "NONE";
        break;
    case CPEvent::NEW_POINT_CHANGED:
        NewPointChange(ev.getPoint(),left);
        break;
    case CPEvent::POINT_SELECTED:
        // need to reset cpEditState
        DEBUG_DEBUG("selected point " << nr);
        SelectLocalPoint(nr);
        cpCreationState = NO_POINT;
        break;

    case CPEvent::POINT_CHANGED:
    {
        DEBUG_DEBUG("move point("<< nr << ")");
        assert(nr < currentPoints.size());
        ControlPoint cp = currentPoints[nr].second;
        cpCreationState = NO_POINT;

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
        GlobalCmdHist::getInstance().addCommand(
            new PT::ChangeCtrlPointCmd(*m_pano, currentPoints[nr].first, cp)
            );

        break;
    }
    case CPEvent::REGION_SELECTED:
    {
        cpCreationState = NO_POINT;
	if (false) {
            text = "REGION_SELECTED";
            wxRect region = ev.getRect();
            int dx = region.GetWidth() / 2;
            int dy = region.GetHeight() / 2;
            CorrelationResult pos;
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
                    point.x2 = pos.pos.x + dx;
                    point.y2 = pos.pos.y + dy;
                    point.mode = PT::ControlPoint::X_Y;
                    found = true;
                } else {
                    DEBUG_DEBUG("No matching point found");
                }
            } else {
                if (FindTemplate(m_rightImageNr, region, m_leftImageNr, pos)) {
                    point.image1Nr = m_leftImageNr;
                    point.x1 = pos.pos.x + dx;
                    point.y1 = pos.pos.y + dy;
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
            } else {
                wxLogError("No corrosponding point found");
            }
        }
        break;
    }
    case CPEvent::RIGHT_CLICK:
    {
        if (cpCreationState == BOTH_POINTS_SELECTED) {
            DEBUG_DEBUG("right click -> adding point");
            CreateNewPoint();
        } else {
            DEBUG_DEBUG("right click without two points..");
        }
        break;
    }
//    default:
//        text = "FATAL: unknown event mode";
    }
}


void CPEditorPanel::CreateNewPoint()
{
    DEBUG_TRACE("");
//    DEBUG_ASSERT(m_leftImg->GetState == NEW_POINT_SELECTED);
//    DEBUG_ASSERT(m_rightImg->GetState == NEW_POINT_SELECTED);
    wxPoint p1 = m_leftImg->getNewPoint();
    wxPoint p2 = m_rightImg->getNewPoint();
    ControlPoint point;
    point.image1Nr = m_leftImageNr;
    point.x1 = p1.x;
    point.y1 = p1.y;
    point.image2Nr = m_rightImageNr;
    point.x2 = p2.x;
    point.y2 = p2.y;
    point.mode = PT::ControlPoint::X_Y;

    m_leftImg->clearNewPoint();
    m_rightImg->clearNewPoint();
    cpCreationState = NO_POINT;

    // create points
    GlobalCmdHist::getInstance().addCommand(
        new PT::AddCtrlPointCmd(*m_pano, point)
        );

    // reset zoom to previous setting
    wxCommandEvent tmpEvt;
    tmpEvt.m_commandInt = XRCCTRL(*this,"cp_editor_zoom_box",wxComboBox)->GetSelection();
    OnZoom(tmpEvt);

    m_leftImg->hideSearchArea();
    m_rightImg->hideSearchArea();

    // select new control Point
    unsigned int lPoint = m_pano->getNrOfCtrlPoints() -1;
    SelectGlobalPoint(lPoint);
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
    m_x1Text->SetValue(wxString::Format("%.1f",p.x1));
    m_y1Text->SetValue(wxString::Format("%.1f",p.y1));
    m_x2Text->SetValue(wxString::Format("%.1f",p.x2));
    m_y2Text->SetValue(wxString::Format("%.1f",p.y2));
    m_cpModeChoice->SetSelection(p.mode);
    m_leftImg->selectPoint(LVpointNr);
    m_rightImg->selectPoint(LVpointNr);
    m_cpList->SetItemState(LVpointNr, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    m_cpList->EnsureVisible(LVpointNr);

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


void CPEditorPanel::NewPointChange(wxPoint p, bool left)
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
        cpCreationState = THIS_POINT;
        // zoom into our window
        if (thisImg->getScale() < 1) {
            thisImg->setScale(1);
            thisImg->showPosition(p.x,p.y);
        }

        // display search area for the other image
        if (XRCCTRL(*this,"cp_editor_fine_tune_check",wxCheckBox)->IsChecked()) {
            int width = m_pano->getImage(otherImgNr).getWidth();
            int templSearchAreaPercent = wxConfigBase::Get()->Read("/CPEditorPanel/templateSearchAreaPercent",10);
            int swidth = (int) (width * templSearchAreaPercent / 200);
            otherImg->showSearchArea(swidth);
            g_MainFrame->SetStatusText("Select Point in other image",0);
        }
    } else if (cpCreationState == THIS_POINT || cpCreationState == OTHER_POINT_RETRY) {
        thisImg->showPosition(p.x,p.y);
        // FIXME approximate position in the other image, and warp cursor
        // there.
    } else if (cpCreationState == OTHER_POINT || cpCreationState == THIS_POINT_RETRY) {
        FDiff2D p2;
        p2.x = -1;
        p2.y = -1;
        // the try for the second point.
        if (cpCreationState == OTHER_POINT) {
            // other point already selected, finalize point.

            if (XRCCTRL(*this,"cp_editor_fine_tune_check",wxCheckBox)->IsChecked()) {
                g_MainFrame->SetStatusText("searching similar point...",0);
                wxPoint newPoint = otherImg->getNewPoint();
                double xcorr = PointFineTune(otherImgNr,
                                             Diff2D(newPoint.x, newPoint.y),
                                             thisImgNr,
                                             Diff2D(p.x, p.y),
                                             p2);
                wxString str = wxConfigBase::Get()->Read("/CPEditorPanel/finetuneThreshold","0.8");
                double thresh = utils::lexical_cast<double>(str);
                if (xcorr < thresh) {
                    // low xcorr
                    // zoom to 100 percent. & set second stage
                    // to abandon finetune this time.
                    thisImg->setScale(1);
                    thisImg->setNewPoint(wxPoint((int)round(p2.x), 
                                                 (int)round(p2.y) ));

                    // Bad correlation result.
                    int answer = wxMessageBox(
                        wxString::Format(_("low correlation coefficient: %f, (threshold: %f)\nPoint might be wrong. Select anyway?"),  xcorr, thresh),
                        "Low correlation",
                        wxYES_NO|wxICON_QUESTION, this);
                    if (answer == wxNO) {
                        cpCreationState = THIS_POINT_RETRY;
                        thisImg->clearNewPoint();
                        return;
                    } else {
                        cpCreationState = BOTH_POINTS_SELECTED;
                    }
                } else {
                    // show point & zoom in if auto add is not set
                    cpCreationState = BOTH_POINTS_SELECTED;
                    if (!m_autoAddCB->IsChecked()) {
                        thisImg->setScale(1);
                    }
                    thisImg->setNewPoint(wxPoint((int)round(p2.x), 
                                                 (int)round(p2.y) ));
                }

                g_MainFrame->SetStatusText(wxString::Format("found corrosponding point, mean xcorr coefficient: %f",xcorr),0);
                
            } else {
                // no finetune. but zoom into picture, when we where zoomed out
                if (thisImg->getScale() < 1) {
                    // zoom to 100 percent. & set second stage
                    // to abandon finetune this time.
                    thisImg->setScale(1);
                    thisImg->clearNewPoint();
                    thisImg->showPosition(p.x, p.y);
                    //thisImg->setNewPoint(p.x, p.y);
                    cpCreationState = THIS_POINT_RETRY;
                    return;
                } else {
                    // point is already set. no need to move.
                    //setNewPoint(p);
                    cpCreationState = BOTH_POINTS_SELECTED;
                }
            }
        } else {
            // selection retry
            // nothing special, no second stage fine tune yet.
        }
        
        // ok, we have determined the other point.. apply if auto add is on
        if (m_autoAddCB->IsChecked()) {
            CreateNewPoint();
        } else {
            // keep both point floating around, until they are
            // added with a right mouse click or the add button
            cpCreationState = BOTH_POINTS_SELECTED;
        }
            
    } else if (cpCreationState == BOTH_POINTS_SELECTED) {
        // nothing to do.. maybe a special fine tune with
        // a small search region
        
    } else {
        // should never reach this, else state machine is broken.
        DEBUG_ASSERT(0);
    }
}


#if 0
void CPEditorPanel::CreateNewPointRight(wxPoint p)
{
    DEBUG_TRACE("CreateNewPointRight");
    switch (cpCreationState) {
    case NO_POINT:
        cpCreationState = RIGHT_POINT;
        // show search area
        if (XRCCTRL(*this,"cp_editor_fine_tune_check",wxCheckBox)->IsChecked()) {
            int width = m_pano->getImage(m_leftImageNr).getWidth();
            int templSearchAreaPercent = wxConfigBase::Get()->Read("/CPEditorPanel/templateSearchAreaPercent",10);
            int swidth = (int) (width * templSearchAreaPercent / 200);
            m_leftImg->showSearchArea(swidth);
            g_MainFrame->SetStatusText("Select Point in right image",0);
        }
    case RIGHT_POINT:
        newPoint = p;
        // FIXME approximate position in left image

        break;
    case LEFT_POINT:
        FDiff2D p2;
        if (XRCCTRL(*this,"cp_editor_fine_tune_check",wxCheckBox)->IsChecked()) {
            g_MainFrame->SetStatusText("searching similar point...",0);
            double xcorr = PointFineTune(m_leftImageNr,
                                         Diff2D(newPoint.x,newPoint.y),
                                         m_rightImageNr,
                                         Diff2D(p.x, p.y),
                                         p2);
            wxString str = wxConfigBase::Get()->Read("/CPEditorPanel/finetuneThreshold","0.8");
            double thresh = utils::lexical_cast<double>(str);
            if (xcorr < thresh) {
                // Bad correlation result.
                wxLogError(wxString::Format(_("could not find corrosponding point, low xcorr: %f, (threshold: %f)"),  xcorr, thresh));
                m_rightImg->clearNewPoint();
                return;
            }
            g_MainFrame->SetStatusText(wxString::Format("found corrosponding point, mean xcorr coefficient: %f",xcorr),0);
        } else {
            p2.x = p.x;
            p2.y = p.y;
        }
        // FIXME: get OptimizeMode from somewhere
        ControlPoint point(m_leftImageNr, newPoint.x, newPoint.y,
                           m_rightImageNr, p2.x, p2.y,
                           PT::ControlPoint::X_Y);
        m_leftImg->clearNewPoint();
        m_rightImg->clearNewPoint();

        m_rightImg->hideSearchArea();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(*m_pano, point)
            );
        // select new control Point
        unsigned int lPoint = m_pano->getNrOfCtrlPoints() -1;
        SelectGlobalPoint(lPoint);

    }
}

#endif

bool CPEditorPanel::FindTemplate(unsigned int tmplImgNr, const wxRect &region,
                                 unsigned int dstImgNr,
                                 CorrelationResult & res)
{
    DEBUG_TRACE("FindTemplate(): tmpl img nr: " << tmplImgNr << " corr src: "
                << dstImgNr);
    if (region.GetWidth() < 1 || region.GetHeight() < 1) {
        DEBUG_DEBUG("Can't correlate with templates < 1x1");
        return false;
    }
    const BImage & tmplsrc = ImageCache::getInstance().getPyramidImage(
        m_pano->getImage(tmplImgNr).getFilename(),0);

    Diff2D tOrigin(region.x, region.y);
    Diff2D tSize(region.width, region.height);
    vigra::BImage templ(tSize);

    vigra::copyImage(tmplsrc.upperLeft() + tOrigin,
                     tmplsrc.upperLeft() + tOrigin + tSize,
                     tmplsrc.accessor(),
                     templ.upperLeft(),
                     templ.accessor()
        );

    findTemplate(templ, m_pano->getImage(dstImgNr).getFilename(), res);
    // FIXME. make this configureable. 0.5 is a quite low value. 0.7 or
    // so is more acceptable. but some features only match with 0.5.
    // but we get more false positives..
    if (res.max > 0.5) {
        return true;
    }
    return false;
}


double CPEditorPanel::PointFineTune(unsigned int tmplImgNr,
                                  const Diff2D & tmplPoint,
                                  unsigned int subjImgNr,
                                  const Diff2D & subjPoint,
                                  FDiff2D & tunedPos)
{
    DEBUG_TRACE("tmpl img nr: " << tmplImgNr << " corr src: "
                << subjImgNr);

    long templSize = wxConfigBase::Get()->Read("/CPEditorPanel/templateSize",14);
    long templSearchAreaPercent = wxConfigBase::Get()->Read("/CPEditorPanel/templateSearchAreaPercent",10);

    const PanoImage & img = m_pano->getImage(subjImgNr);

    const BImage & subjImg = ImageCache::getInstance().getPyramidImage(
        img.getFilename(),0);

    // FIXME user configurable search window?
    int swidth = (int) (subjImg.width() * templSearchAreaPercent / 200);
    DEBUG_DEBUG("search window half width/height: " << swidth << "x" << swidth);
    Diff2D searchUL(subjPoint.x - swidth, subjPoint.y - swidth);
    Diff2D searchLR(subjPoint.x + swidth, subjPoint.y + swidth);
    // clip search window
    if (searchUL.x < 0) searchUL.x = 0;
    if (searchUL.y < 0) searchUL.y = 0;
    if (searchLR.x > subjImg.width()) searchLR.x = subjImg.width();
    if (searchLR.y > subjImg.height()) searchLR.y = subjImg.height();
    DEBUG_DEBUG("search borders: " << searchLR.x << "," << searchLR.y);
    Diff2D searchSize = searchLR - searchUL;

    const BImage & tmplImg = ImageCache::getInstance().getPyramidImage(
        m_pano->getImage(tmplImgNr).getFilename(),0);

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
    DEBUG_DEBUG("starting fine tune");
    // we could use the multiresolution version as well.
    // but usually the region is quite small.
    CorrelationResult res;
    res = correlateImage(subjImg.upperLeft() + searchUL,
                         subjImg.upperLeft() + searchLR,
                         subjImg.accessor(),
                         dest.upperLeft(),
                         dest.accessor(),
                         tmplImg.upperLeft() + tmplPoint,
                         tmplImg.accessor(),
                         tmplUL, tmplLR, -1);

    res.pos += searchUL;
    DEBUG_DEBUG("normal search finished, max:" << res.max
                << " at " << res.pos.x << "," << res.pos.y);

    tunedPos.x = res.pos.x;
    tunedPos.y = res.pos.y;
    return res.max;
}


void CPEditorPanel::panoramaChanged(PT::Panorama &pano)
{
    DEBUG_TRACE("");
}

void CPEditorPanel::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrTabs = m_leftTabs->GetPageCount();
    DEBUG_TRACE("nrImages:" << nrImages << " nrTabs:" << nrTabs);

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
            if (!m_leftTabs->AddPage(t1, wxString::Format("%d",i))) {
                DEBUG_FATAL("could not add dummy window to left notebook");
            }
            if (!m_rightTabs->AddPage(t2, wxString::Format("%d",i))){
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
    for(UIntSet::iterator it = changed.begin(); it != changed.end(); ++it) {
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

    if (update) {
        UpdateDisplay();
    }

    // if there is no selection, select the first one.
    if (nrImages > 0 && nrTabs == 0) {
        setLeftImage(0);
        setRightImage(0);
    }
}

void CPEditorPanel::UpdateDisplay()
{
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
    std::vector<wxPoint> left;
    std::vector<wxPoint> right;

    // create a list of all control points
    unsigned int i = 0;
    for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        PT::ControlPoint point = *it;
        if ((point.image1Nr == m_leftImageNr) && (point.image2Nr == m_rightImageNr)){
            left.push_back(wxPoint( (int) point.x1, (int) point.y1));
            right.push_back(wxPoint( (int) point.x2, (int) point.y2));
            currentPoints.push_back(make_pair(it - controlPoints.begin(), *it));
            i++;
        } else if ((point.image2Nr == m_leftImageNr) && (point.image1Nr == m_rightImageNr)){
            point.mirror();
            mirroredPoints.insert(i);
            left.push_back(wxPoint( (int) point.x1, (int) point.y1));
            right.push_back(wxPoint( (int) point.x2, (int) point.y2));
            currentPoints.push_back(std::make_pair(it - controlPoints.begin(), point));
            i++;
        }
    }
    m_leftImg->setCtrlPoints(left);
    m_rightImg->setCtrlPoints(right);

    // put these control points into our listview.
    int selectedCP = INT_MAX;
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
        m_cpList->InsertItem(i,wxString::Format("%d",currentPoints[i].first));
        m_cpList->SetItem(i,1,wxString::Format("%.1f",p.x1));
        m_cpList->SetItem(i,2,wxString::Format("%.1f",p.y1));
        m_cpList->SetItem(i,3,wxString::Format("%.1f",p.x2));
        m_cpList->SetItem(i,4,wxString::Format("%.1f",p.y2));
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
        m_cpList->SetItem(i,6,wxString::Format("%.1f",p.error));
    }
    if ( selectedCP < m_cpList->GetItemCount() ) { // sets an old selection again
        m_cpList->SetItemState( selectedCP,
                                wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
        m_cpList->EnsureVisible(selectedCP);
    }
    // autosize all columns // not needed , set defaults on InsertColum Kai-Uwe
/*    for (int i=0; i<7; i++) {
        m_cpList->SetColumnWidth(i,wxLIST_AUTOSIZE);
    }*/
    m_cpList->Show();
    // clear selectedPoint marker
    m_selectedPoint = selectedCP;
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
    if (!m_x1Text->GetValue().ToDouble(&cp.x1)) {
        wxBell();
        m_x1Text->Clear();
        *m_x1Text << cp.x1;
        return;
    }
    if (!m_y1Text->GetValue().ToDouble(&cp.y1)) {
        wxBell();
        m_y1Text->Clear();
        *m_y1Text << cp.y1;
        return;
    }
    if (!m_x2Text->GetValue().ToDouble(&cp.x2)) {
        wxBell();
        m_x2Text->Clear();
        *m_x2Text << cp.x2;
        return;
    }
    if (!m_y2Text->GetValue().ToDouble(&cp.y2)) {
        wxBell();
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
    }
}

void CPEditorPanel::OnZoom(wxCommandEvent & e)
{
    double factor;
    switch (e.GetSelection()) {
    case 0:
        factor = 1;
        break;
    case 1:
        // fit to window
        factor = 0;
        break;
    case 2:
        factor = 2;
        break;
    case 3:
        factor = 1.5;
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
    DEBUG_TRACE("key:" << e.m_keyCode);
    if (e.m_keyCode == WXK_DELETE){
        DEBUG_DEBUG("Delete pressed");
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

void CPEditorPanel::OnDeleteButton(wxCommandEvent & e)
{
    DEBUG_TRACE("");
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

// show a global control point
void CPEditorPanel::ShowControlPoint(unsigned int cpNr)
{
    const ControlPoint & p = m_pano->getCtrlPoint(cpNr);
    setLeftImage(p.image1Nr);
    setRightImage(p.image2Nr);

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

    saveScaledImage(leftCornerResponse,"corner_response.png");
    DEBUG_DEBUG("finding local maxima");

    // find local maxima of corner response, mark with 1
    vigra::localMaxima(srcImageRange(leftCornerResponse), destImage(leftCorners));
    saveScaledImage(leftCornerResponse,"corner_response_maxima.png");

    DEBUG_DEBUG("thresholding corner response");
    // threshold corner response to keep only strong corners (above 400.0)
    transformImage(srcImageRange(leftCornerResponse), destImage(leftCornerResponse),
                   vigra::Threshold<double, double>(
                       400.0, DBL_MAX, 0.0, 1.0));
    saveScaledImage(leftCornerResponse,"corner_response_maxima_thresh.png");

    DEBUG_DEBUG("combining corners and response");

    // combine thresholding and local maxima
    vigra::combineTwoImages(srcImageRange(leftCorners), srcImage(leftCornerResponse),
                            destImage(leftCorners), std::multiplies<float>());

    // save image

    saveScaledImage(leftCornerResponse,"corners.png");
}


void CPEditorPanel::OnAddButton(wxCommandEvent & e)
{
    // add a point
    
}

void CPEditorPanel::OnAutoAddCB(wxCommandEvent & e)
{
    // toggle auto add button
    
}
