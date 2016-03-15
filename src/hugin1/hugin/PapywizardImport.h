/**
 * @file PapywizardImport.h
 *
 * @brief read settings from papywizard xml file
 *  
 * @author T. Modes
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

#ifndef PAPYWIZARDIMPORT_H
#define PAPYWIZARDIMPORT_H

#include <panodata/Panorama.h>
#include <wx/string.h>

namespace Papywizard
{
/** import the settings from given filename into pano */
bool ImportPapywizardFile(const wxString& filename, HuginBase::Panorama& pano);

} // namespace Papywizard
#endif
