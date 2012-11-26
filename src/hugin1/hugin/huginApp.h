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
#include <huginapp/ImageCache.h>
#include <PT/ImageGraph.h>

#include "hugin/MainFrame.h"

// utility functions
bool str2double(wxString s, double & d);
wxString Components2Str(const CPComponents & comp);

/// Store window size and position in configfile/registry
void StoreFramePosition(wxTopLevelWindow * frame, const wxString & basename);
/// Restore window size and position from configfile/registry
void RestoreFramePosition(wxTopLevelWindow * frame, const wxString & basename);

/** Resources Definition
 *
 */

#ifdef _INCLUDE_UI_RESOURCES
  void InitXmlResource();
#endif

#if _WINDOWS && defined Hugin_shared
DECLARE_LOCAL_EVENT_TYPE( EVT_IMAGE_READY, )
#else
DECLARE_EVENT_TYPE( EVT_IMAGE_READY, )
#endif

/** Event for when a requested image finished loading.
 *  Glue for HuginBase::ImageCache. We want to load images in a separate thread,
 *  but to write safe UI code we handle the redraw when processing a wxEvent.
 */
class ImageReadyEvent
    : public wxEvent
{
    public:
        HuginBase::ImageCache::RequestPtr request;
        HuginBase::ImageCache::EntryPtr entry;
        
        
        ImageReadyEvent(HuginBase::ImageCache::RequestPtr request,
                        HuginBase::ImageCache::EntryPtr entry)
            : wxEvent (0, EVT_IMAGE_READY)
            , request(request)
            , entry(entry)
        {
        };
        virtual wxEvent * Clone() const
        {
            return new ImageReadyEvent(request, entry);
        }
};

typedef void (wxEvtHandler::*ImageReadyEventFunction)(ImageReadyEvent&);

#define EVT_IMAGE_READY2(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( EVT_IMAGE_READY, id, -1, \
        (wxObjectEventFunction) (wxEventFunction) \
        wxStaticCastEvent( ImageReadyEventFunction, & fn ), (wxObject *) NULL ),

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
    static MainFrame* getMainFrame();
    
    /// Relay image loaded event when the UI thread is ready to process it.
    void relayImageLoaded(ImageReadyEvent & event);
    
    /// Queue up an image loaded event when an image has just loaded.
    static void imageLoadedAsync(HuginBase::ImageCache::RequestPtr request,
                          HuginBase::ImageCache::EntryPtr entry);

    /** return currently active locale */
    wxLocale & GetLocale()
    {
        return locale;
    }

    wxString GetWorkDir()
    {
        return m_workDir;
    }
    
    /** return the current xrc path */
    const wxString & GetXRCPath()
    {
        return m_xrcPrefix;
    }

    /** return the current data path */
    const wxString & GetDataPath()
    {
        return m_DataDir;
    }

    const wxString & GetUtilsBinDir()
    {
        return m_utilsBinDir;
    }

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

	// folder for xrc (GUI resources)
    wxString m_xrcPrefix;
	// folder for data shared by CLI and GUI to enable separate packaging of CLI tools
	wxString m_DataDir;
	// folder for CLI tools
    wxString m_utilsBinDir;

#ifdef __WXMAC__
    bool m_macInitDone;
    bool m_macOpenFileOnStart;
    wxString m_macFileNameToOpenOnStart;
#endif

    DECLARE_EVENT_TABLE()
};

DECLARE_APP(huginApp)

#endif // _HUGINAPP_H
