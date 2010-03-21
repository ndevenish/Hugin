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
*  License along with this software; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#ifndef _APPBASE_PROGRESSDISPLAY_H
#define _APPBASE_PROGRESSDISPLAY_H

#include <iostream>
#include <string>
#include <vector>

#include <hugin_shared.h>
#include <hugin_utils/utils.h>


namespace AppBase {

/**
 *
 */
class IMPEX ProgressDisplay
{
        
    // -- Task object --
        
    protected:

        /**
         *
         */
        struct ProgressSubtask
        {
            ///
            std::string message;
            ///
            double maxProgress;
            ///
            double progressForParentTask;
            ///
            bool propagatesProgress;
            ///
            double progress;
            
            public:
                
                ///
                ProgressSubtask() { };
                
                ///
                ProgressSubtask(const std::string& message,
                                const double& maxProgress,
                                const double& progressForParentTask, 
                                const bool& propagatesProgress)
                    : message(message),
                      maxProgress(maxProgress),
                      progressForParentTask(progressForParentTask), 
                      propagatesProgress(propagatesProgress),
                      progress(0.0)
                {};
                
                ///
                inline bool measuresProgress()
                    { return maxProgress != 0.0; };
            
        };
        
        
    // -- Const/Destructors --
        
    protected:
        ///
        ProgressDisplay()
            : o_canceled(false), o_newSubtaskProgress(0), o_newSubtaskPropagates(true)
        {};
        
    public:
        ///
        virtual ~ProgressDisplay() {};
        
        
    // -- task interface --
        
    protected:
        ///
        void startSubtaskWithTask(const ProgressSubtask& newSubtask);
        
    public:
        ///
        void setParentProgressOfNewSubtasks(double subtaskTotalProgress, bool propagatesProgress = false);
        
        ///
        void startSubtask(const std::string& message,
                          const double& maxProgress,
                          const double& progressForParentTask,
                          const bool& propagatesProgress = false);
        
        ///
        void startSubtask(const std::string& message,
                          const double& maxProgress = 0);
        
        ///
        void startSubtask(const double& maxProgress);
           
        ///
        virtual void setSubtaskMessage(const std::string& message);
        
        ///
        virtual std::string getSubtaskMessage() const;
        
        ///
        virtual double getSubtaskMaxProgress() const;
            
        ///
        virtual double getSubtaskProgress() const;
        
        ///
        virtual void updateSubtaskProgress(const double& newValue);
        
        ///
        virtual void increaseSubtaskProgressBy(const double& deltaValue);
        
        ///
        virtual void finishSubtask();
        
        ///
        virtual bool wasCancelled();
        
    protected:
        ///
        virtual void cancelTask();
        
        
    // -- callback interface --
        
    protected:
        /** Template method, updates the display.
         *  You should override this method with your code of updating the display.
         */
        virtual void updateProgressDisplay() =0;
        
        /** Template method, called when subtask is started.
         *  The default implementation does nothing.
         */
        virtual void subtaskStarted()
            { DEBUG_DEBUG("Subtask started."); };
        
        /** Template method, called when subtask is finishing.
         *  The default implementation does nothing.
         */
        virtual void subtaskFinished()
            { DEBUG_DEBUG("Subtask finished."); };
             
        
    // -- utility methods --
        
    protected:
        ///
        virtual void propagateProgress(const double& newProgress);

        ///
        virtual ProgressSubtask& getCurrentSubtask() const;

        ///
        virtual bool noSubtasksAvailable() const;
        
        
    // -- accessable variables --
        
    protected:
        ///
        std::vector<ProgressSubtask> o_subtasks;
        bool o_canceled;
        
        ///
        double o_newSubtaskProgress;
        bool o_newSubtaskPropagates;
        
};


/** Dummy progress display, without output
 */
class IMPEX DummyProgressDisplay : public ProgressDisplay
{
    public:
        ///
        DummyProgressDisplay()
            : ProgressDisplay()
        {};

        ///
        virtual ~DummyProgressDisplay() {};

        /** update the display */
        virtual void updateProgressDisplay()
        { };
    protected:
};

/** a progress display to print progress reports to a stream
 */
class IMPEX StreamProgressDisplay : public ProgressDisplay
{
    public:
        ///
        StreamProgressDisplay(std::ostream & o)
            : ProgressDisplay(),
              m_stream(o),
              m_printedLines(0), m_whizz("-\\|/"), m_whizzCount(0)
        {};

        ///
        virtual ~StreamProgressDisplay() {};

        /** update the display */
        virtual void updateProgressDisplay();
    protected:
        std::ostream & m_stream;
        int m_printedLines;
        std::string m_whizz;
        int m_whizzCount;
};


}; //namespace
#endif // _H
