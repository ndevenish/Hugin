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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PREVIEWPANEL_H
#define _PREVIEWPANEL_H

#include <vector>

#include <base_wx/wxImageCache.h>

#include <vigra_ext/ROIImage.h>

class wxImage;
class PreviewFrame;

/** A preview panel that renders the pictures using the panotools library
 *
 *  Lets hope this works out fine..
 */
class PreviewPanel : public wxPanel, public HuginBase::PanoramaObserver
{
    typedef HuginBase::Nona::RemappedPanoImage<vigra::BRGBImage, vigra::BImage> RemappedImage;
public:

    /** ctor.
     */
    PreviewPanel();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(PreviewFrame *parent, HuginBase::Panorama * pano );

    /** dtor.
     */
    virtual ~PreviewPanel();

    void panoramaChanged(HuginBase::Panorama &pano);
    void panoramaImagesChanged(HuginBase::Panorama &pano, const HuginBase::UIntSet & imgNr);

    void SetAutoUpdate(bool enabled);

    // forces an update of all images.
    void ForceUpdate();

    // select which images should be shown.
//    void SetDisplayedImages(const HuginBase::UIntSet &images);
    
    // blending modes available
    enum BlendMode { BLEND_COPY, BLEND_DIFFERENCE };
    
    void SetBlendMode(BlendMode b);

private:

    // draw the preview directly onto the canvas
    void DrawPreview(wxDC & dc);

    // remaps the images, called automatically if autopreview is enabled.
    void updatePreview();

    void mapPreviewImage(unsigned int imgNr);

    /** recalculate panorama to fit the panel */
    void OnResize(wxSizeEvent & e);
    void OnDraw(wxPaintEvent & event);
    void OnMouse(wxMouseEvent & e);
    void mousePressRMBEvent(wxMouseEvent & e);
    void mousePressLMBEvent(wxMouseEvent & e);

    void OnUpdatePreview(wxCommandEvent & e);
    void DrawOutline(const std::vector<hugin_utils::FDiff2D> & points, wxDC & dc, int offX, int offY);

    void mouse2erect(int xm, int ym, double &xd, double & yd);

    /** the model */
    HuginBase::Panorama  * pano;

    bool m_autoPreview;

    vigra::Diff2D m_panoImgSize;

	wxBitmap * m_panoBitmap;
    // currently updating the preview.

    HuginBase::UIntSet m_dirtyImgs;

    // panorama options
    HuginBase::PanoramaOptions opts;

    // transformation for current preview coordinates into equirect coordinates
    HuginBase::PTools::Transform * m_pano2erect;

    // cache for remapped images
    SmallRemappedImageCache m_remapCache;

    BlendMode m_blendMode;

    PreviewFrame * parentWindow;
    wxCursor * m_cursor;

    bool m_state_rendering;
    bool m_rerender;
    bool m_imgsDirty;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(PreviewPanel)
};

/** xrc handler */
class PreviewPanelXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(PreviewPanelXmlHandler)

    public:
        PreviewPanelXmlHandler();
        virtual wxObject *DoCreateResource();
        virtual bool CanHandle(wxXmlNode *node);
};

#endif // _PREVIEWPANEL_H
