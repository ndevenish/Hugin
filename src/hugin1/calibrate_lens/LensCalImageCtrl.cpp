// -*- c-basic-offset: 4 -*-

/** @file LensCalImageCtrl.cpp
 *
 *  @brief implementation of preview for lens calibration gui
 *
 *  @author T. Modes
 *
 */

/*  This program is free software; you can redistribute it and/or
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
#include "base_wx/platform.h"
#include "LensCalImageCtrl.h"
#include "vigra/transformimage.hxx"
#include "nona/RemappedPanoImage.h"
#include "nona/ImageRemapper.h"
#include "LensCalApp.h"

BEGIN_EVENT_TABLE(LensCalImageCtrl, wxPanel)
    EVT_SIZE(LensCalImageCtrl::Resize)
    EVT_PAINT(LensCalImageCtrl::OnPaint)
    EVT_MOUSE_EVENTS(LensCalImageCtrl::OnMouseEvent)
END_EVENT_TABLE()

// init some values
LensCalImageCtrl::LensCalImageCtrl() : wxPanel()
{
    m_showLines=true;
    m_imageLines=NULL;
    m_edge.Create(0,0,true);
    m_previewMode=mode_original;
    m_projection=HuginBase::SrcPanoImage::RECTILINEAR;
    m_focallength=30;
    m_cropfactor=1;
    m_a=0;
    m_b=0;
    m_c=0;
    m_d=0;
    m_e=0;
};

const LensCalImageCtrl::LensCalPreviewMode LensCalImageCtrl::GetMode()
{
    return m_previewMode;
};

void LensCalImageCtrl::OnMouseEvent(wxMouseEvent &e)
{
    if(e.Entering() || e.Leaving())
    {
        e.Skip();
        return;
    };
    if(!e.LeftIsDown() && !e.RightIsDown())
    {
        e.Skip();
        return;
    };
    if(m_imageLines==NULL)
    {
        return;
    };
    vigra::Point2D pos(e.GetPosition().x,e.GetPosition().y);
    HuginLines::Lines lines=m_imageLines->GetLines();
    if(lines.empty())
        return;
    int found_line=-1;
    if(m_previewMode==mode_corrected)
    {
        HuginBase::PTools::Transform trans;
        trans.createTransform(m_panoimage,m_opts);
        double x;
        double y;
        if(!trans.transformImgCoord(x,y,pos.x,pos.y))
            return;
        pos.x=x;
        pos.y=y;
    }
    else
    {
        pos.x=hugin_utils::roundi(pos.x/m_scale);
        pos.y=hugin_utils::roundi(pos.y/m_scale);
    };

    //find line which is nearest the clicked position
    std::vector<double> min_distance(lines.size(),10000);
    double shortest_distance=10000;
    for(unsigned int i=0;i<lines.size();i++)
    {
        if(lines[i].status==HuginLines::valid_line || lines[i].status==HuginLines::valid_line_disabled)
        {
            for(unsigned int j=0;j<lines[i].line.size();j++)
            {
                double distance=(lines[i].line[j]-pos).magnitude();
                if(distance<min_distance[i])
                    min_distance[i]=distance;
            };
        };
        if(min_distance[i]<shortest_distance)
        {
            shortest_distance=min_distance[i];
            if(shortest_distance<50)
            {
                found_line=i;
            };
        };
    };
    if(found_line==-1)
    {
        return;
    };
    if(e.LeftIsDown())
    {
        lines[found_line].status=HuginLines::valid_line_disabled;
    }
    else
    {
        lines[found_line].status=HuginLines::valid_line;
    };
    m_imageLines->SetLines(lines);
    wxGetApp().GetLensCalFrame()->UpdateListString(m_imageIndex);
    DrawView();
    Refresh();
};

void LensCalImageCtrl::DrawView()
{
    if(m_imageLines==NULL)
        return;
    if(m_previewMode==mode_corrected)
    {
        m_display_img.Create(m_remapped_img.GetWidth(), m_remapped_img.GetHeight());
    }
    else
    {
        m_display_img.Create(m_scaled_img.GetWidth(), m_scaled_img.GetHeight());
    };
    wxMemoryDC memDC(m_display_img);
    HuginBase::PTools::Transform trans;
    // copy resized image into buffer
    if(m_previewMode==mode_corrected)
    {
        trans.createInvTransform(m_panoimage,m_opts);
        memDC.DrawBitmap(m_remapped_img,0,0,false);
    }
    else
    {
        memDC.DrawBitmap(m_scaled_img,0,0,false);
    };
    if(m_showLines)
    {
        HuginLines::Lines lines=m_imageLines->GetLines();
        for(unsigned int i=0;i<lines.size();i++)
        {
            if(lines[i].line.size()<2)
                continue;
            switch(lines[i].status)
            {
                case HuginLines::valid_line:
                    memDC.SetPen(wxPen(wxColour(0,255,0), 1, wxSOLID));
                    break;
                case HuginLines::valid_line_disabled:
                    memDC.SetPen(wxPen(wxColour(255,0,0), 1, wxSOLID));
                    break;
                default:
                    memDC.SetPen(wxPen(wxColour(128,128,128), 1, wxSOLID));
                    break;
            };
            for(unsigned int j=0;j<lines[i].line.size()-1;j++)
            {
                int x1,y1,x2,y2;
                if(m_previewMode==mode_corrected)
                {
                    double x,y;
                    if(!trans.transformImgCoord(x,y,lines[i].line[j].x,lines[i].line[j].y))
                        continue;
                    x1=hugin_utils::roundi(x);
                    y1=hugin_utils::roundi(y);
                    if(!trans.transformImgCoord(x,y,lines[i].line[j+1].x,lines[i].line[j+1].y))
                        continue;
                    x2=hugin_utils::roundi(x);
                    y2=hugin_utils::roundi(y);
                }
                else
                {
                    x1=hugin_utils::roundi(m_scale*lines[i].line[j].x);
                    y1=hugin_utils::roundi(m_scale*lines[i].line[j].y);
                    x2=hugin_utils::roundi(m_scale*lines[i].line[j+1].x);
                    y2=hugin_utils::roundi(m_scale*lines[i].line[j+1].y);
                };
                memDC.DrawLine(x1,y1,x2,y2);
            };
        };
    };
    memDC.SelectObject(wxNullBitmap);
};

void LensCalImageCtrl::Resize( wxSizeEvent & e )
{
    if(m_imageLines==NULL || !m_img.Ok() || (m_previewMode==mode_edge && !m_edge.Ok()))
    {
        m_scaled_img.Create(0,0);
        m_display_img.Create(0,0);
        Refresh(true);
        return;
    }
    int x = GetSize().x;
    int y = GetSize().y;
    // scale to fit the window
    int new_width;
    int new_height;

    float r_img = (float)m_img.GetWidth() / (float)m_img.GetHeight();
    float r_window = (float)x/(float)y;
    if ( r_img > r_window )
    {
        m_scale = (float)x / m_img.GetWidth();
        new_width =  x;
        new_height = hugin_utils::roundi(m_scale * m_img.GetHeight());
    }
    else
    {
        m_scale = (float)y / m_img.GetHeight();
        new_height = y;
        new_width = hugin_utils::roundi(m_scale * m_img.GetWidth());
    }
    switch(m_previewMode)
    {
        case mode_original:
            m_scaled_img = wxBitmap(m_img.Scale (new_width, new_height));
            break;
        case mode_edge:
            m_scaled_img = wxBitmap(m_edge.Scale(new_width,new_height,wxIMAGE_QUALITY_HIGH));
            break;
        case mode_corrected:
            GenerateRemappedImage(new_width,new_height);
            break;
    };
    // draw new view into offscreen buffer
    DrawView();
    // eventually update the view
    Refresh(false);
};

// Define the repainting behaviour
void LensCalImageCtrl::OnPaint(wxPaintEvent & dc)
{
    wxPaintDC paintDC(this);
    if ( m_display_img.Ok() )
    {
        paintDC.DrawBitmap(m_display_img, 0,0, FALSE);
    }
}

void LensCalImageCtrl::SetImage(ImageLineList* newList, unsigned int newIndex)
{
    m_imageLines=newList;
    m_imageIndex=newIndex;
    std::string filename(newList->GetFilename().mb_str(HUGIN_CONV_FILENAME));
    ImageCache::EntryPtr img = ImageCache::getInstance().getImage(filename);
    m_img = imageCacheEntry2wxImage(img);
    SetEdgeImage();

    wxSizeEvent e;
    Resize (e);
}

void LensCalImageCtrl::SetEmptyImage()
{
    m_imageLines=NULL;
    m_img.Create(0,0,true);
    m_edge.Create(0,0,true);
    m_remapped_img.Create(0,0,true);

    wxSizeEvent e;
    Resize(e);
};

void LensCalImageCtrl::SetShowLines(bool showLines)
{
    m_showLines=showLines;
    // draw new view into offscreen buffer
    DrawView();
    // eventually update the view
    Refresh(false);
};

void LensCalImageCtrl::SetMode(const LensCalPreviewMode newMode)
{
    m_previewMode=newMode;
    wxSizeEvent e;
    Resize (e);
    Refresh(true);
};

void LensCalImageCtrl::SetLens(const HuginBase::SrcPanoImage::Projection newProjection,const double newFocallength, const double newCropfactor)
{
    m_projection=newProjection;
    m_focallength=newFocallength;
    m_cropfactor=newCropfactor;
};

void LensCalImageCtrl::SetLensDistortions(const double newA, const double newB, const double newC, const double newD, const double newE)
{
    m_a=newA;
    m_b=newB;
    m_c=newC;
    m_d=newD;
    m_e=newE;
    if(m_previewMode==mode_corrected)
    {
        wxSizeEvent e;
        Resize(e);
    };
};

vigra::RGBValue<vigra::UInt8> gray2RGB(vigra::UInt8 const& v)
{
    return vigra::RGBValue<vigra::UInt8>(v,v,v);
}  

void LensCalImageCtrl::SetEdgeImage()
{
    vigra::BImage* edgeImage=m_imageLines->GetEdgeImage();
    if(edgeImage->width()>0 && edgeImage->height()>0)
    {
        // we need to convert to RGB
        m_edgeImage.resize(edgeImage->width(),edgeImage->height());
        vigra::transformImage(srcImageRange(*edgeImage), destImage(m_edgeImage), &gray2RGB);
        m_edge.SetData((unsigned char*)m_edgeImage.data(),m_edgeImage.width(),m_edgeImage.height(),true);
    }
    else
    {
        m_edgeImage.resize(0,0);
        m_edge.Create(0,0,true);
    };
    wxSizeEvent e;
    Resize(e);
};

void LensCalImageCtrl::GenerateRemappedImage(const unsigned int newWidth,const unsigned int newHeight)
{
    PT::RemappedPanoImage<vigra::BRGBImage,vigra::BImage>* remapped=new PT::RemappedPanoImage<vigra::BRGBImage,vigra::BImage>;
    AppBase::MultiProgressDisplay* progress=new AppBase::DummyMultiProgressDisplay();
    //generate SrcPanoImage with current settings
    m_panoimage=*(m_imageLines->GetPanoImage());
    m_panoimage.setProjection(m_projection);
    m_panoimage.setExifFocalLength(m_focallength);
    m_panoimage.setExifCropFactor(m_cropfactor);
    m_panoimage.setExposureValue(0);
    m_panoimage.setVar("a",m_a);
    m_panoimage.setVar("b",m_b);
    m_panoimage.setVar("c",m_c);
    m_panoimage.setVar("d",m_d);
    m_panoimage.setVar("e",m_e);
    double hfov=HuginBase::SrcPanoImage::calcHFOV(m_panoimage.getProjection(),m_panoimage.getExifFocalLength(),m_panoimage.getExifCropFactor(),m_panoimage.getSize());
    m_panoimage.setHFOV(hfov);

    std::string filename(m_imageLines->GetFilename().mb_str(HUGIN_CONV_FILENAME));
    ImageCache::EntryPtr img = ImageCache::getInstance().getImage(filename);
    //fill options with current settings
    switch(m_projection)
    {
        case HuginBase::SrcPanoImage::RECTILINEAR:
            m_opts.setProjection(HuginBase::PanoramaOptions::RECTILINEAR);
            break;
        case HuginBase::SrcPanoImage::PANORAMIC:
            m_opts.setProjection(HuginBase::PanoramaOptions::CYLINDRICAL);
            break;
        case HuginBase::SrcPanoImage::CIRCULAR_FISHEYE:
        case HuginBase::SrcPanoImage::FULL_FRAME_FISHEYE:
            m_opts.setProjection(HuginBase::PanoramaOptions::FULL_FRAME_FISHEYE);
            break;
        case HuginBase::SrcPanoImage::EQUIRECTANGULAR:
            m_opts.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
            break;
        case HuginBase::SrcPanoImage::FISHEYE_ORTHOGRAPHIC:
            m_opts.setProjection(HuginBase::PanoramaOptions::ORTHOGRAPHIC);
            break;
        case HuginBase::SrcPanoImage::FISHEYE_STEREOGRAPHIC:
            m_opts.setProjection(HuginBase::PanoramaOptions::STEREOGRAPHIC);
            break;
        case HuginBase::SrcPanoImage::FISHEYE_EQUISOLID:
            m_opts.setProjection(HuginBase::PanoramaOptions::EQUISOLID);
            break;
        case HuginBase::SrcPanoImage::FISHEYE_THOBY:
            m_opts.setProjection(HuginBase::PanoramaOptions::THOBY_PROJECTION);
            break;
    };
    m_opts.setHeight(newHeight);
    m_opts.setWidth(newWidth,false);
    m_opts.setHFOV(m_panoimage.getHFOV(),false);
    m_opts.setROI(vigra::Rect2D(0,0,m_opts.getWidth(),m_opts.getHeight()));
    //now remap image
    remapped->setPanoImage(m_panoimage,m_opts, m_opts.getROI());
    remapped->remapImage(vigra::srcImageRange(*(img->get8BitImage())),vigra_ext::INTERP_CUBIC,*progress);
    m_remappedImage=remapped->m_image;
    m_remapped_img.SetData((unsigned char*)m_remappedImage.data(),m_remappedImage.width(),m_remappedImage.height(),true);
    delete remapped;
    delete progress;

};

IMPLEMENT_DYNAMIC_CLASS(LensCalImageCtrl, wxPanel)

LensCalImageCtrlXmlHandler::LensCalImageCtrlXmlHandler() : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *LensCalImageCtrlXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, LensCalImageCtrl)
    cp->Create(m_parentAsWindow, GetID(), GetPosition(), GetSize(), GetStyle(wxT("style")), GetName());
    SetupWindow(cp);
    return cp;
}

bool LensCalImageCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("LensCalCanvas"));
}

IMPLEMENT_DYNAMIC_CLASS(LensCalImageCtrlXmlHandler, wxXmlResourceHandler)
