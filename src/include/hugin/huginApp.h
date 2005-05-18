// -*- c-basic-offset: 4 -*-
/** @file huginApp.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _HUGINAPP_H
#define _HUGINAPP_H

#include "config.h"

#include "hugin/MainFrame.h"

#ifdef __WXMAC__
#include <CFBundle.h>
#include "wx/mac/private.h"
#endif

// utility functions
bool str2double(wxString s, double & d);
#ifdef __WXMAC__
wxString MacGetPathTOBundledResourceFile(CFStringRef filename);
wxString MacGetPathTOBundledExecutableFile(CFStringRef filename);
#endif

/** Resources Definition
 *
 */

#ifdef _INCLUDE_UI_RESOURCES
  void InitXmlResource();
#endif

/** The application class for hugin.
 *
 *  it contains the main frame.
 */
class huginApp : public wxApp
{
public:

    /** ctor.
     */
    huginApp();

    /** dtor.
     */
    virtual ~huginApp();

    /** pseudo constructor. with the ability to fail gracefully.
     */
    virtual bool OnInit();

    /** just for testing purposes */
    virtual int OnExit();

    /// hack.. kind of a pseudo singleton...
    static huginApp * Get();

    /** return currently active locale */
    wxLocale & GetLocale()
    {
        return locale;
    }
    
    wxString GetWorkDir()
    {
        return m_workDir;
    }

    /** create a default config.
     *
     *  Might be useful to initialize the .huginrc
     *  On the other hand, huginrc is installation
     *  specific so we cant create the entries for locale and xrc
     *  pathes here
     */
    bool createInitialConfig();
    
#ifdef __WXMAC__
    //Defined in wxApp.h; This one lets project file to be opened from Finder and other applications.
    void MacOpenFile(const wxString &fileName);
#endif
    
private:

    
    /** locale for internationalisation */
    wxLocale locale;

    /** temporary working directory */
    wxString m_workDir;

    MainFrame * frame;

    // self
    static huginApp * m_this;

    // the model
    Panorama pano;

#ifdef __WXMAC__
    bool m_macInitDone;
    bool m_macOpenFileOnStart;
    wxString m_macFileNameToOpenOnStart;
#endif
    
};

DECLARE_APP(huginApp)

#endif // _HUGINAPP_H
