// -*- c-basic-offset: 4 -*-
/** @file wxLensDB.h
 *
 *  @brief dialogs for loading and saving information from/to lens database
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

#ifndef WXLENSDB_H
#define WXLENSDB_H

#include <hugin_shared.h>
#include "panoinc_WX.h"
#include "panoinc.h"
#include "base_wx/Command.h"

/** @brief loads the lens parameters from lens database and create approbiate PanoCommand::PanoCommand to apply this parameter.
    it shows a dialog to select which information should be loaded
    @param parent parent window for display window
    @param pano panorama object from which information should be inside lens database
    @param images images into which parameters should be loaded from database
    @param cmd contains the PanoCommand::PanoCommand to apply
    @returns true, if right parameters could be loaded from database */
WXIMPEX bool ApplyLensDBParameters(wxWindow * parent, HuginBase::Panorama *pano, HuginBase::UIntSet images, PanoCommand::PanoCommand*& cmd);

/** saves the lensparameter of the given HuginBase::SrcPanoImage into the lens database
    @param parent parent window for display dialog
    @param img HuginBase::SrcPanoImage, which contains the information which should be stored inside lens database
    @param includeVignetting if the vignetting correction can be selected
    @return true, if information could be stored into the database */
WXIMPEX bool SaveLensParameters(wxWindow * parent, const HuginBase::SrcPanoImage& img, bool includeVignetting=true);

#endif // WXLENSDB_H
