// -*- c-basic-offset: 4 -*-
/** @file ProgressDisplay.h
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
*
*  $Id: $
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

namespace AppBase {

/**
 *
 */
template <class StringType = std::string>
class ProgressDisplay
{
    
// -- Task object --
    
protected:

    /**
     *
     */
    struct ProgressTask
    {
        ///
        ProgressTask(const StringType& message,
                     const double& maxProgress,
                     const double& progressForParentTask, 
                     const bool& propagatesProgress)
            : message(message),
              maxProgress(maxProgress),
              progressForParentTask(progressForParentTask), 
              propagatesProgress(propagatesProgress),
              progress(0)
        { };
        
        ///
        StringType message;
        ///
        double progressForParentTask;
        ///
        double maxProgress;
        ///
        double progress;
        ///
        bool propagatesProgress;
        
        ///
        bool measuresProgress()
            { return maxProgress == 0; };
    };
    
    
// -- Const/Destructors --
    
protected:
    
    ///
    virtual ProgressDisplay()
        : m_nextSubtaskProgress(0)
    {};
    
    ///
    virtual ProgressDisplay(const StringType &title)
        : m_title(title),
          m_nextSubtaskProgress(0)
    {};
    
public:
    
    ///
    virtual ~ProgressDisplay();
    
    
// -- task interface --
    
protected:

    ///
    void startSubtaskWithTask(const ProgressTask& newSubtask)
    {
        subtasks.push_back(newSubtask);
        subtaskStarted();
        updateProgressDisplay();
    }
    
public:
        
    ///
    void setParentProgressOfNewSubtasks(double subtaskTotalProgress, bool propagatesProgress = false)
    {
        if(subtaskTotalProgress < 0)
            return;
        
        m_nextSubtaskProgress = subtaskTotalProgress;
        m_nextSubtaskPropagates = propagatesProgress;
    };
    
    ///
    void startSubtask(const StringType& message,
                      const double& maxProgress,
                      const double& progressForParentTask,
                      const bool& propagatesProgress = false)
    {
        ProgressTask newSubtask = ProgressTask(message, maxProgress, progressForParentTask, propagatesProgress);
        
        startSubtaskWithTask(newSubtask);
    };
    
    ///
    void startSubtask(const StringType& message,
                      const double& maxProgress = 0)
    {
        if(m_nextSubtaskProgress > 0)
            startSubtask(message, maxProgress, m_nextSubtaskProgress, m_nextSubtaskPropagates);
        else
            startSubtask(message, maxProgress, 0, false);
    };
    
    ///
    void startSubtask(const double& maxProgress)
    { 
        startSubtask(StringType(), maxProgress);
    };
        
    ///
    double getSubtaskMaxProgress() const
    {
        assert(!noSubtasksAvailable()); //[TODO] make it nicer:)
        return getCurrentSubtask().maxProgress;
    };
        
    ///
    double getSubtaskProgress() const
    {
        assert(!noSubtasksAvailable()); //[TODO] make it nicer:)
        return getCurrentSubtask().progress;
    };
    
    ///
    void updateSubtaskProgress(const double& newValue)
    {
        if(noSubtasksAvailable())
            return; //[TODO] debug
        
        if(getCurrentSubtask().progress > newValue)
            return; //[TODO] debug
        
        getCurrentSubtask().progress = max(newValue, getSubtaskMaxProgress());
        updateProgressDisplay();
    }
    
    ///
    void finishSubtask()
    {
        subtaskFinished();
        
        if (!tasks.back().measureProgress && tasks.size()>1) {
            tasks[tasks.size()-2].progress += tasks[tasks.size()-2].subStepProgress;
        }
        
        tasks.pop_back();
        updateProgressDisplay();
    };
    
    ///
    StringType getTitle() const
        { return m_title; };
    
    ///
    virtual bool wasCanceled()
        { return false; };
    
    
// -- callback interface --
    
protected:
    
    /** Template method, updates the display.
     *  You should override this method with your code of updating the display.
     *  The default implementation does nothing.
     */
    virtual void updateProgressDisplay() {};
    
    /** Template method, called when subtask is started.
     *  The default implementation does nothing.
     */
    virtual void subtaskStarted() {};
    
    /** Template method, called when subtask is finishing.
     *  The default implementation does nothing.
     */
    virtual void subtaskFinished() {};
         
    
// -- utility methods --
    
protected:
        
    ///
    void propagateProgress(const double& newProgress)
    {
        std::vector<ProgressTask>::reverse_iterator itr = tasks.rbegin();
        
        double diffFromPrev = newProgress - itr->progress;
        
        if(diffFromPrev <= 0)
            return;
        
        do {
            
            itr->progress += diffFromPrev;
            
            if(!itr->propagatesProgress)
                return;
            
            // scale previous change for higher level
            diffFromPrev *= itr->progressForParentTask / itr->maxProgress;
            
            itr++;
            
        } while(itr != tasks.rend());
    }

    ///
    ProgressTask getCurrentSubtask() const
        { return m_subtasks.back(); };

    ///
    bool noSubtasksAvailable() const
        { return !m_subtasks.empty(); };
    
    
// -- accessable variables --
    
protected:
    
    ///
    StringType m_title;
       
    ///
    std::vector<ProgressTask> m_subtasks;
    
    ///
    double m_nextSubtaskProgress;
    bool m_nextSubtaskPropagates;
    
}

}; //namespace
    