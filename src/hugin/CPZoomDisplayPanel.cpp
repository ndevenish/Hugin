// -*- c-basic-offset: 4 -*-

/** @file CPZoomDisplayPanel.cpp
 *
 *  @brief implementation of CPZoomDisplayPanel Class
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

// standard wx include
#include <config.h>
#include "panoinc_WX.h"

// standard hugin include
#include "panoinc.h"

#include <vigra/basicimageview.hxx>
#include "vigra_ext/blend.h"
#include "PT/Stitcher.h"

#include "hugin/CPZoomDisplayPanel.h"
#include "hugin/CPImageCtrl.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/MainFrame.h"

using namespace std;
using namespace utils;
using namespace PT;
using namespace PTools;
using namespace vigra;
using namespace vigra_ext;

BEGIN_EVENT_TABLE(CPZoomDisplayPanel, wxPanel)
    EVT_SIZE(CPZoomDisplayPanel::OnSize)
    EVT_PAINT (CPZoomDisplayPanel::OnRedraw )
    EVT_LEFT_DOWN(CPZoomDisplayPanel::OnLMB)
END_EVENT_TABLE()


CPZoomDisplayPanel::CPZoomDisplayPanel(wxWindow * parent, PT::Panorama & pano)
    : wxPanel(parent), m_validPoint(false), m_validImg(false),
      m_pano(pano)
{
}

CPZoomDisplayPanel::~CPZoomDisplayPanel()
{
}

#if 0
void CPZoomDisplayPanel::updateInternalOptim()
{

    DEBUG_TRACE("image: "<< m_imgNr << " point: " << m_point);

    wxConfigBase * config = wxConfigBase::Get();
    double zoom = 1.0;
    config->Read(wxT("/CPZoomDisplay/Zoom"),&zoom);
    config->Write(wxT("/CPZoomDisplay/Zoom"),zoom);

    const PanoImage & img = m_pano.getImage(m_imgNr);
    string imageFilename = img.getFilename();
    // get source image from image cache
    wxImage * src = ImageCache::getInstance().getImage(imageFilename);
    if (!src || !src->Ok()) {
        DEBUG_ERROR("Could not load image: " << imageFilename);
        throw std::runtime_error("could not retrieve source image for ctrl point zoom view");
    }

    Diff2D srcSize(src->GetWidth(), src->GetHeight());
    DEBUG_DEBUG("src size: " << srcSize);

    // vigra image on this data
    BasicImageView<RGBValue<unsigned char> > srcImg((RGBValue<unsigned char> *)src->GetData(),
                                                    src->GetWidth(),
                                                    src->GetHeight());


    // create transform for this image
    VariableMap vars = m_pano.getImageVariables(m_imgNr);
    // discard position, camera always looks straight into the image,
    // since we want to remap from the image border to the image center.
    map_get(vars,"y").setValue(0);
    map_get(vars,"p").setValue(0);
    map_get(vars,"r").setValue(0);

    // create inverse transform, from image to pano (undistorted patch)
    // coordiantes.
    m_t_img2center.createInvTransform(srcSize,
                                      vars,
                                      m_pano.getLens(img.getLensNr()).getProjection(),
                                      Diff2D(360,180),
                                      PanoramaOptions::EQUIRECTANGULAR,
                                      360,
                                      srcSize);

    //
    // calculate yaw and pitch of the image point, by using the inverse
    // transform

    double yaw, pitch;
    m_t_img2center.transformImgCoord(yaw, pitch, m_point.x, m_point.y);
    DEBUG_DEBUG("pixels: " << m_point << " -> " << yaw << " " << pitch);

    double yaw2, pitch2;
    FDiff2D point2(m_point.x - srcSize.x/2, srcSize.y/2.0 - m_point.y);
    m_t_img2center.transform(yaw2, pitch2,
                             point2.x, point2.y);
    DEBUG_DEBUG("pixels 2: " << point2 << " -> " << yaw2 << " " << pitch2);

    // equirect image coordinates -> equirectangular coordinates
    // transformImgCoord places the origin at the upper left corner.
    yaw = (yaw + 0.5) - 180;
    pitch = 90 - (pitch + 0.5);

    DEBUG_DEBUG("yaw: " << yaw << "  pitch: " << pitch);

    //
    // now start remapping from the periphery to the center, by pointing
    // the camera directly at that position. that is, we shift the image
    // so that our point is located at the middle of the panorama
    map_get(vars,"y").setValue(-yaw);
    map_get(vars,"p").setValue(-pitch);
    map_get(vars,"r").setValue(0);

    // create inverse transform, from image to pano (undistorted patch)
    // coordiantes.
    m_t_img2center.createInvTransform(srcSize,
                                      vars,
                                      m_pano.getLens(img.getLensNr()).getProjection(),
                                      Diff2D(360,180),
                                      PanoramaOptions::EQUIRECTANGULAR,
                                      360,
                                      srcSize);

    // now we have to know how many degrees a pixel is, so
    // that we can remap to a sensible size.
    // do this the trial and error way, by remapping two points
    // and measure the distance after remapping

    // size of remapped image
    wxSize tsz = GetVirtualSize();
    Diff2D wSize(tsz.x, tsz.y);
    // make sure the size is uneven, so that we can draw
    // our mark through the middle
    if (wSize.x%1 == 0) {
        wSize.x -=1;
    }
    if (wSize.y%1 == 0) {
        wSize.y -=1;
    }

    FDiff2D p, phalf;
    m_t_img2center.transform(p, m_point);
    m_t_img2center.transform(phalf, m_point + FDiff2D(wSize.x/2.0,0));
    // hfov at patch.
    double hfov = 2 * fabs(phalf.x - p.x);
    DEBUG_DEBUG("HFOV for remapped patch: " << hfov);

    // allow an external scale factor as well.
    hfov = hfov/zoom;

    if (hfov > 90) {
        // limit hfov to something halfway sensible.. to avoid a crash..
        hfov=90;
    }

    // recreate transforms to include the scale factor..
    // create suitable transforms
    // transform to pano (undistorted patch), into image
    m_t_center2img.createTransform(srcSize,
                                   vars,
                                   m_pano.getLens(img.getLensNr()).getProjection(),
                                   wSize,
                                   PanoramaOptions::RECTILINEAR,
                                   hfov,
                                   srcSize);


    // create inverse transform, from image to pano (undistorted patch)
    // coordiantes.
    m_t_img2center.createInvTransform(srcSize,
                                      vars,
                                      m_pano.getLens(img.getLensNr()).getProjection(),
                                      wSize,
                                      PanoramaOptions::RECTILINEAR,
                                      hfov,
                                      srcSize);

    // create remapped image.
    wxImage undistorted(wSize.x, wSize.y);
    // vigra image on this data
    BasicImageView<RGBValue<unsigned char> > destImg((RGBValue<unsigned char> *)undistorted.GetData(),
                                                     wSize.x,
                                                     wSize.y);
    // mask image
    BImage srcAlpha(src->GetWidth(), src->GetHeight(), 255);

    MultiProgressDisplay dummy;
    // finally remap image...
    transformImage(srcImageRange(srcImg),
                   destImageRange(destImg),
                   destImage(srcAlpha),
                   Diff2D(0,0),
                   m_t_center2img,
                   PanoramaOptions::CUBIC,
                   dummy);

    m_bitmap = wxBitmap(undistorted);
}

#endif

void CPZoomDisplayPanel::updateInternal()
{
    DEBUG_TRACE("image: "<< m_imgNr << " point: " << m_point);

    wxConfigBase * config = wxConfigBase::Get();
    double zoom = 1.0;
    config->Read(wxT("/CPZoomDisplay/Zoom"),&zoom);
    config->Write(wxT("/CPZoomDisplay/Zoom"),zoom);

    const PanoImage & img = m_pano.getImage(m_imgNr);
    string imageFilename = img.getFilename();
    // get source image from image cache
    wxImage * src = ImageCache::getInstance().getImage(imageFilename);
    if (!src || !src->Ok()) {
        DEBUG_ERROR("Could not load image: " << imageFilename);
        throw std::runtime_error("could not retrieve source image for ctrl point zoom view");
    }

    Diff2D srcSize(src->GetWidth(), src->GetHeight());
    DEBUG_DEBUG("src size: " << srcSize);

    // vigra image on this data
    BasicImageView<RGBValue<unsigned char> > srcImg((RGBValue<unsigned char> *)src->GetData(),
                                                    src->GetWidth(),
                                                    src->GetHeight());


    // create transform for this image
    VariableMap vars = m_pano.getImageVariables(m_imgNr);
    // discard position, camera always looks straight into the image,
    // since we want to remap from the image border to the image center.
    map_get(vars,"y").setValue(0);
    map_get(vars,"p").setValue(0);
    map_get(vars,"r").setValue(0);

    // create inverse transform, from image to pano (undistorted patch)
    // coordiantes.
    m_t_img2center.createInvTransform(srcSize,
                                      vars,
                                      m_pano.getLens(img.getLensNr()).getProjection(),
                                      Diff2D(360,180),
                                      PanoramaOptions::EQUIRECTANGULAR,
                                      360,
                                      srcSize);

    //
    // calculate yaw and pitch of the image point, by using the inverse
    // transform

    double yaw, pitch;
    m_t_img2center.transformImgCoord(yaw, pitch, m_point.x, m_point.y);
    DEBUG_DEBUG("pixels: " << m_point << " -> " << yaw << " " << pitch);

    double yaw2, pitch2;
    FDiff2D point2(m_point.x - srcSize.x/2, srcSize.y/2.0 - m_point.y);
    m_t_img2center.transform(yaw2, pitch2,
                             point2.x, point2.y);
    DEBUG_DEBUG("pixels 2: " << point2 << " -> " << yaw2 << " " << pitch2);

    // equirect image coordinates -> equirectangular coordinates
    // transformImgCoord places the origin at the upper left corner.
    yaw = (yaw + 0.5) - 180;
    pitch = 90 - (pitch + 0.5);

    DEBUG_DEBUG("yaw: " << yaw << "  pitch: " << pitch);

    //
    // now start remapping from the periphery to the center, by pointing
    // the camera directly at that position. that is, we shift the image
    // so that our point is located at the middle of the panorama
    map_get(vars,"y").setValue(-yaw);
    map_get(vars,"p").setValue(-pitch);
    map_get(vars,"r").setValue(0);

    // create inverse transform, from image to pano (undistorted patch)
    // coordiantes.
    m_t_img2center.createInvTransform(srcSize,
                                      vars,
                                      m_pano.getLens(img.getLensNr()).getProjection(),
                                      Diff2D(360,180),
                                      PanoramaOptions::EQUIRECTANGULAR,
                                      360,
                                      srcSize);

    // now we have to know how many degrees a pixel is, so
    // that we can remap to a sensible size.
    // do this the trial and error way, by remapping two points
    // and measure the distance after remapping

    // size of remapped image
    wxSize tsz = GetVirtualSize();
    Diff2D wSize(tsz.x, tsz.y);
    // make sure the size is uneven, so that we can draw
    // our mark through the middle
    if (wSize.x%1 == 0) {
        wSize.x -=1;
    }
    if (wSize.y%1 == 0) {
        wSize.y -=1;
    }

    FDiff2D p, phalf;
    m_t_img2center.transform(p, m_point);
    m_t_img2center.transform(phalf, m_point + FDiff2D(wSize.x/2.0,0));
    // hfov at patch.
    double hfov = 2 * fabs(phalf.x - p.x);
    DEBUG_DEBUG("HFOV for remapped patch: " << hfov);

    // allow an external scale factor as well.
    hfov = hfov/zoom;

    if (hfov > 90) {
        // limit hfov to something halfway sensible.. to avoid a crash..
        hfov=90;
    }

    // recreate transforms to include the scale factor..
    // create suitable transforms
    // transform to pano (undistorted patch), into image
    m_t_center2img.createTransform(srcSize,
                                   vars,
                                   m_pano.getLens(img.getLensNr()).getProjection(),
                                   wSize,
                                   PanoramaOptions::RECTILINEAR,
                                   hfov,
                                   srcSize);


    // create inverse transform, from image to pano (undistorted patch)
    // coordiantes.
    m_t_img2center.createInvTransform(srcSize,
                                      vars,
                                      m_pano.getLens(img.getLensNr()).getProjection(),
                                      wSize,
                                      PanoramaOptions::RECTILINEAR,
                                      hfov,
                                      srcSize);

    // create remapped image.
    wxImage undistorted(wSize.x, wSize.y);
    // vigra image on this data
    BasicImageView<RGBValue<unsigned char> > destImg((RGBValue<unsigned char> *)undistorted.GetData(),
                                                     wSize.x,
                                                     wSize.y);
    // mask image
    BImage srcAlpha(src->GetWidth(), src->GetHeight(), 255);

    MultiProgressDisplay dummy;
    // finally remap image...
    transformImage(srcImageRange(srcImg),
                   destImageRange(destImg),
                   destImage(srcAlpha),
                   Diff2D(0,0),
                   m_t_center2img,
                   false,
                   vigra_ext::INTERP_CUBIC,
                   dummy);

    m_bitmap = wxBitmap(undistorted);
}


void CPZoomDisplayPanel::OnSize(wxSizeEvent &e)
{
    // only if we are displaying something
    if (! (m_validImg && m_validPoint)) {
        return;
    }

    // update image
    updateInternal();
    // update display
    wxPaintDC dc(this);
    OnDraw(dc);
}

void CPZoomDisplayPanel::OnRedraw(wxPaintEvent & event)
{
    wxPaintDC dc(this);
    OnDraw(dc);
}

void CPZoomDisplayPanel::OnDraw(wxDC & dc)
{
    // only if we are displaying something
    if (! (m_validImg && m_validPoint)) {
        // clear otherwise
        dc.Clear();
        return;
    }
    wxSize vSize = GetVirtualSize();
    // paint bitmap onto widget (it should be of the same size)
    if (m_bitmap.GetHeight() > vSize.GetHeight() ||
        m_bitmap.GetWidth() > vSize.GetWidth() ) {
        DEBUG_WARN("remapped image is bigger than window.");
    }

    if (m_bitmap.Ok()) {
        dc.DrawBitmap(m_bitmap,0,0);
    }

    // draw crosshair over bitmap
    int mx = m_bitmap.GetWidth() / 2 + 1;
    int my = m_bitmap.GetHeight() / 2 + 1;
    dc.SetPen(wxPen(wxT("WHITE"), 1, wxSOLID));
    dc.SetLogicalFunction(wxINVERT);
    dc.DrawLine(mx, 0,
                mx, m_bitmap.GetHeight());
    dc.DrawLine(0, my,
                m_bitmap.GetWidth(), my);
}

void CPZoomDisplayPanel::OnLMB(wxMouseEvent & e)
{
    FDiff2D mpos(e.GetPosition().x, e.GetPosition().y);
    // transform mouse position into "mathematical" coordiante system
    // y -> top, x -> right

    FDiff2D ipos;
    // transform back to image coordinates
    double x,y;
    m_t_center2img.transformImgCoord(x,y, mpos.x, mpos.y);
    ipos.x = x;
    ipos.y = y;
    DEBUG_DEBUG("zoomed: " << mpos << " -> " << ipos);

}
