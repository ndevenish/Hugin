// -*- c-basic-offset: 4 -*-

/** @file Events.cpp
 *
 *  @brief small event handling Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de>
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


#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include "hugin/Events.h"

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MyEvtHandler, wxEvtHandler)
    EVT_LIST_ITEM_SELECTED ( XRCID("images_list2_unknown"), 
                             MyEvtHandler::ToLensPanel )
END_EVENT_TABLE()

