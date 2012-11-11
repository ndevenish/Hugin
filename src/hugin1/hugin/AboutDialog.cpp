// -*- c-basic-offset: 4 -*-

/** @file AboutDialog.cpp
 *
 *  @brief Definition of about dialog
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

#include "base_wx/wxPlatform.h"
#include "panoinc.h"
#include "hugin/huginApp.h"
#include <hugin_version.h>
#include <wx/version.h>
#include "pano13/version.h"
#include "boost/version.hpp"
#include "exiv2/version.hpp"
#include "lensdb/LensDB.h"

BEGIN_EVENT_TABLE(AboutDialog, wxDialog)
    EVT_NOTEBOOK_PAGE_CHANGED(XRCID("about_notebook"), AboutDialog::OnChangedTab)
END_EVENT_TABLE()


AboutDialog::AboutDialog(wxWindow *parent)
{
	wxString strFile;
	wxString langCode;
	wxTextCtrl *textCtrl;
    m_logoImgCtrl=NULL;
    m_mode=0;
	
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("about_dlg"));

#if 0
// currently authors and about text are not translated, so comment out
#if __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    //rely on the system's locale choice
    strFile = MacGetPathToBundledResourceFile(CFSTR("about.htm"));
    if(strFile!=wxT("")) XRCCTRL(*this,"about_html",wxHtmlWindow)->LoadPage(strFile);
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
            XRCCTRL(*this,"about_html",wxHtmlWindow)->LoadPage(strFile);
        }
    }
#endif
#endif


    // Version
    XRCCTRL(*this,"about_version", wxTextCtrl)->ChangeValue(wxString(DISPLAY_VERSION, wxConvLocal));

	#ifdef __WXMAC__
		wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	#else
		wxFont font(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	#endif
	
	// License
	textCtrl = XRCCTRL(*this, "license_txt", wxTextCtrl);
    strFile = huginApp::Get()->GetXRCPath() + wxT("data/COPYING");
#ifndef _WINDOWS
	textCtrl->SetFont(font);
#endif
	textCtrl->LoadFile(strFile);

	// Upstream
	textCtrl = XRCCTRL(*this, "upstream_txt", wxTextCtrl);
    strFile = huginApp::Get()->GetXRCPath() + wxT("data/upstream.txt");
#ifndef _WINDOWS
	textCtrl->SetFont(font);
#endif
	textCtrl->LoadFile(strFile);
    GetSystemInformation(&font);

    // the notebook
    m_about_notebook = XRCCTRL(*this,"about_dlg",wxNotebook);
    // the logo
    m_logoImgCtrl = XRCCTRL(*this, "about_logo", wxStaticBitmap);

    // load the appropriate icon (.ico for Windows, .png for other systems)
#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    // set the icon in the title bar
    SetIcon(myIcon);

    // set the position and the size (x,y,width,height). -1 = keep existing
    SetSize(1,1,560,560);
    CenterOnParent();
}

void AboutDialog::GetSystemInformation(wxFont *font)
{
    wxTextCtrl* infoText=XRCCTRL(*this,"system_txt",wxTextCtrl);
#ifndef _WINDOWS
    infoText->SetFont(*font);
#endif
    wxString text;
    text=wxString::Format(_("Operating System: %s"),wxGetOsDescription().c_str());
    wxString is64;
    if(wxIsPlatform64Bit())
        is64=_("64 bit");
    else
        is64=_("32 bit");
    text=text+wxT("\n")+wxString::Format(_("Architecture: %s"),is64.c_str());
    // wxGetFreeMemory returns a wxMemorySize, which is undocumented.
    // However, we know -1 is returned on failure, so it must be signed.
    text=text+wxT("\n")+wxString::Format(_("Free memory: %ld kiB"),(long long) wxGetFreeMemory().GetValue()/1024);
#ifdef _WINDOWS
    UINT cp=GetACP();
    text=text+wxT("\n")+wxString::Format(_("Active Codepage: %u"),cp); 
    switch(cp)
    {
    case 1250:
        text=text+wxT(" (Central European Windows)");
        break;
    case 1251:
        text=text+wxT(" (Cyrillic Windows)");
        break;
    case 1252:
        text=text+wxT(" (Western European Windows)");
        break;
    case 1253:
        text=text+wxT(" (Greek Windows)");
        break;
    case 1254:
        text=text+wxT(" (Turkish Windows)");
        break;
    case 1255:
        text=text+wxT(" (Hebrew Windows)");
        break;
    case 1256:
        text=text+wxT(" (Arabic Windows)");
        break;
    case 1257:
        text=text+wxT(" (Baltic Windows)");
        break;
    case 1258:
        text=text+wxT(" (Vietnamese Windows)");
        break;
    };
#endif
    text=text+wxT("\n\nHugin\n")+wxString::Format(_("Version: %s"),wxString(DISPLAY_VERSION,wxConvLocal).c_str());
    text=text+wxT("\n")+wxString::Format(_("Path to resources: %s"),huginApp::Get()->GetXRCPath().c_str());
    text=text+wxT("\n")+wxString::Format(_("Path to data: %s"),huginApp::Get()->GetDataPath().c_str());
    HuginBase::LensDB::LensDB& lensDB=HuginBase::LensDB::LensDB::GetSingleton();
    if(!lensDB.GetMainDBPath().empty())
    {
        text=text+wxT("\n")+wxString::Format(_("Path to public lensfun database: %s"),wxString(lensDB.GetMainDBPath().c_str(), wxConvLocal).c_str());
    };
    text=text+wxT("\n")+wxString::Format(_("Path to user lensfun database: %s"),wxString(lensDB.GetUserDBPath().c_str(), wxConvLocal).c_str());
    text=text+wxT("\n\n")+_("Libraries");
    text=text+wxT("\n")+wxString::Format(wxT("wxWidgets: %i.%i.%i.%i"),
                                            wxMAJOR_VERSION,
                                            wxMINOR_VERSION,
                                            wxRELEASE_NUMBER,
                                            wxSUBRELEASE_NUMBER
                                        );
    text=text+wxT("\nlibpano13: ")+wxT(VERSION);
    text=text+wxT("\n")+wxString::Format(wxT("Boost: %i.%i.%i"),BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100);
    text=text+wxT("\n")+wxString::Format(wxT("Exiv2: %i.%i.%i"),EXIV2_MAJOR_VERSION,EXIV2_MINOR_VERSION,EXIV2_PATCH_VERSION);
#ifdef LF_VERSION_MAJOR
    text=text+wxT("\n")+wxString::Format(wxT("Lensfun: %i.%i.%i.%i"), LF_VERSION_MAJOR, LF_VERSION_MINOR, LF_VERSION_MICRO, LF_VERSION_BUGFIX);
#endif
    infoText->SetValue(text);
}

void AboutDialog::OnChangedTab(wxNotebookEvent &e)
{
    // determine which tab is currently visible
   SetMode(e.GetSelection());
};

void AboutDialog::SetMode(int newMode)
{
    if(m_mode==newMode)
    {
        return;
    }

    switch ( newMode )
    {

        case 0 :
            // about tab
            SetLogo(wxT("splash.png"));
            break;

// dedication tab no longer in use            
//        case 6 :
            // dedication tab
//            SetLogo(wxT("dedication.png"));
//            break;

        default :
            // all other tabs
            SetLogo(wxT("logo.png"));
    }

    m_mode=newMode;
    return;
};

void AboutDialog::SetLogo(wxString newLogoFile)
{
    if(m_logo_file!=newLogoFile)
    {
        if(m_logo.LoadFile(huginApp::Get()->GetXRCPath() +
                        wxT("data/") + newLogoFile,
                        wxBITMAP_TYPE_PNG))
        {
            if(m_logoImgCtrl)
            {
                m_logoImgCtrl->SetBitmap(m_logo);
                m_logo_file=newLogoFile;
            };
        };
    };
};

