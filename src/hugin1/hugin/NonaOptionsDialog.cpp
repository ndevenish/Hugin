// -*- c-basic-offset: 4 -*-

/** @file NonaOptionsDialog.cpp
 *
 *  @brief implementation of NonaOptionsDialog Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: NonaOptionsDialog.cpp 2510 2007-10-28 22:24:11Z dangelo $
 *
 *  This program is free software; you can redistribute it and/or
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

#include <config.h>
#include <errno.h>
#include "panoinc_WX.h"

#include "panoinc.h"

#include "PT/Stitcher.h"

#include "base_wx/wxPlatform.h"
#include "hugin/config_defaults.h"
#include "hugin/CommandHistory.h"
#include "hugin/NonaOptionsDialog.h"

using namespace PT;
using namespace std;
using namespace hugin_utils;

//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(NonaOptionsDialog, wxDialog)
    EVT_CHOICE ( XRCID("nona_choice_interpolator"),NonaOptionsDialog::InterpolatorChanged)
    EVT_CHECKBOX( XRCID("nona_cb_cropped"), NonaOptionsDialog::OnSaveCropped)
END_EVENT_TABLE()


// Define a constructor for the Pano Panel
NonaOptionsDialog::NonaOptionsDialog(wxWindow *parent, Panorama & pano)
    : wxDialog(parent, 1, wxString(_("Nona options"))),
      pano(pano),
      updatesDisabled(false)
{

    // loading xrc resources in selfcreated this panel
    wxXmlResource::Get()->LoadPanel ( this, wxT("nona_options_dialog"));

    // converts KILL_FOCUS events to usable TEXT_ENTER events
    // get gui controls
    m_InterpolatorChoice = XRCCTRL(*this, "nona_choice_interpolator",
                                   wxChoice);
    DEBUG_ASSERT(m_InterpolatorChoice);

    m_SaveCroppedCB = XRCCTRL(*this, "nona_cb_cropped", wxCheckBox);
    DEBUG_ASSERT(m_SaveCroppedCB);

    UpdateDisplay(pano.getOptions());

    // observe the panorama
    pano.addObserver (this);

    Fit();
    wxSize sz = GetSize();
    SetSizeHints(sz.GetWidth(), sz.GetHeight());

    DEBUG_DEBUG("setting minsize to:" << sz.GetWidth() << "x" << sz.GetHeight());
}


NonaOptionsDialog::~NonaOptionsDialog(void)
{
    DEBUG_TRACE("dtor");
    pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}


void NonaOptionsDialog::panoramaChanged (PT::Panorama &pano)
{
	DEBUG_TRACE("");
    PanoramaOptions opt = pano.getOptions();
    // update all options for dialog and notebook tab
    UpdateDisplay(opt);
    m_oldOpt = opt;
}

void NonaOptionsDialog::UpdateDisplay(const PanoramaOptions & opt)
{
    m_InterpolatorChoice->SetSelection(opt.interpolator);
    m_SaveCroppedCB->SetValue(opt.tiff_saveROI);
}


void NonaOptionsDialog::InterpolatorChanged ( wxCommandEvent & e )
{
    if (updatesDisabled) return;
    PanoramaOptions opt = pano.getOptions();
    //Interpolator from PanoramaMemento.h
    int lt = m_InterpolatorChoice->GetSelection();

    opt.interpolator = (vigra_ext::Interpolator) lt;
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetPanoOptionsCmd( pano, opt )
        );
    DEBUG_DEBUG ("Interpolator changed to: " << lt );
}

void NonaOptionsDialog::OnSaveCropped(wxCommandEvent & e)
{
    PanoramaOptions opt = pano.getOptions();

    opt.tiff_saveROI= m_SaveCroppedCB->GetValue();

    GlobalCmdHist::getInstance().addCommand(
            new PT::SetPanoOptionsCmd( pano, opt )
                                           );
}
