// -*- c-basic-offset: 4 -*-

/**  @file HtmlWindow.h
 *
 *  @brief Definition of HTMLWindow class which supports opening external links in default web browser
 *
 *  @author Y. Levy, T. Modes
 *
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

#ifndef _HTMLWINDOW_H
#define _HTMLWINDOW_H

// standard wx include
//#include <config.h>
#include "panoinc.h"
#include "panoinc_WX.h"
#include "wx/xrc/xh_html.h"

/** \brief Modified wxHtmlWindow for open external links
 *
 *  widgets, derived from wxHtmlWindows, which opens external links
 *  (starting with http://) in default web browser of the system 
 *  instead in wxHtmlWindows. Internal/local links are opened in the
 *  widget.
 */
class HtmlWindow: public wxHtmlWindow
{
    public:
        void OnLinkClicked(const wxHtmlLinkInfo&);

    private:
        DECLARE_DYNAMIC_CLASS(HtmlWindow)
};

/** xrc handler for HTMLWindow */
class HtmlWindowXmlHandler : public wxHtmlWindowXmlHandler
{
    DECLARE_DYNAMIC_CLASS(HtmlWindowXmlHandler)

public:
    /** creates the object from ressource */
    virtual wxObject *DoCreateResource();
    /** Internal use to identify right xml handler */
    virtual bool CanHandle(wxXmlNode *node);
};

#endif  // _HTMLWINDOW_H
