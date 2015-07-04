// -*- c-basic-offset: 4 -*-
/** @file AboutDialog.h
 *
 *  @brief Definition of dialog for numeric transforms
 *
 *  @author Yuval Levy <http://www.photopla.net/>
 *
 *  $Id$
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

#ifndef _ABOUTDIALOG_H
#define _ABOUTDIALOG_H

#include "panoinc_WX.h"
#include "panoinc.h"
#include "hugin/MainFrame.h"


/** Dialog for about window
 *
 * make a bit more action possible in the window
 */
class AboutDialog: public wxDialog
{
public:
    /** Constructor, read from xrc ressource */
    explicit AboutDialog(wxWindow *parent);

private:

    int m_mode;
    wxString m_logo_file;
    wxNotebook* m_about_notebook;
    wxStaticBitmap * m_logoImgCtrl;
    wxBitmap m_logo;

    DECLARE_EVENT_TABLE()

    /** retrieves the system information */
    void GetSystemInformation(wxFont *font);
    /** event handler for changing tab */
    void OnChangedTab(wxNotebookEvent &e);
    /** function to switch the logo image based on selected tab */
    void SetMode(int newMode);
    /** function to set the log image */
    void SetLogo(wxString newLogoFile);
};

#endif // _ABOUTDIALOG_H
