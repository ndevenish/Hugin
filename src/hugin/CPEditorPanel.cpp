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
#include "wx/tabctrl.h"
#include "wx/tab.h"

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
    EVT_BUTTON( XRCID("button_wide"), CPEditorPanel::OnMyButtonClicked )
    EVT_CPEVENT(CPEditorPanel::OnCPEvent)
    EVT_NOTEBOOK_PAGE_CHANGED ( XRCID("cp_editor_left_tab"),CPEditorPanel::OnLeftImgChange )
    EVT_NOTEBOOK_PAGE_CHANGED ( XRCID("cp_editor_right_tab"),CPEditorPanel::OnRightImgChange )
END_EVENT_TABLE()

CPEditorPanel::CPEditorPanel(wxWindow * parent, PT::Panorama * pano)
    : pano(pano), leftImage(0), rightImage(0),
      cpCreationState(NO_POINT)

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

    // observe the panorama
    pano->addObserver(this);
}


CPEditorPanel::~CPEditorPanel()
{
    DEBUG_TRACE("");
    pano->addObserver(this);
}


void CPEditorPanel::setLeftImage(unsigned int imgNr)
{
    // get image
    m_leftTabs->SetSelection(imgNr);
}


void CPEditorPanel::setRightImage(unsigned int imgNr)
{
    m_rightTabs->SetSelection(imgNr);
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
            new PT::ChangeCtrlPointCmd(*pano, currentPoints[nr].first, cp)
            );

        break;
    }
    case CPEvent::REGION_SELECTED:
    {
        text = "REGION_SELECTED";
        wxRect region = ev.getRect();
        vigra::CorrelationResult pos;
        ControlPoint point;
        DEBUG_DEBUG("left img: " << leftImage
                    << "  right img: " << rightImage);
        if (left) {
            if (FindTemplate(leftImage, region, rightImage, pos)) {
                point.image1Nr = leftImage;
                point.x1 = region.GetLeft();
                point.y1 = region.GetTop();
                point.image2Nr = rightImage;
                point.x2 = pos.xMax;
                point.y2 = pos.yMax;
                point.mode = PT::ControlPoint::X_Y;
            } else {
                DEBUG_DEBUG("No matching point found");
            }
        } else {
            if (FindTemplate(rightImage, region, leftImage, pos)) {
                point.image1Nr = leftImage;
                point.x1 = pos.xMax;
                point.y1 = pos.yMax;
                point.image2Nr = rightImage;
                point.x2 = region.GetLeft();
                point.y2 = region.GetTop();
                point.mode = PT::ControlPoint::X_Y;
            } else {
                DEBUG_DEBUG("No matching point found");
            }
        }

        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(*pano, point)
            );
        // select new control Point
        unsigned int lPoint = pano->getNrOfCtrlPoints() -1;
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

    // FIXME: update point display (listview etc.) here
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
        ControlPoint point(leftImage, p.x, p.y,
                           rightImage, newPoint.x, newPoint.y,
                           PT::ControlPoint::X_Y);

        m_leftImg->clearNewPoint();
        m_rightImg->clearNewPoint();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(*pano, point)
            );
        // select new control Point
        unsigned int lPoint = pano->getNrOfCtrlPoints() -1;
        SelectGlobalPoint(lPoint);
    }
}

bool CPEditorPanel::FindTemplate(unsigned int tmplImgNr, const wxRect &region,
                                 unsigned int dstImgNr,
                                 vigra::CorrelationResult & res)
{
    DEBUG_TRACE("FindTemplate(): tmpl img nr: " << tmplImgNr << " corr src: "
                << dstImgNr);
    wxImage * tmplImg = ImageCache::getInstance().getImage(
        pano->getImage(tmplImgNr).getFilename());
    wxImage * dstImg = ImageCache::getInstance().getImage(
        pano->getImage(dstImgNr).getFilename());

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
        ControlPoint point(leftImage, newPoint.x, newPoint.y,
                           rightImage, p.x, p.y,
                           PT::ControlPoint::X_Y);
        m_leftImg->clearNewPoint();
        m_rightImg->clearNewPoint();
        cpCreationState = NO_POINT;
        GlobalCmdHist::getInstance().addCommand(
            new PT::AddCtrlPointCmd(*pano, point)
            );
        // select new control Point
        unsigned int lPoint = pano->getNrOfCtrlPoints() -1;
        SelectGlobalPoint(lPoint);
    }
}



void CPEditorPanel::OnMyButtonClicked(wxCommandEvent &e)
{
    DEBUG_DEBUG("on my button");
}

void CPEditorPanel::panoramaChanged(PT::Panorama &pano)
{
    // Its the allways working method
    // update Tabs
    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrTabs = m_leftTabs->GetPageCount();
    // update tab buttons
    DEBUG_TRACE("panoramChanged() images:" << nrImages
                << " tabs:" << nrTabs);
    m_leftTabs->DeleteAllPages();
    m_rightTabs->DeleteAllPages();
//    if (nrTabs < nrImages) { // insecure, better rebuild all
        for (unsigned int img = 0/*nrTabs*/; img <nrImages; ++img) {
            DEBUG_DEBUG("adding tab " << img);
            // ugly.. but needed since we have to add something
            // to wxNotebook to get the TabBar...
            // to beku: this are dummy child windows that will never
            // be displayed.  the just exist to fool wxNotebook, so
            // that it will add another tab to its tabbar.
            //
            // wxTabCtrl is windows/mac only, so we cant use that.
            wxWindow * t1= new wxWindow(m_leftTabs,-1,wxPoint(0,0),wxSize(0,0));
            wxWindow * t2= new wxWindow(m_rightTabs,-1,wxPoint(0,0),wxSize(0,0));
            if (!m_leftTabs->AddPage(t1, wxString::Format("%d",img))) {
                DEBUG_FATAL("could not add dummy window to left notebook");
            }
            if (!m_rightTabs->AddPage(t2, wxString::Format("%d",img))){
                DEBUG_FATAL("could not add dummy window to right notebook");
            }
        }
/*    } else if (nrTabs > nrImages) {
        for (unsigned int img = nrImages; img > nrTabs; img--) {
            m_leftTabs->DeletePage(img);
            m_rightTabs->DeletePage(img);
        }
    }*/
    // update the display
    UpdateDisplay();
}

void CPEditorPanel::ImagesAdded(PT::Panorama &pano, int added)
{
    // This function is for adding only
    // update Tabs
    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrTabs = m_leftTabs->GetPageCount();
    // ugly.. but needed since we have to add something
    // to wxNotebook to get the TabBar...
    wxWindow* t1= new wxWindow(m_leftTabs,-1,wxPoint(0,0),wxSize(0,0));
    wxWindow* t2= new wxWindow(m_rightTabs,-1,wxPoint(0,0),wxSize(0,0));
    // update tab buttons
    DEBUG_TRACE("panoramaImagedAdded() images:" << nrImages
                << " tabs:" << nrTabs);
    for (unsigned int img = nrTabs; img < nrImages; ++img) {
        DEBUG_DEBUG("adding tab " << img);
        if (!m_leftTabs->AddPage(t1, wxString::Format("%d",img + 1))) {
            DEBUG_FATAL("could not add dummy window to left notebook");
        }
        if (!m_rightTabs->AddPage(t2, wxString::Format("%d",img + 1))){
            DEBUG_FATAL("could not add dummy window to right notebook");
        }
    }
/*    if ( t1->GetSelection() == -1 ) { // FIXME How to set CPE the first time - is never selected
        DEBUG_INFO(__FUNCTION__ << " setImage " << wxString::Format("%d",t1->GetSelection()) << "/" )
        wxImage * ptr = ImageCache::getInstance().getImage(
          pano.getImage(0).getFilename());
        m_leftImg->setImage(ptr);
        m_rightImg->setImage(ptr);
        setLeftImage(0);
        setRightImage(0);
    }*/
    // update the display
    UpdateDisplay();
}

void CPEditorPanel::ImagesRemoved(PT::Panorama &pano, int removed[512])
{
    // update Tabs
    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrTabs = m_leftTabs->GetPageCount();
    // update tab buttons
    DEBUG_TRACE(__FUNCTION__ << " images:" << nrImages
                << " tabs:" << nrTabs << " = " << removed[0]);
    // remove erased tabs  in CPEditorPanel
    if ( pano.getNrOfImages() <= 0 ) {
       m_leftTabs->DeleteAllPages();
       m_rightTabs->DeleteAllPages();
    } else {
      for ( int i = 1 ; i <= removed[0] ; i++ ) {
        DEBUG_INFO(__FUNCTION__ << " removeTab " << wxString::Format("%d",removed[i]) << "/" << wxString::Format("%d",i) )
        m_leftTabs->DeletePage(removed[i]);
        m_rightTabs->DeletePage(removed[i]);
      }
      // renumbering the tabs in CPEditorPanel
      for ( int i = 1 ; i <= (int)pano.getNrOfImages() ; i++ ) {
        m_leftTabs->SetPageText( i, wxString::Format("%d",i + 1) );
        m_rightTabs->SetPageText( i, wxString::Format("%d",i + 1) );
      }
    }
    // update the display
    UpdateDisplay();
}


void CPEditorPanel::UpdateDisplay()
{
    int fI = m_leftTabs->GetSelection();
    int sI = m_rightTabs->GetSelection();
    if (fI < 0) {
        return;
    }
    leftImage = (unsigned int) fI;
    rightImage = (unsigned int) sI;
    // update control points
    const PT::CPVector & controlPoints = pano->getCtrlPoints();
    currentPoints.clear();
    mirroredPoints.clear();
    std::vector<wxPoint> left;
    std::vector<wxPoint> right;

    // create a list of all control points
    unsigned int i = 0;
    for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        PT::ControlPoint point = *it;
        if ((point.image1Nr == leftImage) && (point.image2Nr == rightImage)){
            left.push_back(wxPoint( (int) point.x1, (int) point.y1));
            right.push_back(wxPoint( (int) point.x2, (int) point.y2));
            currentPoints.push_back(make_pair(it - controlPoints.begin(), *it));
            i++;
        } else if ((point.image2Nr == leftImage) && (point.image1Nr == rightImage)){
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
}

void CPEditorPanel::OnLeftImgChange(wxNotebookEvent & e)
{
    DEBUG_TRACE("OnLeftImgChange() to " << e.GetSelection());
    if (e.GetSelection() >= 0) {
        leftImage = (unsigned int) e.GetSelection();
        setLeftImage(leftImage);
        wxImage * ptr = ImageCache::getInstance().getImage(
            pano->getImage(leftImage).getFilename());
        m_leftImg->setImage(*ptr);
    }
}

void CPEditorPanel::OnRightImgChange(wxNotebookEvent & e)
{
    DEBUG_TRACE("OnRightImgChange() to " << e.GetSelection());
    if (e.GetSelection() >= 0) {
        rightImage = (unsigned int) e.GetSelection();

        setRightImage(rightImage);
        wxImage *ptr = ImageCache::getInstance().getImage(
            pano->getImage(rightImage).getFilename());
        m_rightImg->setImage(*ptr);
    }
}
