// -*- c-basic-offset: 4 -*-
/** @file NumTransDialog.h
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _NUMTRANSDIALOG_H
#define _NUMTRANSDIALOG_H

#include "panoinc_WX.h"
#include "hugin/MainFrame.h"

using namespace PT;

/** Dialog for numeric transform
 *
 * Dialog let user increment yaw, pitch and roll of a panorama
 */
class NumTransDialog: public wxDialog, public PT::PanoramaObserver
{
public:
    /** Constructor, read from xrc ressource */
    NumTransDialog(wxWindow *parent, PT::Panorama &pano);

private:

    PT::Panorama * m_pano;

    wxTextCtrl *m_numtrans_Yaw;
    wxTextCtrl *m_numtrans_Pitch;
    wxTextCtrl *m_numtrans_Roll;
    wxButton   *m_apply_numeric_transform;
    DECLARE_EVENT_TABLE()

    // apply numeric transform
    void OnApplyNumTransform(wxCommandEvent & e);

};

#endif // _NUMTRANSDIALOG_H
