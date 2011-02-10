// -*- c-basic-offset: 4 -*-

/** @file HtmlWindow.cpp
 *
 *  @brief Implementation of HTMLWindow class which supports opening external links in default web browser
 *
 *  @author Y. Levy, T. Modes
 *
 */
 
/*  This program is free software; you can redistribute it and/or
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

#include "hugin/HtmlWindow.h"

IMPLEMENT_DYNAMIC_CLASS(HtmlWindow, wxHtmlWindow)

void HtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
    if (link.GetHref().StartsWith(_T("http://")))
        wxLaunchDefaultBrowser(link.GetHref());
    else
        wxHtmlWindow::OnLinkClicked(link);
}

IMPLEMENT_DYNAMIC_CLASS(HtmlWindowXmlHandler, wxHtmlWindowXmlHandler)

wxObject *HtmlWindowXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(control, HtmlWindow)

    control->Create(m_parentAsWindow,
                    GetID(),
                    GetPosition(), GetSize(),
                    GetStyle(wxT("style"), wxHW_SCROLLBAR_AUTO),
                    GetName());

    if (HasParam(wxT("borders")))
    {
        control->SetBorders(GetDimension(wxT("borders")));
    }

    if (HasParam(wxT("url")))
    {
        wxString url = GetParamValue(wxT("url"));
        wxFileSystem& fsys = GetCurFileSystem();

        wxFSFile *f = fsys.OpenFile(url);
        if (f)
        {
            control->LoadPage(f->GetLocation());
            delete f;
        }
        else
            control->LoadPage(url);
    }

    else if (HasParam(wxT("htmlcode")))
    {
        control->SetPage(GetText(wxT("htmlcode")));
    }

    SetupWindow(control);

    return control;
}


bool HtmlWindowXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("HtmlWindow"));
}
