// -*- c-basic-offset: 4 -*-
/** @file LensCalApp.h
 *
 *  @author T. Modes
 *
 *  @brief declaration of application class for lens calibrate application
 *
 */

/*
 *  This is free software; you can redistribute it and/or
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

#ifndef LENSCALAPP_H
#define LENSCALAPP_H

#include <wx/wx.h>
#include <huginapp/ImageCache.h>

#include "LensCalFrame.h"

/** Store window size and position in configfile/registry */
void StoreFramePosition(wxTopLevelWindow * frame, const wxString & basename);
/** Restore window size and position from configfile/registry */
void RestoreFramePosition(wxTopLevelWindow * frame, const wxString & basename);

/** The application class for lens_calibrate_gui .
 */
class LensCalApp : public wxApp
{
public:
    virtual bool OnInit();
#if wxUSE_ON_FATAL_EXCEPTION
#if wxCHECK_VERSION(3,1,0)
    virtual void OnFatalException() wxOVERRIDE;
#else
    virtual void OnFatalException();
#endif
#endif

    /** return currently active locale */
    wxLocale & GetLocale()
    {
        return locale;
    }
   /** return the current xrc path */
    const wxString & GetXRCPath()
    {
        return m_xrcPrefix;
    }
    /** returns pointer to main frame */
    LensCalFrame* GetLensCalFrame()
    {
        return m_frame;
    };

private:
    /** locale for internationalisation */
    wxLocale locale;
    wxString m_xrcPrefix;
    LensCalFrame* m_frame;
    DECLARE_EVENT_TABLE()
};

DECLARE_APP(LensCalApp)

#endif // LENSCALAPP_H
