/////////////////////////////////////////////////////////////////////////////
// Name:        xh_tbctl.h
// Purpose:     XML resource handler for wxTabCtrl
// Author:      Pablo d'Angelo
// Created:     2003/05/21
// RCS-ID:      $Id$
// Copyright:   (c) 2003 Pablo d'Angelo
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_STBOX_H_
#define _WX_XH_STBOX_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "xh_stbox.h"
#endif

#include "wx/xrc/xmlres.h"


class WXXMLDLLEXPORT wxTabCtrlXmlHandler : public wxXmlResourceHandler
{
public:
    wxTabCtrlXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};


#endif // _WX_XH_STBOX_H_
