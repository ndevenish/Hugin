// -*- c-basic-offset: 4 -*-
/**  @file FailedProjectsDialog.h
 *
 *  @brief Definition of failed projects dialog
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

#ifndef _FAILEDPROJECTSDIALOG_H
#define _FAILEDPROJECTSDIALOG_H

#include "panoinc_WX.h"
#include "panoinc.h"
#include "BatchFrame.h"

/** Dialog for finding panorama in given directory
 *
 * The algorithm transverse all directories for suitable image files (currently only jpeg and tiff)
 * If it found images, it compares EXIF information to deduce which images could belong
 * to a panorama.
 * After it the user can select which panoramas should created and added to detection queue
 *
 */
class FailedProjectsDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    FailedProjectsDialog(wxWindow* parent,Batch* batch,wxString xrcPrefix);
    /** destructor, saves size and position */
    ~FailedProjectsDialog();

protected:
    /** event handler, if new project was selected */
    void OnSelectProject(wxCommandEvent& e);
private:
    Batch* m_batch;
    wxListBox* m_list;
    wxTextCtrl* m_log;

    DECLARE_EVENT_TABLE()
};


#endif //_FAILEDPROJECTSDIALOG_H
