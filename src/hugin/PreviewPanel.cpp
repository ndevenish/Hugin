// -*- c-basic-offset: 4 -*-

/** @file PreviewPanel.cpp
 *
 *  @brief implementation of PreviewPanel Class
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

#include <config.h>

#include "panoinc_WX.h"
#include "panoinc.h"

#include <vigra/basicimageview.hxx>
#include "vigra_ext/blend.h"
#include "PT/Stitcher.h"

#include "hugin/ImageCache.h"
#include "hugin/PreviewPanel.h"
#include "hugin/PreviewFrame.h"
#include "hugin/MainFrame.h"
#include "hugin/CommandHistory.h"
#include "hugin/config_defaults.h"
#include "hugin/huginApp.h"
//#include "hugin/ImageProcessing.h"

using namespace PT;
using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace utils;

typedef RGBValue<unsigned char> BRGBValue;

BEGIN_EVENT_TABLE(PreviewPanel, wxPanel)
//    EVT_PAINT(CPImageCtrl::OnPaint)
//    EVT_LEFT_DOWN(CPImageCtrl::mousePressEvent)
//    EVT_MOTION(CPImageCtrl::mouseMoveEvent)
//    EVT_LEFT_UP(CPImageCtrl::mouseReleaseEvent)
    EVT_SIZE(PreviewPanel::OnResize)
    EVT_LEFT_DOWN(PreviewPanel::mousePressLMBEvent)
    EVT_RIGHT_DOWN(PreviewPanel::mousePressRMBEvent)
    EVT_MOUSE_EVENTS ( PreviewPanel::OnMouse )
    EVT_PAINT ( PreviewPanel::OnDraw )
END_EVENT_TABLE()

PreviewPanel::PreviewPanel(PreviewFrame *parent, Panorama * pano2)
    : wxPanel (parent, -1, wxDefaultPosition,
               wxSize(256,128), wxEXPAND),
    pano(*pano2), m_autoPreview(false),m_panoImgSize(1,1),
    m_panoBitmap(0), 
    m_pano2erect(0), m_blendMode(BLEND_COPY), parentWindow(parent),
    m_state_rendering(false), m_rerender(false), m_imgsDirty(true)

{
    DEBUG_TRACE("");
    DEBUG_DEBUG("m_state_rendering = " << m_state_rendering);

#if defined(__WXMSW__) 
    wxString cursorPath = huginApp::Get()->GetXRCPath() + wxT("/data/cursor_cp_pick.cur");
    m_cursor = new wxCursor(cursorPath, wxBITMAP_TYPE_CUR);
#else
    m_cursor = new wxCursor(wxCURSOR_CROSS);
#endif
    SetCursor(*m_cursor);

    pano.addObserver(this);
}

PreviewPanel::~PreviewPanel()
{
    DEBUG_TRACE("dtor");
    delete m_cursor;
    pano.removeObserver(this);
    if (m_panoBitmap) {
        delete m_panoBitmap;
    }
    if (m_pano2erect) {
        delete m_pano2erect;
    }
	delete & vigra_ext::ThreadManager::get();
    DEBUG_TRACE("dtor end");
}

void PreviewPanel::panoramaChanged(Panorama &pano)
{
    // avoid recursive calls.. don't know if they can happen at all,
    // but they might lead to crashes.
    bool dirty = false;

    const PanoramaOptions & newOpts = pano.getOptions();

    // check if an important options has been changed
    if (newOpts.getHFOV() != opts.getHFOV()) {
        DEBUG_DEBUG("HFOV changed");
        dirty = true;
    }
    if (newOpts.getVFOV() != opts.getVFOV()) {
        DEBUG_DEBUG("VFOV changed");
        dirty = true;
    }
    if (newOpts.getProjection() != opts.getProjection()) {
        DEBUG_DEBUG("projection changed");
        dirty = true;
    }

    opts = newOpts;
    if (dirty) {
        // have to remap all images
        m_remapCache.invalidate();
    }

    if (m_autoPreview && dirty) {
        DEBUG_DEBUG("forcing preview update");
        ForceUpdate();
        // resize
    } else if(m_autoPreview && m_imgsDirty ) {
        DEBUG_DEBUG("updating preview after image change");
        updatePreview();
        m_imgsDirty=false;
    }
}

void PreviewPanel::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    /*
    for(PT::UIntSet::const_iterator it = changed.begin(); it != changed.end();
        ++it)
    {
        // TODO: need to check if just the active flag changed and skip invalidate in that case
        if (pano.getSrcImage(*it) != m_remapCache.getSrcDescription()) {
            m_remapCache.invalidate(*it);
        }
    }
    */

    m_imgsDirty = true;
    /*
    if (m_autoPreview) {
        DEBUG_DEBUG("updating preview after image change");
        updatePreview();
    }
    */
}

/*
void PreviewPanel::SetDisplayedImages(const UIntSet & imgs)
{
    m_displayedImages = imgs;
    if (m_autoPreview) {
        updatePreview();
    }
}
*/

void PreviewPanel::SetBlendMode(BlendMode b)
{
    m_blendMode = b;
    updatePreview();
}

void PreviewPanel::ForceUpdate()
{
    updatePreview();
}

void PreviewPanel::SetAutoUpdate(bool enabled)
{
    m_autoPreview = enabled;
    if (enabled) {
        updatePreview();
    }
}

void PreviewPanel::updatePreview()
{
    DEBUG_TRACE("");

    // we can accidentally end up here recursively, because wxWidgets
    // allows user input during redraw of the progress in the bottom
    if (m_state_rendering) {
        DEBUG_DEBUG("m_state_rendering == true, aborting rendering");
        m_rerender = true;
        return;
    }

    DEBUG_DEBUG("m_state_rendering = true");
    m_state_rendering = true;
    m_rerender = false;

    long nthreads = wxConfigBase::Get()->Read(wxT("/Nona/NumberOfThreads"), wxThread::GetCPUCount());
    if (nthreads < 1) nthreads = 1;
    vigra_ext::ThreadManager::get().setNThreads(nthreads);


	{
	  // Even though the frame is hidden, the panel is not
	  // so check the parent instead
	  if (parentWindow) {
  		if (parentWindow->IsShown() && (! parentWindow->IsIconized())) {
		  DEBUG_INFO("Parent window shown - updating");
		} else {
		  DEBUG_INFO("Parent window hidden - not updating");
                  m_state_rendering = false;
		  return;
		}
	  }
	}
//    bool seaming = wxConfigBase::Get()->Read("/PreviewPanel/UseSeaming",0l) != 0;

    // temporary bitmap for our remapped image
    // calculate the image size from panel widht, height from vfov

//    long cor = wxConfigBase::Get()->Read("/PreviewPanel/correctDistortion",0l);
//    bool corrLens = cor != 0;

    wxBusyCursor wait;
    double finalWidth = pano.getOptions().getWidth();
    double finalHeight = pano.getOptions().getHeight();

    m_panoImgSize = Diff2D(GetClientSize().GetWidth(), GetClientSize().GetHeight());

    double ratioPano = finalWidth / finalHeight;
    double ratioPanel = (double)m_panoImgSize.x / (double)m_panoImgSize.y;

    DEBUG_DEBUG("panorama ratio: " << ratioPano << "  panel ratio: " << ratioPanel);

    if (ratioPano < ratioPanel) {
        // panel is wider than pano
        m_panoImgSize.x = ((int) (m_panoImgSize.y * ratioPano));
        DEBUG_DEBUG("portrait: " << m_panoImgSize);
    } else {
        // panel is taller than pano
        m_panoImgSize.y = ((int)(m_panoImgSize.x / ratioPano));
        DEBUG_DEBUG("landscape: " << m_panoImgSize);
    }

    PanoramaOptions opts = pano.getOptions();
    opts.setWidth(m_panoImgSize.x, false);
    opts.setHeight(m_panoImgSize.y);
    //m_panoImgSize.y = opts.getHeight();
    // always use bilinear for preview.
    opts.interpolator = vigra_ext::INTERP_BILINEAR;

    // create images
    wxImage panoImage(m_panoImgSize.x, m_panoImgSize.y);
    try {
        vigra::BasicImageView<RGBValue<unsigned char> > panoImg((RGBValue<unsigned char> *)panoImage.GetData(), panoImage.GetWidth(), panoImage.GetHeight());
        BImage alpha(m_panoImgSize);
        // the empty panorama roi
//        Rect2D panoROI;
        DEBUG_DEBUG("about to stitch images, pano size: " << m_panoImgSize);
        UIntSet displayedImages = pano.getActiveImages();
        if (displayedImages.size() > 0) {
//            FileRemapper<BRGBImage, BImage> m;
            switch (m_blendMode) {
            case BLEND_COPY:
            {
                StackingBlender blender;
//                SimpleStitcher<BRGBImage, BImage> stitcher(pano, *(MainFrame::Get()));
                SimpleStitcher<BRGBImage, BImage> stitcher(pano, *parentWindow);
                stitcher.stitch(opts, displayedImages,
                                destImageRange(panoImg), destImage(alpha),
                                m_remapCache,
                                blender);
                break;
            }
            case BLEND_DIFFERENCE:
            {

                MultiBlendingStitcher<BRGBImage, BImage> stitcher(pano, *parentWindow);
                stitcher.stitch(opts, displayedImages,
                                destImageRange(panoImg), destImage(alpha),
                                m_remapCache);
                break;
/*

                WeightedStitcher<BRGBImage, BImage> stitcher(pano, *(MainFrame::Get()));
                stitcher.stitch(opts, m_displayedImages,
                                destImageRange(panoImg), destImage(alpha),
                                m_remapCache);
                break;
*/
            }
/*
            case BLEND_DIFFERENCE:
            {
                DifferenceBlender blender;
                SimpleStitcher<BRGBImage, BImage> stitcher(pano, *(MainFrame::Get()));
                stitcher.stitch(opts, m_displayedImages,
                                destImageRange(panoImg), destImage(alpha),
                                m_remapCache,
                                blender);
                break;
            }
*/
            }
        }
    } catch (std::exception & e) {
        m_state_rendering = false;
        DEBUG_ERROR("error during stitching: " << e.what());
        wxMessageBox(wxString(e.what(), *wxConvCurrent), _("Error during Stitching"));
    }

    // update the transform for pano -> erect coordinates
    if (m_pano2erect) delete m_pano2erect;
    SrcPanoImage src;
    src.setProjection(SrcPanoImage::EQUIRECTANGULAR);
    src.setHFOV(360);
    src.setSize(Size2D(360,180));
    m_pano2erect = new PTools::Transform;
    m_pano2erect->createTransform(src, opts);

    if (m_panoBitmap) {
        delete m_panoBitmap;
    }
    m_panoBitmap = new wxBitmap(panoImage);


    // always redraw
    wxClientDC dc(this);
    DrawPreview(dc);

    m_state_rendering = false;
    DEBUG_DEBUG("m_state_rendering = false");
    m_rerender = false;
}


void PreviewPanel::DrawPreview(wxDC & dc)
{
    if (!IsShown()){
        return;
    }
    DEBUG_TRACE("");

//    bool drawOutlines = wxConfigBase::Get()->Read("/PreviewPanel/drawOutlines",1l) != 0;

    int offsetX = 0;
    int offsetY = 0;

    wxSize sz = GetClientSize();
    if (sz.GetWidth() > m_panoImgSize.x) {
        offsetX = (sz.GetWidth() - m_panoImgSize.x) / 2;
    }
    if (sz.GetHeight() > m_panoImgSize.y) {
        offsetY = (sz.GetHeight() - m_panoImgSize.y) / 2;
    }

    dc.SetPen(wxPen(GetBackgroundColour(),1,wxSOLID));
    dc.SetBrush(wxBrush(GetBackgroundColour(),wxSOLID));
    dc.DrawRectangle(0, 0, offsetX, sz.GetHeight());
    dc.DrawRectangle(offsetX, 0, sz.GetWidth(), offsetY);
    dc.DrawRectangle(offsetX, sz.GetHeight() - offsetY,
                     sz.GetWidth(), sz.GetHeight());
    dc.DrawRectangle(sz.GetWidth() - offsetX, offsetY,
                     sz.GetWidth(), sz.GetHeight() - offsetY);

    // set a clip region to draw stuff accordingly
    dc.DestroyClippingRegion();
    dc.SetClippingRegion(offsetX, offsetY,
                         m_panoImgSize.x, m_panoImgSize.y);

    dc.SetPen(wxPen(wxT("BLACK"),1,wxSOLID));
    dc.SetBrush(wxBrush(wxT("BLACK"),wxSOLID));
    dc.DrawRectangle(offsetX, offsetY, m_panoImgSize.x, m_panoImgSize.y);


    // draw panorama image
    if (m_panoBitmap) {
        dc.DrawBitmap(*m_panoBitmap, offsetX, offsetY);
    }

#if 0
    // currently disabled
    if (drawOutlines) {
        for (UIntSet::iterator it = m_displayedImages.begin();
             it != m_displayedImages.end();
             ++it)
        {
            dc.SetPen(wxPen(wxT("GREY"), 1, wxSOLID));
            DrawOutline(m_remapped[*it]->getOutline(), dc, offsetX, offsetY);
        }
    }
#endif

    wxCoord w = m_panoImgSize.x;
    wxCoord h = m_panoImgSize.y;


    // draw center lines over display
    dc.SetPen(wxPen(wxT("WHITE"), 1, wxSOLID));
    dc.SetLogicalFunction(wxINVERT);
    dc.DrawLine(offsetX + w/2, offsetY,
                offsetX + w/2, offsetY + h);
    dc.DrawLine(offsetX, offsetY + h/2,
                offsetX + w, offsetY+ h/2);
}

void PreviewPanel::OnDraw(wxPaintEvent & event)
{
    wxPaintDC dc(this);
    DrawPreview(dc);
}

void PreviewPanel::OnResize(wxSizeEvent & e)
{
    DEBUG_TRACE("");
    wxSize sz = GetClientSize();
    if (sz.GetWidth() != m_panoImgSize.x && sz.GetHeight() != m_panoImgSize.y) {
        m_remapCache.invalidate();
        if (m_autoPreview) {
            ForceUpdate();
        }
    }
}


void PreviewPanel::mousePressLMBEvent(wxMouseEvent & e)
{
    DEBUG_DEBUG("mousePressLMBEvent: " << e.m_x << "x" << e.m_y);
    double yaw, pitch;
    mouse2erect(e.m_x, e.m_y,  yaw, pitch);
    // calculate new rotation angles.
    Matrix3 rotY;
    rotY.SetRotationPT(DEG_TO_RAD(-yaw), 0, 0);
    Matrix3 rotP;
    rotP.SetRotationPT(0, DEG_TO_RAD(pitch), 0);

    double y,p,r;
    Matrix3 rot = rotP * rotY;
    rot.GetRotationPT(y,p,r);
    y = RAD_TO_DEG(y);
    p = RAD_TO_DEG(p);
    r = RAD_TO_DEG(r);
    DEBUG_DEBUG("rotation angles pitch*yaw: " << y << " " << p << " " << r);

    GlobalCmdHist::getInstance().addCommand(
            new PT::RotatePanoCmd(pano, y, p, r)
        );
    if (!m_autoPreview) {
        ForceUpdate();
    }
}


void PreviewPanel::mousePressRMBEvent(wxMouseEvent & e)
{
    DEBUG_DEBUG("mousePressRMBEvent: " << e.m_x << "x" << e.m_y);
    double yaw, pitch;
    mouse2erect(e.m_x, e.m_y,  yaw, pitch);
    // theta, phi: spherical coordinates in mathworld notation.
    double theta = DEG_TO_RAD(yaw);
    double phi = DEG_TO_RAD(90+pitch);
    // convert to cartesian coordinates.
    double x = cos(theta)* sin(phi);
    double y = sin(theta)* sin(phi);
    double z = cos(phi);
    DEBUG_DEBUG("theta: " << theta << " phi: " << phi << " x y z:" << x << " " << y << " " << z);
    double roll = RAD_TO_DEG(atan(z/y));

    DEBUG_DEBUG("roll correction: " << roll);

    GlobalCmdHist::getInstance().addCommand(
            new PT::RotatePanoCmd(pano, 0, 0, roll)
        );
    if (!m_autoPreview) {
        ForceUpdate();
    }
}


void PreviewPanel::OnMouse(wxMouseEvent & e)
{
    double yaw, pitch;
    mouse2erect(e.m_x, e.m_y,  yaw, pitch);
    
    /*
    wxSize sz = GetClientSize();
    int offsetX = 0;
    int offsetY = 0;
    if (sz.GetWidth() > m_panoImgSize.x) {
        offsetX = (sz.GetWidth() - m_panoImgSize.x) / 2;
    }
    if (sz.GetHeight() > m_panoImgSize.y) {
        offsetY = (sz.GetHeight() - m_panoImgSize.y) / 2;
    }
    double x = e.m_x - offsetX - m_panoImgSize.x/2;
    double y = e.m_y - offsetY - m_panoImgSize.y/2;

    int w = pano.getOptions().getWidth();
    double scale = w/(double)m_panoImgSize.x;
    x *= scale;
    y *= scale;
    */

    parentWindow->SetStatusText(_("Left click to define new center point, right click to move point to horizon."),0);
    parentWindow->SetStatusText(wxString::Format(wxT("%.1f %.1f"), yaw, pitch), 1);
}

void PreviewPanel::mouse2erect(int xm, int ym, double &xd, double & yd)
{
    int offsetX=0, offsetY=0;

    if (m_pano2erect) {
        wxSize sz = GetClientSize();
        if (sz.GetWidth() > m_panoImgSize.x) {
            offsetX = (sz.GetWidth() - m_panoImgSize.x) / 2;
        }
        if (sz.GetHeight() > m_panoImgSize.y) {
            offsetY = (sz.GetHeight() - m_panoImgSize.y) / 2;
        }
        double x = xm - offsetX - m_panoImgSize.x/2;
        double y = ym - offsetY - m_panoImgSize.y/2;
        m_pano2erect->transform(xd, yd, x, y);
        //DEBUG_DEBUG("pano: " << x << " " << y << "  erect: " << xd << "° " << yd << "°");
     }
}

void PreviewPanel::DrawOutline(const vector<FDiff2D> & points, wxDC & dc, int offX, int offY)
{
    for (vector<FDiff2D>::const_iterator pnt = points.begin();
         pnt != points.end() ;
         ++pnt)
    {
        Diff2D point = pnt->toDiff2D();
        if (point.x < 0) point.x = 0;
        if (point.y < 0) point.y = 0;
        if (point.x >= m_panoImgSize.x)
            point.x = m_panoImgSize.y-1;
        if (point.y >= m_panoImgSize.y)
            point.y = m_panoImgSize.y -1;
        dc.DrawPoint(roundi(offX + point.x), roundi(offY + point.y));
    }
}
