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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _MYPROGRESSDIALOG_H
#define _MYPROGRESSDIALOG_H

#include <hugin_shared.h>
#include "panoinc.h"
#include "panoinc_WX.h"
#include <appbase/ProgressDisplayOld.h>
#include <appbase/ProgressReporterOld.h>

class WXIMPEX ProgressReporterDialog : public AppBase::ProgressReporter, public wxProgressDialog
{
public:
    ProgressReporterDialog(double maxProgress, const wxString& title, const wxString& message,
                         wxWindow * parent = NULL, 
                         int style = wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME,
                         const wxSize & sz = wxDefaultSize)
    : wxProgressDialog(title, message+wxString((wxChar)' ',10), 100, parent, style), 
      m_progress(0),m_maxProgress(maxProgress), m_abort(false)
      {  };

    virtual ~ProgressReporterDialog();

    virtual bool increaseProgress(double delta);
    virtual bool increaseProgress(double delta, const std::string & msg);
    
    // TODO entire ProgressReporter and ProgressDisplay API needs be updated to use wstring.
    // Temporarily implemented only for this function. from here -->
    virtual bool increaseProgress(double delta, const std::wstring & msg);
    // <- to here

    virtual void setMessage(const std::string & msg);

protected:
    double m_progress;
    double m_maxProgress;
    wxString m_message;
    bool m_abort;
};

/** wxProgressDialog with interface for my progress dialog
 *
 *  Also allows cancellation
 */
class WXIMPEX MyProgressDialog : public wxProgressDialog, public AppBase::MultiProgressDisplay
{
public:
    /** ctor.
     */
    MyProgressDialog(const wxString& title, const wxString& message,
                     wxWindow * parent = NULL, 
                     int style = wxPD_AUTO_HIDE | wxPD_APP_MODAL,
                     const wxSize & sz = wxDefaultSize)
        : wxProgressDialog(title, message, 100, parent, style)
        { 
            SetSize(sz);
        }
    /** dtor.
     */
    virtual ~MyProgressDialog() {};

    /** update the progress display */
    virtual void updateProgressDisplay();

    // override to abort the current operation.
    virtual void abortOperation()
    {
        DEBUG_TRACE("");
    }
private:
};

class OptProgressDialog : public MyProgressDialog
{
public:
    // work around a flaw in wxProgresDialog that results in incorrect layout
	// by pre-allocting sufficient horizontal and vertical space
    OptProgressDialog(wxWindow * parent = NULL,
                      int style = wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT )
        : MyProgressDialog(_("Optimizing Panorama"), (wxString((wxChar)' ', 80) + wxT("\n \n \n \n ")), parent, style)
        { }

    virtual void abortOperation();
};


#endif // _MYPROGRESSDIALOG_H
