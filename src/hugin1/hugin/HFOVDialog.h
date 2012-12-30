// -*- c-basic-offset: 4 -*-
/** @file HFOVDialog.h
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

#ifndef _HFOVDIALOG_H
#define _HFOVDIALOG_H

#include "panoinc.h"
#include "panoinc_WX.h"

#include "PT/Panorama.h"

/** A dialog for HFOV
 *
 *  Also allows cancellation
 */
class HFOVDialog : public wxDialog
{
public:
    /** ctor.
     */
    HFOVDialog(wxWindow * parent, PT::SrcPanoImage & srcImg, double focalLength, double cropFactor);
    /** dtor.
     */
    virtual ~HFOVDialog() {};

    PT::SrcPanoImage GetSrcImage();
    double GetCropFactor();
    double GetFocalLength();

private:

    void OnTypeChanged(wxCommandEvent & e);
    void OnHFOVChanged(wxCommandEvent & e);
    void OnFocalLengthChanged(wxCommandEvent & e);
    void OnCropFactorChanged(wxCommandEvent & e);
    void OnLoadLensParameters(wxCommandEvent & e);
    void OnOk(wxCommandEvent & e);

    wxTextCtrl * m_cropText;
    wxTextCtrl * m_focalLengthText;
    wxTextCtrl * m_hfovText;
    wxChoice   * m_projChoice;
    wxButton   * m_okButton;

    PT::SrcPanoImage m_srcImg;
    wxString m_focalLengthStr;
    double m_focalLength;
    wxString m_cropFactorStr;
    double m_cropFactor;
    wxString m_HFOVStr;
    double m_HFOV;

//    bool m_ignoreHFOV;
//    bool m_ignoreCrop;
//    bool m_ignoreFL;

    DECLARE_EVENT_TABLE()
};


#endif // _HFOVDIALOG_H
