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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef LENSTOOLS_H
#define LENSTOOLS_H

#include <hugin_shared.h>
#include "panoinc_WX.h"
#include "panoinc.h"

/** Fills a wxControlWithItem with all input projection formats, 
  * the client data contains the associated projection number */
WXIMPEX void FillLensProjectionList(wxControlWithItems* list);
/** Selects the given projection in the given list item */
WXIMPEX void SelectProjection(wxControlWithItems* list,size_t new_projection);
/** Returns the selected projection number from list */
WXIMPEX size_t GetSelectedProjection(wxControlWithItems* list);

/** save the lens parameters of the image to a lens file named filename */
WXIMPEX void SaveLensParameters(const wxString filename, HuginBase::Panorama* pano, unsigned int imgNr);

/** applies lens parameter from user selected file to pano using GlobalCmdHist
  *  @param parent parent window for showing message boxes
  *  @param pano Panorama in which the lens data should read 
  *  @param images images for which the lens parameters should be set, check image sizes before running this function
  *  @param command pointer to PT::PanoCommand for insertion into command history
  *  @returns true if sucessful loaded lens parameters
  */
WXIMPEX bool ApplyLensParameters(wxWindow * parent, PT::Panorama *pano, HuginBase::UIntSet images,PT::PanoCommand*& command);
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


#endif // LENSTOOLS_H
