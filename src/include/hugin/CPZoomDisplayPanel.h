// -*- c-basic-offset: 4 -*-
/** @file CPZoomDisplayPanel.h
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

#ifndef _CPZOOMDISPLAYPANEL_H
#define _CPZOOMDISPLAYPANEL_H

#include <vector>

#include <PT/ImageTransforms.h>
//#include <hugin/ImageCache.h>


/** Display a zoomed control point, undistorted to the middle of the image.
 *
 *  Can be used to display the images, and move the point.
 */
class CPZoomDisplayPanel : public wxPanel, public PT::PanoramaObserver
{
    typedef PT::RemappedPanoImage<vigra::BRGBImage, vigra::BImage> RemappedImage;
public:
    /** ctor.
     */
    CPZoomDisplayPanel(wxWindow *parent, PT::Panorama & pano );

    /** dtor.
     */
    virtual ~CPZoomDisplayPanel();

//    void panoramaChanged(PT::Panorama &pano);
//    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /// set the point of interest, updates the display accordingly
    void SetPoint(const FDiff2D & point)
        {
            m_validPoint = true;
            m_point = point;
            if (m_validImg) {
                updateInternal();
                // update display
                wxPaintDC dc(this);
                OnDraw(dc);
            }
        }

    /// set image number. does not update display, a setPoint() call
    /// is needed for that.
    void SetImage(unsigned int imgNr)
        {
            m_validImg = true;
            m_imgNr = imgNr;
        }

    // do not display anything
    void Clear()
        {
            m_validImg = false;
            m_validPoint = false;
            // todo: draw blank space. (erase old image)
        }

    // redraw
    void OnRedraw(wxPaintEvent & e);
    void OnSize(wxSizeEvent & e);
protected:

    // update the bitmap, and the transforms.
    void updateInternal();

    void OnDraw(wxDC & dc);
    // on left mousbutton
    void OnLMB(wxMouseEvent & e);

    bool m_validPoint;
    bool m_validImg;

    PT::Panorama & m_pano;

    // the remapped bitmap
    wxBitmap m_bitmap;

    // the other stuff

    // the point
    FDiff2D m_point;
    unsigned int m_imgNr;

    // transforms used for visualisation
    PTools::Transform m_t_img2center;
    PTools::Transform m_t_center2img;

    DECLARE_EVENT_TABLE();
};



#endif // _CPZOOMDISPLAYPANEL_H
