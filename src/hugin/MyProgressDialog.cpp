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

#include "hugin/MyProgressDialog.h"

#include "PT/PTOptimise.h"

// So that the translation is captured
#define ELAPSED_TIME _("Elapsed time : ")

void MyProgressDialog::updateProgressDisplay()
{
    wxString msg;
    // build the message:
    for (std::vector<utils::ProgressTask>::iterator it = tasks.begin();
         it != tasks.end(); ++it)
    {
        wxString cMsg;
        if (it->getProgress() > 0) {
            cMsg.Printf(wxT("%s: %s [%3.0f%%]\n"),
                        wxString(it->getShortMessage().c_str(), *wxConvCurrent).c_str(),
                        wxString(it->getMessage().c_str(), *wxConvCurrent).c_str(),
                        100 * it->getProgress());
        } else {
            cMsg.Printf(wxT("%s %s\n"),
	                wxString(it->getShortMessage().c_str(), *wxConvCurrent).c_str(),
                        wxString(it->getMessage().c_str(), *wxConvCurrent).c_str());
        }
        // append to main message
        msg.Append(cMsg);
    }
    int percentage = 0;
    if (tasks.size() > 0 && tasks.front().measureProgress) {
        percentage = (int) (tasks.front().getProgress() * 100.0);
    }
    if (!Update(percentage, msg)) {
        abortOperation();
    }

    // finally redraw
	Layout();
}

void OptProgressDialog::abortOperation()
{
#if PT_CUSTOM_OPT
    PTools::stopOptimiser();
#endif
}


