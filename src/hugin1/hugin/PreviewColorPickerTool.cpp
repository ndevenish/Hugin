// -*- c-basic-offset: 4 -*-

/** @file PreviewColorPickerTool.cpp
 *
 *  @author Thomas Modes
 *
 */

/*
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

#include <hugin/PreviewColorPickerTool.h>
#include <wx/platform.h>
#include "GLPreviewFrame.h"
#include <huginapp/ImageCache.h>
#include <photometric/ResponseTransform.h>
#include <vigra_ext/ImageTransforms.h>
#include <vigra/copyimage.hxx>
#include <vigra/inspectimage.hxx>

/** half size of color picker window size */
#define ColorPickerSize 10

using namespace HuginBase;

// we want to be notified if the user click into the panorama
void PreviewColorPickerTool::Activate()
{
    helper->NotifyMe(PreviewToolHelper::MOUSE_PRESS, this);
    helper->SetStatusMessage(_("Click on a area which should be neutral gray / white in the final panorama."));
};

// The user click, get the current position and update white balance
void PreviewColorPickerTool::MouseButtonEvent(wxMouseEvent &e)
{
    if ( e.LeftDown() && helper->IsMouseOverPano())
    {
        CalcCorrection(helper->GetMousePanoPosition());
        if(m_count>0 && m_red!=0 && m_blue!=0)
        {
            helper->GetPreviewFrame()->UpdateGlobalWhiteBalance(m_red,m_blue);
        }
        else
        {
            wxBell();
        };
    };
};

void PreviewColorPickerTool::CalcCorrection(hugin_utils::FDiff2D pos)
{
    m_red=0;
    m_blue=0;
    m_count=0;
    PT::Panorama* pano=helper->GetPanoramaPtr();
    UIntSet activeImages=pano->getActiveImages();
    if(activeImages.size()>0)
    {
        for(UIntSet::iterator it=activeImages.begin();it!=activeImages.end();it++)
        {
            //check if point is inside the image, check also all 4 corners of rectangle
            HuginBase::PTools::Transform trans;
            trans.createTransform(pano->getImage(*it),pano->getOptions());
            double x;
            double y;
            if(trans.transformImgCoord(x,y,pos.x,pos.y))
            {
                vigra::Point2D imagePos(x,y);
                if(pano->getImage(*it).isInside(imagePos) && 
                    pano->getImage(*it).isInside(imagePos + vigra::Point2D(-ColorPickerSize,-ColorPickerSize)) &&
                    pano->getImage(*it).isInside(imagePos + vigra::Point2D(-ColorPickerSize, ColorPickerSize)) &&
                    pano->getImage(*it).isInside(imagePos + vigra::Point2D( ColorPickerSize,-ColorPickerSize)) &&
                    pano->getImage(*it).isInside(imagePos + vigra::Point2D( ColorPickerSize, ColorPickerSize)) 
                   )
                {
                    CalcCorrectionForImage(*it,imagePos);
                };
            };
        };
    };
    if(m_count>0)
    {
        m_red=m_red/m_count;
        m_blue=m_blue/m_count;
    };
};

void PreviewColorPickerTool::CalcCorrectionForImage(unsigned int i,vigra::Point2D pos)
{
    const SrcPanoImage & img = helper->GetPanoramaPtr()->getImage(i);
    ImageCache::ImageCacheRGB8Ptr cacheImage8 = ImageCache::getInstance().getImage(img.getFilename())->get8BitImage();

    //copy only region to be inspected
    vigra::BRGBImage tempImage(2*ColorPickerSize,2*ColorPickerSize);
    vigra::copyImage(vigra::make_triple((*cacheImage8).upperLeft() + pos + vigra::Point2D(-ColorPickerSize,-ColorPickerSize),
                                        (*cacheImage8).upperLeft() + pos + vigra::Point2D( ColorPickerSize, ColorPickerSize),
                                        vigra::BRGBImage::Accessor()),
                     destImage(tempImage) );

    //now apply photometric corrections
    Photometric::InvResponseTransform<vigra::UInt8,double> invResponse(img);
    if (helper->GetPanoramaPtr()->getOptions().outputMode == PanoramaOptions::OUTPUT_LDR)
    {
        // select exposure and response curve for LDR output
        std::vector<double> outLut;
        vigra_ext::EMoR::createEMoRLUT(helper->GetPanoramaPtr()->getImage(0).getEMoRParams(), outLut);
        vigra_ext::enforceMonotonicity(outLut);
        invResponse.setOutput(1.0/pow(2.0,helper->GetPanoramaPtr()->getOptions().outputExposureValue), outLut,
                              255.0);
    }
    else
    {
        invResponse.setHDROutput(true,1.0/pow(2.0,helper->GetPanoramaPtr()->getOptions().outputExposureValue));
    }
    vigra::DRGBImage floatTemp(tempImage.size());
    vigra_ext::transformImageSpatial(srcImageRange(tempImage), destImage(floatTemp), invResponse, vigra::Diff2D(pos.x-ColorPickerSize,pos.y-ColorPickerSize));

    //calculate average
    vigra::FindAverage<vigra::DRGBImage::PixelType> average;
    vigra::inspectImage(srcImageRange(floatTemp), average);
    //range check
    vigra::RGBValue<double> RGBaverage=average.average();
    if(RGBaverage[0]>2 && RGBaverage[0]<253 &&
       RGBaverage[1]>2 && RGBaverage[1]<253 &&
       RGBaverage[2]>2 && RGBaverage[2]<253)
    {
        m_red+=RGBaverage[0]/RGBaverage[1];
        m_blue+=RGBaverage[2]/RGBaverage[1];
        m_count++;
    };
};
