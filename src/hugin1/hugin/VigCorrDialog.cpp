// -*- c-basic-offset: 4 -*-

/** @file PreferencesDialog.cpp
 *
 *  @brief implementation of VigCorrDialog Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: VigCorrDialog.cpp 1994 2007-05-09 11:57:45Z dangelo $
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
#include "common/wxPlatform.h"

#include <iostream>
#include <fstream>
#include <vigra/basicimage.hxx>
#include <vigra/basicimageview.hxx>
#include "vigra/functorexpression.hxx"

#include <PT/RemappedPanoImage.h>
#include <vigra_ext/VigQuotientEstimator.h>
#include <PT/RandomPointSampler.h>
#include <PT/PhotometricOptimizer.h>

#include "hugin/config_defaults.h"
#include "hugin/huginApp.h"
#include "hugin/VigCorrDialog.h"
#include "hugin/CommandHistory.h"
#include "base_wx/ImageCache.h"
#include "base_wx/MyProgressDialog.h"


//#define VIG_DATA_FILE "C:\\users\\padange\\temp\\huginRandPnt_all_"
#define VIG_DATA_FILE "CorrPoints.txt"

// validators are working different somehow...
//#define MY_STR_VAL(id, filter) { XRCCTRL(*this, "prefs_" #id, wxTextCtrl)->SetValidator(wxTextValidator(filter, &id)); }
//#define MY_SPIN_VAL(id) {     XRCCTRL(*this, "prefs_" #id, wxSpinCtrl)->SetValidator(wxGenericValidator(&id)); }

#define MY_STR_VAL(id, val) { XRCCTRL(*this, id, wxTextCtrl)->SetValue(val); };

#define MY_G_STR_VAL(id)  XRCCTRL(*this, id, wxTextCtrl)->GetValue()

using namespace PT;
using namespace utils;
using namespace vigra;
using namespace vigra::functor;
using namespace vigra_ext;
using namespace std;



BEGIN_EVENT_TABLE(VigCorrDialog, wxFrame)
    EVT_BUTTON(wxID_OK, VigCorrDialog::OnOk)
    EVT_BUTTON(wxID_APPLY,VigCorrDialog::OnApply)
    EVT_BUTTON(wxID_CANCEL,VigCorrDialog::OnCancel)
    EVT_BUTTON(XRCID("vig_corr_flatfield_select"), VigCorrDialog::OnFlatfieldSelect)
END_EVENT_TABLE()


VigCorrDialog::VigCorrDialog(wxWindow *parent, Panorama & pano, unsigned int imgNr)
//    : wxFrame(parent, -1, _("Preferences - hugin"))
    : m_pano(pano), m_imgNr(imgNr)
{
    DEBUG_TRACE("");

    pano.addObserver(this);

    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("vig_corr_dlg"));

#ifdef __WXMSW__
    wxIcon myIcon(MainFrame::Get()->GetXRCPath() + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(MainFrame::Get()->GetXRCPath() + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    // create xrc variables
    m_corrModeRBB = XRCCTRL(*this, "vig_corr_mode_rbbox", wxRadioBox);
    DEBUG_ASSERT(m_corrModeRBB);

    m_corrFlatRB = XRCCTRL(*this, "vig_corr_flatfield_rb", wxRadioButton);
    DEBUG_ASSERT(m_corrFlatRB);
    m_corrPolyRB = XRCCTRL(*this, "vig_corr_poly_rb", wxRadioButton);
    DEBUG_ASSERT(m_corrPolyRB);

    m_flatEdit = XRCCTRL(*this, "vig_corr_flatfile_edit", wxTextCtrl);
    DEBUG_ASSERT(m_flatEdit);

    m_coef0Edit = XRCCTRL(*this, "vig_corr_coef0_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coef0Edit);
    m_coef1Edit = XRCCTRL(*this, "vig_corr_coef1_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coef1Edit);
    m_coef2Edit = XRCCTRL(*this, "vig_corr_coef2_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coef2Edit);
    m_coef3Edit = XRCCTRL(*this, "vig_corr_coef3_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coef3Edit);

    m_coefxEdit = XRCCTRL(*this, "vig_corr_coefx_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coefxEdit);
    m_coefyEdit = XRCCTRL(*this, "vig_corr_coefy_edit", wxTextCtrl);
    DEBUG_ASSERT(m_coefxEdit);

    m_plot = new Plot2DWindow( this, -1, wxPoint(0,0), wxSize(500,200));
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("vig_corr_plot_poly"),
               m_plot);
//    m_plot->SetAxis(0,1.01, 0,1.01);

    // update display with values from panorama
    UpdateDisplayData();

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows, copy color from a child widget
    this->SetBackgroundColour(XRCCTRL(*this, "vig_corr_mode_rbbox", wxRadioBox)->GetBackgroundColour());
#endif

//    RestoreFramePosition(this, wxT("VigCorrDialog"));
}


VigCorrDialog::~VigCorrDialog()
{
    DEBUG_TRACE("begin dtor");
//    SaveFramePosition(this, wxT("VigCorrDialog"));
    m_pano.removeObserver(this);

    DEBUG_TRACE("end dtor");
}


void VigCorrDialog::OnApply(wxCommandEvent & e)
{
    UpdatePanorama();
}


void VigCorrDialog::OnOk(wxCommandEvent & e)
{
    if (UpdatePanorama()) {
        Close();
    }
}

void VigCorrDialog::OnCancel(wxCommandEvent & e)
{
    Close();
}

void VigCorrDialog::OnFlatfieldSelect(wxCommandEvent & e)
{
    wxString wildcard (_("All Image files|*.jpg;*.JPG;*.tif;*.TIF;*.tiff;*.TIFF;*.png;*.PNG;*.bmp;*.BMP;*.gif;*.GIF;*.pnm;*.PNM;*.sun;*.viff;*.hdr|JPEG files (*.jpg,*.jpeg)|*.jpg;*.JPG;*.jpeg;*.JPEG|All files (*)|*"));
    wxFileDialog dlg(this,_("Select flatfield image"),
                     wxConfigBase::Get()->Read(wxT("/flatfieldPath"),wxT("")), wxT(""),
                     wildcard,
                     wxOPEN, wxDefaultPosition);
    dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/flatfieldPath"),wxT("")));
    if (dlg.ShowModal() == wxID_OK) {
        XRCCTRL(*this, "vig_corr_flatfile_edit", wxTextCtrl)->SetValue(
		dlg.GetPath());
        wxConfig::Get()->Write(wxT("/flatfieldPath"), dlg.GetDirectory());
    }
}


void VigCorrDialog::panoramaChanged(Panorama &pano)
{
    UpdateDisplayData();
}


void VigCorrDialog::UpdateDisplayData()
{
    DEBUG_DEBUG("Updating display data");
    // get current Lens.
    if (m_imgNr >= m_pano.getNrOfImages()) {
        Close();
        return;
    }

    const PanoImage & img = m_pano.getImage(m_imgNr);
    const PT::ImageOptions & iopts = img.getOptions();

    if (iopts.m_vigCorrMode  == 0) {
        m_corrModeRBB->SetSelection(0);
    } else if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_DIV) {
        m_corrModeRBB->SetSelection(2);
    } else {
        m_corrModeRBB->SetSelection(1);
    }

    if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_FLATFIELD) {
        m_corrFlatRB->SetValue(true);
        m_corrPolyRB->SetValue(false);
    } else {
        m_corrFlatRB->SetValue(false);
        m_corrPolyRB->SetValue(true);
    }

    // update the coefficients
    unsigned int ndigits = 3;
    std::vector<double> coeff(4);
    coeff[0] = const_map_get(m_pano.getImageVariables(m_imgNr),"Va").getValue();
    coeff[1] = const_map_get(m_pano.getImageVariables(m_imgNr),"Vb").getValue();
    coeff[2] = const_map_get(m_pano.getImageVariables(m_imgNr),"Vc").getValue();
    coeff[3] = const_map_get(m_pano.getImageVariables(m_imgNr),"Vd").getValue();

    m_coef0Edit->SetValue(doubleTowxString(coeff[0],ndigits));
    m_coef1Edit->SetValue(doubleTowxString(coeff[1],ndigits));
    m_coef2Edit->SetValue(doubleTowxString(coeff[2],ndigits));
    m_coef3Edit->SetValue(doubleTowxString(coeff[3],ndigits));
    m_coefxEdit->SetValue(doubleTowxString(const_map_get(m_pano.getImageVariables(m_imgNr),"Vx").getValue(),1));
    m_coefyEdit->SetValue(doubleTowxString(const_map_get(m_pano.getImageVariables(m_imgNr),"Vy").getValue(),1));
    
    // update the flatfield filename
    m_flatEdit->SetValue(wxString (iopts.m_flatfield.c_str(), *wxConvCurrent));

    // update vignetting curve
    VigPlotCurve f(coeff);
    m_plot->Plot(f,0,1);
    if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_DIV) {
        m_plot->SetAxis(0.0, 1, 0.0, 1.2);
    } else {
        m_plot->AutoSizeAxis();
    }
}

bool VigCorrDialog::UpdatePanorama()
{
    // always apply to all images with the same lens.

    unsigned int mode(ImageOptions::VIGCORR_NONE);
    int moderbb = m_corrModeRBB->GetSelection();
    if (moderbb != 0) {
        if (moderbb == 2) {
            mode |= ImageOptions::VIGCORR_DIV;
        }
        if (m_corrPolyRB->GetValue()) {
            mode |= ImageOptions::VIGCORR_RADIAL;
        } else if(m_corrFlatRB->GetValue()) {
            mode |= ImageOptions::VIGCORR_FLATFIELD;
        }
    }

    if ( (mode & ImageOptions::VIGCORR_FLATFIELD) &&  !wxFileExists(m_flatEdit->GetValue())) {
        wxMessageBox(_("Error: could not find flatfile image file."), _("File not found"));
        return false;
    }
    std::string flat(m_flatEdit->GetValue().mb_str());

    std::vector<double> coeff(6);
    if (!str2double(m_coef0Edit->GetValue(), coeff[0]))  return false;
    if (!str2double(m_coef1Edit->GetValue(), coeff[1]))  return false;
    if (!str2double(m_coef2Edit->GetValue(), coeff[2]))  return false;
    if (!str2double(m_coef3Edit->GetValue(), coeff[3]))  return false;
    if (!str2double(m_coefxEdit->GetValue(), coeff[4]))  return false;
    if (!str2double(m_coefyEdit->GetValue(), coeff[5]))  return false;

    GlobalCmdHist::getInstance().addCommand(
            new SetVigCorrCmd(m_pano, m_pano.getImage(m_imgNr).getLensNr(),
                              mode, coeff, flat) );
    return true;
}
