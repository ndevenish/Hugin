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

#include "panoinc.h"
#include "panoinc_WX.h"

#include "common/utils.h"

/** wxProgressDialog with interface for my progress dialog
 *
 *  Also allows cancellation
 */
class MyProgressDialog : public wxProgressDialog, public utils::MultiProgressDisplay
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
    OptProgressDialog(wxWindow * parent = NULL,
                      int style = wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT )
        : MyProgressDialog(_("Optimizing Panorama"), wxT(""), parent, style)
        { }

    virtual void abortOperation();
};


#endif // _MYPROGRESSDIALOG_H
