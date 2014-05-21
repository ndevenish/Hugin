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
#include "algorithms/basic/CalculateCPStatistics.h"

using namespace PT;
using namespace std;
using namespace hugin_utils;

BEGIN_EVENT_TABLE(CPListCtrl, wxListCtrl)
    EVT_CHAR(CPListCtrl::OnChar)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, CPListCtrl::OnCPListSelectionChanged)
    EVT_LIST_ITEM_DESELECTED(wxID_ANY, CPListCtrl::OnCPListSelectionChanged)
    EVT_LIST_COL_CLICK(wxID_ANY, CPListCtrl::OnCPListHeaderClick)
    EVT_LIST_COL_END_DRAG(wxID_ANY, CPListCtrl::OnColumnWidthChange)
END_EVENT_TABLE()

std::string makePairId(unsigned int id1, unsigned int id2)
{
    // Control points from same image pair, regardless of which is left or right
    // are counted the same so return the identical hash id.
    std::ostringstream oss;

    if (id1 < id2) {
        oss << id1 << "_" << id2;
    }
    else if (id2 < id1)  {
        oss << id2 << "_" << id1;
    }
    else {
        // Control points are from same image.
        oss << id1;
    }
    return oss.str();
}

CPListCtrl::CPListCtrl() : m_pano(NULL)
{
    m_sortCol = 0;
    m_sortAscend = true;
};

CPListCtrl::~CPListCtrl()
{
    if (m_pano)
    {
        m_pano->removeObserver(this);
    };
};

bool CPListCtrl::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, long style, const wxValidator& validator, const wxString& name)
{
    if (!wxListCtrl::Create(parent, id, pos, size, style))
    {
        return false;
    };
    InsertColumn(0, _("G CP#"), wxLIST_FORMAT_RIGHT, 25);
    InsertColumn(1, _("Left Img."), wxLIST_FORMAT_RIGHT, 65);
    InsertColumn(2, _("Right Img."), wxLIST_FORMAT_RIGHT, 65);
    InsertColumn(3, _("P CP#"), wxLIST_FORMAT_RIGHT, 25);
    InsertColumn(4, _("Alignment"), wxLIST_FORMAT_LEFT, 80);
    InsertColumn(5, _("Distance"), wxLIST_FORMAT_RIGHT, 80);

    //get saved width
    for (int j = 0; j < GetColumnCount(); j++)
    {
        // -1 is auto
        int width = wxConfigBase::Get()->Read(wxString::Format(wxT("/CPListFrame/ColumnWidth%d"), j), -1);
        if (width != -1)
        {
            SetColumnWidth(j, width);
        };
    };
#if wxCHECK_VERSION(3,0,0)
    EnableAlternateRowColours(true);
#endif
    return true;
};

void CPListCtrl::Init(Panorama* pano)
{
    m_pano = pano;
    m_pano->addObserver(this);
    panoramaChanged(*pano);
};

wxString CPListCtrl::OnGetItemText(long item, long column) const
{
    if (item > m_internalCPList.size())
    {
        return wxEmptyString;
    };
    const ControlPoint& cp = m_pano->getCtrlPoint(m_internalCPList[item].globalIndex);
    switch (column)
    {
        case 0:
            return wxString::Format(wxT("%d"), m_internalCPList[item].globalIndex);
            break;
        case 1:
            return wxString::Format(wxT("%d"), cp.image1Nr);
            break;
        case 2:
            return wxString::Format(wxT("%d"), cp.image2Nr);
            break;
        case 3:
            return wxString::Format(wxT("%d"), m_internalCPList[item].localNumber);
            break;
        case 4:
            switch (cp.mode)
            {
                case ControlPoint::X_Y:
                    return wxString(_("normal"));
                    break;
                case ControlPoint::X:
                    return wxString(_("vert. Line"));
                    break;
                case ControlPoint::Y:
                    return wxString(_("horiz. Line"));
                    break;
                default:
                    return wxString::Format(_("Line %d"), cp.mode);
                    break;
            };
            break;
        case 5:
            return wxString::Format(wxT("%.2f"), cp.error);
            break;
        default:
            return wxEmptyString;
    };
    return wxEmptyString;
};

void CPListCtrl::panoramaChanged(PT::Panorama &pano)
{
    UpdateInternalCPList();
    SetItemCount(m_pano->getNrOfCtrlPoints());
    Refresh();
};

void CPListCtrl::UpdateInternalCPList()
{
    const CPVector& cps = m_pano->getCtrlPoints();
    // Rebuild the global->local CP map on each update as CPs might have been
    // removed.
    m_localIds.clear();

    if (m_internalCPList.size() != cps.size())
    {
        m_internalCPList.resize(cps.size());
    };
    for (size_t i = 0; i < cps.size(); i++)
    {
        m_internalCPList[i].globalIndex = i;
        const ControlPoint& cp = cps[i];
        string pairId = makePairId(cp.image1Nr, cp.image2Nr);
        std::map<std::string, int>::iterator it = m_localIds.find(pairId);
        if (it != m_localIds.end())
        {
            ++(it->second);
        }
        else
        {
            m_localIds[pairId] = 0;
        }
        m_internalCPList[i].localNumber=m_localIds[pairId];
    };
    SortInternalList(true);
};

// sort helper function
#define CompareStruct(VAR) \
struct Compare##VAR\
{\
    bool operator()(const CPListItem& item1, const CPListItem& item2)\
    {\
        return item1.VAR < item2.VAR;\
    };\
};
CompareStruct(globalIndex)
CompareStruct(localNumber)
#undef CompareStruct

#define CompareStruct(VAR) \
struct Compare##VAR##Greater\
{\
    bool operator()(const CPListItem& item1, const CPListItem& item2)\
    {\
        return item1.VAR > item2.VAR;\
    };\
};
CompareStruct(globalIndex)
CompareStruct(localNumber)
#undef CompareStruct

#define CompareStruct(VAR)\
struct Compare##VAR\
{\
    Compare##VAR(const HuginBase::CPVector& cps) : m_cps(cps) {};\
    bool operator()(const CPListItem& item1, const CPListItem& item2)\
    {\
         return m_cps[item1.globalIndex].VAR < m_cps[item2.globalIndex].VAR; \
    }\
private:\
    const HuginBase::CPVector& m_cps;\
};
CompareStruct(image1Nr)
CompareStruct(image2Nr)
CompareStruct(mode)
CompareStruct(error)
#undef CompareStruct

#define CompareStruct(VAR)\
struct Compare##VAR##Greater\
{\
    Compare##VAR##Greater(const HuginBase::CPVector& cps) : m_cps(cps) {};\
    bool operator()(const CPListItem& item1, const CPListItem& item2)\
    {\
         return m_cps[item1.globalIndex].VAR > m_cps[item2.globalIndex].VAR; \
    }\
private:\
    const HuginBase::CPVector& m_cps;\
};
CompareStruct(image1Nr)
CompareStruct(image2Nr)
CompareStruct(mode)
CompareStruct(error)
#undef CompareStruct

void CPListCtrl::SortInternalList(bool isAscending)
{
    // nothing to sort
    if (m_internalCPList.empty())
    {
        return;
    };

    switch (m_sortCol)
    {
        case 0:
            if (m_sortAscend)
            {
                if (!isAscending)
                {
                    std::sort(m_internalCPList.begin(), m_internalCPList.end(), CompareglobalIndex());
                };
            }
            else
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), CompareglobalIndexGreater());
            };
            break;
        case 1:
            if (m_sortAscend)
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), Compareimage1Nr(m_pano->getCtrlPoints()));
            }
            else
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), Compareimage1NrGreater(m_pano->getCtrlPoints()));
            };
            break;
        case 2:
            if (m_sortAscend)
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), Compareimage2Nr(m_pano->getCtrlPoints()));
            }
            else
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), Compareimage2NrGreater(m_pano->getCtrlPoints()));
            };
            break;
        case 3:
            if (m_sortAscend)
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), ComparelocalNumber());
            }
            else
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), ComparelocalNumberGreater());
            };
            break;
        case 4:
            if (m_sortAscend)
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), Comparemode(m_pano->getCtrlPoints()));
            }
            else
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), ComparemodeGreater(m_pano->getCtrlPoints()));
            };
            break;
        case 5:
            if (m_sortAscend)
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), Compareerror(m_pano->getCtrlPoints()));
            }
            else
            {
                std::sort(m_internalCPList.begin(), m_internalCPList.end(), CompareerrorGreater(m_pano->getCtrlPoints()));
            };
            break;
    };
};

void CPListCtrl::OnCPListSelectionChanged(wxListEvent & e)
{
    if (GetSelectedItemCount() == 1)
    {
        if (e.GetIndex() < m_internalCPList.size())
        {
            MainFrame::Get()->ShowCtrlPoint(m_internalCPList[e.GetIndex()].globalIndex);
        };
    };
};

void CPListCtrl::OnCPListHeaderClick(wxListEvent& e)
{
    const int newCol = e.GetColumn();
    if (m_sortCol == newCol)
    {
        m_sortAscend = !m_sortAscend;
    }
    else
    {
        m_sortCol = newCol;
        m_sortAscend = true;
    }
    SortInternalList(false);
    Refresh();
};

void CPListCtrl::OnColumnWidthChange(wxListEvent& e)
{
    const int colNum = e.GetColumn();
    wxConfigBase::Get()->Write(wxString::Format(wxT("/CPListFrame/ColumnWidth%d"), colNum), GetColumnWidth(colNum));
};

void CPListCtrl::DeleteSelected()
{
    // no selected item.
    const int nSelected = GetSelectedItemCount();
    if (nSelected == 0)
    {
        wxBell();
        return;
    };

    UIntSet selected;
    long item = GetFirstSelected();
    long newSelection = -1;
    long newSelectionCPIndex = -1;
    if (m_internalCPList.size() - nSelected > 0)
    {
        newSelection = item;
        if (item >= m_internalCPList.size() - nSelected)
        {
            newSelection = m_internalCPList.size() - nSelected - 1;
        };
        if (newSelection >= 0)
        {
            newSelectionCPIndex = m_internalCPList[newSelection].globalIndex;
        };
    };
    while (item>=0)
    {
        // deselect item
        Select(item, false);
        selected.insert(m_internalCPList[item].globalIndex);
        item = GetNextSelected(item);
    }
    DEBUG_DEBUG("about to delete " << selected.size() << " points");
    GlobalCmdHist::getInstance().addCommand(new PT::RemoveCtrlPointsCmd(*m_pano, selected));

    if (newSelection >= 0)
    {
        if (newSelectionCPIndex >= 0)
        {
            MainFrame::Get()->ShowCtrlPoint(newSelectionCPIndex);
        };
        Select(newSelection, true);
    };
};

void CPListCtrl::SelectDistanceThreshold(double threshold)
{
    const bool invert = threshold < 0;
    if (invert)
    {
        threshold = -threshold;
    };
    const CPVector& cps = m_pano->getCtrlPoints();
    Freeze();
    for (size_t i = 0; i < m_internalCPList.size(); i++)
    {
        const double error = cps[m_internalCPList[i].globalIndex].error;
        Select(i, ((error > threshold) && (!invert)) || ((error < threshold) && (invert)));
    };
    Thaw();
};

void CPListCtrl::SelectAll()
{
    for (long i = 0; i < m_internalCPList.size(); i++)
    {
        Select(i, true);
    };
};

#if !wxCHECK_VERSION(3,0,0)
#define WXK_CONTROL_A 1
#endif

void CPListCtrl::OnChar(wxKeyEvent& e)
{
    switch (e.GetKeyCode())
    {
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            DeleteSelected();
            break;
        case WXK_CONTROL_A:
            SelectAll();
            break;
        default:
            e.Skip();
    };
};


IMPLEMENT_DYNAMIC_CLASS(CPListCtrl, wxListCtrl)

IMPLEMENT_DYNAMIC_CLASS(CPListCtrlXmlHandler, wxListCtrlXmlHandler)

CPListCtrlXmlHandler::CPListCtrlXmlHandler()
: wxListCtrlXmlHandler()
{
    AddWindowStyles();
}

wxObject *CPListCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, CPListCtrl)
    cp->Create(m_parentAsWindow, GetID(), GetPosition(), GetSize(), GetStyle(wxT("style")), wxDefaultValidator, GetName());
    SetupWindow(cp);
    return cp;
}

bool CPListCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("CPListCtrl"));
}


BEGIN_EVENT_TABLE(CPListFrame, wxFrame)
    EVT_CLOSE(CPListFrame::OnClose)
    EVT_BUTTON(XRCID("cp_list_delete"), CPListFrame::OnDeleteButton)
    EVT_BUTTON(XRCID("cp_list_select"), CPListFrame::OnSelectButton)
END_EVENT_TABLE()

CPListFrame::CPListFrame(wxFrame* parent, Panorama& pano) : m_pano(pano)
{
    DEBUG_TRACE("");
    bool ok = wxXmlResource::Get()->LoadFrame(this, parent, wxT("cp_list_frame"));
    DEBUG_ASSERT(ok);
    m_list = XRCCTRL(*this, "cp_list_frame_list", CPListCtrl);
    DEBUG_ASSERT(m_list);
    m_list->Init(&m_pano);

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

    //set minumum size
    SetSizeHints(200, 300);
    //size
    RestoreFramePosition(this, wxT("CPListFrame"));
}

CPListFrame::~CPListFrame()
{
    DEBUG_TRACE("dtor");
    StoreFramePosition(this, wxT("CPListFrame"));
    DEBUG_TRACE("dtor end");
}

void CPListFrame::OnClose(wxCloseEvent& event)
{
    DEBUG_DEBUG("OnClose");
    MainFrame::Get()->OnCPListFrameClosed();
    DEBUG_DEBUG("closing");
    Destroy();
}

void CPListFrame::OnDeleteButton(wxCommandEvent & e)
{
    m_list->DeleteSelected();
}

void CPListFrame::OnSelectButton(wxCommandEvent & e)
{
    // calculate the mean error and the standard deviation
    const CPVector & cps = m_pano.getCtrlPoints();
    HuginBase::PTools::calcCtrlPointErrors(m_pano);
    double min, max, mean, var;
    HuginBase::CalculateCPStatisticsError::calcCtrlPntsErrorStats(m_pano, min, max, mean, var);

    // select points whos distance is greater than the mean
    // hmm, maybe some theory would be nice.. this is just a
    // guess.
    double threshold = mean + sqrt(var);
    wxString t;
    do
    {
        t = wxGetTextFromUser(_("Enter minimum control point error.\nAll points with a higher error will be selected"), _("Select Control Points"),
            doubleTowxString(threshold, 2));
        if (t == wxEmptyString) {
            // do not select anything
            return;
        }
    }
    while (!str2double(t, threshold));

    m_list->SelectDistanceThreshold(threshold);
};
