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

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>
#include <wx/config.h>

#include <algorithm>
#include <utility>
#include <functional>

#include "hugin/CPListFrame.h"
#include "hugin/MainFrame.h"
#include "common/utils.h"

using namespace PT;
using namespace std;

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

BEGIN_EVENT_TABLE(CPListFrame, wxFrame)
    EVT_CLOSE(CPListFrame::OnClose)
    EVT_LIST_ITEM_SELECTED(XRCID("cp_list_frame_list"), CPListFrame::OnCPListSelect)
    EVT_LIST_COL_CLICK(XRCID("cp_list_frame_list"), CPListFrame::OnCPListHeaderClick)
END_EVENT_TABLE()


CPListFrame::CPListFrame(MainFrame * parent, Panorama & pano)
    : m_mainFrame(parent), m_pano(pano),m_verbose(false),
    m_sortCol(0), m_sortAscend(true)
{
    DEBUG_TRACE("");
    bool ok = wxXmlResource::Get()->LoadFrame(this, parent, wxT("cp_list_frame"));
    DEBUG_ASSERT(ok);
    m_list = XRCCTRL(*this, "cp_list_frame_list", wxListCtrl);
    DEBUG_ASSERT(m_list);

    wxConfigBase * config = wxConfigBase::Get();
    m_verbose = (config->Read("/CPListFrame/verbose",0l) != 0);

    if (m_verbose) {
        // setup list display
        m_list->InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25);
        m_list->InsertColumn( 1, _("left Img."), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 2, _("x"), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 3, _("y"), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 4, _("right Img."), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 5, _("x"), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 6, _("y"), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 7, _("Alignment"), wxLIST_FORMAT_LEFT,110 );
        m_list->InsertColumn( 8, _("Distance"), wxLIST_FORMAT_RIGHT, 110);
    } else {
        m_list->InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25);
        m_list->InsertColumn( 1, _("left Img."), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 2, _("right Img."), wxLIST_FORMAT_RIGHT, 65);
        m_list->InsertColumn( 3, _("Alignment"), wxLIST_FORMAT_LEFT,110 );
        m_list->InsertColumn( 4, _("Distance"), wxLIST_FORMAT_RIGHT, 110);
    }

    long w = config->Read("/CPListFrame/width",-1);
    long h = config->Read("/CPListFrame/height",-1);
    if (w != -1) {
        SetClientSize(w,h);
    }

    m_list->Show();
    // observe the panorama
    m_pano.addObserver(this);
    DEBUG_TRACE("ctor end");
}

CPListFrame::~CPListFrame()
{
    DEBUG_TRACE("dtor");
    wxSize sz = GetClientSize();
    wxConfigBase * config = wxConfigBase::Get();
    config->Write("/CPListFrame/width",sz.GetWidth());
    config->Write("/CPListFrame/height",sz.GetHeight());
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
                m_list->InsertItem(i, wxString::Format("%d",i));
            }
        }
    }
    // update list.
    updateList();
    // force a nice size
    int nrCol = m_verbose ? 9 : 5;
    for (int col=0; col < nrCol ; col++) {
        m_list->SetColumnWidth(col,wxLIST_AUTOSIZE);
    }
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
    }

    if (m_verbose) {
        m_list->SetItem(i,0,wxString::Format("%d",i));
        m_list->SetItem(i,1,wxString::Format("%d",p.image1Nr));
        m_list->SetItem(i,2,wxString::Format("%.1f",p.x1));
        m_list->SetItem(i,3,wxString::Format("%.1f",p.y1));
        m_list->SetItem(i,4,wxString::Format("%d",p.image2Nr));
        m_list->SetItem(i,5,wxString::Format("%.1f",p.x2));
        m_list->SetItem(i,6,wxString::Format("%.1f",p.y2));
        m_list->SetItem(i,7,mode);
        m_list->SetItem(i,8,wxString::Format("%.1f",p.error));
    } else {
        m_list->SetItem(i,0,wxString::Format("%d",i));
        m_list->SetItem(i,1,wxString::Format("%d",p.image1Nr));
        m_list->SetItem(i,2,wxString::Format("%d",p.image2Nr));
        m_list->SetItem(i,3,mode);
        m_list->SetItem(i,4,wxString::Format("%.1f",p.error));
    }
}


void CPListFrame::OnCPListSelect(wxListEvent & ev)
{
    int t = ev.GetIndex();
    if (t >=0) {
        int cp = m_list->GetItemData(t);
        m_mainFrame->ShowCtrlPoint((unsigned int) cp);
    }
}

void CPListFrame::updateList()
{
    const CPVector & cps = m_pano.getCtrlPoints();
    int nrCP = cps.size();

    // we need to save the point numbers...
    vector<pair<int,ControlPoint> > cpv(nrCP);
    for (int i=0; i < nrCP; ++i) {
        cpv[i] = make_pair(i,cps[i]);
    }
    int colLeftImg = 1;
    int colRightImg = 4;
    int colMode = 7;
    int colError = 8;

    if (!m_verbose) {
        colRightImg = 2;
        colMode = 3;
        colError = 4;
    }

    DEBUG_TRACE("sorting column " << m_sortCol);
    if (m_sortCol == colLeftImg) {
        if (m_sortAscend) {
            sort(cpv.begin(),cpv.end(), compareImg1Nr());
        } else {
            sort(cpv.begin(),cpv.end(), compareImg1NrGreater());
        }
    } else if (m_sortCol == colRightImg) {
        if (m_sortAscend) {
            sort(cpv.begin(),cpv.end(), compareImg2Nr());
        } else {
            sort(cpv.begin(),cpv.end(), compareImg2NrGreater());
        }
    } else if (m_sortCol == colMode) {
        if (m_sortAscend) {
            sort(cpv.begin(),cpv.end(), compareMode());
        } else {
            sort(cpv.begin(),cpv.end(), compareModeGreater());
        }
    } else if (m_sortCol == colError) {
        if (m_sortAscend) {
            sort(cpv.begin(),cpv.end(), compareError());
        } else {
            sort(cpv.begin(),cpv.end(), compareErrorGreater());
        }
    }

    for (int i=0; i < (int) nrCP; i++) {
        SetCPItem(i,cpv[i].second);
        m_list->SetItemData(i, cpv[i].first);
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
    updateList();
}

void CPListFrame::OnClose(wxCloseEvent& event)
{
    DEBUG_DEBUG("OnClose");
    // do not close, just hide if we're not forced
    if (event.CanVeto()) {
        event.Veto();
        Hide();
        DEBUG_DEBUG("hiding");
    } else {
        DEBUG_DEBUG("closing");
        Destroy();
    }
}


