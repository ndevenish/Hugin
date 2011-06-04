// -*- c-basic-offset: 4 -*-
/** @file PreferencesDialog.h
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

#ifndef _PREFERENCESDIALOG_H
#define _PREFERENCESDIALOG_H

#include "panoinc.h"
#include "panoinc_WX.h"

#include "icpfind/CPDetectorConfig.h"

/** hugin preferences dialog
 *
 *  A simple preferences dialog, used
 *  to inspect and set the various prefs stored
 *  in the wxConfig object
 */
class PreferencesDialog : public wxDialog
{
public:

    /** ctor.
     */
    PreferencesDialog(wxWindow *parent);

    /** dtor.
     */
    virtual ~PreferencesDialog();

    /** Config to Window
     *  @param panel to update (index starts with 1), use 0 to update all panels
     */
    void UpdateDisplayData(int panel);

    /** Window to Config */
    void UpdateConfigData();

protected:
    void OnOk(wxCommandEvent & e);
    void OnApply(wxCommandEvent & e);
    void OnCancel(wxCommandEvent & e);
    void OnDefaults(wxCommandEvent & e);
    void OnRotationCheckBox(wxCommandEvent & e);
    void OnClose(wxCloseEvent& event);
    void OnPTStitcherExe(wxCommandEvent & e);
    void OnPTDetails(wxCommandEvent & e);
    void OnEditorExe(wxCommandEvent & e);
    void OnEnblendExe(wxCommandEvent & e);
    void OnEnfuseExe(wxCommandEvent & e);
    void OnRestoreDefaults(wxCommandEvent & e);
    void OnCustomEnblend(wxCommandEvent & e);
    void OnCustomEnfuse(wxCommandEvent & e);
    void OnCustomPTStitcher(wxCommandEvent & e);
    void OnCPDetectorAdd(wxCommandEvent & e);
    void OnCPDetectorEdit(wxCommandEvent & e);
    void OnCPDetectorDelete(wxCommandEvent & e);
    void OnCPDetectorMoveUp(wxCommandEvent & e);
    void OnCPDetectorMoveDown(wxCommandEvent & e);
    void OnCPDetectorDefault(wxCommandEvent & e);
    void OnCPDetectorListDblClick(wxCommandEvent & e);
    /** event handler for loading cp detector settings */
    void OnCPDetectorLoad(wxCommandEvent & e);
    /** event handler for saving cp detector settings */
    void OnCPDetectorSave(wxCommandEvent & e);
    /** event handler for showing help for cp detector settings */
    void OnCPDetectorHelp(wxCommandEvent & e);
    /** event handler if default file format was changed */
    void OnFileFormatChanged(wxCommandEvent & e);
    /** event handler if processor was changed */
    void OnProcessorChanged(wxCommandEvent & e);
    void EnableRotationCtrls(bool enable);
    bool GetPanoVersion();

private:
    void UpdateFileFormatControls();
    void UpdateProcessorControls();
    wxString m_PTVersion;
    wxString m_PTDetails;
    wxListBox* m_CPDetectorList;
    CPDetectorConfig cpdetector_config_edit;

    DECLARE_EVENT_TABLE()

};



#endif // _PREFERENCESDIALOG_H
