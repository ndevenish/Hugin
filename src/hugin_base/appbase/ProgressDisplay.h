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

#ifndef _APPBASE_PROGRESSDISPLAY_H
#define _APPBASE_PROGRESSDISPLAY_H

#include <iostream>
#include <string>

#include <hugin_shared.h>
#include <hugin_utils/utils.h>


namespace AppBase {

/**
 *
 */
class IMPEX ProgressDisplay
{
protected:
    /** constructor */
    explicit ProgressDisplay(int maximum=0) : m_canceled(false), m_maximum(maximum), m_progress(0) {};
public:
    /* virtual destructor */
    virtual ~ProgressDisplay() {};
    /** sets the message to given string */
    void setMessage(const std::string& message, const std::string& filename="");
    /** call when a task has finished and the status message should be cleared */
    void taskFinished();
    /** updates the display, return true, if update was successfull, false if cancel was pressed */
    bool updateDisplay();
    bool updateDisplay(const std::string& message);
    bool updateDisplayValue();
    /** sets the new maximum value of the progress value */
    void setMaximum(int newMaximum);
    /** return true, if process should be canceled by user
      *  e.g. user pressed cancel button */
    bool wasCancelled();

protected:
    /** Template method, updates the display.
    *  You should override this method with your code of updating the display.
    */
    virtual void updateProgressDisplay() = 0;

protected:
    bool m_canceled;
    std::string m_message;
    std::string m_filename;
    int m_maximum;
    int m_progress;
};


/** Dummy progress display, without output
 */
class IMPEX DummyProgressDisplay : public ProgressDisplay
{
protected:
    /** update the display, does output nothing */
    virtual void updateProgressDisplay() {};
};

/** a progress display to print progress reports to a stream
 */
class IMPEX StreamProgressDisplay : public ProgressDisplay
{
    public:
        /** constructor, connect with given outputstream */
        explicit StreamProgressDisplay(std::ostream & o) : ProgressDisplay(), m_stream(o) {};
        /** update the display, print the message to stream */
        virtual void updateProgressDisplay();
    protected:
        std::ostream & m_stream;
};

}; //namespace

#endif // _H
