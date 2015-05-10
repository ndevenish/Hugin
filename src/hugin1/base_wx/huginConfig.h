// -*- c-basic-offset: 4 -*-

/** @file huginConfig.h
 *
 *  @brief functions for interaction with the hugin configuration file
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _Hgn1_huginConfig_H
#define _Hgn1_huginConfig_H

#include <hugin_shared.h>
#include <panodata/Panorama.h>
#include <panoinc_WX.h>

/** gets the default project name, as defined in the preferences */
WXIMPEX wxString getDefaultProjectName(const HuginBase::Panorama & pano, const wxString filenameTemplate=wxT(""));
/** gets the default output prefix, based on filename and images in project
  * the setting is read from the preferences */
WXIMPEX wxString getDefaultOutputName(const wxString projectname, const HuginBase::Panorama & pano, const wxString filenameTemplate=wxT(""));

#endif
