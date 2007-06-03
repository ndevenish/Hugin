// -*- c-basic-offset: 4 -*-
/** @file CPEditorPanelHandler.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: CPEditorPanelHandler.h 99 2003-05-22 23:47:10Z dangelo $
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

#ifndef _CPEDITORPANELHANDLER_H
#define _CPEDITORPANELHANDLER_H

/** Factory class for CPEditorPanel
 *
 *  Needed by xrc to create our CPEditorPanel class
 *  when it encounters it in the xrc file
 *  This must be registred during application startup.
 *
 *  What this does
 */
class CPEditorPanelHandler : public wxXmlResourceHandler
{
public:
    CPEditorPanelHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};


#endif // _CPEDITORPANELHANDLER_H
