// -*- c-basic-offset: 4 -*-
/** @file ProgressDisplay.h
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
*
*  $Id$
*
*  !! based on ProgressDisplay in utils.h 1952 
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

#include "ProgressDisplay.h"

namespace AppBase
{

void ProgressDisplay::setMessage(const std::string& message, const std::string& filename)
{
    m_message = message;
    m_filename = filename;
    updateProgressDisplay();
}

void ProgressDisplay::taskFinished()
{
    setMessage("");
}

bool ProgressDisplay::updateDisplay()
{
    return !m_canceled;
}

bool ProgressDisplay::updateDisplay(const std::string& message)
{
    setMessage(message);
    return !m_canceled;
}

bool ProgressDisplay::updateDisplayValue()
{
    if (m_progress < m_maximum)
    {
        ++m_progress;
    };
    updateProgressDisplay();
    return !m_canceled;
}

void ProgressDisplay::setMaximum(int newMaximum)
{
    m_maximum = newMaximum;
    if (m_progress > m_maximum)
    {
        m_progress = m_maximum;
    };
}

bool ProgressDisplay::wasCancelled()
{
    return m_canceled;
}

void StreamProgressDisplay::updateProgressDisplay()
{
    // TODO: check for Ctrl-C then cancelTask() ?
    if (!m_message.empty())
    {
        // don't print empty messages
        if (m_filename.empty())
        {
            m_stream << m_message << std::endl;
        }
        else
        {
            m_stream << m_message << " " << m_filename << std::endl;
        }
        m_stream.flush();
    };
}

}; //namespace
