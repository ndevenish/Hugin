// -*- c-basic-offset: 4 -*-
/** @file MaskImageCtrl.h
 *
 *  @author Thomas Modes
 *
 *  $Id$
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _MaskImageCtrl_H
#define _MaskImageCtrl_H

#include <base_wx/wxImageCache.h>

#if defined __WXOSX_CARBON__ || defined __WXOSX_COCOA__
// on wxMac wxInvert does not work for drawing rubberband, so we are the following workaround
// instead of using wxInvert we are drawing the background image before the selection rectangle
// or the crop rectangle, the color of the selection is defined by the overall colour of the image
// this approach is already used for the polygon editor also on wxMSW and wxGTK
#undef SUPPORTS_WXINVERT
#else
#define SUPPORTS_WXINVERT 1
#endif

class MaskEditorPanel;

/** @brief mask editor
 *
 *  This class handles all mask editing. 
 */
class MaskImageCtrl : public wxScrolledWindow
{
public:
    /** ctor.
     */
    MaskImageCtrl()
        : m_scaleFactor(1),m_fitToWindow(false),m_maskEditState(NO_IMAGE)
        { }

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(MaskEditorPanel * parent);

    /** image rotation.
     *  Useful to display images depending on their roll setting.
     *  rotation is clockwise
     */ 
    enum ImageRotation { ROT0=0, ROT90, ROT180, ROT270 };

    /** if called, the mouse handlers are deactivated*/
    void setPreviewOnly() { m_previewOnly=true; };
    /** set the current image and mask list, this loads also the image from cache */
    void setImage (const std::string & filename, HuginBase::MaskPolygonVector newMask, HuginBase::MaskPolygonVector masksToDraw, ImageRotation rot);
    /** updates masks for currently selected image */
    void setNewMasks(HuginBase::MaskPolygonVector newMasks, HuginBase::MaskPolygonVector masksToDraw);
    /** updates the crop mode and crop rect */
    void setCrop(HuginBase::SrcPanoImage::CropMode newCropMode,vigra::Rect2D newCropRect, bool isCentered, hugin_utils::FDiff2D center, bool isCircleCrop);
    /** returns the current crop rect */
    vigra::Rect2D getCrop() { return m_cropRect; };
    /** mark mask with image as beeing editing */
    void setActiveMask(unsigned int newMask, bool doUpdate=true);
    /** returns the vector of all mask (including new created mask) */
    HuginBase::MaskPolygonVector getNewMask() const { return m_imageMask; };
    /** select all points of active mask */
    void selectAllMarkers();

    /** event handler when mouse is moving */
    void OnMouseMove(wxMouseEvent& mouse);
    /** event handler when left mouse button is pressed */
    void OnLeftMouseDown(wxMouseEvent& mouse);
    /** event handler when right mouse button is released */
    void OnLeftMouseUp(wxMouseEvent& mouse);
    /** event handler for left double click */
    void OnLeftMouseDblClick(wxMouseEvent& mouse);
    /** event handler when right mouse button is pressed */
    void OnRightMouseDown(wxMouseEvent& mouse);
    /** event handler when right mouse button is released */
    void OnRightMouseUp(wxMouseEvent& mouse);
    /** event handler for keyboard */
    void OnKeyUp(wxKeyEvent &e);
    /** event handler, when mouse capture is lost, e.g. user click outside of window 
     *  cancels creating of new mask */
    void OnCaptureLost(wxMouseCaptureLostEvent &e);
    /** event handler, when editor lost focus, mainly cancels creating new polygon */
    void OnKillFocus(wxFocusEvent &e);

    /** starts creating a new polygon */
    void startNewPolygon();
    /** returns size of currently scaled image */
    wxSize DoGetBestSize() const;

    /** sets the control to mask (newMaskMode=true) or crop (newMaskMode=false) mode */
    void SetMaskMode(bool newMaskMode);

    /** set the scaling factor for mask editing display.
     *
     *  @param factor zoom factor, 0 means fit to window.
     */
    void setScale(double factor);

    /** return scale factor, 0 for autoscale */
    double getScale()
        { return m_fitToWindow ? 0 : m_scaleFactor; }

    /** returns the current rotation of displayed image */
    ImageRotation getCurrentRotation() { return m_imgRotation; };

    /** set if active masks should be drawn */
    void setDrawingActiveMasks(bool newDrawActiveMasks);
    /** initiate redraw */
    void update();

    /** sets the colour for different parts */
    void SetUserColourPolygonNegative(wxColour newColour) { m_colour_polygon_negative=newColour; };
    void SetUserColourPolygonPositive(wxColour newColour) { m_colour_polygon_positive=newColour; };
    void SetUserColourPointSelected(wxColour newColour) { m_colour_point_selected=newColour; };
    void SetUserColourPointUnselected(wxColour newColour) { m_colour_point_unselected=newColour; };

protected:
    /** drawing routine */
    void OnDraw(wxDC& dc);
    /** handler called when size of control was changed */
    void OnSize(wxSizeEvent & e);

    /** get scale factor (calculates factor when fit to window is active) */
    double getScaleFactor() const;
    /** calculate new scale factor for this image */
    double calcAutoScaleFactor(wxSize size);
    /** rescale the image */
    void rescaleImage();

 private:

    //scaled bitmap
    wxBitmap m_bitmap;
    wxBitmap m_disabledBitmap;
    //filename of current editing file
    std::string m_imageFilename;
    // stores rotation of image
    ImageRotation m_imgRotation;
    // size of displayed (probably scaled) image
    wxSize m_imageSize;
    // size of real image
    wxSize m_realSize;
    // variables for crop
    HuginBase::SrcPanoImage::CropMode m_cropMode;
    vigra::Rect2D m_cropRect;
    bool m_cropCentered;
    bool m_cropCircle;
    hugin_utils::FDiff2D m_cropCenter;
    // draw active masks 
    bool m_showActiveMasks;
    // mask or crop mode
    bool m_maskMode;

    /** scale of width/height */
    int scale(int x) const
        {  return (int) (x * getScaleFactor() + 0.5); }

    double scale(double x) const
        {  return x * getScaleFactor(); }

    /** convert image coordinate to screen coordinates, considers additional added border */
    int transform(int x) const
        {  return (int) ((x+HuginBase::maskOffset) * getScaleFactor() + 0.5); }

    double transform(double x) const
        {  return (x+HuginBase::maskOffset) * getScaleFactor(); }

    wxPoint transform(const hugin_utils::FDiff2D & p) const
        {
            wxPoint r;
            r.x = transform(p.x);
            r.y = transform(p.y);
            return r;
        };

    /** translate screen coordinates to image coordinates, considers additional added border */
    int invtransform(int x) const
        {  return (int) (x/getScaleFactor()-HuginBase::maskOffset + 0.5); };

    double invtransform(double x) const
        {  return (x/getScaleFactor()-HuginBase::maskOffset); };

    hugin_utils::FDiff2D invtransform(const wxPoint & p) const
        {
            hugin_utils::FDiff2D r;
            r.x = invtransform(p.x);
            r.y = invtransform(p.y);
            return r;
        };
    
    // rotate coordinate to fit possibly rotated image display
    // useful for drawing something on the rotated display
    template <class T>
    T applyRot(const T & p) const
    {
        switch (m_imgRotation) {
            case ROT0:
                return p;
                break;
            case ROT90:
                return T(m_realSize.GetHeight()-1 - p.y, p.x);
                break;
            case ROT180:
                return T(m_realSize.GetWidth()-1 - p.x, m_realSize.GetHeight()-1 - p.y);
                break;
            case ROT270:
                return T(p.y, m_realSize.GetWidth()-1 - p.x);
                break;
            default:
                return p;
                break;
        }
    }

    // rotate coordinate to fit possibly rotated image display
    // useful for converting rotated display coordinates to image coordinates
    template <class T>
    T applyRotInv(const T & p) const
    {
        switch (m_imgRotation) {
            case ROT90:
                return T(p.y, m_realSize.GetHeight()-1 - p.x);
                break;
            case ROT180:
                return T(m_realSize.GetWidth()-1 - p.x, m_realSize.GetHeight()-1 - p.y);
                break;
            case ROT270:
                return T(m_realSize.GetWidth()-1 - p.y, p.x);
                break;
            case ROT0:
            default:
                return p;
                break;
        }
    }

    //draw the given polygon
    void DrawPolygon(wxDC &dc, HuginBase::MaskPolygon poly, bool isSelected, bool drawMarker);
    //draw a selection rectange, when called the second time the rectangle is deleted
    void DrawSelectionRectangle();
    // draws the crop rectangle and/or circle
    void DrawCrop(wxDC & dc);
    void DrawCrop();
    // find the polygon for which the point p is inside the polygon
    void FindPolygon(hugin_utils::FDiff2D p);
    // returns a set of points which are in the selection rectangle 
    bool SelectPointsInsideMouseRect(HuginBase::UIntSet &points,const bool considerSelectedOnly);
    // updates the crop
    void UpdateCrop(hugin_utils::FDiff2D delta);

    // where the cursor is 
    enum ClickPos
    {
        CLICK_OUTSIDE, CLICK_INSIDE, CLICK_LEFT, CLICK_RIGHT, CLICK_TOP, CLICK_BOTTOM, CLICK_CIRCLE
    };
    // test, where the curos is
    ClickPos GetClickPos(vigra::Point2D pos);

    /**  different states of the editor */
    enum MaskEditorState
    {
        NO_IMAGE=0, // no image selected
        NO_MASK, // image loaded, but none active mask
        NO_SELECTION, // active polygon, but no point selected
        POINTS_SELECTED, // points selected
        POINTS_MOVING, // dragging points
        POINTS_DELETING,  // remove points inside rect
        POINTS_ADDING,  // adding new points add mouse position
        POLYGON_SELECTING, // selecting an region to select an other polygon
        REGION_SELECTING, // currently selecting an region
        NEW_POLYGON_STARTED, // modus is new polygon, but no point setted yet
        NEW_POLYGON_CREATING,  // currently creating new polygon
        CROP_SHOWING,  // only showing the crop
        CROP_MOVING,  // dragging crop
        CROP_CIRCLE_SCALING, // circular crop changing
        CROP_LEFT_MOVING, // dragging left border of crop
        CROP_RIGHT_MOVING, // dragging right border of crop
        CROP_TOP_MOVING,  // dragging top border of crop
        CROP_BOTTOM_MOVING // dragging bottom border of crop
    };
    MaskEditorState m_maskEditState;

    double m_scaleFactor;
    bool m_fitToWindow;
    bool m_previewOnly;

    MaskEditorPanel * m_editPanel;
    HuginBase::MaskPolygonVector m_imageMask;
    HuginBase::MaskPolygonVector m_masksToDraw;
    unsigned int m_activeMask;
    // the active masks, the one which is currently editing
    HuginBase::MaskPolygon m_editingMask;
    // all selected points
    HuginBase::UIntSet m_selectedPoints;

    ImageCache::EntryPtr m_img;

    // positions of mouse drag
    wxPoint m_dragStartPos;
    wxPoint m_currentPos;

    // colours for different parts
    wxColor m_colour_polygon_negative;
    wxColor m_colour_polygon_positive;
    wxColor m_colour_point_selected;
    wxColor m_colour_point_unselected;
#ifndef SUPPORTS_WXINVERT
    wxColour m_color_selection;
#endif

    DECLARE_EVENT_TABLE();
    DECLARE_DYNAMIC_CLASS(MaskImageCtrl)
};

/** xrc handler for mask editor */
class MaskImageCtrlXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(MaskImageCtrlXmlHandler)

public:
    MaskImageCtrlXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};


#endif // _MaskImageCtrl_H
