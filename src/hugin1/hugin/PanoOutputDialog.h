// -*- c-basic-offset: 4 -*-
/**  @file PanoOutputDialog.h
 *
 *  @brief Definition of PanoOutputDialog class
 *
 *  @author T. Modes
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

#ifndef _PANOOUTPUTDIALOG_H
#define _PANOOUTPUTDIALOG_H

#include "panoinc_WX.h"
#include "panoinc.h"
#include "GuiLevel.h"

/** Dialog for setting output parameters for simple user interface */
class PanoOutputDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource; restore last uses settings, size and position */
    PanoOutputDialog(wxWindow *parent, HuginBase::Panorama& pano, GuiLevel guiLevel);
    /** destructor, save position */
    ~PanoOutputDialog();
    HuginBase::PanoramaOptions GetNewPanoramaOptions() {return m_newOpt; } ;
protected:
	/** Saves current state of all checkboxes when closing dialog with Ok */
	void OnOk(wxCommandEvent & e);
    /** enabled Ok button and LDR/HDR format settings depeding on selected output settings */
    void OnOutputChanged(wxCommandEvent & e);
    /** LDR format changed */
    void OnLDRFormatChanged(wxCommandEvent & e);
    /** HDR format changed */
    void OnHDRFormatChanged(wxCommandEvent & e);
    /** width changed */
    void OnWidthChanged(wxSpinEvent & e);
    /** height changed */
    void OnHeightChanged(wxSpinEvent & e);

private:
    void EnableOutputOptions();

    HuginBase::PanoramaOptions m_newOpt;
    HuginBase::Panorama& m_pano;
    double m_initalWidth;
    double m_initalROIWidth;
    double m_aspect;
    GuiLevel m_guiLevel;

    wxSpinCtrl* m_edit_width;
    wxSpinCtrl* m_edit_height;

    std::vector<HuginBase::UIntSet> m_stacks;
    std::vector<HuginBase::UIntSet> m_exposureLayers;

    DECLARE_EVENT_TABLE()
};

#endif //_PANOOUTPUTDIALOG_H
