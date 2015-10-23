// -*- c-basic-offset: 4 -*-
/** @file nona/StitcherOptions.cpp
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

#include "StitcherOptions.h"
#include "hugin_utils/utils.h"

namespace HuginBase
{
namespace Nona
{

bool GetAdvancedOption(const AdvancedOptions& opts, const std::string& name, const bool defaultValue)
{
    AdvancedOptions::const_iterator it = opts.find(name);
    if (it != opts.end())
    {
        //option is stored
        const std::string value(it->second);
        if (value == "true" || value == "1")
        {
            return true;
        }
        return false;
    }
    else
    {
        return defaultValue;
    };
};

std::string GetAdvancedOption(const AdvancedOptions& opts, const std::string& name, const std::string& defaultValue)
{
    AdvancedOptions::const_iterator it = opts.find(name);
    if (it != opts.end())
    {
        //option is stored
        return it->second;
    }
    else
    {
        return defaultValue;
    };
};

float GetAdvancedOption(const AdvancedOptions& opts, const std::string& name, const float defaultValue)
{
    AdvancedOptions::const_iterator it = opts.find(name);
    if (it != opts.end())
    {
        //option is stored
        double value;
        if (hugin_utils::stringToDouble(it->second, value))
        {
            return static_cast<float>(value);
        }
        else
        {
            return defaultValue;
        };
    }
    else
    {
        return defaultValue;
    };
};

void SetAdvancedOption(AdvancedOptions& opts, const std::string& name, const bool value)
{
    if (value)
    {
        opts[name] = "true";
    }
    else
    {
        opts[name] = "false";
    };
};

void SetAdvancedOption(AdvancedOptions& opts, const std::string& name, const std::string& value)
{
    opts[name] = value;
};

void SetAdvancedOption(AdvancedOptions& opts, const std::string& name, const float value)
{
    opts[name] = hugin_utils::doubleToString(value);
};

} // namespace Nona
} // namespace HuginBase
