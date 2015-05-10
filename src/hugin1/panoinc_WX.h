// -*- c-basic-offset: 4 -*-

/** @file panoinc_WX.h
 *
 *  @brief include file for the hugin project
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MY_PANO_INC_WX_H
#define MY_PANO_INC_WX_H

//-----------------------------------------------------------------------------
// Standard wxWindows headers
//-----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

// include additional wx stuff..
#include <wx/xrc/xmlres.h>      // XRC XML resouces
#include <wx/config.h>
#include <wx/process.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/image.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/spinctrl.h>
#include <wx/config.h>
#include <wx/splash.h>
#include <wx/helphtml.h>
#include <wx/html/htmlwin.h>
#include <wx/progdlg.h>
#include <wx/dnd.h>
#include <wx/tglbtn.h>
#include <wx/txtstrm.h>
#include <wx/statline.h>
#include <wx/regex.h>
#include <wx/valtext.h>
#include <wx/valgen.h>
#include <wx/radiobox.h>
#include <wx/filesys.h>
#include <wx/fs_zip.h>

#include <wx/numdlg.h>
#include <wx/imaglist.h>

// remove stupid #defines from the evil windows.h
#ifdef __WXMSW__
//#include <wx/msw/winundef.h>
#undef DIFFERENCE
#undef FindWindow
#undef min
#undef max
#endif

#endif

