// -*- c-basic-offset: 4 -*-
/** @file ImageOrientationFrame.h
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

#ifndef _IMAGEORIENTATIONFRAME_H
#define _IMAGEORIENTATIONFRAME_H

#include "wx/frame.h"

#include <PT/Panorama.h>

class wxListCtrl;
class MainFrame;
class ImageOrientationPanel;


/** List all control points of this project
 *
 *  useful to jump to a specific point, or see which point are bad
 */
class ImageOrientationFrame : public wxFrame, public PT::PanoramaObserver
{
public:

    /** ctor.
     */
    ImageOrientationFrame(wxWindow * parent, PT::Panorama & pano);

    /** dtor.
     */
    virtual ~ImageOrientationFrame();

private:

    void OnClose(wxCloseEvent& event);

    PT::Panorama & m_pano;

    ImageOrientationPanel * m_orientationPanel;

    // needed for receiving events.
    DECLARE_EVENT_TABLE();
};



#endif // _IMAGEORIENTATIONFRAME_H
