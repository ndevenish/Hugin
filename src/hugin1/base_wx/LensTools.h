// -*- c-basic-offset: 4 -*-
/** @file LensTools.h
 *
 *  @brief some helper classes for working with lenses
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LENSTOOLS_H
#define LENSTOOLS_H

#include <hugin_shared.h>
#include "panoinc_WX.h"
#include "panoinc.h"
#include "base_wx/Command.h"

/** Fills a wxControlWithItem with all input projection formats, 
  * the client data contains the associated projection number */
WXIMPEX void FillLensProjectionList(wxControlWithItems* list);
/** Fills a wxControlWithItem with all possible blender options,
* the client data contains the associated blender mod from PanoramaOptions */
WXIMPEX void FillBlenderList(wxControlWithItems* list);

/** Selects the given value (stored in the client data) in the given list item */
WXIMPEX void SelectListValue(wxControlWithItems* list,size_t newValue);
/** Returns the client value of the selected item from list */
WXIMPEX size_t GetSelectedValue(wxControlWithItems* list);

/** Returns translated projection for given image */
WXIMPEX wxString getProjectionString(const HuginBase::SrcPanoImage& img);
/** Returns translated response type for given SrcPanoImage */
WXIMPEX wxString getResponseString(const HuginBase::SrcPanoImage& img);

/** save the lens parameters of the image to a lens file named filename */
WXIMPEX void SaveLensParameters(const wxString filename, HuginBase::Panorama* pano, unsigned int imgNr);
/** saves the lens parameters to ini files, provides all necessary dialogs */
WXIMPEX void SaveLensParametersToIni(wxWindow * parent, HuginBase::Panorama *pano, const HuginBase::UIntSet images);

/** applies lens parameter from user selected file to pano using GlobalCmdHist
  *  @param parent parent window for showing message boxes
  *  @param pano Panorama in which the lens data should read 
  *  @param images images for which the lens parameters should be set, check image sizes before running this function
  *  @param command pointer to PanoCommand::PanoCommand for insertion into command history
  *  @returns true if sucessful loaded lens parameters
  */
WXIMPEX bool ApplyLensParameters(wxWindow * parent, HuginBase::Panorama *pano, HuginBase::UIntSet images,PanoCommand::PanoCommand*& command);
/** load lens parameters from lens ini file 
  *  @param parent parent window, for showing messageboxes
  *  @param lens lens, for reading projection and lensparameters
  *  @param cropped read if cropped enabled
  *  @param autoCenterCrop read if automatic center crop
  *  @param cropRect read crop rect
  *  @returns true if lens parameters were sucessful read
  */
WXIMPEX bool LoadLensParametersChoose(wxWindow * parent, HuginBase::Lens & lens, 
    bool & cropped, bool & autoCenterCrop, vigra::Rect2D & cropRect);

/** check, if lens and stacks are correctly linked 
 *   shows message box with short information if not 
 *  @param pano Panorama which should be checked 
 *  @param allowCancel if true the message box contains also a Cancel button, if false there is only ok button 
 *  @returns true, if all conditions are satisfied, false if user selected cancel in dialog box */
WXIMPEX bool CheckLensStacks(HuginBase::Panorama* pano, bool allowCancel);

#endif // LENSTOOLS_H
