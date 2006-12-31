// -*- c-basic-offset: 4 -*-

/** @file HFOVDialog.cpp
 *
 *  @brief implementation of HFOVDialog Class
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

#include "common/wxPlatform.h"
#include "hugin/huginApp.h"
#include "hugin/HFOVDialog.h"
#include "hugin/MainFrame.h"
#include "hugin/LensPanel.h"

using namespace PT;
using namespace std;
using namespace utils;


BEGIN_EVENT_TABLE(HFOVDialog, wxDialog)
    EVT_CHOICE (XRCID("lensdlg_type_choice"),HFOVDialog::OnTypeChanged)
    EVT_TEXT ( XRCID("lensdlg_cropfactor_text"), HFOVDialog::OnCropFactorChanged )
    EVT_TEXT ( XRCID("lensdlg_hfov_text"), HFOVDialog::OnHFOVChanged )
    EVT_TEXT ( XRCID("lensdlg_focallength_text"), HFOVDialog::OnFocalLengthChanged )
    EVT_BUTTON( XRCID("lensdlg_load_lens_button"), HFOVDialog::OnLoadLensParameters )
END_EVENT_TABLE()

HFOVDialog::HFOVDialog(wxWindow * parent, SrcPanoImage & srcImg, double focalLength, double cropFactor)
    : m_srcImg(srcImg), m_focalLength(focalLength), m_cropFactor(cropFactor)
{
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("dlg_focallength"));

    m_cropText = XRCCTRL(*this, "lensdlg_cropfactor_text", wxTextCtrl);
    DEBUG_ASSERT(m_cropText);

    m_hfovText = XRCCTRL(*this, "lensdlg_hfov_text", wxTextCtrl);
    DEBUG_ASSERT(m_hfovText);

    m_focalLengthText = XRCCTRL(*this, "lensdlg_focallength_text", wxTextCtrl);
    DEBUG_ASSERT(m_focalLengthText);

    m_projChoice = XRCCTRL(*this, "lensdlg_type_choice", wxChoice);
    DEBUG_ASSERT(m_projChoice);

    m_okButton = XRCCTRL(*this, "wxID_OK", wxButton);
    DEBUG_ASSERT(m_okButton);

    m_ignoreHFOV = false;
    m_ignoreCrop = false;
    m_ignoreFL = false;
    // fill fields
    wxString fn(srcImg.getFilename().c_str(), *wxConvCurrent);
    wxString message;
    message.Printf(_("No or only partial information about field of view was found in image file\n%s\n\nPlease enter the the horizontal field of view (HFOV) or the focal length and crop factor."), fn.c_str());
    XRCCTRL(*this, "lensdlg_message", wxStaticText)->SetLabel(message);

    if (m_cropFactor > 0 && m_focalLength > 0) {
        // everything is well known.. compute HFOV
        m_HFOV = calcHFOV(m_srcImg.getProjection(), m_focalLength,
                          m_cropFactor, m_srcImg.getSize());

//        m_ignoreHFOV = true;
        m_hfovText->SetValue(doubleTowxString(m_HFOV,2));
//        m_ignoreFL = true;
        m_focalLengthText->SetValue(doubleTowxString(m_focalLength,2));
//        m_ignoreCrop = true;
        m_cropText->SetValue(doubleTowxString(m_cropFactor,2));
    } else if (m_cropFactor > 0 && m_focalLength <= 0) {
        // focal length unknown
//        m_ignoreCrop = true;
        m_cropText->SetValue(doubleTowxString(m_cropFactor,2));
        m_okButton->Disable();
    } else if (m_cropFactor <= 0 && m_focalLength > 0) {
        // crop factor unknown
//        m_ignoreFL = true;
        m_focalLengthText->SetValue(doubleTowxString(m_focalLength,2));
        m_okButton->Disable();
    } else {
        // everything unknown
        // assume a crop factor of one
        m_cropFactor = 1;
        m_cropText->SetValue(wxT("1"));
        m_okButton->Disable();
    }
}

void HFOVDialog::OnTypeChanged(wxCommandEvent & e)
{
    m_srcImg.setProjection( (SrcPanoImage::Projection)m_projChoice->GetSelection() );
    if (m_cropFactor > 0 && m_focalLength > 0) {
        m_HFOV = calcHFOV(m_srcImg.getProjection(), m_focalLength,
                          m_cropFactor, m_srcImg.getSize());
        m_ignoreHFOV = true;
        m_hfovText->SetValue(doubleTowxString(m_HFOV,2));

    }
}

void HFOVDialog::OnHFOVChanged(wxCommandEvent & e)
{
    // ignore changes cause by ourself
    if (m_ignoreHFOV) {
        DEBUG_DEBUG("ignore HFOV change");
        m_ignoreHFOV = false;
        return;
    }
    wxString text = m_hfovText->GetValue();
    if (text.empty()) {
        m_okButton->Disable();
        return;
    }
    if (!str2double(text, m_HFOV)) {
        m_okButton->Disable();
        return;
    }
    DEBUG_DEBUG(m_HFOV);

    if (m_HFOV <= 0) {
        wxMessageBox(_("The horizontal field of view must be positive."));
        m_HFOV = 1;
        m_ignoreHFOV = true;
        m_hfovText->SetValue(doubleTowxString(m_HFOV,2));
        return;
    }

    if (m_srcImg.getProjection() == SrcPanoImage::RECTILINEAR && m_HFOV > 179) {
        m_HFOV=179;
    }

    if (m_cropFactor > 0) {
        // set focal length only if crop factor is known
        m_focalLength = calcFocalLength(m_srcImg.getProjection(), m_HFOV, m_cropFactor, m_srcImg.getSize());
        m_ignoreFL = true;
        m_focalLengthText->SetValue(doubleTowxString(m_focalLength,2));
    }
    m_okButton->Enable();
}

void HFOVDialog::OnFocalLengthChanged(wxCommandEvent & e)
{
    // ignore changes cause by ourself
    if (m_ignoreFL) {
        DEBUG_DEBUG("ignore focal length change");
        m_ignoreFL = false;
        return;
    }

    wxString text = m_focalLengthText->GetValue();
    if (text.empty()) {
        return;
    }
    if (!str2double(text, m_focalLength)) {
        return;
    }
    DEBUG_DEBUG(m_focalLength);

    if (m_focalLength <= 0) {
        m_ignoreFL = true;
        m_focalLength=1;
        m_focalLengthText->SetValue(doubleTowxString(m_focalLength,2));
        wxMessageBox(_("The focal length must be positive."));
    }

    if (m_cropFactor > 0) {
        m_HFOV = calcHFOV(m_srcImg.getProjection(), m_focalLength,
                          m_cropFactor, m_srcImg.getSize());
        m_ignoreHFOV = true;
        m_hfovText->SetValue(doubleTowxString(m_HFOV,2));
        m_okButton->Enable();
    }
}

void HFOVDialog::OnCropFactorChanged(wxCommandEvent & e)
{
    // ignore changes cause by ourself
    if (m_ignoreCrop) {
        DEBUG_DEBUG("ignore crop change");
        m_ignoreCrop = false;
        return;
    }

    wxString text = m_cropText->GetValue();
    if (text.empty()) {
        return;
    }
    if (!str2double(text, m_cropFactor)) {
        return;
    }

    if (m_cropFactor <= 0) {
        wxMessageBox(_("The focal length must be positive."));
        m_ignoreCrop = true;
        m_cropFactor=1;
        m_cropText->SetValue(doubleTowxString(m_cropFactor,2));
        return;
    }

    if (m_focalLength > 0) {
        m_HFOV = calcHFOV(m_srcImg.getProjection(), m_focalLength,
                          m_cropFactor, m_srcImg.getSize());
        m_ignoreHFOV = true;
        m_hfovText->SetValue(doubleTowxString(m_HFOV,2));
        m_okButton->Enable();
    }
}

void HFOVDialog::OnLoadLensParameters(wxCommandEvent & e)
{
    Lens lens;
    VariableMap vars;
    ImageOptions opts;

    if (LoadLensParametersChoose(lens, vars, opts)) {
        m_HFOV = lens.getHFOV();
        m_cropFactor = lens.getCropFactor();

        m_srcImg.setExifCropFactor(lens.getCropFactor());
        m_srcImg.setExifFocalLength(lens.getFocalLength());
        m_srcImg.setHFOV(const_map_get(vars,"v").getValue());
        m_srcImg.setProjection((SrcPanoImage::Projection) lens.getProjection());

        m_focalLength = calcFocalLength(m_srcImg.getProjection(), m_HFOV, m_cropFactor, m_srcImg.getSize());

        // geometrical distortion correction
        std::vector<double> radialDist(4);
        radialDist[0] = const_map_get(vars,"a").getValue();
        radialDist[1] = const_map_get(vars,"b").getValue();
        radialDist[2] = const_map_get(vars,"c").getValue();
        radialDist[3] = 1 - radialDist[0] - radialDist[1] - radialDist[2];
        m_srcImg.setRadialDistortion(radialDist);
        FDiff2D t;
        t.x = const_map_get(vars,"d").getValue();
        t.y = const_map_get(vars,"e").getValue();
        m_srcImg.setRadialDistortionCenterShift(t);
        t.x = const_map_get(vars,"g").getValue();
        t.y = const_map_get(vars,"t").getValue();
        m_srcImg.setShear(t);

    // vignetting
        m_srcImg.setVigCorrMode(opts.m_vigCorrMode);
        m_srcImg.setFlatfieldFilename(opts.m_flatfield);
        std::vector<double> vigCorrCoeff(4);
        vigCorrCoeff[0] = const_map_get(vars,"Va").getValue();
        vigCorrCoeff[1] = const_map_get(vars,"Vb").getValue();
        vigCorrCoeff[2] = const_map_get(vars,"Vc").getValue();
        vigCorrCoeff[3] = const_map_get(vars,"Vd").getValue();
        m_srcImg.setRadialVigCorrCoeff(vigCorrCoeff);
        t.x = const_map_get(vars,"Vx").getValue();
        t.y = const_map_get(vars,"Vy").getValue();
        m_srcImg.setRadialVigCorrCenterShift(t);

        std::vector<double> k(3);
        k[0] = const_map_get(vars,"K0a").getValue();
        k[1] = const_map_get(vars,"K1a").getValue();
        k[2] = const_map_get(vars,"K2a").getValue();
        m_srcImg.setBrightnessFactor(k);

        k[0] = const_map_get(vars,"K0b").getValue();
        k[1] = const_map_get(vars,"K1b").getValue();
        k[2] = const_map_get(vars,"K2b").getValue();
        m_srcImg.setBrightnessOffset(k);

        if (!opts.docrop) {
            m_srcImg.setCropMode(SrcPanoImage::NO_CROP);
        } else if (m_srcImg.getProjection() == SrcPanoImage::CIRCULAR_FISHEYE) {
            m_srcImg.setCropMode(SrcPanoImage::CROP_CIRCLE);
            m_srcImg.setCropRect(opts.cropRect);
        } else {
            m_srcImg.setCropMode(SrcPanoImage::CROP_RECTANGLE);
            m_srcImg.setCropRect(opts.cropRect);
        }
    }
}


SrcPanoImage HFOVDialog::GetSrcImage()
{
    m_srcImg.setExifFocalLength(m_focalLength);
    m_srcImg.setExifCropFactor(m_cropFactor);
    m_srcImg.setHFOV(m_HFOV);
    return m_srcImg;
}

double HFOVDialog::GetCropFactor()
{
    return m_cropFactor;
}

double HFOVDialog::GetFocalLength()
{
    return m_focalLength;
}
