// -*- c-basic-offset: 4 -*-
/** @file nona/StitcherOptions.h
 *
 *  Helper class for storing different options
 *
 *  @author T. Modes
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

#ifndef _NONA_STITCHEROPTIONS_H
#define _NONA_STITCHEROPTIONS_H

#include <map>
#include <string>
#include <hugin_shared.h>

namespace HuginBase
{
namespace Nona
{

typedef std::map<std::string, std::string> AdvancedOptions;

/** check if given option is saved and return its boolean value, otherwise return defaultValue */
IMPEX bool GetAdvancedOption(const AdvancedOptions& opts, const std::string& name, const bool defaultValue);
IMPEX std::string GetAdvancedOption(const AdvancedOptions& opts, const std::string& name, const std::string& defaultValue = std::string(""));
IMPEX float GetAdvancedOption(const AdvancedOptions& opts, const std::string& name, const float defaultValue);

/** store the option with name in AdvancedOptions*/
IMPEX void SetAdvancedOption(AdvancedOptions& opts, const std::string& name, const bool value);
IMPEX void SetAdvancedOption(AdvancedOptions& opts, const std::string& name, const std::string& value);
IMPEX void SetAdvancedOption(AdvancedOptions& opts, const std::string& name, const float value);

} // namespace Nona
} // namespace HuginBase

#endif // _NONA_STITCHEROPTIONS_H
