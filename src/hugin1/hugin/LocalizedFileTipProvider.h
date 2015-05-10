// -*- c-basic-offset: 4 -*-
/** @file LocalizedFileTipProvider.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
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

#ifndef _LOCALIZED_FILE_TIP_PROVIDER_H
#define _LOCALIZED_FILE_TIP_PROVIDER_H

#include "panoinc_WX.h"

#include <wx/tipdlg.h>


/** A tip file provider that uses gettext to translate the tips.
 *  Based on the wxTipFileProvider class which just strips
 *  _(" "), but doesn't translate the entries.
 */

class LocalizedFileTipProvider : public wxTipProvider
{
public:
    LocalizedFileTipProvider(const wxString& filename, size_t currentTip);

    virtual wxString GetTip();

private:
    wxTextFile m_textfile;
};

#endif // _LOCALIZED_FILE_TIP_PROVIDER_H
