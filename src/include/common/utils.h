// -*- c-basic-offset: 4 -*-
/** @file utils.h
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _UTILS_H
#define _UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cassert>

#include "platform.h"

#ifdef __WXMSW__
#include <wx/log.h>
#endif

// misc utility functions / macros

#ifdef __GNUC__
// the full function name is too long..
//#define DEBUG_HEADER utils::CurrentTime() << " (" << __FILE__ << ":" << __LINE__ << ") " << __PRETTY_FUNCTION__ << "()" << std::endl << "    "
#define DEBUG_HEADER utils::CurrentTime() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __func__ << "(): "
#else
#if _MSC_VER > 1300
#define DEBUG_HEADER utils::CurrentTime() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __FUNCTION__ << "(): "
#else
#define DEBUG_HEADER utils::CurrentTime() <<" (" << __FILE__ << ":" << __LINE__ << ") "  << __func__ << "(): "
#endif
#endif

// use trace function under windows, because usually there is
// no stdout under windows
//#ifdef __WXMSW__
#ifdef __WXMSW__
 #ifdef DEBUG
  // debug trace
    #define DEBUG_TRACE(msg) { std::stringstream o; o << "TRACE " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str());}
  // low level debug info
  #define DEBUG_DEBUG(msg) { std::stringstream o; o << "DEBUG " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str()); }
  // informational debug message,
  #define DEBUG_INFO(msg) { std::stringstream o; o << "INFO " << DEBUG_HEADER << msg; wxLogDebug(o.str().c_str()); }
  // major change/operation should use this
  #define DEBUG_NOTICE(msg) { std::stringstream o; o << "NOTICE " << DEBUG_HEADER << msg; wxLogMessage(o.str().c_str()); }
 #else
  #define DEBUG_TRACE(msg)
  #define DEBUG_DEBUG(msg)
  #define DEBUG_INFO(msg)
  #define DEBUG_NOTICE(msg)
 #endif

 // when an error occured, but can be handled by the same function
 #define DEBUG_WARN(msg) { std::stringstream o; o << "WARN: " << DEBUG_HEADER << msg; wxLogWarning(o.str().c_str());}
 // an error occured, might be handled by a calling function
 #define DEBUG_ERROR(msg) { std::stringstream o; o << "ERROR: " << DEBUG_HEADER << msg; wxLogError(o.str().c_str());}
// a fatal error occured. further program execution is unlikely
 #define DEBUG_FATAL(msg) { std::stringstream o; o << "FATAL: " << DEBUG_HEADER << "(): " << msg; wxLogError(o.str().c_str()); }
 #define DEBUG_ASSERT(cond) \
 do { \
     if (!(cond)) { \
         std::stringstream o; o << "ASSERTATION: " << DEBUG_HEADER << "(): " << #cond; \
         wxLogFatalError(o.str().c_str()); \
    } \
 } while(0)

#else

 #ifdef DEBUG
  // debug trace
  #define DEBUG_TRACE(msg) { std::cerr << "TRACE " << DEBUG_HEADER << msg << std::endl; }
  // low level debug info
  #define DEBUG_DEBUG(msg) { std::cerr << "DEBUG " << DEBUG_HEADER << msg << std::endl; }
  // informational debug message,
  #define DEBUG_INFO(msg) { std::cerr << "INFO " << DEBUG_HEADER << msg << std::endl; }
  // major change/operation should use this
  #define DEBUG_NOTICE(msg) { std::cerr << "NOTICE " << DEBUG_HEADER << msg << std::endl; }
 #else
  #define DEBUG_TRACE(msg)
  #define DEBUG_DEBUG(msg)
  #define DEBUG_INFO(msg)
  #define DEBUG_NOTICE(msg)
 #endif

 // when an error occured, but can be handled by the same function
 #define DEBUG_WARN(msg) { std::cerr << "WARN: " << DEBUG_HEADER << msg << std::endl; }
 // an error occured, might be handled by a calling function
 #define DEBUG_ERROR(msg) { std::cerr << "ERROR: " << DEBUG_HEADER << msg << std::endl; }
 // a fatal error occured. further program execution is unlikely
 #define DEBUG_FATAL(msg) { std::cerr << "FATAL: " << DEBUG_HEADER << "(): " << msg << std::endl; }

 #define DEBUG_ASSERT(cond) assert(cond)

#endif


namespace utils
{
    std::string CurrentTime();

    std::string doubleToString(double d);

    template <typename Target, typename Source>
    Target lexical_cast(Source arg) {

        std::stringstream interpreter;

        Target result;

        if (!(interpreter << arg) ||
            !(interpreter >> result) ||
            !(interpreter >> std::ws).eof()) {

            DEBUG_ERROR("lexical cast error");
            // cast error.  handle it somehow
            // boost guys throw an exception here
        };

        return result;

    }; // lexical cast

    class ProgressDisplay
    {
    public:
        virtual ~ProgressDisplay() {};
        /** receive notification about progress
         *
         *  @param msg message text
         *  @param progress optional progress indicator (0-100%)
         */
        virtual void progressMessage(const std::string & msg,
                                     double progress=-1) = 0;

    };

    /** desribes a subprogess task */
    struct ProgressTask
    {
        /** A progress task describes one operation
         *
         *  it consists of a one or two word heading, \p shortMessage
         *  and a longer description, \p message.
         *
         *  Progress can be set directly, or with substeps, \p subStepProgress
         *
         *  nSteps * subStepProgress = 100%.
         *  The progress is also increased by a subStep, if a lower operation
         *  completes.
         */
        ProgressTask(std::string shortMessage, std::string message,
                     double subStepProgress, double progress=0)
            : shortMessage(shortMessage), message(message),
              measureProgress(true), progress(progress),
              subStepProgress(subStepProgress), last_displayed_progress(-1)
            { };

        // create a progress task without a progress percentage
        // display
        ProgressTask(std::string shortMessage, std::string message)
            : shortMessage(shortMessage), message(message),
              measureProgress(false),
              progress(0), subStepProgress(0),
              last_displayed_progress(-1)
            { };


        const std::string & getShortMessage()
        {
            return shortMessage;
        }

        const std::string & getMessage()
        {
            return message;
        }

        double getProgress()
        {
            return progress;
        }

        std::string shortMessage;
        std::string message;
        bool measureProgress;
        double progress;
        double subStepProgress;
        double last_displayed_progress;
    };

    /** The progress display is used to report progress to another
     *  part of the program.
     *
     *  This enables the utility classes to report progress both to
     *  the statusbar if there is one, or a textmode, for applications
     *  without GUI, or no progress at all, with this default class.
     *
     *  This is the better class, and shows the whole hierachy of
     *  progress messages.
     */
    class MultiProgressDisplay
    {
    public:

        MultiProgressDisplay(double minPrintStep=0.02)
            : m_minProgressStep(minPrintStep)
        { }

        virtual ~MultiProgressDisplay() {};

        /** create a new progress display for a task.
         *
         *  once the operation is finished int must popTask()
         *  the progress display.
         *
         *  @param msg string of the message
         *  @param subStepIncr finishing the subtask below increases
         *                     the current progress by subTaskIncr
         *
         */
        void pushTask(const ProgressTask & task)
        {
            tasks.push_back(task);
            taskAdded();
            updateProgressDisplay();
        };

        /** remove a task from the progress display */
        void popTask()
        {
            taskRemove();
            if (!tasks.back().measureProgress && tasks.size()>1) {
                tasks[tasks.size()-2].progress += tasks[tasks.size()-2].subStepProgress;
            }
            tasks.pop_back();
            updateProgressDisplay();
        }

        /** change the message text of the current task */
        void setShortMessage(const std::string & msg)
        {
            tasks.back().shortMessage = msg;
            updateProgressDisplay();
        }

        /** change the message text of the current task */
        void setMessage(const std::string & msg)
        {
            tasks.back().message = msg;
            updateProgressDisplay();
        }

        /** set progress (affects this task and all tasks above it) */
        void setProgress(double progress)
        {
            propagateProgress(progress);
            double displayStep = tasks.back().progress - tasks.back().last_displayed_progress;
            if (displayStep > m_minProgressStep)
            {
                updateProgressDisplay();
                tasks.back().last_displayed_progress = tasks.back().progress;
            }
        }

        /** increase progress by a substep. */
        void increase()
        {
            // substep progress.
            tasks.back().progress + tasks.back().subStepProgress;
            setProgress(tasks.back().progress + tasks.back().subStepProgress);
        }


        /** template method, to update the display
         *
         *  should be provided by subclasses.
         */
        virtual void updateProgressDisplay() { }

        /** template method, called when a task is added */
        virtual void taskAdded() {};

        /** template method, called just before the task
         *  is removed
         */
        virtual void taskRemove() {};


    protected:

        /** propagate progress to next level */
        void propagateProgress(double progress)
        {
            std::vector<ProgressTask>::reverse_iterator it = tasks.rbegin();
            double diff = progress - it->progress;
            it->progress = progress;
            it++;
            while (it != tasks.rend()) {
                // scale previous change
                diff *= it->subStepProgress;
                // propagate to next level
                it->progress += diff;
                ++it;
            }
        }

        std::vector<ProgressTask> tasks;
        double m_minProgressStep;
    };

    /** a progress display to print stuff to stdout (doesn't work properly on the
     *  windows console.
     */
    class StreamMultiProgressDisplay : public MultiProgressDisplay
    {
    public:
        StreamMultiProgressDisplay(std::ostream & o, double minPrintStep=0.02)
            : MultiProgressDisplay(minPrintStep),
              m_stream(o), m_printedLines(0),
              m_whizz("-\\|/"), m_whizzCount(0)
        {

        }

        virtual ~StreamMultiProgressDisplay() {};

        /** update the display */
        virtual void updateProgressDisplay()
        {
            int lines = m_printedLines;
            // step back the line printed before.
            if (lines !=0) {
                m_stream << "\033[" << m_printedLines << "A"
                         << "\r";
            }
            m_printedLines = 0;
            // build the message:
            for (std::vector<ProgressTask>::iterator it = tasks.begin();
                 it != tasks.end(); ++it)
            {
                m_printedLines++;
                char tmp[81];
                tmp[80]=0;
                if (it->measureProgress) {
                    snprintf(tmp,80,"%15s : %-50s : %3.0f %%",
                             it->getShortMessage().c_str(),
                             it->getMessage().c_str(),
                             100 * it->getProgress());
                } else if (! it->measureProgress && it+1 == tasks.end()) {
                    m_whizzCount = (++m_whizzCount) % m_whizz.size();
                    snprintf(tmp,80,"%20s: %-50s :   %c ",
                             it->getShortMessage().c_str(),
                             it->getMessage().c_str(),
                             m_whizz[m_whizzCount]);
                } else {
                    snprintf(tmp,80,"%20s: %-50s :   - ",
                             it->getShortMessage().c_str(),
                             it->getMessage().c_str());
                }

                m_stream << tmp << std::endl;
            }
            // print empty lines..
            while (m_printedLines < lines) {
                m_stream << "                                                                               " << std::endl;
                m_printedLines++;
            }
        }
    protected:
        std::ostream & m_stream;
        int m_printedLines;
        std::string m_whizz;
        int m_whizzCount;
    };

    // print progress to cout.
    class CoutProgressDisplay : public ProgressDisplay
    {
    public:
        virtual ~CoutProgressDisplay() {};

        /** receive notification about progress
         *
         *  @param msg message text
         *  @param progress optional progress indicator (0-100%)
         */
        virtual void progressMessage(const std::string & msg, double progress=-1)
            {
                if (msg == last_msg && progress != -1) {
                    // just print the progress
                    if (progress != -1) {
                        std::cout << "\r" << msg << ": "
                                  << progress << "%" << "             " << std::flush;
                    }
                } else {
                    if (progress != -1) {
                        std::cout << std::endl << msg << ": " << progress << "%" << std::flush;
                    } else {
                        std::cout << std::endl << msg << std::flush;
                    }
                    last_msg = msg;
                }
            }

    private:
        std::string last_msg;
    };

} // namespace


#endif // _UTILS_H
