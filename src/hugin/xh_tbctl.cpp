/////////////////////////////////////////////////////////////////////////////
// Name:        xh_tbctl.cpp
// Purpose:     XML resource handler for wxTabCtrl
// Author:      Pablo d'Angelo
// Created:     2003/05/21
// RCS-ID:      $Id$
// Copyright:   (c) 2003 Pablo d'Angelo
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "xh_stbox.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "gui/xrc/xh_tbctl.h"
#include "wx/tabctrl.h"

wxTabCtrlXmlHandler::wxTabCtrlXmlHandler()
: wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *wxTabCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(tab, wxTabCtrl)

    tab->Create(m_parentAsWindow,
                GetID(),
                GetText(wxT("label")),
                GetPosition(), GetSize(),
                GetStyle(),
                GetName());

    SetupWindow(tab);

    return tab;
}

bool wxTabCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxTabCtrl"));
}
