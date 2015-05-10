// -*- c-basic-offset: 4 -*-
/**  @file ResetDialog.h
 *
 *  @brief Definition of ResetDialog class
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

#ifndef _RESETDIALOG_H
#define _RESETDIALOG_H

#include "panoinc_WX.h"
#include "GuiLevel.h"

/** Dialog for reset panorama settings
 *
 * Dialog let user select, which parameter should be reseted. 
 * The user can select
 * - reset position (yaw, pitch, roll)
 * - reset fov (when images files have the right EXIF values, otherwise fov remains unchanged)
 * - reset lens parameter (a, b, c, d, e, g, t)
 * - reset exposure: to EXIF value (exposure correction) or zero (no exposure correction)
 * - reset color: to EXIF value or one
 * - reset vignetting
 * - reset camera response
 */
class ResetDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    ResetDialog(wxWindow *parent, GuiLevel guiLevel);
	/** Return TRUE, when user selected "Reset position" */
	bool GetResetPos();
	/** Return TRUE, when user selected "Reset translation" */
	bool GetResetTranslation();
	/** Return TRUE, when user selected "Reset FOV" */
	bool GetResetFOV();
	/** Return TRUE, when user selected "Reset lens" */
	bool GetResetLens();
	/** Return TRUE, when user selected "Reset exposure" */
	bool GetResetExposure();
	/** Return TRUE, when user selected "Reset exposure to EXIF",
	 * Return FALSE, when user selected "Reset exposure to ZERO"
	 */
	bool GetResetExposureToExif();
	/** Return TRUE, when user selected "Reset color" */
	bool GetResetColor();
	/** Return TRUE, when user selected "Reset color to EXIF",
	 * Return FALSE, when user selected "Reset color to ONE"
	 */
	bool GetResetColorToExif();
	/** Return TRUE, when user selected "Reset vignetting" */
	bool GetResetVignetting();
	/** Return TRUE, when user selected "Reset Camera Response" */
	bool GetResetResponse();

    /** limits the displayed parameters to geometric parameters */
    void LimitToGeometric();
    /** limits the displayed parameters to photometric parameters */
    void LimitToPhotometric();
protected:
	/** Method for enabling/disable combobox to select reset exposure to EXIF or ZERO
	 * depending on state of "Reset exposure" checkbox
	 */
	void OnSelectExposure(wxCommandEvent & e);
	/** Method for enabling/disable combobox to select reset color to EXIF or ONE
	 * depending on state of "Reset color" checkbox
	 */
	void OnSelectColor(wxCommandEvent & e);
	/** Saves current state of all checkboxes when closing dialog with Ok */
	void OnOk(wxCommandEvent & e);

private:
    DECLARE_EVENT_TABLE()
};

#endif //_RESETDIALOG_H
