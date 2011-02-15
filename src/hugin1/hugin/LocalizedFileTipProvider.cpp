// -*- c-basic-offset: 4 -*-

/** @file LocalizedFileTipProvider.cpp
 *
 *  @brief FileTipProvider that uses gettext to translate the tips
 *
 *  Based on wxFileTipProvider shipped with wxWidgets.
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>
#include "hugin_utils/utils.h"
#include "panoinc_WX.h"

#include "hugin/LocalizedFileTipProvider.h"


LocalizedFileTipProvider::LocalizedFileTipProvider(const wxString& filename,
                                                   size_t currentTip)
                 : wxTipProvider(currentTip), m_textfile(filename)
{
    m_textfile.Open();
}

wxString LocalizedFileTipProvider::GetTip()
{
    size_t count = m_textfile.GetLineCount();
    if ( !count )
    {
        return _("Tips not available, sorry!");
    }

    wxString tip;

    // Comments start with a # symbol.
    // Loop reading lines until get the first one that isn't a comment.
    // The max number of loop executions is the number of lines in the
    // textfile so that can't go into an eternal loop in the [oddball]
    // case of a comment-only tips file, or the developer has vetoed
    // them all via PreprecessTip().
    for ( size_t i=0; i < count; i++ )
    {
        // The current tip may be at the last line of the textfile, (or
        // past it, if the number of lines in the textfile changed, such
        // as changing to a different textfile, with less tips). So check
        // to see at last line of text file, (or past it)...
        if ( m_currentTip >= count )
        {
            // .. and if so, wrap back to line 0.
            m_currentTip = 0;
        }

        // Read the tip, and increment the current tip counter.
        tip = m_textfile.GetLine(m_currentTip++);

        // Allow a derived class's overrided virtual to modify the tip
        // now if so desired.
        tip = PreprocessTip(tip);

        // Break if tip isn't a comment, and isn't an empty string
        // (or only stray space characters).
        if ( !tip.StartsWith(wxT("#")) && (tip.Trim() != wxEmptyString) )
        {
            break;
        }
    }

    // If tip starts with '_(', then it is a gettext string of format
    // _("My \"global\" tip text") so first strip off the leading '_("'...
    if ( tip.StartsWith(wxT("_(\"" ), &tip))
    {
        //...and strip off the trailing '")'...
        tip = tip.BeforeLast(wxT('\"'));
        // ...and replace escaped quotes
        tip.Replace(wxT("\\\""), wxT("\""));

        DEBUG_DEBUG("Tip before translation " << tip);
        // translate tip
        tip = wxGetTranslation(tip);
        DEBUG_DEBUG("Tip after translation " << tip);
    }

    return tip;
}

#ifdef ThisNeverHappens
// provide some translatable strings for tip window
    wxLogMessage(_("&Next Tip"));
    wxLogMessage(_("&Show tips at startup"));
    wxLogMessage(_("Tip of the Day"));
    wxLogMessage(_("&Close"));
    wxLogMessaeg(_("Did you know..."));
#endif
