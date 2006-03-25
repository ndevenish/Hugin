// -*- c-basic-offset: 4 -*-
/** @file PreferencesDialog.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
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

#ifndef _VIG_CORR_DIALOG_H
#define _VIG_CORR_DIALOG_H

#include "panoinc.h"
#include "panoinc_WX.h"

#include "common/utils.h"
#include "hugin/Plot2D.h"

class VigPlotCurve
{
public:
    VigPlotCurve(const std::vector<double> & coeff) 
     : m_coeff(coeff) 
    {
        assert(coeff.size() == 4);
    }

    VigPlotCurve() : m_coeff(4) 
    {
        m_coeff[0] = 1;
        m_coeff[1] = 0;
        m_coeff[2] = 0;
        m_coeff[3] = 0;
    }

    void setCoeff(const std::vector<double> & coeff)
    {
        assert(coeff.size() == 4);
        m_coeff = coeff;
    }

    double operator()( double x)
    {
        double x2 = x*x;
        return m_coeff[0]+x2*m_coeff[1]+x2*x2*m_coeff[2]+x2*x2*x2*m_coeff[3];
    }
private:
    std::vector<double> m_coeff;
};

/**
 *  Dialog for vignetting correction settings
 */
class VigCorrDialog : public wxFrame, public PT::PanoramaObserver
{
public:

    /** ctor.
     */
    VigCorrDialog(wxWindow *parent, PT::Panorama & pano, unsigned int imgNr);

    /** dtor.
     */
    virtual ~VigCorrDialog();

    /** Config to Window*/
    void UpdateDisplayData();

    /** Window to Panorama */
    bool UpdatePanorama();

    void panoramaChanged(PT::Panorama &pano);

protected:
    void OnOk(wxCommandEvent & e);
    void OnApply(wxCommandEvent & e);
    void OnCancel(wxCommandEvent & e);
    void OnFlatfieldSelect(wxCommandEvent & e);
    void OnEstimate(wxCommandEvent & e);

    PT::Panorama & m_pano;
    unsigned int m_imgNr;

    wxRadioBox * m_corrModeRBB;

    wxRadioButton * m_corrFlatRB;
    wxRadioButton * m_corrPolyRB;

    Plot2DWindow * m_plot;

    wxTextCtrl * m_flatEdit;
    wxTextCtrl * m_coef0Edit;
    wxTextCtrl * m_coef1Edit;
    wxTextCtrl * m_coef2Edit;
    wxTextCtrl * m_coef3Edit;
    wxTextCtrl * m_coefxEdit;
    wxTextCtrl * m_coefyEdit;
    private:
	
    DECLARE_EVENT_TABLE()

};



#endif // _PREFERENCESDIALOG_H
