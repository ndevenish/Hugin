// -*- c-basic-offset: 4 -*-

/** @file OptimizePanel.cpp
 *
 *  @brief implementation of OptimizePanel
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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
#include "panoinc.h"
#include "panoinc_WX.h"
#include "PTWXDlg.h"
#include <wx/app.h>

extern "C" {
#include <pano13/filter.h>
#include <pano13/queryfeature.h>
}

// Error reporting

wxWindow* appParent = NULL;

static void PTPrintErrorWX(char* fmt, va_list ap)
{
    char message[257];

    vsprintf(message, fmt, ap);
	
//		MessageBox(GetFocus(), (LPSTR)message, (LPSTR)"", MB_OK | MB_ICONHAND) ;
    wxMessageBox(wxString(message,wxConvLocal), _("Panorama Tools"), wxOK | wxICON_HAND,appParent);
}	


// Progress report; return false if canceled


static int PTProgressWX( int command, char* argument ){

    static wxProgressDialog * dlg = 0;
//    MSG	msg;
    long percent;	
    switch( command ){
        case _initProgress:
            if (dlg) {
                dlg->Destroy();
                wxTheApp->Yield();
                dlg = 0;
            } else {
                dlg = new wxProgressDialog(_("Panorama Tools"),
                                           wxT("\n\n\n"), 100, NULL,
                                           wxPD_APP_MODAL | wxPD_CAN_ABORT);
                if (dlg == 0) {
                    return FALSE;
                }
                dlg->Update(0, wxString(argument, wxConvLocal));
            }
            return TRUE;
        case _setProgress:
            if (dlg) {
                sscanf(argument,"%ld", &percent);
                if(percent>100) percent = 100;
                if(percent<0  ) percent = 0;
                if (! dlg->Update(percent)) {
                    return FALSE;
                }
            }
            return TRUE;
            break;
        case _disposeProgress:
            if( dlg != 0 )
            {
                dlg->Destroy();
                wxTheApp->Yield();
                dlg=0;
            }

            return TRUE;

        case _idleProgress:
            return TRUE;

    }
    return TRUE;
}


static int PTInfoDlgWX ( int command, char* argument )	// Display info: same argumenmts as progress
{
    char 				text[256];
    static char			mainMessage[256];						

    static wxProgressDialog * dlg = 0;
//    MSG	msg;
    switch( command ){
        case _initProgress:
            if (dlg) {
                dlg->Destroy();
                wxTheApp->Yield();
                dlg = 0;
            } else {
                // we need to ensure that there is are enough lines in the dialog..
                // create progress dialog
                dlg = new wxProgressDialog(_("Panorama Tools"),
#ifdef __WXMAC__
                                           wxT("0123456789012345678901234567890123456789012345\n\n\n\n\n"),
#else
                                           wxT("0123456789012345678901234567890123456789012345\n\n\n"),
#endif
                                           100, NULL,
                                           wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME);
                if (dlg == 0) {
                    return FALSE;
                }
#if wxCHECK_VERSION(2,8,0)
                dlg->Pulse(wxString(argument, wxConvLocal));
#else
                dlg->Update(0, wxString(argument, wxConvLocal));
#endif
            }
            return TRUE;
        case _setProgress:
            if (dlg) {
                if( *argument != 0 )
                {
                    bool cont;

                    if( *argument != '+' )
                    {
                        strcpy( mainMessage, argument );
                        strcpy( text, argument );
                    }
                    else
                    {
                        sprintf( text,"%s%s", mainMessage, &(argument[1]) );
                    }
#if wxCHECK_VERSION(2,8,0)
                    cont = dlg->Pulse(wxString(argument, wxConvLocal));
#else
                    cont = dlg->Update(1, wxString(argument, wxConvLocal));
#endif
                    if (! cont) {
                        return FALSE;
                    }
                }
            }
            return TRUE;
            break;
        case _disposeProgress:
            if( dlg != 0 )
            {
                dlg->Destroy();
                wxTheApp->Yield();
                dlg=0;
            }

            return TRUE;

        case _idleProgress:
            return TRUE;
    }
    return TRUE;
}

void registerPTWXDlgFcn(wxWindow* parent)
{
    appParent = parent;
    PT_setProgressFcn(&PTProgressWX);
    PT_setErrorFcn(&PTPrintErrorWX);
    PT_setInfoDlgFcn(&PTInfoDlgWX);
};

void deregisterPTWXDlgFcn()
{
    PT_setProgressFcn(NULL);
    PT_setErrorFcn(NULL);
    PT_setInfoDlgFcn(NULL);
}

