// -*- c-basic-offset: 4 -*-
/** @file ImageOrientationPanel.h
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

#ifndef _IMAGE_ORIENTATION_PANEL_H
#define _IMAGE_ORIENTATION_PANEL_H

#include "PT/Transforms.h"
#include "PT/PanoToolsInterface.h"
#include "hugin/ImageProcessing.h"

/** Select image orientation
 *
 */
class ImageOrientationPanel : public wxPanel, public PT::PanoramaObserver
{
public:

    /** ctor.
     */
    ImageOrientationPanel(wxFrame *parent, PT::Panorama * pano);

    /** dtor.
     */
    virtual ~ImageOrientationPanel();

    void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

private:
    void updateDisplay();

    void UpdateYawPitch(wxPoint p);
    void InitRollChange(wxPoint p);
    void UpdateRoll(wxPoint p);

    // update transformation on change
    void updateTransforms();


    // draw the image and guides
    void DrawImage(wxDC & dc);

    /** recalculate panorama to fit the panel */
    void OnResize(wxSizeEvent & e);
    void OnDraw(wxPaintEvent & event);
    void OnMouse(wxMouseEvent & e);

    // rescale the bitmap to fit the window
    void ScaleBitmap();
    /** the model */
    PT::Panorama &pano;

    wxSize m_panoImgSize;

    wxBitmap m_bitmap;

    wxFrame * parentWindow;

    unsigned int m_refImgNr;
    wxSize m_imageSize;
    double m_scaleFactor;
    PT::VariableMap m_vars;

    PTools::Transform m_transform;
    PTools::Transform m_invTransform;

    PT::TRANSFORM::CartToImg m_tCartToImg;
    PT::TRANSFORM::ImgToCart m_tImgToCart;

    int m_offsetX;
    int m_offsetY;
    FDiff2D m_origin;
    double m_roll_start;
    double m_roll_display_start;

    enum EditState { NONE, SELECT_POINT, ROTATE };
    EditState m_state;

    DECLARE_EVENT_TABLE()
};



#endif // _PREVIEWPANEL_H
