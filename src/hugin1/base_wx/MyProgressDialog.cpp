// -*- c-basic-offset: 4 -*-

/** @file MyProgressDialog.cpp
 *
 *  @brief implementation of MyProgressDialog Class
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
#include "panoinc_WX.h"

#include "MyProgressDialog.h"

// provide some more translations for strings in huginbase
#if 0
_("Sampling points");
_("Loading image:");
_("Scaling image:");
_("sampling points");
_("extracting good points");
_("Elapsed time : ")
#endif

void ProgressReporterDialog::setMessage(const std::string& message, const std::string& filename)
{
    setMessage(wxString(message.c_str(), wxConvLocal), wxString(filename.c_str(), wxConvLocal));
}

void ProgressReporterDialog::setMessage(const wxString& message, const wxString& filename)
{
    m_wxmessage = message;
    m_wxfilename = filename;
    updateProgressDisplay();
}

bool ProgressReporterDialog::updateDisplay(const wxString& message)
{
    setMessage(message);
    return !m_canceled;
}

bool ProgressReporterDialog::updateDisplayValue(const wxString& message, const wxString& filename)
{
    m_wxmessage = message;
    m_wxfilename = filename;
    return ProgressDisplay::updateDisplayValue();
}

void ProgressReporterDialog::updateProgressDisplay()
{
    wxString msg;
    if (!m_wxmessage.empty())
    {
        msg = wxGetTranslation(m_wxmessage);
        if (!m_wxfilename.empty())
        {
            msg.Append(wxT(" "));
            msg.Append(m_wxfilename);
        };
    };
    if (ProgressDisplay::m_maximum == 0)
    {
        if (!wxProgressDialog::Pulse(msg))
        {
            m_canceled = true;
        }
    }
    else
    {
        if (!wxProgressDialog::Update(m_progress * 100 / ProgressDisplay::m_maximum, msg))
        {
            m_canceled = true;
        }
    }
}
