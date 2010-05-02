// -*- c-basic-offset: 4 -*-

/** @file CPImagesComboBox.cpp
 *
 *  @brief Implementation of CPImagesComboBox and CPImagesComboBoxXmlHandler class
 *
 *  @author Thomas Modes
 *
 *  $Id$
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

#include "hugin/CPImagesComboBox.h"

BEGIN_EVENT_TABLE(CPImagesComboBox,wxOwnerDrawnComboBox)
    EVT_MOUSEWHEEL(CPImagesComboBox::OnMouseWheel)
    EVT_KEY_DOWN(CPImagesComboBox::OnKeyDown)
END_EVENT_TABLE()

void CPImagesComboBox::OnMouseWheel(wxMouseEvent & e)
{
    //mimic wxChoice behaviour
    //when popup is shown, there could be a scrollbar which processes the mouse wheel event
    if(!IsPopupShown())
    {
        if(e.GetWheelRotation()<0)
            SelectNext();
        else
            SelectPrev();
    }
    else
    {
        e.Skip();
    };
};

void CPImagesComboBox::OnKeyDown(wxKeyEvent & e)
{
    if(!IsPopupShown())
    {
        //mimic wxChoice behaviour
        switch (e.GetKeyCode()){
            case WXK_DOWN:
            case WXK_RIGHT:
            case WXK_NUMPAD_DOWN:
            case WXK_NUMPAD_RIGHT:
                SelectNext();
                break;
            case WXK_UP:
            case WXK_LEFT:
            case WXK_NUMPAD_UP:
            case WXK_NUMPAD_LEFT:
                SelectPrev();
                break;
            case WXK_PAGEDOWN:
            case WXK_NUMPAD_PAGEDOWN:
                SelectNext(5);
                break;
            case WXK_PAGEUP:
            case WXK_NUMPAD_PAGEUP:
                SelectPrev(5);
                break;
            case WXK_HOME:
            case WXK_NUMPAD_HOME:
                SelectPrev(GetCount());
                break;
            case WXK_END:
            case WXK_NUMPAD_END:
                SelectNext(GetCount());
                break;
            case WXK_SPACE:
                ShowPopup();
                break;
            default:
                e.Skip();
        };
    }
    else
    {
        e.Skip();
    }
};

void CPImagesComboBox::SelectNext(int step)
{
    int index=min<int>(GetSelection()+step,GetCount()-1);
    if(index!=GetSelection())
    {
        Select(index);
        Update();
        NotifyParent();
    };
};

void CPImagesComboBox::SelectPrev(int step)
{
    int index=max<int>(GetSelection()-step,0);
    if(index!=GetSelection())
    {
        Select(index);
        Update();
        NotifyParent();
    };
};

void CPImagesComboBox::NotifyParent()
{
    //notify parent about changed selection
    //select doesn't send the corresponding event
    wxCommandEvent ne(wxEVT_COMMAND_COMBOBOX_SELECTED);
    ne.SetEventObject(this);
    ne.SetId(this->GetId());
    ne.SetInt(GetSelection());
    ProcessEvent(ne);
};

void CPImagesComboBox::Init()
{
    CPConnection.resize(0);
    refImage=0;
};

void CPImagesComboBox::OnDrawItem(wxDC& dc,
                                  const wxRect& rect,
                                  int item,
                                  int WXUNUSED(flags)) const
{
    if ( item == wxNOT_FOUND )
       return;

    wxCoord w, h;
    GetTextExtent(GetString(item), &w, &h);
    wxCoord maxWidth=0.73*rect.width-3;

    // TODO: note that since wxWidgets 2.9.0 you should not use wxT anymore <http://docs.wxwidgets.org/trunk/group__group__funcmacro__string.html#g437ea6ba615b75dac8603e96ec864160>

    // if image connected by control points, add number of CPs to width equation as well
    wxCoord qty_w = 0;
    wxString qty_cp = wxT("");
    if(CPConnection[item]>-1.0)
    {
        qty_cp = wxString::Format(wxT(" %d"), CPCount[item]);
        GetTextExtent(qty_cp, &qty_w, &h);
    }

    // determine if the string can fit inside the current combo box
    if (w +qty_w <= maxWidth)
    {
        // it can, draw it 
        dc.DrawText(GetString(item),rect.x + 3,rect.y + ((rect.height - dc.GetCharHeight())/2));

    }
    else // otherwise, truncate and add an ellipsis
    {
        // determine the base width
        wxString ellipsis(wxT("..."));
        wxCoord base_w;
        GetTextExtent(ellipsis, &base_w, &h);

        // continue until we have enough space or only one character left
        wxString drawntext = GetString(item);
        while (drawntext.length() > 1)
        {
            drawntext.RemoveLast();
            GetTextExtent(drawntext,&w,&h);
            if (w + base_w + qty_w <= maxWidth)
                break;
        }

        // now draw the text
        dc.DrawText(drawntext, rect.x + 3, rect.y + ((rect.height - dc.GetCharHeight())/2));
        dc.DrawText(ellipsis, rect.x + 3 + w, rect.y + ((rect.height - dc.GetCharHeight())/2));
    }

    // draw rectangle when images are connected by control points
    if(CPConnection[item]>-1.0)
    {
        wxCoord x;
        x=rect.width / 5 *(1-min<double>(CPConnection[item],10)/10);
        //ensure that always a bar is drawn
        x=max<wxCoord>(5,x);
        const wxPen * oldPen = & dc.GetPen();
        const wxBrush * oldBrush= & dc.GetBrush();
        //inner rectangle with color proportional to max cp error (max. 10)
        wxPen MyPen(wxColour(255,0,0),1,wxSOLID);
        wxBrush MyBrush(wxColour(255,0,0),wxSOLID);
        double red, green, blue;

        hugin_utils::ControlPointErrorColour(CPConnection[item],red,green,blue);

        //Scale colour to 0-255
        red *= 255;
        green *= 255;

        MyPen.SetColour(wxColour(red,green,0));
        MyBrush.SetColour(wxColour(red,green,0));
        dc.SetPen(MyPen);
        dc.SetBrush(MyBrush);
        dc.DrawRectangle(rect.x+0.75*rect.width,rect.y+rect.height/6+1,x,2*rect.height/3);
/*
        // half the rectangle
        int half=rect.width/10;
        // color steps
        double step_red=255.0/half;
        double step_green=192.0/half;
        // starting color
        double red=255.0;
        double green=0.0;
        for(int i=0;i<x;i++)
        {
            MyPen.SetColour(wxColour(red,green,0));
            MyBrush.SetColour(wxColour(red,green,0));
            dc.SetPen(MyPen);
            dc.SetBrush(MyBrush);
            dc.DrawRectangle(rect.x+0.75*rect.width+i,rect.y+rect.height/6+1,1,2*rect.height/3);
            // gradient calculation
            if(i<half)
            {
                // until half-way increase the green
                green=green+step_green;
            }
            else
            {
                // after half-way decrease the red
                red=red-step_red;
            }
        }
*/
        //outer rectangle, same colour as text
        MyPen.SetColour(dc.GetTextForeground());
        dc.SetPen(MyPen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect.x+0.75*rect.width,rect.y+rect.height/6+1,rect.width/5,2*rect.height/3);
        dc.SetPen(*oldPen);
        dc.SetBrush(*oldBrush);

        // draw number of connecting CPs
        dc.DrawText(qty_cp, rect.x - 3 - qty_w +0.75*rect.width , rect.y + ((rect.height - dc.GetCharHeight())/2));

    };
};

void CPImagesComboBox::CalcCPDistance(Panorama * pano)
{
    CPConnection.clear();
    CPCount.clear();
    CPConnection.resize(this->GetCount(),-1.0);
    CPCount.resize(this->GetCount(),0);
    unsigned int noPts = pano->getNrOfCtrlPoints();
    // loop over all points to get the maximum error and to count the number of CPs
    for (unsigned int ptIdx = 0 ; ptIdx < noPts ; ptIdx++)
    {
        const ControlPoint & cp = pano->getCtrlPoint(ptIdx);
        if(cp.image1Nr==refImage)
        {
            CPConnection[cp.image2Nr]=max<double>(cp.error,CPConnection[cp.image2Nr]);
            CPCount[cp.image2Nr]++;
        }
        else if(cp.image2Nr==refImage)
        {
            CPConnection[cp.image1Nr]=max<double>(cp.error,CPConnection[cp.image1Nr]);
            CPCount[cp.image1Nr]++;
        };
    }
};

IMPLEMENT_DYNAMIC_CLASS(CPImagesComboBox, wxOwnerDrawnComboBox)

IMPLEMENT_DYNAMIC_CLASS(CPImagesComboBoxXmlHandler, wxOwnerDrawnComboBoxXmlHandler)

CPImagesComboBoxXmlHandler::CPImagesComboBoxXmlHandler()
                : wxOwnerDrawnComboBoxXmlHandler()
{
    AddWindowStyles();
}

wxObject *CPImagesComboBoxXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, CPImagesComboBox)

    cp->Create(m_parentAsWindow,
                   GetID(), wxEmptyString,
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")), wxDefaultValidator,
                   GetName());

    SetupWindow(cp);

    return cp;
}

bool CPImagesComboBoxXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("CPImagesComboBox"));
}
