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

#endif // LENSTOOLS_H
