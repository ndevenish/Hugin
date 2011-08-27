// -*- c-basic-offset: 4 -*-

/** @file CPListFrame.cpp
 *
 *  @brief implementation of CPListFrame Class
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
#include "panoinc.h"

#include <algorithm>
#include <utility>
#include <functional>

#include "base_wx/wxPlatform.h"
#include "hugin/CPListFrame.h"
#include "hugin/MainFrame.h"
#include "hugin/CommandHistory.h"
#include "hugin/huginApp.h"

using namespace PT;
using namespace std;
using namespace hugin_utils;

class DelKeyHandler: public wxEvtHandler
{
public:
    DelKeyHandler(CPListFrame & list)
        : m_list(list)
        {
        }

    void OnKey(wxKeyEvent & e)
        {
            switch(e.m_keyCode)
            {
                case WXK_DELETE:
                    m_list.DeleteSelected();
                    break;
                case 1: //Ctrl+A
                    m_list.SelectAll();
                    break;
                default:
                    e.Skip();
            }
        }

private:
    CPListFrame & m_list;

    DECLARE_EVENT_TABLE()

};

BEGIN_EVENT_TABLE(DelKeyHandler, wxEvtHandler)
    EVT_CHAR(DelKeyHandler::OnKey)
END_EVENT_TABLE()

static Panorama * g_pano;

#if wxCHECK_VERSION(2,9,2)
#define COMPARETYPEITEM wxIntPtr
#define COMPARETYPESORTDATA wxIntPtr
#else
#if wxCHECK_VERSION(2,9,0)
#define COMPARETYPEITEM long
#define COMPARETYPESORTDATA wxIntPtr
#else
#define COMPARETYPEITEM long
#define COMPARETYPESORTDATA long
#endif
#endif

static int wxCALLBACK compareError(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    const ControlPoint &p1 = g_pano->getCtrlPoint(item1);
    const ControlPoint &p2 = g_pano->getCtrlPoint(item2);
    if (p1.error < p2.error)
        return -1;
    else if (p1.error > p2.error)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareErrorGreater(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    const ControlPoint &p1 = g_pano->getCtrlPoint(item1);
    const ControlPoint &p2 = g_pano->getCtrlPoint(item2);
    if (p1.error > p2.error)
        return -1;
    else if (p1.error < p2.error)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareCPNr(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    if (item1 < item2)
        return -1;
    else if (item1 > item2)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareCPNrGreater(COMPARETYPEITEM p1, COMPARETYPEITEM p2, COMPARETYPESORTDATA sortData)
{
    if (p1 > p2)
        return -1;
    else if (p1 < p2)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareImg1Nr(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    const ControlPoint &p1 = g_pano->getCtrlPoint(item1);
    const ControlPoint &p2 = g_pano->getCtrlPoint(item2);
    if (p1.image1Nr < p2.image1Nr)
        return -1;
    else if (p1.image1Nr > p2.image1Nr)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareImg1NrGreater(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    const ControlPoint &p1 = g_pano->getCtrlPoint(item1);
    const ControlPoint &p2 = g_pano->getCtrlPoint(item2);
    if (p1.image1Nr > p2.image1Nr)
        return -1;
    else if (p1.image1Nr < p2.image1Nr)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareImg2Nr(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    const ControlPoint &p1 = g_pano->getCtrlPoint(item1);
    const ControlPoint &p2 = g_pano->getCtrlPoint(item2);
    if (p1.image2Nr < p2.image2Nr)
        return -1;
    else if (p1.image2Nr > p2.image2Nr)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareImg2NrGreater(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    const ControlPoint &p1 = g_pano->getCtrlPoint(item1);
    const ControlPoint &p2 = g_pano->getCtrlPoint(item2);
    if (p1.image2Nr > p2.image2Nr)
        return -1;
    else if (p1.image2Nr < p2.image2Nr)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareMode(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    const ControlPoint &p1 = g_pano->getCtrlPoint(item1);
    const ControlPoint &p2 = g_pano->getCtrlPoint(item2);
    if (p1.mode < p2.mode)
        return -1;
    else if (p1.mode > p2.mode)
        return 1;
    else
        return 0;
}

static int wxCALLBACK compareModeGreater(COMPARETYPEITEM item1, COMPARETYPEITEM item2, COMPARETYPESORTDATA sortData)
{
    const ControlPoint &p1 = g_pano->getCtrlPoint(item1);
    const ControlPoint &p2 = g_pano->getCtrlPoint(item2);
    if (p1.mode > p2.mode)
        return -1;
    else if (p1.mode < p2.mode)
        return 1;
    else
        return 0;
}

#if 0
// sort helper function
struct compareError
{
    bool operator()(const pair<int, ControlPoint> &p1, const pair<int, ControlPoint> &p2)
        { return p1.second.error < p2.second.error; }
};

// sort helper function
struct compareErrorGreater
{
    bool operator()(const pair<int, ControlPoint> &p1, const pair<int, ControlPoint> &p2)
        { return p1.second.error > p2.second.error; }
};

struct compareImg1Nr
{
    bool operator()(const pair<int, ControlPoint> &p1, const pair<int, ControlPoint> &p2)
        { return p1.second.image1Nr < p2.second.image1Nr; }
};

struct compareImg1NrGreater
{
    bool operator()(const pair<int, ControlPoint> &p1, const pair<int, ControlPoint> &p2)
        { return p1.second.image1Nr > p2.second.image1Nr; }
};

struct compareImg2Nr
{
    bool operator()(const pair<int, ControlPoint> &p1, const pair<int, ControlPoint> &p2)
        { return p1.second.image2Nr < p2.second.image2Nr; }
};

struct compareImg2NrGreater
{
    bool operator()(const pair<int, ControlPoint> &p1, const pair<int, ControlPoint> &p2)
        { return p1.second.image2Nr > p2.second.image2Nr; }
};

struct compareMode
{
    bool operator()(const pair<int, ControlPoint> &p1, const pair<int, ControlPoint> &p2)
        { return p1.second.mode < p2.second.mode; }
};

struct compareModeGreater
{
    bool operator()(const pair<int, ControlPoint> &p1, const pair<int, ControlPoint> &p2)
        { return p1.second.mode > p2.second.mode; }
};

#endif

BEGIN_EVENT_TABLE(CPListFrame, wxFrame)
    EVT_CLOSE(CPListFrame::OnClose)
    EVT_LIST_ITEM_SELECTED(XRCID("cp_list_frame_list"), CPListFrame::OnCPListSelectionChanged)
    EVT_LIST_ITEM_DESELECTED(XRCID("cp_list_frame_list"), CPListFrame::OnCPListSelectionChanged)
    EVT_LIST_COL_CLICK(XRCID("cp_list_frame_list"), CPListFrame::OnCPListHeaderClick)
    EVT_LIST_COL_END_DRAG(XRCID("cp_list_frame_list"), CPListFrame::OnColumnWidthChange)
    EVT_BUTTON(XRCID("cp_list_delete"), CPListFrame::OnDeleteButton)
    EVT_BUTTON(XRCID("cp_list_select"), CPListFrame::OnSelectButton)
    EVT_BUTTON(XRCID("cp_list_finetune"), CPListFrame::OnFineTuneButton)
//    EVT_CHECKBOX(XRCID("cp_list_multiselect"), CPListFrame::OnMuliSelectCheck)
END_EVENT_TABLE()


CPListFrame::CPListFrame(MainFrame * parent, Panorama & pano)
    : m_mainFrame(parent), m_pano(pano),m_verbose(false),
      m_sortCol(0), m_sortAscend(true), m_freeze(false)
{
    DEBUG_TRACE("");
    bool ok = wxXmlResource::Get()->LoadFrame(this, parent, wxT("cp_list_frame"));
    DEBUG_ASSERT(ok);
    m_list = XRCCTRL(*this, "cp_list_frame_list", wxListCtrl);
    DEBUG_ASSERT(m_list);

    wxConfigBase * config = wxConfigBase::Get();
    m_verbose = (config->Read(wxT("/CPListFrame/verbose"),0l) != 0);

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows, copy color from a child widget
    this->SetBackgroundColour(XRCCTRL(*this, "cp_list_select", wxButton)->GetBackgroundColour());
#endif
#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    if (m_verbose) {
        // setup list display
        m_list->InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25);
        m_list->InsertColumn( 1, _("Left Img."), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 2, _("x"), wxLIST_FORMAT_RIGHT, 40);
        m_list->InsertColumn( 3, _("y"), wxLIST_FORMAT_RIGHT, 40);
        m_list->InsertColumn( 4, _("Right Img."), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 5, _("x"), wxLIST_FORMAT_RIGHT, 40);
        m_list->InsertColumn( 6, _("y"), wxLIST_FORMAT_RIGHT, 40);
        m_list->InsertColumn( 7, _("Alignment"), wxLIST_FORMAT_LEFT, 80);
        m_list->InsertColumn( 8, _("Distance"), wxLIST_FORMAT_RIGHT, 80);
    } else {
        m_list->InsertColumn( 0, _("G CP#"), wxLIST_FORMAT_RIGHT, 25);
        m_list->InsertColumn( 1, _("Left Img."), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 2, _("Right Img."), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 3, _("P CP#"), wxLIST_FORMAT_RIGHT, 25);
        m_list->InsertColumn( 4, _("Alignment"), wxLIST_FORMAT_LEFT, 80);
        m_list->InsertColumn( 5, _("Distance"), wxLIST_FORMAT_RIGHT, 80);
    }

    //get saved width
    for ( int j=0; j < m_list->GetColumnCount() ; j++ )
    {
        // -1 is auto
        int width = wxConfigBase::Get()->Read(wxString::Format( wxT("/CPListFrame/ColumnWidth%d"), j ), -1);
        if(width != -1)
            m_list->SetColumnWidth(j, width);
    }

    //set minumum size
    SetSizeHints(200, 300);
    
    //size
    RestoreFramePosition(this, wxT("CPListFrame"));
    
    m_list->PushEventHandler(new DelKeyHandler(*this));

    m_list->Show();
    // observe the panorama
    m_pano.addObserver(this);

    g_pano = & m_pano;

    // force a update of the control points
    UIntSet dummy;
    panoramaImagesChanged(m_pano, dummy);
    /*
    if (config->Read(wxT("/CPListFrame/isShown"), 0l) != 0) {
        Show();
        Raise();
    }
    */

    DEBUG_TRACE("ctor end");
}

CPListFrame::~CPListFrame()
{
    DEBUG_TRACE("dtor");
    // delete our event handler
    m_list->PopEventHandler(true);

    StoreFramePosition(this, wxT("CPListFrame"));

    wxConfigBase * config = wxConfigBase::Get();

    if ( (!this->IsIconized()) && (!this->IsMaximized()) && this->IsShown()) {
        DEBUG_DEBUG("IsShown()");
        config->Write(wxT("/CPListFrame/isShown"), 1l);
    } else {
        DEBUG_DEBUG(" not shown ");
        config->Write(wxT("/CPListFrame/isShown"), 0l);
    }

    config->Flush();
    m_pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}

void CPListFrame::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr)
{
    DEBUG_TRACE("");
    const CPVector & cpv = pano.getCtrlPoints();
    unsigned int nrCP = cpv.size();
    unsigned int nrItems = m_list->GetItemCount();
    DEBUG_TRACE("nr CP:" << nrCP << " nr listentries:" << nrItems);

    m_list->Freeze();
//    m_list->Hide();
    // remove items for nonexisting CP's
    for (int i=nrItems-1; i>=(int)nrCP; i--)
    {
        m_list->DeleteItem(i);
    }

    // update existing items
    if ( nrCP >= nrItems ) {
        for (int i=0; i < (int) nrCP; i++) {
            if (i >= (int) nrItems) {
                // create item
                m_list->InsertItem(i, wxString::Format(wxT("%d"),i));
            }
        }
    }
    // update list.
    updateList();
    /*
    // force a nice size
    int nrCol = m_verbose ? 9 : 5;
    for (int col=0; col < nrCol ; col++) {
        m_list->SetColumnWidth(col,wxLIST_AUTOSIZE);

        //saved column width
        int width = wxConfigBase::Get()->Read(wxString::Format( wxT("/CPListFrame/ColumnWidth%d"), col ), -1);
        if(width != -1)
            m_list->SetColumnWidth(col, width);
    }
    */
    m_list->Thaw();
}

void CPListFrame::SetCPItem(int i, const ControlPoint & p)
{
    DEBUG_ASSERT(i < m_list->GetItemCount());
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
        break;
    default:
        mode = wxString::Format(_("Line %d"), p.mode);
        break;
    }

    // Map the Global CP number to a local "Pair" CP number
    string pairId = makePairId(p.image1Nr,p.image2Nr);
    if (m_localIds.count(pairId)) {
        m_localIds[pairId]++;
    } else {
        m_localIds[pairId] = 0;
    }
    
    int localId = m_localIds[pairId];
    DEBUG_INFO ("Global ID: " << i << " -> Local ID: " << localId);

    if (m_verbose) {
        m_list->SetItem(i,0,wxString::Format(wxT("%d"),i));
        m_list->SetItem(i,1,wxString::Format(wxT("%d"),p.image1Nr));
        m_list->SetItem(i,2,wxString::Format(wxT("%.1f"),p.x1));
        m_list->SetItem(i,3,wxString::Format(wxT("%.1f"),p.y1));
        m_list->SetItem(i,4,wxString::Format(wxT("%d"),p.image2Nr));
        m_list->SetItem(i,5,wxString::Format(wxT("%.1f"),p.x2));
        m_list->SetItem(i,6,wxString::Format(wxT("%.1f"),p.y2));
        m_list->SetItem(i,7,mode);
        m_list->SetItem(i,8,wxString::Format(wxT("%.2f"),p.error));
    } else {
        m_list->SetItem(i,0,wxString::Format(wxT("%d"),i));
        m_list->SetItem(i,1,wxString::Format(wxT("%d"),p.image1Nr));
        m_list->SetItem(i,2,wxString::Format(wxT("%d"),p.image2Nr));
        m_list->SetItem(i,3,wxString::Format(wxT("%d"),localId));
        m_list->SetItem(i,4,mode);
        m_list->SetItem(i,5,wxString::Format(wxT("%.2f"),p.error));
    }
}


void CPListFrame::updateList()
{
    const CPVector & cps = m_pano.getCtrlPoints();

    int sortCol = m_sortCol;
    bool sortAscend = m_sortAscend;

    // sort by number, else the updating will shuffle the selected points..
    // it seems that the list is traversed in order it is visible on screen

    m_sortCol = 0;
    m_sortAscend = true;

    // Rebuild the global->local CP map on each update as CPs might have been
    // removed.
    m_localIds.clear();

    SortList();

    int nrCP = cps.size();
    for (int i=0; i < (int) nrCP; i++) {
        SetCPItem(i,cps[i]);
        m_list->SetItemData(i, i);
        m_list->SetItemState(i, 0, wxLIST_STATE_SELECTED);
    }

#if 0
    // we need to save the point numbers...
    vector<pair<int,ControlPoint> > cpv(nrCP);
    for (int i=0; i < nrCP; ++i) {
        cpv[i] = make_pair(i,cps[i]);
    }
#endif

    m_sortCol = sortCol;
    m_sortAscend = sortAscend;
    SortList();
}

void CPListFrame::SortList()
{
    // nothing to sort
    if (m_list->GetItemCount() == 0) return;

    int colNumber = 0;
    int colLeftImg = 1;
    int colRightImg = 4;
    int colMode = 7;
    int colError = 8;

    int colPairCP = 3;

    if (!m_verbose) {
        colRightImg = 2;
        colMode = 4;
        colError = 5;
    }

    DEBUG_TRACE("sorting column " << m_sortCol);
    if (m_sortCol == colNumber) {
        if (m_sortAscend) {
            m_list->SortItems(&compareCPNr, 0);
//            sort(cpv.begin(),cpv.end(), compareImg1Nr());
        } else {
            m_list->SortItems(&compareCPNrGreater, 0);
//            sort(cpv.begin(),cpv.end(), compareImg1NrGreater());
        }
    } else if (m_sortCol == colLeftImg) {
        if (m_sortAscend) {
            m_list->SortItems(&compareImg1Nr, 0);
//            sort(cpv.begin(),cpv.end(), compareImg1Nr());
        } else {
            m_list->SortItems(&compareImg1NrGreater, 0);
//            sort(cpv.begin(),cpv.end(), compareImg1NrGreater());
        }
    } else if (m_sortCol == colRightImg) {
        if (m_sortAscend) {
            m_list->SortItems(&compareImg2Nr, 0);
//            sort(cpv.begin(),cpv.end(), compareImg2Nr());
        } else {
            m_list->SortItems(&compareImg2NrGreater, 0);
//            sort(cpv.begin(),cpv.end(), compareImg2NrGreater());
        }
    } else if (m_sortCol == colMode) {
        if (m_sortAscend) {
            m_list->SortItems(&compareMode, 0);
//            sort(cpv.begin(),cpv.end(), compareMode());
        } else {
            m_list->SortItems(&compareModeGreater, 0);
//            sort(cpv.begin(),cpv.end(), compareModeGreater());
        }
    } else if (m_sortCol == colError) {
        if (m_sortAscend) {
            m_list->SortItems(&compareError, 0);
//            sort(cpv.begin(),cpv.end(), compareError());
        } else {
            m_list->SortItems(&compareErrorGreater, 0);
//            sort(cpv.begin(),cpv.end(), compareErrorGreater());
        }
    } else if (m_sortCol == colPairCP) {
        DEBUG_DEBUG("Skip sort of pair control points");
    } else {
        DEBUG_ERROR("Unknown sorting column: " << m_sortCol);
    }
}

void CPListFrame::OnCPListHeaderClick(wxListEvent & e)
{
    // do the sorting here
    // wxListCtrl has a horrible interface
    int newCol = e.GetColumn();
    if (m_sortCol == newCol) {
        m_sortAscend = ! m_sortAscend;
    } else {
        m_sortCol = newCol;
        m_sortAscend = true;
    }
    SortList();
}

void CPListFrame::OnClose(wxCloseEvent& event)
{
    DEBUG_DEBUG("OnClose");
    m_mainFrame->OnCPListFrameClosed();
    DEBUG_DEBUG("closing");
    Destroy();
}

void CPListFrame::OnDeleteButton(wxCommandEvent & e)
{
    DeleteSelected();
}

#if 0
void CPListFrame::OnSelectOutliers
{
    // calculate the mean error between all image pairs.

}
#endif

void CPListFrame::OnSelectButton(wxCommandEvent & e)
{
    // calculate the mean error and the standard deviation

    m_freeze = true;
    const CPVector & cps = m_pano.getCtrlPoints();

    double mean_error = 0;
    double squared_error = 0;
    double max_error = 0;
    CPVector::const_iterator it;
    for (it = cps.begin() ; it != cps.end(); it++) {
        mean_error += (*it).error;
        squared_error += (*it).error * (*it).error;
        if ((*it).error > max_error) {
            max_error = (*it).error;
        }
    }
    mean_error = mean_error / cps.size();
    double std_dev = sqrt(squared_error/cps.size());

    // select points whos distance is greater than the mean
    // hmm, maybe some theory would be nice.. this is just a
    // guess.
    double threshold = mean_error + std_dev;
    wxString t;
    do
    {
        t=wxGetTextFromUser(_("Enter minimum control point error.\nAll points with a higher error will be selected"), _("Select Control Points"),
                                 doubleTowxString(threshold,2));

        if (t == wxEmptyString) {
            // do not select anything
            return;
        }
    } while (!str2double(t,threshold));

    bool invert = threshold < 0;
    if (invert) {
        threshold = -threshold;
    }

    m_list->Freeze();
    int sortCol = m_sortCol;
    bool sortAscend = m_sortAscend;

    long row = -1;
    for(;;) {
        row = m_list->GetNextItem(row,
                                   wxLIST_NEXT_ALL);
        if (row < 0) {
            break;
        }
        unsigned int cpNr = (unsigned int) m_list->GetItemData(row);
        if (    ((cps[cpNr].error > threshold ) && (!invert)) 
             || ((cps[cpNr].error < threshold ) && (invert))  )
        {
            // select control point
            DEBUG_DEBUG("selecting row: " << row << " cpNr: " << cpNr);

            m_list->SetItemState(row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        } else {
            m_list->SetItemState(row, 0, wxLIST_STATE_SELECTED);
        }
    } while (row != -1);

    m_list->Thaw();

    m_freeze = false;
}

void CPListFrame::DeleteSelected()
{
    DEBUG_DEBUG("Delete pressed");
    // find selected point
    // no selected item.
    int nSelected = m_list->GetSelectedItemCount();
    DEBUG_DEBUG(nSelected << " point selected, deleting them");
    if (nSelected == 0) {
        wxBell();
        return;
    }

    UIntSet selected;
    long item = -1;
    for(;;) {
        item = m_list->GetNextItem(item,
                                   wxLIST_NEXT_ALL,
                                   wxLIST_STATE_SELECTED);
        // deselect item
        m_list->SetItemState(item, 0, wxLIST_STATE_SELECTED);
        if (item < 0) {
            break;
        }
        DEBUG_DEBUG("scheduling point " << item << " for deletion");
        selected.insert((unsigned int) (m_list->GetItemData(item)));
    }
    DEBUG_DEBUG("about to delete " << selected.size() << " points");
    GlobalCmdHist::getInstance().addCommand(
        new PT::RemoveCtrlPointsCmd(m_pano,selected)
        );

    item = m_list->GetNextItem(item,
                               wxLIST_NEXT_ALL);
    if (item >=0) {
        int cp = m_list->GetItemData(item);
        m_mainFrame->ShowCtrlPoint((unsigned int) cp);
    }
}

void CPListFrame::SelectAll()
{
    unsigned int nrItems = m_list->GetItemCount();
    for (unsigned int i=0; i < nrItems ; i++)
    {
        m_list->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
};

void CPListFrame::OnFineTuneButton(wxCommandEvent & e)
{
    DEBUG_WARN("Not yet implemented");
}


void CPListFrame::OnColumnWidthChange( wxListEvent & e )
{
    int colNum = e.GetColumn();
    wxConfigBase::Get()->Write( wxString::Format(wxT("/CPListFrame/ColumnWidth%d"),colNum), m_list->GetColumnWidth(colNum) );
}


void CPListFrame::OnCPListSelectionChanged(wxListEvent & e)
{
    DEBUG_TRACE(e.GetIndex());
    int itemsSelected = m_list->GetSelectedItemCount();
    DEBUG_DEBUG("selected control points: " << itemsSelected);
    if (1 == itemsSelected) {
        int cp = m_list->GetItemData(e.GetIndex());
        m_mainFrame->ShowCtrlPoint((unsigned int) cp);
    } else if (0 == itemsSelected) {
        DEBUG_DEBUG("nothing to do");
    } else if (itemsSelected > 1) {
        DEBUG_DEBUG("Multiselection nothing to do");
    }

}


std::string CPListFrame::makePairId(unsigned int id1, unsigned int id2) 
{
    // Control points from same image pair, regardless of which is left or right
    // are counted the same so return the identical hash id.
    std::ostringstream oss;

    if (id1 < id2) {
        oss << id1 << "_" << id2;
    } else if (id2 < id1)  {
        oss << id2 << "_" << id1;
    } else {
        // Control points are from same image.
        oss << id1;
    }
    return oss.str();
}
