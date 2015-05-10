// -*- c-basic-offset: 4 -*-
/** @file MyProgressDialog.h
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

#ifndef _MYPROGRESSDIALOG_H
#define _MYPROGRESSDIALOG_H

#include <hugin_shared.h>
#include <wx/progdlg.h>
#include <appbase/ProgressDisplay.h>

class WXIMPEX ProgressReporterDialog : public wxProgressDialog, public AppBase::ProgressDisplay
{
public:
    ProgressReporterDialog(int maxProgress, const wxString& title, const wxString& message,
                         wxWindow * parent = NULL, 
                         int style = wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME)
                         : wxProgressDialog(title, message + wxString((wxChar)' ', 10), 100, parent, style), 
                         ProgressDisplay(maxProgress)
      {  };
    // overwritten to work with wxString
    void setMessage(const std::string& message, const std::string& filename = "");
    // wxString versions for GUI
    void setMessage(const wxString& message, const wxString& filename = wxEmptyString);
    using ProgressDisplay::updateDisplay;
    bool updateDisplay(const wxString& message);
    using ProgressDisplay::updateDisplayValue;
    bool updateDisplayValue(const wxString& message, const wxString& filename = wxEmptyString);

protected:
    virtual void updateProgressDisplay();
    wxString m_wxmessage;
    wxString m_wxfilename;
};

#endif // _MYPROGRESSDIALOG_H
