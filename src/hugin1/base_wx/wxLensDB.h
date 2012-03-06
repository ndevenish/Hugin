// -*- c-basic-offset: 4 -*-
/** @file wxLensDB.h
 *
 *  @brief dialogs for loading and saving information from/to lensfun database
 *
 *  @author T. Modes
 *
 */

/*
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

#ifndef WXLENSDB_H
#define WXLENSDB_H

#include <hugin_shared.h>
#include "panoinc_WX.h"
#include "panoinc.h"

/** @brief loads the lens parameters from lensfun database and create approbiate PT::PanoCommand to apply this parameter.
    it shows a dialog to select which informations should be loaded
    @param parent parent window for display window
    @param pano panorama object from which information should be inside lensfun database
    @param images images into which parameters should be loaded from database
    @param cmd contains the PT::PanoCommand to apply
    @returns true, if right parameters could be loaded from database */
WXIMPEX bool ApplyLensDBParameters(wxWindow * parent, PT::Panorama *pano, HuginBase::UIntSet images, PT::PanoCommand*& cmd);

/** saves the lensparameter of the given HuginBase::SrcPanoImage into the lensfun database
    @param parent parent window for display dialog
    @param img HuginBase::SrcPanoImage, which contains the information which should be stored inside lensfun database
    @param includeVignetting if the vignetting correction can be selected
    @return true, if information could be stored into the database */
WXIMPEX bool SaveLensParameters(wxWindow * parent, const HuginBase::SrcPanoImage& img, bool includeVignetting=true);
/** saves the crop factor (camera) of the given HuginBase::SrcPanoImage into the lensfun database
    @param parent parent window for display dialog
    @param img HuginBase::SrcPanoImage, which contains the information which should be stored inside lensfun database
    @return true, if information could be stored into the database */
WXIMPEX bool SaveCameraCropFactor(wxWindow * parent, const HuginBase::SrcPanoImage& img);

#endif // WXLENSDB_H
