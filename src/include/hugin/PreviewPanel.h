// -*- c-basic-offset: 4 -*-
/** @file PreviewPanel.h
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

#ifndef _PREVIEWPANEL_H
#define _PREVIEWPANEL_H

#include "wx/frame.h"
#include "wx/dnd.h"

#include "common/math.h"

#include "PT/Panorama.h"

extern "C" {
#include <pano12/panorama.h>
}

class wxImage;

/** A preview panel that renders the pictures using the panotools library
 *
 *  Lets hope this works out fine..
 */
class PreviewPanel : public wxPanel, public PT::PanoramaObserver
{
public:

    /** ctor.
     */
    PreviewPanel(wxWindow *parent, PT::Panorama * pano );

    /** dtor.
     */
    virtual ~PreviewPanel();

    void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    void SetAutoUpdate(bool enabled)
        {
            m_autoPreview = enabled;
            updatePreview();
        }

    // forces an update of all images.
    void ForceUpdate();

    // select which images should be shown.
    void SetDisplayedImages(const PT::UIntSet &images);

private:

    // draw the preview directly onto the canvas
    void DrawPreview(wxDC & dc);

    // remaps the images, called automatically if autopreview is enabled.
    void updatePreview();

    void mapPreviewImage(wxImage & dest, int imgNr);

    /** recalculate panorama to fit the panel */
    void OnResize(wxSizeEvent & e);
    void OnDraw(wxPaintEvent & event);
    void OnMouse(wxMouseEvent & e);
    void OnUpdatePreview(wxCommandEvent & e);
    void DrawOutline(const std::vector<FDiff2D> & points, wxDC & dc, int offX, int offY);

    /** the model */
    PT::Panorama &pano;

    bool m_autoPreview;

    wxSize m_panoImgSize;

    PT::UIntSet m_displayedImages;
    
    // outlines of all images
    std::vector<std::vector<FDiff2D> > m_outlines;

    // a single image, we just show the first picture (layer) for now
    std::vector<wxBitmap *> m_remappedBitmaps;
    PT::UIntSet m_dirtyImgs;

    // panorama options
    PT::PanoramaOptions opts;

    wxWindow * parentWindow;

    DECLARE_EVENT_TABLE()
};



#endif // _PREVIEWPANEL_H
