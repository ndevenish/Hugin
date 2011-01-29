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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _CPLISTFRAME_H
#define _CPLISTFRAME_H

class MainFrame;

/** List all control points of this project
 *
 *  useful to jump to a specific point, or see which point are bad
 */
class CPListFrame : public wxFrame, public PT::PanoramaObserver
{
public:
    /** ctor.
     */
    CPListFrame(MainFrame * parent, PT::Panorama & pano);
	
    /** dtor.
     */
    virtual ~CPListFrame();
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** Delete the selected points */
    void DeleteSelected();
    /** Select all points */
    void SelectAll();

private:

    void SortList();

    void SetCPItem(int i, const PT::ControlPoint & p);

    void OnCPListSelectionChanged(wxListEvent & e);
    void OnCPListHeaderClick(wxListEvent & e);
    void OnDeleteButton(wxCommandEvent & e);
    void OnFineTuneButton(wxCommandEvent & e);
    void OnSelectButton(wxCommandEvent & e);

    void updateList();

    void OnColumnWidthChange( wxListEvent & e );
    
    void OnClose(wxCloseEvent& event);

    std::string makePairId(unsigned int id1, unsigned int id2);

    MainFrame * m_mainFrame;
    PT::Panorama & m_pano;
    wxListCtrl * m_list;

    // show point coordinates?
    bool m_verbose;

    // current sorting column
    int m_sortCol;

    bool m_sortAscend;

    // disable list updates..
    bool m_freeze;

    std::map<std::string, int> m_localIds;

    // needed for receiving events.
    DECLARE_EVENT_TABLE();
};



#endif // _CPLISTFRAME_H
