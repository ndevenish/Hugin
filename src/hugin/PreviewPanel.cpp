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

#include "panoinc_WX.h"
#include "panoinc.h"

#include <vigra/basicimageview.hxx>
#include "vigra_ext/blend.h"
#include "PT/Stitcher.h"

#include "hugin/ImageCache.h"
#include "hugin/PreviewPanel.h"
#include "hugin/MainFrame.h"
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
//    EVT_MOUSE_EVENTS ( PreviewPanel::OnMouse )
    EVT_PAINT ( PreviewPanel::OnDraw )
END_EVENT_TABLE()

PreviewPanel::PreviewPanel(wxWindow *parent, Panorama * pano)
    : wxPanel (parent, -1, wxDefaultPosition,
               wxSize(256,128), wxEXPAND),
    pano(*pano), m_autoPreview(false),m_panoImgSize(1,1),
    m_panoBitmap(0), parentWindow(parent)
{
    DEBUG_TRACE("");
}

PreviewPanel::~PreviewPanel()
{
    DEBUG_TRACE("dtor");
    for (RemappedVector::iterator it = m_remapped.begin();
         it != m_remapped.end();
         ++it)
    {
        delete *it;
    }
    DEBUG_TRACE("dtor end");
}

void PreviewPanel::panoramaChanged(Panorama &pano)
{
    // avoid recursive calls.. don't know if they can happen at all,
    // but they might lead to crashes.
    bool dirty = false;

    const PanoramaOptions & newOpts = pano.getOptions();

    // check if an important options has been changed
    if (newOpts.HFOV != opts.HFOV) {
        DEBUG_DEBUG("HFOV changed");
        dirty = true;
    }
    if (newOpts.VFOV != opts.VFOV) {
        DEBUG_DEBUG("VFOV changed");
        dirty = true;
    }
    if (newOpts.projectionFormat != opts.projectionFormat) {
        DEBUG_DEBUG("projection changed");
        dirty = true;
    }

    opts = newOpts;
    if (m_autoPreview && dirty) {
        DEBUG_DEBUG("forcing preview update");
        for (unsigned int i = 0; i < m_remapped.size(); i++) {
            m_dirtyImgs.insert(i);
        }
        updatePreview();
        // resize
    }
}

void PreviewPanel::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    bool dirty = false;

    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrRemapped = m_remapped.size();

    // remove items for nonexisting images
    for (int i=nrRemapped-1; i>=(int)nrImages; i--)
    {
        DEBUG_DEBUG("Deleting remapped wxImage" << i);
        delete m_remapped[i];
        m_remapped.pop_back();
//        m_outlines.pop_back();
        dirty = true;
    }
    // update existing items
//    if ( nrImages >= nrRemapped ) {
    for(PT::UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it){
        if (*it >= nrRemapped) {
            // create new item.
//                wxImage * bmp = new wxImage(sz.GetWidth(), sz.GetHeight());
            m_remapped.push_back(new RemappedImage());
            m_dirtyImgs.insert(*it);
        } else {
            // update existing item
            m_dirtyImgs.insert(*it);
        }
    }
//    }
    if (m_autoPreview) {
        DEBUG_DEBUG("updating preview after image change");
        updatePreview();
    }
}

void PreviewPanel::SetDisplayedImages(const UIntSet & imgs)
{
    m_displayedImages = imgs;
    if (m_autoPreview) {
        updatePreview();
    }
}

void PreviewPanel::ForceUpdate()
{
    unsigned int nImages = pano.getNrOfImages();
    for (unsigned int i=0; i < nImages; i++)
    {
        m_dirtyImgs.insert(i);
    }
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
    bool seaming = wxConfigBase::Get()->Read("/PreviewPanel/UseSeaming",0l) != 0;

    // temporary bitmap for our remapped image
    // calculate the image size from panel widht, height from vfov

//    long cor = wxConfigBase::Get()->Read("/PreviewPanel/correctDistortion",0l);
//    bool corrLens = cor != 0;

    double finalWidth = pano.getOptions().width;
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
    opts.width = m_panoImgSize.x;
    m_panoImgSize.y = opts.getHeight();
    // always use bilinear for preview.
    opts.interpolator = PanoramaOptions::BILINEAR;

    // create images
    wxImage panoImage(m_panoImgSize.x, m_panoImgSize.y);
    try {
        vigra::BasicImageView<RGBValue<unsigned char> > panoImg((RGBValue<unsigned char> *)panoImage.GetData(), panoImage.GetWidth(), panoImage.GetHeight());
        BImage alpha(m_panoImgSize);
        // the empty panorama roi
        ROI<Diff2D> panoROI;
        DEBUG_DEBUG("about to stitch images");
        if (m_displayedImages.size() > 0) {
            FileRemapper<BRGBImage, BImage> m;
            if (seaming) {
                WeightedStitcher<BRGBImage, BImage> stitcher(pano, *(MainFrame::Get()));
                stitcher.stitch(opts, m_displayedImages,
                                destImageRange(panoImg), destImage(alpha),
                                m);
            } else {
                WeightedStitcher<BRGBImage, BImage> stitcher(pano, *(MainFrame::Get()));
                stitcher.stitch(opts, m_displayedImages, 
                                destImageRange(panoImg), destImage(alpha),
                                m);
            }
        }
    } catch (std::exception & e) {
        DEBUG_ERROR("error during stitching: " << e.what());
        wxMessageBox(e.what(), _("Error during Stitching"));
    }
    if (m_panoBitmap) {
        delete m_panoBitmap;
    }
    m_panoBitmap = new wxBitmap(panoImage);

    // always redraw
    wxClientDC dc(this);
    DrawPreview(dc);
}


#if 0

void PreviewPanel::updatePreview()
{
    DEBUG_TRACE("");
    bool seaming = wxConfigBase::Get()->Read("/PreviewPanel/UseSeaming",0l) != 0;
    // in case of recursive calls, do not start to reupdate.
    if (!m_updating) {
        m_updating = true;

        // temporary bitmap for our remapped image
        // calculate the image size from panel widht, height from vfov

//    long cor = wxConfigBase::Get()->Read("/PreviewPanel/correctDistortion",0l);
//    bool corrLens = cor != 0;

        double finalWidth = pano.getOptions().width;
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

        UIntSet::iterator it = m_dirtyImgs.begin();
        while(it != m_dirtyImgs.end()) {
//            DEBUG_ASSERT(*it < m_remapped.size());
            if (*it >= m_remapped.size()) {
                // I don't understand how this can happen,
                // as a workaround, ignore the image.
                continue;
            }
            // remapp the image, using the given small image as default
            mapPreviewImage(*it);
            UIntSet::iterator tit = it;
            ++it;
            m_dirtyImgs.erase(tit);
        }

        // update the preview image

        wxImage panoImage(m_panoImgSize.x, m_panoImgSize.y);
        vigra::BasicImageView<RGBValue<unsigned char> > panoImg((RGBValue<unsigned char> *)panoImage.GetData(), panoImage.GetWidth(), panoImage.GetHeight());
        BImage alpha(m_panoImgSize);
        // the empty panorama roi
        ROI<Diff2D> panoROI;
        DEBUG_DEBUG("about to merge images");
        for (RemappedVector::iterator it = m_remapped.begin();
             it != m_remapped.end();
             ++it)
        {
            // draw only images that are scheduled to be drawn
            // TODO: blending order is different from final panorama..
            // should fix that.
            if (set_contains(m_displayedImages, it-m_remapped.begin())) {
                if ((*it)->roi().size().x > 0) {
                    DEBUG_DEBUG("about to copy/seam image " << it - m_remapped.begin()
                                << " pano roi: " << panoROI << " img roi: " << (*it)->roi());

                    // calculate the currently active roi (union of pano and image)
                    vigra_ext::ROI<vigra::Diff2D> overlap;

                    if (seaming) {
                        DEBUG_DEBUG("drawing image " << it - m_remapped.begin());
                        // merge into view!
                        MultiProgressDisplay dummy;
                        blend(*(*it),
                              destImageRange(panoImg), destImage(alpha), panoROI,
                              dummy);
                    } else {
                        // copy image with mask.
                        vigra::copyImageIf((*it)->image(),
                                           destIter((*it)->alpha().first),
                                           (*it)->roi().apply(destImage(panoImg),panoROI) );
                        // copy mask
                        vigra::copyImageIf((*it)->alpha(),
                                           destIter((*it)->alpha().first),
                                           (*it)->roi().apply(destImage(alpha),panoROI) );
                        img.roi().unite(panoROI, panoROI);
                    }
                } else {
                    DEBUG_WARN("image should be drawn, but has not been remapped");
                }
            }
        }

        if (m_panoBitmap) {
            delete m_panoBitmap;
        }
        m_panoBitmap = new wxBitmap(panoImage);

        // always redraw
        wxClientDC dc(this);
        DrawPreview(dc);

        m_updating = false;
    }
}

#endif

void PreviewPanel::DrawPreview(wxDC & dc)
{
    if (!IsShown()){
        return;
    }
    DEBUG_TRACE("");

    bool drawOutlines = wxConfigBase::Get()->Read("/PreviewPanel/drawOutlines",1l) != 0;

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

    dc.SetPen(wxPen("BLACK",1,wxSOLID));
    dc.SetBrush(wxBrush("BLACK",wxSOLID));
    dc.DrawRectangle(offsetX, offsetY, m_panoImgSize.x, m_panoImgSize.y);


    // draw panorama image
    if (m_panoBitmap) {
        dc.DrawBitmap(*m_panoBitmap, offsetX, offsetY);
    }

    if (drawOutlines) {
        for (UIntSet::iterator it = m_displayedImages.begin();
             it != m_displayedImages.end();
             ++it)
        {
            dc.SetPen(wxPen("GREY", 1, wxSOLID));
            DrawOutline(m_remapped[*it]->getOutline(), dc, offsetX, offsetY);
        }
    }

    wxCoord w = m_panoImgSize.x;
    wxCoord h = m_panoImgSize.y;


    // draw center lines over display
    dc.SetPen(wxPen("WHITE", 1, wxSOLID));
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
        if (m_autoPreview) {
            ForceUpdate();
        }
    }
}

void PreviewPanel::OnMouse(wxMouseEvent & e)
{
    DEBUG_DEBUG("OnMouse: " << e.m_x << "x" << e.m_y);
}

void PreviewPanel::mapPreviewImage(unsigned int imgNr)
{
    DEBUG_ASSERT(imgNr < pano.getNrOfImages());
    DEBUG_ASSERT(imgNr < m_remapped.size());

    const PanoImage & pimg = pano.getImage(imgNr);
    wxImage * src = ImageCache::getInstance().getSmallImage(
        pimg.getFilename());

    Diff2D srcSize(src->GetWidth(), src->GetHeight());
    PanoramaOptions opts = pano.getOptions();
    opts.width = m_panoImgSize.x;
    // always use bilinear for preview.
    opts.interpolator = PanoramaOptions::BILINEAR;
    Diff2D panoSize(opts.width, opts.getHeight());

    MultiProgressDisplay dummy;

    vigra::BasicImageView<BRGBValue> img((BRGBValue*)src->GetData(), src->GetWidth(),
                              src->GetHeight());
    m_remapped[imgNr]->remapImage(pano, opts,
                                 srcImageRange(img),
                                 imgNr,
                                 dummy);
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
