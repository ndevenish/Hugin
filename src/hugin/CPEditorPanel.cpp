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

#include <algorithm>

#include <vigra/imageiterator.hxx>
#include <vigra/stdimage.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/copyimage.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/correlation.hxx>

#include "common/utils.h"
#include "common/stl_utils.h"
#include "PT/PanoCommand.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/CPEditorPanel.h"


using namespace std;
using namespace PT;
using namespace vigra;
using namespace vigra::functor;

typedef vigra::ImageIterator<vigra::RGBValue<unsigned char> > wxImageIterator;

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

wxImageIterator
wxImageUpperLeft(wxImage & img)
{
    return wxImageIterator((vigra::RGBValue<unsigned char> *)img.GetData(), img.GetWidth());
}

wxImageIterator
wxImageLowerRight(wxImage & img)
{
    return wxImageUpperLeft(img) + vigra::Dist2D(img.GetWidth(), img.GetHeight());
}

BEGIN_EVENT_TABLE(CPEditorPanel, wxPanel)
    EVT_CPEVENT(CPEditorPanel::OnCPEvent)
    EVT_NOTEBOOK_PAGE_CHANGED ( XRCID("cp_editor_left_tab"),CPEditorPanel::OnLeftImgChange )
    EVT_NOTEBOOK_PAGE_CHANGED ( XRCID("cp_editor_right_tab"),CPEditorPanel::OnRightImgChange )
    EVT_LIST_ITEM_SELECTED(XRCID("cp_editor_cp_list"), CPEditorPanel::OnCPListSelect)
END_EVENT_TABLE()

CPEditorPanel::CPEditorPanel(wxWindow * parent, PT::Panorama * pano)
    : m_pano(pano), m_leftImageNr(UINT_MAX), m_rightImageNr(UINT_MAX),
      m_listenToPageChange(true), cpCreationState(NO_POINT)

{
    DEBUG_TRACE("");
    wxXmlResource::Get()->LoadPanel(this, parent, wxT("cp_editor_panel"));

    // left image
    m_leftTabs = XRCCTRL(*this, "cp_editor_left_tab", wxNotebook);
    m_leftImg = new CPImageCtrl(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_left_img"),
                                               m_leftImg);

    // right image
    m_rightTabs = XRCCTRL(*this, "cp_editor_right_tab", wxNotebook);
    m_rightImg = new CPImageCtrl(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_right_img"),
                                               m_rightImg);

    // setup list view
    m_cpList = XRCCTRL(*this, "cp_editor_cp_list", wxListCtrl);
    m_cpList->InsertColumn( 0, _("#"));
    m_cpList->InsertColumn( 1, _("left x"));
    m_cpList->InsertColumn( 2, _("left y"));
    m_cpList->InsertColumn( 3, _("right x"));
    m_cpList->InsertColumn( 4, _("right y"));
    m_cpList->InsertColumn( 5, _("Alignment"));
    m_cpList->InsertColumn( 6, _("Distance"));

    
    // other controls
    m_x1Text = XRCCTRL(*this,"cp_editor_x1", wxTextCtrl);
    m_y1Text = XRCCTRL(*this,"cp_editor_y1", wxTextCtrl);
    m_x2Text = XRCCTRL(*this,"cp_editor_x2", wxTextCtrl);
    m_y2Text = XRCCTRL(*this,"cp_editor_y2", wxTextCtrl);
    m_cpModeChoice = XRCCTRL(*this, "cp_editor_mode", wxChoice);

    // observe the panorama
    m_pano->addObserver(this);
}


CPEditorPanel::~CPEditorPanel()
{
    DEBUG_TRACE("");
    m_pano->addObserver(this);
}


void CPEditorPanel::setLeftImage(unsigned int imgNr)
{
    DEBUG_TRACE("image " << imgNr);
    if (m_leftImageNr != imgNr) {
        wxImage *ptr = ImageCache::getInstance().getImage(
            m_pano->getImage(imgNr).getFilename());
        m_leftImg->setImage(*ptr);
        m_leftTabs->SetSelection(imgNr);
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
        wxImage *ptr = ImageCache::getInstance().getImage(
            m_pano->getImage(imgNr).getFilename());
        m_rightImg->setImage(*ptr);
        // select tab
        m_rightTabs->SetSelection(imgNr);
        m_rightImageNr = imgNr;
        m_rightFile = m_pano->getImage(imgNr).getFilename();
        // update the rest of the display (new control points etc)
        UpdateDisplay();
    }
}


void CPEditorPanel::OnCPEvent( CPEvent&  ev)
{
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
        if (left) {
            CreateNewPointLeft(ev.getPoint());
        } else {
            CreateNewPointRight(ev.getPoint());
        }
        break;
    case CPEvent::POINT_SELECTED:
        SelectLocalPoint(nr);
        break;

    case CPEvent::POINT_CHANGED:
    {
        DEBUG_DEBUG("move point("<< nr << ")");
        assert(nr < currentPoints.size());
        ControlPoint cp = currentPoints[nr].second;

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
        text = "REGION_SELECTED";
        wxRect region = ev.getRect();
        vigra::CorrelationResult pos;
        ControlPoint point;
        DEBUG_DEBUG("left img: " << m_leftImageNr
                    << "  right img: " << m_rightImageNr);
        if (left) {
            if (FindTemplate(m_leftImageNr, region, m_rightImageNr, pos)) {
                point.image1Nr = m_leftImageNr;
                point.x1 = region.GetLeft();
                point.y1 = region.GetTop();
                point.image2Nr = m_rightImageNr;
                point.x2 = pos.xMax;
                point.y2 = pos.yMax;
                point.mode = PT::ControlPoint::X_Y;
            } else {
                DEBUG_DEBUG("No matching point found");
            }
        } else {
            if (FindTemplate(m_rightImageNr, region, m_leftImageNr, pos)) {
                point.image1Nr = m_leftImageNr;
                point.x1 = pos.xMax;
                point.y1 = pos.yMax;
                point.image2Nr = m_rightImageNr;
                point.x2 = region.GetLeft();
                point.y2 = region.GetTop();
                point.mode = PT::ControlPoint::X_Y;
            } else {
                DEBUG_DEBUG("No matching point found");
            }
        }

        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(*m_pano, point)
            );
        // select new control Point
        unsigned int lPoint = m_pano->getNrOfCtrlPoints() -1;
        SelectGlobalPoint(lPoint);

        break;
//    default:
//        text = "FATAL: unknown event mode";
    }
    }
}


void CPEditorPanel::SelectLocalPoint(unsigned int LVpointNr)
{
    DEBUG_TRACE("selectLocalPoint(" << LVpointNr << ")");

    // update point display (listview etc.) here
    m_cpList->SetItemState(LVpointNr, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    const ControlPoint & p = currentPoints[LVpointNr].second;
    m_x1Text->SetValue(wxString::Format("%.1f",p.x1));
    m_y1Text->SetValue(wxString::Format("%.1f",p.y1));
    m_x2Text->SetValue(wxString::Format("%.1f",p.x2));
    m_y2Text->SetValue(wxString::Format("%.1f",p.y2));
    m_cpModeChoice->SetSelection(p.mode);
//        m_leftImg->selectPoint(LVpointNr);
//        m_rightImg->selectPoint(LVpointNr);

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
    vector<CPoint>::const_iterator it;
    // just wanted to try the advanced stl stuff here.  this searches
    // the currentPoints list for a CPoint (std::pair), whose first
    // element (global point nr) matches pointNr
    it = find_if(currentPoints.begin(),
                 currentPoints.end(),
                 compose1(std::bind2nd(std::equal_to<unsigned int>(), globalNr),
                               select1st<CPoint>()));
    if (it != currentPoints.end()) {
        localNr = it - currentPoints.begin();
        return true;
    } else {
        return false;
    }
}


void CPEditorPanel::CreateNewPointLeft(wxPoint p)
{
    DEBUG_TRACE("CreateNewPointLeft");
    switch (cpCreationState) {
    case NO_POINT:
        cpCreationState = FIRST_POINT;
    case FIRST_POINT:
        newPoint = p;
        break;
    case SECOND_POINT:
        // FIXME: get OptimizeMode from somewhere
        ControlPoint point(m_leftImageNr, p.x, p.y,
                           m_rightImageNr, newPoint.x, newPoint.y,
                           PT::ControlPoint::X_Y);

        m_leftImg->clearNewPoint();
        m_rightImg->clearNewPoint();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(*m_pano, point)
            );
        // select new control Point
        unsigned int lPoint = m_pano->getNrOfCtrlPoints() -1;
        SelectGlobalPoint(lPoint);
    }
}

bool CPEditorPanel::FindTemplate(unsigned int tmplImgNr, const wxRect &region,
                                 unsigned int dstImgNr,
                                 vigra::CorrelationResult & res)
{
    DEBUG_TRACE("FindTemplate(): tmpl img nr: " << tmplImgNr << " corr src: "
                << dstImgNr);
    if (region.GetWidth() < 1 || region.GetHeight() < 1) {
        DEBUG_DEBUG("Can't correlate with templates < 1x1");
        return false;
    }
    wxImage * tmplImg = ImageCache::getInstance().getImage(
        m_pano->getImage(tmplImgNr).getFilename());
    wxImage * dstImg = ImageCache::getInstance().getImage(
        m_pano->getImage(dstImgNr).getFilename());

    // our template image
    wxImageIterator tmplUpperCorner = wxImageUpperLeft(*tmplImg)
                                      + vigra::Dist2D(region.GetLeft(), region.GetTop());
    wxImageIterator tmplLowerCorner = wxImageUpperLeft(*tmplImg)
                                      + vigra::Dist2D(region.GetRight(), region.GetBottom());
    vigra::BImage templ(region.GetWidth(), region.GetHeight());

    vigra::copyImage(tmplUpperCorner,
                     tmplLowerCorner,
                     RGBToGrayAccessor<RGBValue<unsigned char> >(),
                     templ.upperLeft(),
                     StandardValueAccessor<unsigned char>());

//    exportImage(srcImageRange(templ), vigra::ImageExportInfo("template_normal.gif"));
//    exportImage(srcImageRange(templ), vigra::ImageExportInfo("template.gif"));
    vigra::BImage dst(dstImg->GetWidth(), dstImg->GetHeight());

    DEBUG_DEBUG("dest image to grey");
    vigra::copyImage(wxImageUpperLeft(*dstImg),
                     wxImageLowerRight(*dstImg),
                     RGBToGrayAccessor<RGBValue<unsigned char> >(),
                     dst.upperLeft(),
                     StandardValueAccessor<unsigned char>());

//    ToGray(wxImageUpperLeft(*dstImg), wxImageLowerRight(*dstImg),
//           dst.upperLeft());

//    DEBUG_DEBUG("exporting image");
//    exportImage(srcImageRange(dst), vigra::ImageExportInfo("src_correlation.gif"));

    vigra::BImage corr_res(dst.width(), dst.height());
    DEBUG_DEBUG("correlating image");
    // correlate Image
    res = vigra::correlateImage(dst.upperLeft(),
                              dst.lowerRight(),
                              StandardValueAccessor<unsigned char>(),
                              corr_res.upperLeft(),
                              StandardValueAccessor<unsigned char>(),
                              templ.upperLeft(),
                              StandardValueAccessor<unsigned char>(),
                              Diff2D(0,0),
                              templ.size()
            );
    DEBUG_DEBUG("correlation: max=" << res.max/127 - 1.0 << ", " << res.max
                << " at: " << res.xMax << ","
                << res.yMax);
//    DEBUG_DEBUG("exporting correlated image");
//    exportImage(srcImageRange(corr_res), vigra::ImageExportInfo("correlation.gif"));
    // FIXME use a threshold set by the user, or calculate a sensible one.
    if (res.max > 0.7) {
        return true;
    }
    return false;
}


void CPEditorPanel::CreateNewPointRight(wxPoint p)
{
    DEBUG_TRACE("CreateNewPointRight");
    switch (cpCreationState) {
    case NO_POINT:
        cpCreationState = SECOND_POINT;
    case SECOND_POINT:
        newPoint = p;
        break;
    case FIRST_POINT:
        // FIXME: get OptimizeMode from somewhere
        ControlPoint point(m_leftImageNr, newPoint.x, newPoint.y,
                           m_rightImageNr, p.x, p.y,
                           PT::ControlPoint::X_Y);
        m_leftImg->clearNewPoint();
        m_rightImg->clearNewPoint();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(*m_pano, point)
            );
        // select new control Point
        unsigned int lPoint = m_pano->getNrOfCtrlPoints() -1;
        SelectGlobalPoint(lPoint);
    }
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
            wxWindow* t2= new wxWindow(m_rightTabs,-1,wxPoint(0,0),wxSize(0,0));
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
                wxImage *ptr = ImageCache::getInstance().getImage(
                    pano.getImage(nrImages-1).getFilename());
                m_leftImg->setImage(*ptr);
            }
            if (right >= (int)nrImages) {
                setRightImage(nrImages -1);
                wxImage *ptr = ImageCache::getInstance().getImage(
                    pano.getImage(nrImages-1).getFilename());
                m_rightImg->setImage(*ptr);
            }
        } else {
            m_leftImageNr = UINT_MAX;
            m_rightImageNr = UINT_MAX;
            // no image anymore..
            m_leftImg->setImage(0);
            m_rightImg->setImage(0);
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
        if (m_leftImageNr == imgNr) {
            if (m_leftFile != pano.getImage(imgNr).getFilename()) {
                wxImage *ptr = ImageCache::getInstance().getImage(
                    pano.getImage(imgNr).getFilename());
                m_leftImg->setImage(*ptr);
                m_leftFile = pano.getImage(imgNr).getFilename();
            }
            update=true;
        }

        if (m_rightImageNr == imgNr) {
            if (m_rightFile != pano.getImage(imgNr).getFilename()) {
                wxImage *ptr = ImageCache::getInstance().getImage(
                    pano.getImage(imgNr).getFilename());
                m_rightImg->setImage(*ptr);
                m_rightFile = pano.getImage(imgNr).getFilename();
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
    m_cpList->Hide();
    m_cpList->DeleteAllItems();
    
    for (unsigned int i=0; i < currentPoints.size(); ++i) {
        const ControlPoint & p = currentPoints[i].second;
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
        m_cpList->SetItem(i,6,wxString::Format("%f",p.error));
    }
    // autosize all columns
    for (int i=0; i<7; i++) {
        m_cpList->SetColumnWidth(i,wxLIST_AUTOSIZE);
    }
    m_cpList->Show();
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
    if (t >0) {
        SelectLocalPoint((unsigned int) t);
    }
}
