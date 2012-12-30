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
#include "panoinc.h"

#include "base_wx/wxPlatform.h"
#include "base_wx/platform.h"
#include "base_wx/LensTools.h"
#include "hugin/huginApp.h"
#include "hugin/HFOVDialog.h"
#include "hugin/MainFrame.h"

using namespace PT;
using namespace std;
using namespace hugin_utils;

BEGIN_EVENT_TABLE(HFOVDialog, wxDialog)
    EVT_CHOICE (XRCID("lensdlg_type_choice"),HFOVDialog::OnTypeChanged)
    EVT_TEXT ( XRCID("lensdlg_cropfactor_text"), HFOVDialog::OnCropFactorChanged )
    EVT_TEXT ( XRCID("lensdlg_hfov_text"), HFOVDialog::OnHFOVChanged )
    EVT_TEXT ( XRCID("lensdlg_focallength_text"), HFOVDialog::OnFocalLengthChanged )
    EVT_BUTTON( XRCID("lensdlg_load_lens_button"), HFOVDialog::OnLoadLensParameters )
    EVT_BUTTON ( wxID_OK, HFOVDialog::OnOk)
END_EVENT_TABLE()

HFOVDialog::HFOVDialog(wxWindow * parent, SrcPanoImage & srcImg, double focalLength, double cropFactor)
    : m_srcImg(srcImg), m_focalLength(focalLength), m_cropFactor(cropFactor)
{
    m_HFOV = srcImg.getHFOV();
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("dlg_focallength"));

    m_cropText = XRCCTRL(*this, "lensdlg_cropfactor_text", wxTextCtrl);
    DEBUG_ASSERT(m_cropText);

    m_hfovText = XRCCTRL(*this, "lensdlg_hfov_text", wxTextCtrl);
    DEBUG_ASSERT(m_hfovText);

    m_focalLengthText = XRCCTRL(*this, "lensdlg_focallength_text", wxTextCtrl);
    DEBUG_ASSERT(m_focalLengthText);

    m_projChoice = XRCCTRL(*this, "lensdlg_type_choice", wxChoice);
    DEBUG_ASSERT(m_projChoice);
    FillLensProjectionList(m_projChoice);

    m_okButton = XRCCTRL(*this, "wxID_OK", wxButton);
    DEBUG_ASSERT(m_okButton);

    // fill fields
    wxString fn(srcImg.getFilename().c_str(), HUGIN_CONV_FILENAME);
    wxString message;
    message.Printf(_("No or only partial information about field of view was found in image file\n%s\n\nPlease enter the horizontal field of view (HFOV) or the focal length and crop factor."), fn.c_str());
    XRCCTRL(*this, "lensdlg_message", wxStaticText)->SetLabel(message);
    SelectProjection(m_projChoice,m_srcImg.getProjection());

    if (m_cropFactor > 0 && m_focalLength > 0) {
        // everything is well known.. compute HFOV
        m_HFOV = calcHFOV(m_srcImg.getProjection(), m_focalLength,
                          m_cropFactor, m_srcImg.getSize());

        m_HFOVStr = doubleTowxString(m_HFOV,2);
        m_hfovText->SetValue(m_HFOVStr);
        m_focalLengthStr = doubleTowxString(m_focalLength,2);
        m_focalLengthText->SetValue(m_focalLengthStr);
        m_cropFactorStr = doubleTowxString(m_cropFactor,2);
        m_cropText->SetValue(m_cropFactorStr);
    } else if (m_cropFactor > 0 && m_focalLength <= 0) {
        // focal length unknown
        m_cropFactorStr = doubleTowxString(m_cropFactor,2);
        m_cropText->SetValue(m_cropFactorStr);
        m_okButton->Disable();
    } else if (m_cropFactor <= 0 && m_focalLength > 0) {
        // crop factor unknown
        m_focalLengthStr = doubleTowxString(m_focalLength,2);
        m_focalLengthText->SetValue(m_focalLengthStr);
        m_okButton->Disable();
    } else {
        // everything unknown
        // assume a crop factor of one
        m_cropFactor = 1;
        m_cropFactorStr = doubleTowxString(m_cropFactor,2);
        m_cropText->SetValue(m_cropFactorStr);
        m_okButton->Disable();
    }
    // set a proper size for this dialog
    this->GetSizer()->SetSizeHints(this);
}

void HFOVDialog::OnTypeChanged(wxCommandEvent & e)
{
    SrcPanoImage::Projection new_proj=(SrcPanoImage::Projection)(GetSelectedProjection(m_projChoice));
    DEBUG_DEBUG("new type: " << new_proj);
    m_srcImg.setProjection(new_proj);
    if (m_cropFactor > 0 && m_focalLength > 0) {
        m_HFOV = calcHFOV(m_srcImg.getProjection(), m_focalLength,
                          m_cropFactor, m_srcImg.getSize());
        m_HFOVStr = doubleTowxString(m_HFOV,2);
        m_hfovText->SetValue(m_HFOVStr);
    }
}

void HFOVDialog::OnHFOVChanged(wxCommandEvent & e)
{
    wxString text = m_hfovText->GetValue();
    DEBUG_DEBUG("state: " <<  m_HFOVStr.mb_str(wxConvLocal) << ", change:" << text.mb_str(wxConvLocal));
    DEBUG_DEBUG("cmd str: " << e.GetString().mb_str(wxConvLocal));
    if (text.empty()) {
        // ignore all empty hfov changes
        return;
    }
    // ignore changes caused by ourself
    if (m_hfovText->GetValue() == m_HFOVStr) {
        DEBUG_DEBUG("ignoring programatic HFOV change");
        return;
    }

    // accept change
    m_HFOVStr = text;

    if (text.empty()) {
        m_HFOV = 0;
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
        m_HFOV = 50;
        m_HFOVStr = doubleTowxString(m_HFOV,2);
        m_hfovText->SetValue(m_HFOVStr);
        return;
    }

    if (m_srcImg.getProjection() == SrcPanoImage::RECTILINEAR && m_HFOV > 179) {
        DEBUG_DEBUG("HFOV " << m_HFOV << " too big, resetting to 179");
        m_HFOV=179;
        m_HFOVStr = doubleTowxString(m_HFOV,2);
        m_hfovText->SetValue(m_HFOVStr);
    }

    if (m_cropFactor > 0) {
        // set focal length only if crop factor is known
        m_focalLength = calcFocalLength(m_srcImg.getProjection(), m_HFOV, m_cropFactor, m_srcImg.getSize());
        m_focalLengthStr = doubleTowxString(m_focalLength,2);
        m_focalLengthText->SetValue(m_focalLengthStr);
    }
    m_okButton->Enable();
}

void HFOVDialog::OnFocalLengthChanged(wxCommandEvent & e)
{
    wxString text = m_focalLengthText->GetValue();
    DEBUG_DEBUG(m_focalLengthStr.mb_str(wxConvLocal) << " => " << text.mb_str(wxConvLocal));
    // ignore changes caused by ourself
    if (m_focalLengthText->GetValue() == m_focalLengthStr) {
        DEBUG_DEBUG("ignore focal length change");
        return;
    }
    // accept change
    m_focalLengthStr = text;

    if (text.empty()) {
        m_focalLength = 0;
        return;
    }
    if (!str2double(text, m_focalLength)) {
        return;
    }
    DEBUG_DEBUG(m_focalLength);

    // ignore leading zeros..
    if (m_focalLength == 0) {
        return;
    }
    if (m_focalLength <= 0) {
        m_focalLength=1;
        m_focalLengthStr = doubleTowxString(m_focalLength,2);
        m_focalLengthText->SetValue(m_focalLengthStr);
        wxMessageBox(_("The focal length must be positive."));
    }

    if (m_cropFactor > 0) {
        // calculate HFOV.
        m_HFOV = calcHFOV(m_srcImg.getProjection(), m_focalLength,
                          m_cropFactor, m_srcImg.getSize());
        m_HFOVStr = doubleTowxString(m_HFOV,2);
        m_hfovText->SetValue(m_HFOVStr);
        m_okButton->Enable();
    }
}

void HFOVDialog::OnCropFactorChanged(wxCommandEvent & e)
{
    // ignore changesd cause by ourself
    wxString text = m_cropText->GetValue();
    DEBUG_DEBUG(m_cropFactorStr.mb_str(wxConvLocal) << " => " << text.mb_str(wxConvLocal));
    if (text == m_cropFactorStr) {
        DEBUG_DEBUG("ignore crop change");
        return;
    }
        // accept change
    m_cropFactorStr = text;

    if (text.empty()) {
        m_cropFactor = 0;
        return;
    }
    if (!str2double(text, m_cropFactor)) {
        return;
    }

    // ignore leading zeros..
    if (m_cropFactor == 0) {
        m_cropFactorStr = text;
        return;
    }

    if (m_cropFactor <= 0) {
        wxMessageBox(_("The crop factor must be positive."));
        m_cropFactor=1;
        m_cropFactorStr = doubleTowxString(m_cropFactor,2);
        m_cropText->SetValue(m_cropFactorStr);
        return;
    }

    if (m_focalLength > 0) {
        m_HFOV = calcHFOV(m_srcImg.getProjection(), m_focalLength,
                          m_cropFactor, m_srcImg.getSize());
        m_HFOVStr = doubleTowxString(m_HFOV,2);
        m_hfovText->SetValue(m_HFOVStr);
        m_okButton->Enable();
    }
}

void HFOVDialog::OnLoadLensParameters(wxCommandEvent & e)
{
    Lens lens;
    lens.setImageSize(m_srcImg.getSize());

    bool cropped=false;
    bool autoCenterCrop=false;
    vigra::Rect2D cropRect;

    if (LoadLensParametersChoose(this, lens, cropped, autoCenterCrop, cropRect)) {
        m_HFOV = lens.getHFOV();
        m_cropFactor = lens.getCropFactor();

        m_srcImg.setExifCropFactor(lens.getCropFactor());
        m_srcImg.setExifFocalLength(lens.getFocalLength());
        m_srcImg.setHFOV(const_map_get(lens.variables,"v").getValue());
        m_srcImg.setProjection((SrcPanoImage::Projection) lens.getProjection());

        m_focalLength = calcFocalLength(m_srcImg.getProjection(), m_HFOV, m_cropFactor, m_srcImg.getSize());

        // geometrical distortion correction
        std::vector<double> radialDist(4);
        radialDist[0] = const_map_get(lens.variables,"a").getValue();
        radialDist[1] = const_map_get(lens.variables,"b").getValue();
        radialDist[2] = const_map_get(lens.variables,"c").getValue();
        radialDist[3] = 1 - radialDist[0] - radialDist[1] - radialDist[2];
        m_srcImg.setRadialDistortion(radialDist);
        FDiff2D t;
        t.x = const_map_get(lens.variables,"d").getValue();
        t.y = const_map_get(lens.variables,"e").getValue();
        m_srcImg.setRadialDistortionCenterShift(t);
        t.x = const_map_get(lens.variables,"g").getValue();
        t.y = const_map_get(lens.variables,"t").getValue();
        m_srcImg.setShear(t);

    // vignetting
        std::vector<double> vigCorrCoeff(4);
        vigCorrCoeff[0] = const_map_get(lens.variables,"Va").getValue();
        vigCorrCoeff[1] = const_map_get(lens.variables,"Vb").getValue();
        vigCorrCoeff[2] = const_map_get(lens.variables,"Vc").getValue();
        vigCorrCoeff[3] = const_map_get(lens.variables,"Vd").getValue();
        m_srcImg.setRadialVigCorrCoeff(vigCorrCoeff);
        t.x = const_map_get(lens.variables,"Vx").getValue();
        t.y = const_map_get(lens.variables,"Vy").getValue();
        m_srcImg.setRadialVigCorrCenterShift(t);

//        m_srcImg.setExposureValue(const_map_get(vars,"Eev").getValue());
        m_srcImg.setWhiteBalanceRed(const_map_get(lens.variables,"Er").getValue());
        m_srcImg.setWhiteBalanceBlue(const_map_get(lens.variables,"Eb").getValue());

        std::vector<float> resp(5);
        resp[0] = const_map_get(lens.variables,"Ra").getValue();
        resp[1] = const_map_get(lens.variables,"Rb").getValue();
        resp[2] = const_map_get(lens.variables,"Rc").getValue();
        resp[3] = const_map_get(lens.variables,"Rd").getValue();
        resp[4] = const_map_get(lens.variables,"Re").getValue();
        m_srcImg.setEMoRParams(resp);

        if (!cropped)
        {
            m_srcImg.setCropMode(SrcPanoImage::NO_CROP);
        }
        else
        {
            if (m_srcImg.isCircularCrop())
            {
                m_srcImg.setCropMode(SrcPanoImage::CROP_CIRCLE);
            }
            else
            {
                m_srcImg.setCropMode(SrcPanoImage::CROP_RECTANGLE);
            };
            m_srcImg.setCropRect(cropRect);
        };

        // display in GUI
        m_focalLengthStr = doubleTowxString(m_focalLength,2);
        m_focalLengthText->SetValue(m_focalLengthStr);
        m_cropFactorStr = doubleTowxString(m_cropFactor,2);
        m_cropText->SetValue(m_cropFactorStr);
        m_HFOVStr = doubleTowxString(m_HFOV,2);
        m_hfovText->SetValue(m_HFOVStr);
        SelectProjection(m_projChoice, m_srcImg.getProjection());

        // update lens type
        m_okButton->Enable();
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

void HFOVDialog::OnOk(wxCommandEvent & e)
{
    if(m_srcImg.getProjection()==SrcPanoImage::FISHEYE_ORTHOGRAPHIC && m_HFOV>190)
    {
        if(wxMessageBox(
            wxString::Format(_("You have given a field of view of %.2f degrees.\n But the orthographic projection is limited to a field of view of 180 degress.\nDo you want still use that high value?"), m_HFOV),
#ifdef __WXMSW__
            _("Hugin"),
#else
            wxT(""),
#endif
            wxICON_EXCLAMATION | wxYES_NO)==wxNO)
        {
            return;
        };
    };
    EndModal(wxID_OK);
};