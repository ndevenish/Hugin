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
#include <wx/app.h>

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

    /** create a default config.
     *
     *  Might be useful to initialize the .huginrc
     *  On the other hand, huginrc is installation
     *  specific so we cant create the entries for locale and xrc
     *  pathes here
     */
    bool createInitialConfig();
    
private:
    /** locale for internationalication
     */
    wxLocale locale;

};

DECLARE_APP(huginApp)

#endif // _HUGINAPP_H
