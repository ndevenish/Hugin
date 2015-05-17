// -*- c-basic-offset: 4 -*-
/** @file LensCalImageCtrl.h
 *
 *
 *  @brief declaration of preview for lens calibration gui
 *
 *  @author T. Modes
 *
 */

/*  This is free software; you can redistribute it and/or
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

#ifndef LensCalImageCtrl_H
#define LensCalImageCtrl_H

#include <base_wx/wxImageCache.h>
#include <lcms2.h>
#include "lines/FindLines.h"
#include "LensCalTypes.h"

/** image previewer for lens calibration GUI
 *
 */
class LensCalImageCtrl : public wxPanel
{
public:
    enum LensCalPreviewMode
    {
        mode_original=0,
        mode_edge=1,
        mode_corrected=2
    };
    /** constructor */
    LensCalImageCtrl();
    /** set preview setting to given ImageLineList */
    void SetImage(ImageLineList* newList, unsigned int newIndex);
    /** set preview to empty image */
    void SetEmptyImage();
    /**  
     *   @param showLines true, if detected lines should be drawn above the image
     */
    void SetShowLines(bool showLines);
    /** set which image (original, edge, remapped/corrected) should be drawn */
    void SetMode(const LensCalPreviewMode newMode);
    /** return actual preview mode */
    const LensCalPreviewMode GetMode();
    /** updates the internal values of the lens (needed only for remapped image) */
    void SetLens(const HuginBase::SrcPanoImage::Projection newProjection,const double newFocallength, const double newCropfactor);
    /** updates the internal values of the lens distortions parameters (needed only for remapped image) */
    void SetLensDistortions(const double newA, const double newB, const double newC, const double newD, const double newE);

protected:
    /** draw the view into the offscreen buffer */
    void DrawView();
    void OnMouseEvent(wxMouseEvent &e);

private:
    /** resize event, recalculates the offscreen buffer */
    void Resize ( wxSizeEvent & e );
    /** paint event */
    void OnPaint(wxPaintEvent & dc);
    /** converts the edge image into wxImage */
    void SetEdgeImage();
    /** generates the remapped image suitable for wxImage */
    void GenerateRemappedImage(const unsigned int newWidth,const unsigned int newHeight);

    /** struct with filename, edge image and detected lines */
    ImageLineList* m_imageLines;
    unsigned int m_imageIndex;
    /** the image to adjust (full scale) */
    wxImage m_img;
    /** the edge image as RGBImage (in m_imageLines the edge image is grayscale (vigra::BImage)) */
    vigra::BRGBImage m_edgeImage;
    /** the remapped image */
    vigra::BRGBImage m_remappedImage;
    /** the edge detect image (resized scale) */
    wxImage m_edge;
    /** the remapped image as wxImage */
    wxImage m_remapped_img;
    /** the scaled image to save resizing */
    wxBitmap m_scaled_img;
    /** the image to display, e.g. with lines, in wxPanel resolution */
    wxBitmap m_display_img;
    /** scale factor for scaling from m_img to m_scaled_img */
    float m_scale;
    /** true, if the lines should be drawn above the image */
    bool m_showLines;
    /** which image should be drawn */
    LensCalPreviewMode m_previewMode;
    /** monitor profile */
    cmsHPROFILE m_monitorProfile;
    /** true, if we found a real monitor profile */
    bool m_hasMonitorProfile;

    // some actual lens parameters
    HuginBase::SrcPanoImage::Projection m_projection;
    double m_focallength;
    double m_cropfactor;
    double m_a;
    double m_b;
    double m_c;
    double m_d;
    double m_e;
    // SrcPanoImage and PanoramaOptions to calculate remapped image and transform lines
    HuginBase::SrcPanoImage m_panoimage;
    HuginBase::PanoramaOptions m_opts;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(CenterCanvas)
};

/** xrc handler for LensCalImageCtrl */
class LensCalImageCtrlXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(LensCalImageCtrlXmlHandler)

public:
    LensCalImageCtrlXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // LensCalImageCtrl_H
