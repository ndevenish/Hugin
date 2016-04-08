// -*- c-basic-offset: 4 -*-
/** @file CPListFrame.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _CPLISTFRAME_H
#define _CPLISTFRAME_H

#include <wx/xrc/xh_listc.h>
#include <vector>

class MainFrame;

/** helper class for virtual listview control */
struct CPListItem
{
    size_t globalIndex;
    size_t localNumber;
};

/** List all control points of this project
 *
 *  useful to jump to a specific point, or see which point are bad
 */
class CPListCtrl : public wxListView, public HuginBase::PanoramaObserver
{
public:
    CPListCtrl();
    ~CPListCtrl();
    bool Create(wxWindow *parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxLC_REPORT | wxLC_VIRTUAL,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxListCtrlNameStr);

    void Init(HuginBase::Panorama* pano);
    virtual void panoramaChanged(HuginBase::Panorama &pano);
    virtual void panoramaImagesChanged(HuginBase::Panorama &pano, const HuginBase::UIntSet& changed) {};
    /** Delete the selected points */
    void DeleteSelected();
    /** select all cp with the given error bigger than the threshold */
    void SelectDistanceThreshold(double threshold);
    /** select all items */
    void SelectAll();
protected:
    /** create labels for virtual list control */
    virtual wxString OnGetItemText(long item, long column) const;
    /** selection event handler */
    void OnCPListSelectionChanged(wxListEvent & e);
    /** sort criterium changed */
    void OnCPListHeaderClick(wxListEvent & e);
    /** column width changed */
    void OnColumnWidthChange(wxListEvent & e);
    /** handle keystrokes */
    void OnChar(wxKeyEvent& e);
private:
    void UpdateInternalCPList();
    void SortInternalList(bool isAscending);

    HuginBase::Panorama* m_pano;
    // current sorting column
    int m_sortCol;
    bool m_sortAscend;
    bool m_onlyActiveImages = false;
    std::vector<CPListItem> m_internalCPList;
    std::map<std::string, int> m_localIds;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(CPListCtrl)
};

/** xrc handler for CPImagesComboBox */
class CPListCtrlXmlHandler : public wxListCtrlXmlHandler
{
    DECLARE_DYNAMIC_CLASS(CPListCtrlXmlHandler)

public:
    /** Constructor */
    CPListCtrlXmlHandler();
    /** Create CPImagesComboBox from resource */
    virtual wxObject *DoCreateResource();
    /** Internal use to identify right xml handler */
    virtual bool CanHandle(wxXmlNode *node);
};

class CPListFrame : public wxFrame
{
public:
    /** ctor.
     */
    CPListFrame(wxFrame* parent, HuginBase::Panorama & pano);
	
    /** dtor.
     */
    virtual ~CPListFrame();

protected:
    void OnDeleteButton(wxCommandEvent & e);
    void OnSelectButton(wxCommandEvent & e);
    void OnClose(wxCloseEvent& event);
private:
    CPListCtrl* m_list;
    HuginBase::Panorama& m_pano;
    // needed for receiving events.
    DECLARE_EVENT_TABLE();
};



#endif // _CPLISTFRAME_H
