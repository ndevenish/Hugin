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

#include "panoinc.h"
#include "panoinc_WX.h"

#include "hugin/ImageCache.h"
#include "hugin/PreviewPanel.h"
#include "hugin/MainFrame.h"

using namespace PT;
using namespace std;
using namespace vigra;

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
    parentWindow(parent)
{
    DEBUG_TRACE("");
}

PreviewPanel::~PreviewPanel()
{
    DEBUG_TRACE("dtor");
    for (vector<wxBitmap *>::iterator it = m_remappedBitmaps.begin();
         it != m_remappedBitmaps.end();
         ++it)
    {
        delete *it;
    }
    DEBUG_TRACE("dtor end");
}

void PreviewPanel::panoramaChanged(Panorama &pano)
{
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
        for (unsigned int i = 0; i < m_remappedBitmaps.size(); i++) {
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
    unsigned int nrRemapped = m_remappedBitmaps.size();

    // remove items for nonexisting images
    for (int i=nrRemapped-1; i>=(int)nrImages; i--)
    {
        DEBUG_DEBUG("Deleting remapped wxImage" << i);
        delete m_remappedBitmaps[i];
        m_remappedBitmaps.pop_back();
        m_outlines.pop_back();
        dirty = true;
    }
    // update existing items
//    if ( nrImages >= nrRemapped ) {
        for(PT::UIntSet::const_iterator it = changed.begin(); it != changed.end(); ++it){
            if (*it >= nrRemapped) {
                // create new item.
//                wxImage * bmp = new wxImage(sz.GetWidth(), sz.GetHeight());
                m_remappedBitmaps.push_back(new wxBitmap());
                m_outlines.push_back(vector<FDiff2D>(512*4));
                m_dirtyImgs.insert(*it);
            } else {
                // update existing item
                m_dirtyImgs.insert(*it);
            }
        }
//    }
    if (m_autoPreview) {
        updatePreview();
    }
}

void PreviewPanel::SetDisplayedImages(const UIntSet & imgs)
{
    m_displayedImages = imgs;
    updatePreview();
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

void PreviewPanel::updatePreview()
{
    DEBUG_TRACE("");
    bool dirty = false;
    // temporary bitmap for our remapped image
    // calculate the image size from panel widht, height from vfov

//    long cor = wxConfigBase::Get()->Read("/PreviewPanel/correctDistortion",0l);
//    bool corrLens = cor != 0;

    double finalWidth = pano.getOptions().width;
    double finalHeight = pano.getOptions().getHeight();

    m_panoImgSize = GetClientSize();

    double ratioPano = finalWidth / finalHeight;
    double ratioPanel = (double)m_panoImgSize.GetWidth() / (double)m_panoImgSize.GetHeight();

    DEBUG_DEBUG("panorama ratio: " << ratioPano << "  panel ratio: " << ratioPanel);

    if (ratioPano < ratioPanel) {
        // panel is wider than pano
        m_panoImgSize.SetWidth((int) (m_panoImgSize.GetHeight() * ratioPano));
        DEBUG_DEBUG("portrait: w: " << m_panoImgSize.GetWidth() << " h: " << m_panoImgSize.GetHeight());
    } else {
        // panel is taller than pano
        m_panoImgSize.SetHeight((int)(m_panoImgSize.GetWidth() / ratioPano));
        DEBUG_DEBUG("landscape: w: " << m_panoImgSize.GetWidth() << " h: " << m_panoImgSize.GetHeight());
    }


    UIntSet::iterator it = m_dirtyImgs.begin();
    while(it != m_dirtyImgs.end()) {
        if (set_contains(m_displayedImages, *it)) {
            wxImage timg(m_panoImgSize.GetWidth(), m_panoImgSize.GetHeight());
            mapPreviewImage(timg, *it);
            // FIXME.. we just mask out the black areas and hope that the
            // image doesn't contain some..
            timg.SetMaskColour(0,0,0);
            // convert to a drawable bitmap, lets hope that the mask
            // is transfered
            DEBUG_ASSERT(*it < m_remappedBitmaps.size());
            *(m_remappedBitmaps[*it]) = timg.ConvertToBitmap();
            dirty = true;
            UIntSet::iterator tit = it;
            ++it;
            m_dirtyImgs.erase(tit);
        } else {
            // do not update image if it is not shown.
            ++it;
        }
    }
//    if (dirty) {
    // always redraw
    wxClientDC dc(this);
    DrawPreview(dc);
//    }
}

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
    if (sz.GetWidth() > m_panoImgSize.GetWidth()) {
        offsetX = (sz.GetWidth() - m_panoImgSize.GetWidth()) / 2;
    }
    if (sz.GetHeight() > m_panoImgSize.GetHeight()) {
        offsetY = (sz.GetHeight() - m_panoImgSize.GetHeight()) / 2;
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
                         m_panoImgSize.GetWidth(), m_panoImgSize.GetHeight());

    dc.SetPen(wxPen("BLACK",1,wxSOLID));
    dc.SetBrush(wxBrush("BLACK",wxSOLID));
    dc.DrawRectangle(offsetX, offsetY, m_panoImgSize.GetWidth(), m_panoImgSize.GetHeight());



    for (vector<wxBitmap *>::iterator it = m_remappedBitmaps.begin();
         it != m_remappedBitmaps.end();
         ++it)
    {
        // draw only images that are scheduled to be drawn
        if (set_contains(m_displayedImages, it-m_remappedBitmaps.begin())) {
            if ((*it)->Ok()) {
                DEBUG_DEBUG("drawing image " << it - m_remappedBitmaps.begin());
                dc.DrawBitmap(*(*it), offsetX, offsetY, true);
            }
        }
    }

    if (drawOutlines) {
        for (UIntSet::iterator it = m_displayedImages.begin();
             it != m_displayedImages.end();
             ++it)
        {
            dc.SetPen(wxPen("GREY", 1, wxSOLID));
            DrawOutline(m_outlines[*it], dc, offsetX, offsetY);
        }
    }

    wxCoord w = m_panoImgSize.GetWidth();
    wxCoord h = m_panoImgSize.GetHeight();


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
    if (sz != m_panoImgSize) {
        if (m_autoPreview) {
            ForceUpdate();
        }
    }
}

void PreviewPanel::OnMouse(wxMouseEvent & e)
{
    DEBUG_DEBUG("OnMouse: " << e.m_x << "x" << e.m_y);
}


void PreviewPanel::mapPreviewImage(wxImage & dest, int imgNr)
{
//    PT::SpaceTransform t;
//    PT::SpaceTransform invT;
    PTools::Transform t;
    PTools::Transform invT;
    const PanoImage & pimg = pano.getImage(imgNr);
    wxImage * src = ImageCache::getInstance().getSmallImage(
        pimg.getFilename());

    Diff2D srcSize(src->GetWidth(), src->GetHeight());
    PanoramaOptions opts = pano.getOptions();
    opts.width = m_panoImgSize.GetWidth();
    Diff2D panoSize(opts.width, opts.getHeight());
    DEBUG_DEBUG("imgnr: " << imgNr << "  panoSize.x: " << panoSize.x << " dest.x: " << dest.GetWidth());
    DEBUG_DEBUG("imgnr: " << imgNr << "  panoSize.y: " << panoSize.y << " dest.y: " << dest.GetHeight());
//    DEBUG_ASSERT(panoSize.x == dest.GetWidth());
//    DEBUG_ASSERT(panoSize.y == dest.GetHeight());

    t.createTransform(pano, imgNr, opts, srcSize);
    invT.createInvTransform(pano, imgNr, opts, srcSize);

    // bounding box
    FDiff2D ul;
    FDiff2D lr;
    // outline of this image in final panorama
    m_outlines[imgNr].clear();
    PT::calcBorderPoints(srcSize, invT, back_inserter(m_outlines[imgNr]),
                             ul, lr);

    Diff2D ulInt((int)floor(ul.x), (int)floor(ul.y));
    Diff2D lrInt((int)ceil(lr.x), (int)ceil(lr.y));
    if (ulInt.x < 0) ulInt.x = 0;
    if (ulInt.y < 0) ulInt.y = 0;
    if (ulInt.x >= panoSize.x) ulInt.x = panoSize.x -1;
    if (ulInt.y >= panoSize.y) ulInt.y = panoSize.y -1;
    if (lrInt.x < 0) lrInt.x = 0;
    if (lrInt.y < 0) lrInt.y = 0;
    if (lrInt.x >= panoSize.x) lrInt.x = panoSize.x -1;
    if (lrInt.y >= panoSize.y) lrInt.y = panoSize.y -1;
    DEBUG_DEBUG("after clipping: upper left: " << ulInt.x << "," << ulInt.y
                << "  lower right: " << lrInt.x << "," << lrInt.y);

    FImage empty(1,1);
    // remap image with that transform
    PT::transformImage(srcIterRange(wxImageUpperLeft(*src),
                                        wxImageLowerRight(*src)),
                       destIterRange(wxImageUpperLeft(dest)+ ulInt,
                                     wxImageUpperLeft(dest)+lrInt),
                       ulInt,
                       t,
                       vigra::make_triple(empty.upperLeft(), empty.upperLeft(),
                                          empty.accessor()),
                       interp_bilin(),
                       *MainFrame::Get() );
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
        if (point.x >= m_panoImgSize.GetWidth())
            point.x = m_panoImgSize.GetWidth()-1;
        if (point.y >= m_panoImgSize.GetHeight())
            point.y = m_panoImgSize.GetHeight() -1;
        dc.DrawPoint((int)round(offX + point.x), (int)round(offY + point.y));
    }
}
