// -*- c-basic-offset: 4 -*-

/** @file ImageOrientationPanel.cpp
 *
 *  @brief implementation of ImageOrientationPanel Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
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


#include "panoinc.h"
#include "panoinc_WX.h"
#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>	// needed on mingw
#include <wx/imaglist.h>
#include <wx/spinctrl.h>
#include <wx/config.h>

#include "hugin/ImageCache.h"
#include "hugin/ImageProcessing.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageOrientationPanel.h"

using namespace PT;
using namespace PT::TRANSFORM;
using namespace std;
using namespace PTools;
using namespace vigra;

BEGIN_EVENT_TABLE(ImageOrientationPanel, wxPanel)
    EVT_SIZE(ImageOrientationPanel::OnResize)
    EVT_MOUSE_EVENTS ( ImageOrientationPanel::OnMouse )
    EVT_PAINT ( ImageOrientationPanel::OnDraw )
END_EVENT_TABLE()

ImageOrientationPanel::ImageOrientationPanel(wxFrame *parent, Panorama * pano)
    : wxPanel (parent, -1, wxDefaultPosition,
               wxSize(256,128), wxEXPAND),
    pano(*pano), parentWindow(parent),m_refImgNr(32000),
    m_scaleFactor(1), m_tCartToImg(0,0), m_tImgToCart(0,0), m_state(NONE)
{

    panoramaChanged(*pano);
    pano->addObserver(this);

    DEBUG_TRACE("");
}


ImageOrientationPanel::~ImageOrientationPanel()
{
    DEBUG_TRACE("dtor");
    pano.removeObserver(this);
    DEBUG_TRACE("dtor end");
}


void ImageOrientationPanel::panoramaChanged(Panorama &pano)
{
    const PanoramaOptions & newOpts = pano.getOptions();

    // check if reference image has changed
    if (newOpts.optimizeReferenceImage != m_refImgNr) {
        m_refImgNr = newOpts.optimizeReferenceImage;
        if (pano.getNrOfImages() <= m_refImgNr) {
            return;
        }

        m_vars = pano.getImageVariables(m_refImgNr);
        updateTransforms();

        // rescale to fit window
        ScaleBitmap();
        // update display
        updateDisplay();
    }
}

void ImageOrientationPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgs)
{
    if (set_contains(imgs,m_refImgNr)) {
        m_vars = pano.getImageVariables(m_refImgNr);
        updateTransforms();
        updateDisplay();
    }
}

void ImageOrientationPanel::updateTransforms()
{
    // hmmm.. to much data conversion is going on here...
    const PanoImage & img = pano.getImage(m_refImgNr);
    // create suitable transform, pano -> image
    m_transform.createTransform(Diff2D(img.getWidth(), img.getHeight()),
                                m_vars, pano.getLens(img.getLensNr()).projectionFormat,
                                Diff2D(360,180), PanoramaOptions::EQUIRECTANGULAR,
                                360);
    VariableMap nullvars = m_vars;
    double roll = map_get(nullvars,"r").getValue();
    map_get(nullvars,"r").setValue(-roll);
    map_get(nullvars,"p").setValue(0);
    map_get(nullvars,"y").setValue(0);
    // create transform image -> pano
    m_invTransform.createInvTransform(Diff2D(img.getWidth(), img.getHeight()),
                                      nullvars, pano.getLens(img.getLensNr()).projectionFormat,
                                      Diff2D(360,180), PanoramaOptions::EQUIRECTANGULAR,
                                      360);

    // update image transforms
    m_tCartToImg = CartToImg(img.getWidth(), img.getHeight());
    m_tImgToCart = ImgToCart(img.getWidth(), img.getHeight());
}

void ImageOrientationPanel::updateDisplay()
{
    DEBUG_TRACE("");

    wxClientDC dc(this);
    DrawImage(dc);

    parentWindow->SetStatusText(wxString::Format(_("Yaw: %.1f Pitch:%.1f Roll:%.1f"),
                                                 map_get(m_vars,"y").getValue(),
                                                 map_get(m_vars,"p").getValue(),
                                                 map_get(m_vars,"r").getValue()),
                                0);
}


void ImageOrientationPanel::DrawImage(wxDC & dc)
{
    if (!IsShown()){
        return;
    }
    DEBUG_TRACE("");


    // plot image & lines
    if (m_bitmap.Ok() && pano.getNrOfImages() > m_refImgNr) {

        m_offsetX = m_offsetY = 0;
        wxSize sz = GetClientSize();
        if (sz.GetWidth() > m_bitmap.GetWidth()) {
            m_offsetX = (sz.GetWidth() - m_bitmap.GetWidth()) / 2;
        }
        if (sz.GetHeight() > m_bitmap.GetHeight()) {
            m_offsetY = (sz.GetHeight() - m_bitmap.GetHeight()) / 2;
        }

        dc.DestroyClippingRegion();
        dc.SetClippingRegion(m_offsetX, m_offsetY,
                             m_bitmap.GetWidth(), m_bitmap.GetHeight());

        DEBUG_DEBUG("m_offset: " << m_offsetX << "," << m_offsetY);

        dc.DrawBitmap(m_bitmap,m_offsetX,m_offsetY,false);


        // calculate origin.
        m_transform.transform(m_origin,FDiff2D(0,0));
        m_tCartToImg(m_origin,m_origin);
        DEBUG_DEBUG("origin: 0,0 -> " << m_origin.x << ", " << m_origin.y << " pixel");

        double maxh=0;
        double maxv=0;
        const PanoImage & img = pano.getImage(m_refImgNr);
        switch (pano.getLens(img.getLensNr()).projectionFormat) {
        case Lens::RECTILINEAR:
            maxh = 85;
            maxv = 85;
            break;
        case Lens::PANORAMIC:
            maxh = 180;
            maxv = 85;
            break;
        case Lens::CIRCULAR_FISHEYE:
        case Lens::FULL_FRAME_FISHEYE:
            maxh = 180;
            maxv = 180;
            break;
        case Lens::EQUIRECTANGULAR_LENS:
            maxh = 180;
            maxv = 90;
            break;
        }

        // draw horizontal "line"
        dc.SetPen(wxPen("WHITE", 2, wxSOLID));
        dc.SetLogicalFunction(wxINVERT);

        const int nSteps = 20;

        double yaw = map_get(m_vars,"y").getValue();
        double pitch = map_get(m_vars,"p").getValue();
        double hfov = map_get(m_vars,"v").getValue();

        double byaw =  yaw - hfov/2;
        double stepwidth = hfov/nSteps;
        DEBUG_DEBUG("begin yaw: " << byaw << " end yaw" << byaw + nSteps*stepwidth);
        FDiff2D old_pos(byaw, 0);
        m_transform.transform(old_pos,old_pos);
        m_tCartToImg(old_pos,old_pos);
        for (double y = yaw - maxh; y <yaw+maxh; y += stepwidth) {
            FDiff2D pos(y, 0);
//            DEBUG_DEBUG("eqrect point "<< i << ":" << pos.x << ", " << pos.y);
            m_transform.transform(pos,pos);
            m_tCartToImg(pos,pos);
//            DEBUG_DEBUG("line point "<< i << ":" << pos.x << ", " << pos.y);
            if (fabs(pos.x) < 32000 && fabs(pos.y) < 32000 &&
                fabs(old_pos.x) < 32000 && fabs(pos.y) < 32000) {
                dc.DrawLine((int) round(m_offsetX + (old_pos.x * m_scaleFactor)),
                            (int) round(m_offsetY + (old_pos.y * m_scaleFactor)),
                            (int) round(m_offsetX + (pos.x * m_scaleFactor)),
                            (int) round(m_offsetY + (pos.y * m_scaleFactor)));
            } else {
                DEBUG_DEBUG("discarting point, too far outside");
            }
            old_pos = pos;
        }

        // draw vertical "line"
        // FIXME, calculate vfov here...
        double bpitch =  pitch - hfov/2.0;
        stepwidth = hfov/nSteps;
        DEBUG_DEBUG("begin pitch: " << bpitch << " end pitch" << bpitch + nSteps*stepwidth);

        dc.SetPen(wxPen("RED", 2, wxSOLID));
        dc.SetLogicalFunction(wxXOR);
        m_transform.transform(old_pos,FDiff2D(0, bpitch));
        m_tCartToImg(old_pos,old_pos);
        for (int i=1; i <nSteps; i++) {
            FDiff2D pos(0,bpitch + i*stepwidth);
            m_transform.transform(pos,pos);
            m_tCartToImg(pos,pos);
//            DEBUG_DEBUG("line point "<< i << ":" << pos.x << ", " << pos.y);
            dc.DrawLine((int) round(m_offsetX + (old_pos.x * m_scaleFactor)),
                        (int) round(m_offsetY + (old_pos.y * m_scaleFactor)),
                        (int) round(m_offsetX + (pos.x * m_scaleFactor)),
                        (int) round(m_offsetY + (pos.y * m_scaleFactor)));
            old_pos = pos;
        }
    }
}

void ImageOrientationPanel::OnDraw(wxPaintEvent & event)
{
    wxPaintDC dc(this);
    DrawImage(dc);
}

void ImageOrientationPanel::OnResize(wxSizeEvent & e)
{
    DEBUG_TRACE("");
    ScaleBitmap();
    Clear();
}

void ImageOrientationPanel::ScaleBitmap()
{
    if (pano.getNrOfImages() <= m_refImgNr) {
        // nothing to scale
        return;
    }
    const string & fname = pano.getImage(m_refImgNr).getFilename();
    wxImage * img = ImageCache::getInstance().getImage(fname);

    m_imageSize = wxSize(img->GetWidth(), img->GetHeight());

    // calculate scale factor
    wxSize csize = GetClientSize();
    double s1 = (double)csize.GetWidth()/m_imageSize.GetWidth();
    double s2 = (double)csize.GetHeight()/m_imageSize.GetHeight();
    m_scaleFactor = s1 < s2 ? s1 : s2;

    m_bitmap = img->Scale((int) floor(m_scaleFactor * m_imageSize.GetWidth()),
                          (int) floor(m_scaleFactor * m_imageSize.GetHeight())).ConvertToBitmap();
}

/*
void ImageOrientationPanel::OnLMBDown(wxMouseEvent & e)
{
    // check for click on horizontal line.

}
*/

void ImageOrientationPanel::OnMouse(wxMouseEvent & e)
{
    wxPoint p = e.GetPosition();
    p.x -= m_offsetX;
    p.y -= m_offsetY;

    if (e.RightDown()) {
        InitRollChange(p);
    } else if (e.LeftIsDown()) {
        UpdateYawPitch(p);
    } else if (e.RightIsDown()) {
        // update rotation
        UpdateRoll(p);
    }
    if (e.LeftUp() || e.RightUp()) {
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateImageVariablesCmd(pano, m_refImgNr, m_vars)
            );
    }

}

void ImageOrientationPanel::InitRollChange(wxPoint p)
{
    DEBUG_TRACE("");
    FDiff2D diff( p.x / m_scaleFactor, p.y / m_scaleFactor);
    m_tImgToCart(diff,diff);
//    diff = diff - FDiff2D(m_imageSize.GetWidth()/1, m_imageSize.GetHeight()/2);
    m_roll_display_start = atan2(diff.y, diff.x);
    m_roll_start = map_get(m_vars,"r").getValue();
}

void ImageOrientationPanel::UpdateRoll(wxPoint p)
{
    FDiff2D diff( p.x / m_scaleFactor, p.y / m_scaleFactor);

    m_tImgToCart(diff,diff);
//    diff = diff - FDiff2D(m_imageSize.GetWidth()/1, m_imageSize.GetHeight()/2);
    double angle = atan2(diff.y, diff.x);

    double alpha = m_roll_display_start - angle;
    DEBUG_TRACE("abs angle: " << RAD_TO_DEG(angle) << " diff: " << RAD_TO_DEG(alpha));
    double roll = m_roll_start + RAD_TO_DEG(alpha);
    map_get(m_vars,"r").setValue(roll);

    DEBUG_TRACE("diff: " << diff.x << "," << diff.y << " -> roll: " << roll);
    updateTransforms();
    updateDisplay();

}


void ImageOrientationPanel::UpdateYawPitch(wxPoint p)
{
    double x = p.x / m_scaleFactor;
    double y = p.y / m_scaleFactor;

    FDiff2D origin(x,y);
    m_tImgToCart(origin,origin);
    // why -x ???
    origin.x = - origin.x;

    DEBUG_DEBUG("click coordinates: " << origin.x << "," << origin.y);
    // transform into orientation
    m_invTransform.transform(origin,origin);

    map_get(m_vars,"y").setValue(origin.x);
    map_get(m_vars,"p").setValue(origin.y);
    map_get(m_vars,"r").setValue(0);


#if 0
    // fixme.. should use the real image center (a transform stack
    // based on the image should be created here.)
    ImgToRect img2r(m_imageSize.GetWidth(),m_imageSize.GetHeight());
    RectToERect r2er(calcRectFocalLength(m_hfov, m_imageSize.GetWidth()));

    DEBUG_DEBUG("image: " << origin.x << "," << origin.y);
    img2r(origin,origin);
    DEBUG_DEBUG("rect: " << origin.x << "," << origin.y);
    r2er(origin,origin);
    DEBUG_DEBUG("erect: " << origin.x << "," << origin.y);

#endif

    updateDisplay();

}

