// -*- c-basic-offset: 4 -*-
/** @file TestPanel.h
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

#ifndef _TESTPANEL_H
#define _TESTPANEL_H


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif


/** brief description.
 *
 *  What this does
 */


class TestPanel: public wxPanel
{
public:
  TestPanel()
    { };
  TestPanel(wxWindow* parent, wxWindowID id = -1);
  virtual ~TestPanel();
  
  
  // Call Layout()
  void OnSize(wxSizeEvent& event);
  
  virtual wxSize DoGetBestSize() const
    { return wxSize(100,100);};

private:
  
  wxBoxSizer *m_sizer;

  DECLARE_CLASS(TestPanel)
  DECLARE_EVENT_TABLE()

};

#endif // _TESTPANEL_H
