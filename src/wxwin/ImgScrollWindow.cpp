// -*- c-basic-offset: 4 -*-

/** @file ImgScrollWindow.cpp
 *
 *  @brief implementation of ImgScrollWindow Class
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

#include "../include/common/utils.h"
#include "ImgScrollWindow.h"


IMPLEMENT_DYNAMIC_CLASS(ImgScrollWindow, wxScrolledWindow);

BEGIN_EVENT_TABLE(ImgScrollWindow, wxScrolledWindow)
  EVT_PAINT(ImgScrollWindow::OnPaint)
END_EVENT_TABLE()

ImgScrollWindow::ImgScrollWindow()
{
}

ImgScrollWindow::ImgScrollWindow(wxWindow* parent)
  : wxScrolledWindow(parent, -1),
    bitmap(0)
{
  SetVirtualSize(150,150);
  SetScrollRate(10,10);
}

ImgScrollWindow::~ImgScrollWindow()
{
}


void ImgScrollWindow::OnPaint(wxPaintEvent& event)
{
  wxPaintDC dc(this);
  PrepareDC( dc );

  if (bitmap) {
    dc.DrawBitmap(*bitmap,0,0);
  } else {
    dc.SetPen(*wxBLACK_PEN);
    dc.DrawLine(0, 0, 100, 100);
  }
}

void ImgScrollWindow::setImage(const wxImage & img)
{
    DEBUG_TRACE("setting Image image w:" << img.GetWidth()
                << "h: " << img.GetHeight());
    delete bitmap;
    bitmap = new wxBitmap(img);
    // FIXME update size & relayout dialog
    //SetSizeHints(-1,-1,img.GetWidth(), img.GetHeight(),1,1);
    SetVirtualSize(img.GetWidth(), img.GetHeight());

//    SetVirtualSizeHints(bitmap->GetWidth(),bitmap->GetHeight(),bitmap->GetWidth(), bitmap->GetHeight());
//    SetScrollbars(16, 16, bitmap->GetWidth()/16 ,bitmap->GetHeight()/16 );
    Layout();
    Refresh(false);
}
