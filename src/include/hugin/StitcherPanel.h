// -*- c-basic-offset: 4 -*-
/** @file StitcherPanel.h
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

#ifndef _STITCHERPANEL_H
#define _STITCHERPANEL_H

#include "PT/Panorama.h"

/** base class for a stitcher property page
 *  and associated stitch function.
 */
class StitcherPanel : public wxPanel
{
public:

    StitcherPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr)
        : wxPanel(parent, id, pos, size, style, name)
    { };
    /** dtor.
     */
    virtual ~StitcherPanel() {};

    /** perform stitching */
    virtual bool Stitch(const PT::Panorama & pano,
                        PT::PanoramaOptions opts) = 0;
private:

};



#endif // _STITCHERPANEL_H
