// -*- c-basic-offset: 4 -*-
/** @file ProgressDisplayOld.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  !!from utils.h 1952
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

#ifndef _APPBASE_POGRESSDISPLAYOLD_H
#define _APPBASE_POGRESSDISPLAYOLD_H

#include <hugin_shared.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>

#include <appbase/ProgressDisplay.h>


namespace AppBase
{
    
    /** desribes a subprogess task 
    * A progress task describes one operation
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
    struct IMPEX ProgressTask
    {
        ///
        ProgressTask(std::string shortMessage, std::string message,
                     double subStepProgress, double progress=0);

        /// create a progress task without a progress percentage display
        ProgressTask(std::string shortMessage, std::string message);

        
        std::string shortMessage;
        std::string message;
        double subStepProgress;
        double progress;
        
        bool measureProgress;
        double last_displayed_progress;
        
        inline const std::string& getShortMessage()
            { return shortMessage; }
        
        inline const std::string& getMessage()
            { return message; }
        
        inline double getProgress()
            { return progress; }
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
    class IMPEX MultiProgressDisplay
    {
    public:

        MultiProgressDisplay(double minPrintStep=0.02);

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
        void pushTask(const ProgressTask & task);
        
        /** remove a task from the progress display */
        void popTask();

        /** change the message text of the current task */
        void setShortMessage(const std::string & msg);

        /** change the message text of the current task */
        void setMessage(const std::string & msg);

        /** set progress (affects this task and all tasks above it) */
        void setProgress(double progress);

        /** increase progress by a substep. */
        void increase();

        
    protected:
        /** template method, to update the display
         *
         *  should be provided by subclasses.
         */
        virtual void updateProgressDisplay() {};

        /** template method, called when a task is added */
        virtual void taskAdded() {};

        /** template method, called just before the task
         *  is removed
         */
        virtual void taskRemove() {};


    protected:
        /** propagate progress to next level */
        void propagateProgress(double progress);

        std::vector<ProgressTask> tasks;
        double m_minProgressStep;
    };
    
    
    
    /**
     *
     */
    class DummyMultiProgressDisplay : public MultiProgressDisplay
    {
        public:
            void pushTask(const ProgressTask & task) {};
            void popTask() {};
            void setShortMessage(const std::string & msg) {};
            void setMessage(const std::string & msg) {};
            void setProgress(double progress) {};
            void increase() {};
            
            virtual void updateProgressDisplay() {};
            virtual void taskAdded() {};
            virtual void taskRemove() {};
    };
    
    
    /**
     *
     */
    class MultiProgressDisplayAdaptor : public MultiProgressDisplay
    {
        
    public:
        ///
        MultiProgressDisplayAdaptor(ProgressDisplay& myProgressDisplay);
        
        ///
        virtual ~MultiProgressDisplayAdaptor() {};
        
        ///
        static MultiProgressDisplay* newMultiProgressDisplay(ProgressDisplay* myProgressDisplay);
        
        
    protected:
        ///
        void taskAdded();
        
        ///
        void taskRemove();
        
        ///
        void updateProgressDisplay();
                
        
    protected:
        ProgressDisplay& o_progressDisplay;
        
    };
    
    
    /** a progress display to print stuff to stdout (doesn't work properly on the
    *  windows console.
    */
    class IMPEX StreamMultiProgressDisplay : public MultiProgressDisplay
    {
    public:
        ///
        StreamMultiProgressDisplay(std::ostream& o, double minPrintStep=0.02);
        
        ///
        virtual ~StreamMultiProgressDisplay() {};
        
        
    protected:
        /** update the display */
        virtual void updateProgressDisplay();
        virtual void taskAdded() {};
        virtual void taskRemove() {};
        
    protected:
        std::ostream & m_stream;
        int m_printedLines;
        std::string m_whizz;
        int m_whizzCount;
    };
    
    
} // namespace


#endif // _H
