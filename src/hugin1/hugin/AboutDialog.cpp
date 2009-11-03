// -*- c-basic-offset: 4 -*-

/** @file AboutDialog.cpp
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

#include "hugin/AboutDialog.h"
#include "common/wxPlatform.h"
#include "hugin/huginApp.h"


BEGIN_EVENT_TABLE(AboutDialog, wxDialog)
    EVT_BUTTON(XRCID("about_me"), AboutDialog::OnAboutMe)
END_EVENT_TABLE()


AboutDialog::AboutDialog(wxWindow *parent)
{

    wxDialog dlg;
	wxString strFile;
	wxString langCode;
	wxBitmap image;
	wxStaticBitmap *imageCtrl;
	wxTextCtrl *textCtrl;
	
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("about_dlg"));

#if __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    //rely on the system's locale choice
    strFile = MacGetPathToBundledResourceFile(CFSTR("about.htm"));
    if(strFile!=wxT("")) XRCCTRL(dlg,"about_html",wxHtmlWindow)->LoadPage(strFile);
#else    
    //if the language is not default, load custom About file (if exists)
    langCode = huginApp::Get()->GetLocale().GetName().Left(2).Lower();
    DEBUG_INFO("Lang Code: " << langCode.mb_str(wxConvLocal));
    if(langCode != wxString(wxT("en")))
    {
        strFile = huginApp::Get()->GetXRCPath() + wxT("data/about_") + langCode + wxT(".htm");
        if(wxFile::Exists(strFile))
        {
            DEBUG_TRACE("Using About: " << strFile.mb_str(wxConvLocal));
            XRCCTRL(dlg,"about_html",wxHtmlWindow)->LoadPage(strFile);
        }
    }
#endif


    /** bitmap with logo */
    wxBitmap logo;
    wxStaticBitmap *logoImgCtrl;
    imageCtrl = XRCCTRL(*this, "about_logo", wxStaticBitmap);

    image.LoadFile(huginApp::Get()->GetXRCPath() +
                     wxT("data/") + wxT("logo.png"),
                     wxBITMAP_TYPE_PNG);
    imageCtrl->SetBitmap(image);

	// License
	textCtrl = XRCCTRL(*this, "license_txt", wxTextCtrl);
    strFile = huginApp::Get()->GetXRCPath() + wxT("data/COPYING");
	textCtrl->LoadFile(strFile);

	// About
	textCtrl = XRCCTRL(*this, "about_txt", wxTextCtrl);
    strFile = huginApp::Get()->GetXRCPath() + wxT("data/about.txt");
	textCtrl->LoadFile(strFile);

	// Upstream
	textCtrl = XRCCTRL(*this, "upstream_txt", wxTextCtrl);
    strFile = huginApp::Get()->GetXRCPath() + wxT("data/upstream.txt");
	textCtrl->LoadFile(strFile);

    // load the appropriate icon (.ico for Windows, .png for other systems)
#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    // set the icon in the title bar
    SetIcon(myIcon);

    // set the position and the size (x,y,width,height). -1 = keep existing
    this->SetSize(1,1,560,560);
    this->CenterOnParent();
    // make the window modal
    this->ShowModal();

}

// class destructor
AboutDialog::~AboutDialog()
{
	// insert your code here
}

void AboutDialog::OnAboutMe(wxCommandEvent & e)
{
    return;
}
