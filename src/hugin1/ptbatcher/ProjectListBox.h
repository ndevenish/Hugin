// -*- c-basic-offset: 4 -*-

/** @file ProjectListBox.h
 *
 *  @brief Batch processor for Hugin with GUI
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: ProjectListBox.h 3322 2008-08-16 5:00:07Z mkuder $
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

#include <wx/listctrl.h>
#include <wx/xrc/xh_listc.h>
#include "ProjectArray.h"
#include "Batch.h"

class ProjectListBox : public wxListCtrl
{
public:
    //Constructor
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    //Appends project to list
    void AppendProject(Project* project);
    void ChangePrefix(int index, wxString newPrefix);
    void Deselect(int index);
    void Fill(Batch* batch);
    int GetIndex(int id);
    int GetProjectCountByPath(wxString path);
    int GetProjectId(int index);
    int GetSelectedIndex();
    wxString GetSelectedProject();
    /** gets the prefix of the currently selected project */
    wxString GetSelectedProjectPrefix();
    /** return the target of the currently selected project */
    Project::Target GetSelectedProjectTarget();
    wxString GetText(int row, int column);
    void ReloadProject(int index, Project* project);
    void Select(int index);
    void SetMissing(int index);
    void SwapProject(int index);
    bool UpdateStatus(int index, Project* project);

protected:
    void OnContextMenu(wxContextMenuEvent& e);
    void OnChangePrefix(wxCommandEvent& e);
    void OnResetProject(wxCommandEvent& e);
    void OnEditProject(wxCommandEvent& e);
    void OnRemoveProject(wxCommandEvent& e);

private:
    int m_selected;
    Batch* m_batch;
    IntArray columns;

    wxString GetAttributeString(int i, Project* project);
    wxString GetLongerFormatName(std::string str);
    void OnColumnWidthChange(wxListEvent& event);
    void OnDeselect(wxListEvent& event);
    void OnSelect(wxListEvent& event);

    enum ColumnName
    {
        ID,
        PROJECT,
        PREFIX,
        MODDATE,
        FORMAT,
        PROJECTION,
        SIZE,
        STATUS
    };
    //options taken from enum in PanoramaOptions.h. Should it change
    //in the future, these arrays should be corrected also
    static const wxString fileFormat[];
    static const wxString outputMode[];
    static const wxString HDRMergeType[];
    static const wxString blendingMechanism[];
    static const wxString colorCorrection[];

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(ProjectListBox)
};

/** xrc handler */
class ProjectListBoxXmlHandler : public wxListCtrlXmlHandler
{
    DECLARE_DYNAMIC_CLASS(ProjectListBoxXmlHandler)

public:
    ProjectListBoxXmlHandler();
    virtual wxObject* DoCreateResource();
    virtual bool CanHandle(wxXmlNode* node);
};
