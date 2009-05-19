// -*- c-basic-offset: 4 -*-
/**  @file CPImagesComboBox.h
 *
 *  @brief Definition of CPImagesComboBox and CPImagesComboBoxXmlHandler class
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */

/*  This is free software; you can redistribute it and/or
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

/**  Owner Drawn ComboBox for showing connected images on CP tab */
class CPImagesComboBox : public wxOwnerDrawnComboBox
{
public:
    /** Paint method for drawing text and indication bar of combo box	*/
    virtual void OnDrawItem(wxDC& dc,
                            const wxRect& rect,
                            int item,
                            int WXUNUSED(flags)) const;
    /** Get maximum CP distance for all images pairs containing the reference image	*/
    void CalcCPDistance(Panorama * pano);
    /** Set new reference image	*/
    void SetRefImage(Panorama * pano, unsigned int newRefImg)
    {
        refImage=newRefImg;
        CalcCPDistance(pano);
    };
    /** Returns the reference image number	*/
    const unsigned int GetRefImage()
    {
        return refImage;
    };
protected:
    /** Mouse wheel handler to mimic wxChoice behavior	*/
    void OnMouseWheel(wxMouseEvent & e);
    /** Key handler to mimic wxChoice behavior	*/
    void OnKeyDown(wxKeyEvent & e);
private:
    unsigned int refImage;
    std::vector<double> CPConnection;
    std::vector<unsigned int> CPCount;
    void Init();
    void SelectNext(int step = 1);
    void SelectPrev(int step = 1);
    void NotifyParent();

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(CPImagesComboBox)
};

/** xrc handler for CPImagesComboBox */
class CPImagesComboBoxXmlHandler : public wxOwnerDrawnComboBoxXmlHandler
{
    DECLARE_DYNAMIC_CLASS(CPImagesComboBoxXmlHandler)

public:
    /** Constructor */
    CPImagesComboBoxXmlHandler();
    /** Create CPImagesComboBox from resource */
    virtual wxObject *DoCreateResource();
    /** Internal use to identify right xml handler */
    virtual bool CanHandle(wxXmlNode *node);
};

#endif
