// -*- c-basic-offset: 4 -*-
/**  @file CPDetectorDialog.h
 *
 *  @brief declaration of CPDetectorDialog class,
 *         which are for storing and changing settings of different autopano generator 
 *
 *  @author Thomas Modes
 *
 */
 
 /*  This is free software; you can redistribute it and/or
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

#ifndef _CPDETECTORDIALOG_H
#define _CPDETECTORDIALOG_H

#include "panoinc.h"
#include "panoinc_WX.h"
#include "icpfind/CPDetectorConfig.h"

#include <wx/choicebk.h>

/** dialog for input settings of one autopano generator */
class CPDetectorDialog : public wxDialog
{
public:
    /** constructor */
    CPDetectorDialog(wxWindow* parent);
    /** destructor, saves position and size */
    virtual ~CPDetectorDialog();
    /** updates edit fields with values from settings 
     *  @param cpdet_config CPDetectorConfig class, which stores the settings
     *  @param index index, from which the settings should be read */
    void UpdateFields(CPDetectorConfig* cpdet_config,int index);
    /** return inputed settings 
     *  @param cpdet_config CPDetectorConfig class, which stores the settings
     *  @param index index, to which the changed settings should be written */
    void UpdateSettings(CPDetectorConfig* cpdet_config,int index);
protected:
    /** check inputs */
    void OnOk(wxCommandEvent & e);
    /** select program with file open dialog */
    void OnSelectPath(wxCommandEvent &e);
    /** select program for feature descriptor with file open dialog */
    void OnSelectPathDescriptor(wxCommandEvent &e);
    /** select program for feature matcher with file open dialog */
    void OnSelectPathMatcher(wxCommandEvent &e);
    /** select program for stack with file open dialog */
    void OnSelectPathStack(wxCommandEvent &e);
    /** update dialog, when other cp detector type is changed */
    void OnTypeChange(wxCommandEvent &e);
    /** block selection of two step detector for autopano setting */
    void OnStepChanging(wxChoicebookEvent &e);
    /** shows file dialog */
    bool ShowFileDialog(wxString & prog);
private:
    wxTextCtrl *m_edit_desc;
    wxTextCtrl *m_edit_prog;
    wxTextCtrl *m_edit_args;
    wxStaticText *m_label_args_cleanup;
    wxTextCtrl *m_edit_args_cleanup;
    wxTextCtrl *m_edit_prog_descriptor;
    wxTextCtrl *m_edit_args_descriptor;
    wxTextCtrl *m_edit_prog_matcher;
    wxTextCtrl *m_edit_args_matcher;
    wxTextCtrl *m_edit_prog_stack;
    wxTextCtrl *m_edit_args_stack;
    wxCheckBox *m_check_option;
    wxChoice *m_cpdetector_type;
    wxChoicebook * m_choice_step;
    bool twoStepAllowed;

    void ChangeType();
    DECLARE_EVENT_TABLE();
};

#endif
