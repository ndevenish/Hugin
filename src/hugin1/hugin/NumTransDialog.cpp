// -*- c-basic-offset: 4 -*-

/** @file NumTransDialog.cpp
 *
 *  @brief Definition of dialog for numeric transforms
 *
 *  @author Yuval Levy <http://www.photopla.net/>
 *
 *  $Id$
 */

/*  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// often necessary before panoinc.h
#ifdef __APPLE__
#include "panoinc_WX.h"
#endif
// standard hugin include
#include "panoinc.h"
// both includes above need to come before other wx includes on OSX

#include "hugin/NumTransDialog.h"
#include "common/wxPlatform.h"
#include "base_wx/platform.h"
#include <wx/glcanvas.h>
#include "hugin/huginApp.h"
#include "hugin/CommandHistory.h"

using namespace PT;

BEGIN_EVENT_TABLE(NumTransDialog, wxDialog)
    EVT_BUTTON(XRCID("apply_numeric_transform"), NumTransDialog::OnApplyNumTransform)
END_EVENT_TABLE()

NumTransDialog::NumTransDialog(wxWindow *parent, PT::Panorama &pano)
    : m_pano(&pano)
{

	// load the GUI from the XRC file
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("dlg_numtrans"));

    // load the appropriate icon (.ico for Windows, .png for other systems)
#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif

    // set the icon in the title bar
    SetIcon(myIcon);
    // set the position and the size (x,y,width,height). -1 = keep existing
    this->SetSize(1,1,-1,-1);
    // this->CenterOnParent();
    // make the window modal
    this->ShowModal();
}

void NumTransDialog::OnApplyNumTransform(wxCommandEvent & e)
{
	std::cout << "apply\n";

        wxString text = XRCCTRL(*this, "numtrans_yaw", wxTextCtrl)->GetValue();
        double y;
        if (!str2double(text, y)) {
            wxLogError(_("Yaw value must be numeric."));
            return;
        }
        text = XRCCTRL(*this, "numtrans_pitch", wxTextCtrl)->GetValue();
        double p;
        if (!str2double(text, p)) {
            wxLogError(_("Pitch value must be numeric."));
            return;
        }
        text = XRCCTRL(*this, "numtrans_roll", wxTextCtrl)->GetValue();
        double r;
        if (!str2double(text, r)) {
            wxLogError(_("Roll value must be numeric."));
            return;
        }
        GlobalCmdHist::getInstance().addCommand(
            new PT::RotatePanoCmd(*m_pano, y, p, r)
            );

}

