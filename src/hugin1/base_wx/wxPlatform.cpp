// -*- c-basic-offset: 4 -*-

/** @file wxPlatform.cpp
*
*  @brief implementation of utility function
*
*  @author Pablo d'Angelo <pablo.dangelo@web.de>
*/

/*  This program is free software; you can redistribute it and/or
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

#include "wxPlatform.h"
#include <hugin_utils/utils.h>

namespace hugin_utils
{
    wxString doubleTowxString(double d, int digits)
    {
        std::string t = hugin_utils::doubleToString(d, digits);
        return wxString(t.c_str(), wxConvLocal);
    }

    bool str2double(const wxString& s, double & d)
    {
        if (!hugin_utils::stringToDouble(std::string(s.mb_str(wxConvLocal)), d))
        {
            return false;
        }
        return true;
    }

} // namespace

#ifdef __WXMSW__
  // workaround for wxWidgets bug 14888
  // see: http://trac.wxwidgets.org/ticket/14888
  // if this is fixed upstreams this workaround can be removed
void HuginCHMHelpController::DisplayHelpPage(const wxString& name)
{
    // instead of passing filename as dwData to HH_DISPLAY_TOPIC
    // we pass chmFilename::filename to pszfile 
    wxString command(GetValidFilename());
    command.Append(wxT("::"));
    command.Append(name);
    CallHtmlHelp(GetParentWindow(), command.t_str(), 0 /* =HH_DISPLAY_TOPIC */);
};
#endif
