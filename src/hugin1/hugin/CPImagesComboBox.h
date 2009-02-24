// -*- c-basic-offset: 4 -*-
/** @file CPImagesComboBox.h
 *
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

#ifndef _CPIMAGESCOMBOBOX_H
#define _CPIMAGESCOMBOBOX_H

// standard wx include
//#include <config.h>
#include "panoinc.h"
#include "panoinc_WX.h"
#include "wx/odcombo.h"
#include "wx/xrc/xh_odcombo.h"


using namespace std;
using namespace PT;

class CPImagesComboBox : public wxOwnerDrawnComboBox
{
public:
    virtual void OnDrawItem(wxDC& dc,
                            const wxRect& rect,
                            int item,
                            int WXUNUSED(flags)) const;
    void CalcCPDistance(Panorama * pano);
    void SetRefImage(Panorama * pano, unsigned int newRefImg)
    {
        refImage=newRefImg;
        CalcCPDistance(pano);
    };
    const unsigned int GetRefImage()
    {
        return refImage;
    };
protected:
    void OnMouseWheel(wxMouseEvent & e);
    void OnKeyDown(wxKeyEvent & e);
private:
    unsigned int refImage;
    std::vector<double> CPConnection;
    void Init();
    void SelectNext(int step = 1);
    void SelectPrev(int step = 1);
    void NotifyParent();

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(CPImagesComboBox)
};

/** xrc handler */
class CPImagesComboBoxXmlHandler : public wxOwnerDrawnComboBoxXmlHandler
{
    DECLARE_DYNAMIC_CLASS(CPImagesComboBoxXmlHandler)

public:
    CPImagesComboBoxXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif
