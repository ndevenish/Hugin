// -*- c-basic-offset: 4 -*-
/**  @file MaskLoadDialog.h
 *
 *  @brief Definition of mask load dialog
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _MASKLOADDIALOG_H
#define _MASKLOADDIALOG_H

#include "panoinc_WX.h"
#include "panoinc.h"
#include <panodata/Panorama.h>
#include <base_wx/wxImageCache.h>
#include <hugin/MaskImageCtrl.h>

/** Dialog for loading masks */
class MaskLoadDialog : public wxDialog
{
public:
    /** Constructor, read from xrc ressource */
    MaskLoadDialog(wxWindow *parent);
    virtual ~MaskLoadDialog();
    /** sets the default values  */
    void initValues(const HuginBase::SrcPanoImage image, const HuginBase::MaskPolygonVector newMask, const vigra::Size2D maskSize);
    /** return the processed mask */
    HuginBase::MaskPolygonVector getProcessedMask() const { return m_processedMask; };
    void OnSize(wxSizeEvent &e);
    void ProcessMask(wxCommandEvent &e);
    void UpdatePreviewImage();

private:
    MaskImageCtrl *m_image;

    wxRadioBox *m_maskScaleMode;
    wxRadioBox *m_maskRotateMode;
    vigra::Size2D m_imageSize;
    vigra::Size2D m_maskSize;

    HuginBase::MaskPolygonVector m_loadedMask;
    HuginBase::MaskPolygonVector m_processedMask;

    DECLARE_EVENT_TABLE()
};

#endif //_MASKLOADDIALOG_H
