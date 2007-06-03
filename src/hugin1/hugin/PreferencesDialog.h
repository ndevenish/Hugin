// -*- c-basic-offset: 4 -*-
/** @file PreferencesDialog.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PreferencesDialog.h 1855 2007-01-01 20:44:20Z dangelo $
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

#include "common/utils.h"

/** hugin preferences dialog
 *
 *  A simple preferences dialog, used
 *  to inspect and set the various prefs stored
 *  in the wxConfig object
 */
class PreferencesDialog : public wxFrame
{
public:

    /** ctor.
     */
    PreferencesDialog(wxWindow *parent);

    /** dtor.
     */
    virtual ~PreferencesDialog();

    /** Config to Window*/
    void UpdateDisplayData();

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
    void OnAutopanoSiftExe(wxCommandEvent & e);
    void OnAutopanoKolorExe(wxCommandEvent & e);
    void OnRestoreDefaults(wxCommandEvent & e);
    void OnCustomAPSIFT(wxCommandEvent & e);
    void OnCustomAPKolor(wxCommandEvent & e);
    void OnCustomEnblend(wxCommandEvent & e);
    void OnCustomPTStitcher(wxCommandEvent & e);
    void EnableRotationCtrls(bool enable);
    bool GetPanoVersion();

private:
	wxString m_PTVersion;
	wxString m_PTDetails;
	
    DECLARE_EVENT_TABLE()

};



#endif // _PREFERENCESDIALOG_H
