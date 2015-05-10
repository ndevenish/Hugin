// -*- c-basic-offset: 4 -*-
/**  @file HDRMergeOptionDialog.h
 *
 *  @brief Definition of dialog for hdrmerge options
 *
 *  @author Thomas Modes
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

#ifndef _HDRMERGEOPTIONDIALOG_H
#define _HDRMERGEOPTIONDIALOG_H

#include "panoinc_WX.h"

/** Dialog for reset panorama settings
 *
 * Dialog let user select, which parameter should be reseted. 
 * The user can select
 * - reset position (yaw, pitch, roll)
 * - reset fov (when images files have the right EXIF values, otherwise fov remains unchanged)
 * - reset lens parameter (a, b, c, d, e, g, t)
 * - reset exposure: to EXIF value (exposure correction) or zero (no exposure correction)
 * - reset color
 * - reset vignetting
 * - reset camera response
 */
class HDRMergeOptionsDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource */
    HDRMergeOptionsDialog(wxWindow *parent);
    /** sets the currents state of the hdrmerge options */
    void SetCommandLineArgument(wxString cmd);
    /** returns the hdrmerge options as command line arguments */
    wxString GetCommandLineArgument() { return m_cmd;};
    /** check inputs */
    void OnOk(wxCommandEvent & e);

protected:
    /** event handler when user selected different mode, refresh advanced option display */
    void OnModeChanged(wxCommandEvent &e);

private:
    bool BuildCommandLineArgument();
    wxChoice *m_mode;
    wxPanel *m_panel_avg;
    wxPanel *m_panel_avgslow;
    wxPanel *m_panel_khan;
    wxCheckBox *m_option_c;
    wxSpinCtrl *m_khan_iter;
    wxTextCtrl *m_khan_sigma;
    wxCheckBox *m_option_khan_af;
    wxCheckBox *m_option_khan_ag;
    wxCheckBox *m_option_khan_am;
    wxString m_cmd;
    DECLARE_EVENT_TABLE()
};

#endif //_HDRMERGEOPTIONDIALOG_H
