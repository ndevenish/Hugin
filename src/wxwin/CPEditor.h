// -*- c-basic-offset: 4 -*-
/** @file CPEditor.h
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

#ifndef _CPEDITOR_H
#define _CPEDITOR_H


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif


#include "CPImageCtrl.h"

/** brief description.
 *
 *  What this does
 */
class CPEditor: public wxPanel
{
public:
    /// @bug: needed by wxwin rtti system. doesn't do anything useful now.
    CPEditor()
        { };
    CPEditor(wxWindow* parent, wxWindowID id = -1,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = 0,
             const wxString& name="CPEditor");

    void OnCPEvent( ptCPEvent& event );

    void OnSize(wxSizeEvent& event);
    
    void setLeftImage(wxImage & img);
    void setRightImage(wxImage & img);
    
//    virtual wxSize DoGetBestSize() const;

private:
    CPImageCtrl * m_leftImg, *m_rightImg;
    wxBoxSizer * cpeSizer;

    DECLARE_CLASS(CPEditor)
    DECLARE_EVENT_TABLE()
};



#endif // _CPEDITOR_H
