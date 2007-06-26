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

#include <cmath>

#include "ProgressDisplay.h"


namespace AppBase {

    

ProgressDisplay::ProgressSubtask::ProgressSubtask(const std::string& message,
                                                  const double& maxProgress,
                                                  const double& progressForParentTask, 
                                                  const bool& propagatesProgress)
            : message(message),
              maxProgress(maxProgress),
              progressForParentTask(progressForParentTask), 
              propagatesProgress(propagatesProgress),
              progress(0.0)
{

}





ProgressDisplay::ProgressDisplay()
    : o_newSubtaskProgress(0)
{

}

void ProgressDisplay::startSubtaskWithTask(const ProgressSubtask& newSubtask)
{
    o_subtasks.push_back(newSubtask);
    subtaskStarted();
    updateProgressDisplay();
}

void ProgressDisplay::setParentProgressOfNewSubtasks(double subtaskTotalProgress, bool propagatesProgress)
{
    if(subtaskTotalProgress < 0)
        return;
    
    o_newSubtaskProgress = subtaskTotalProgress;
    o_newSubtaskPropagates = propagatesProgress;
};

///
void ProgressDisplay::startSubtask(const std::string& message,
                  const double& maxProgress,
                  const double& progressForParentTask,
                  const bool& propagatesProgress)
{
    ProgressSubtask newSubtask = ProgressSubtask(message, maxProgress, progressForParentTask, propagatesProgress);
    
    startSubtaskWithTask(newSubtask);
};

///
void ProgressDisplay::startSubtask(const std::string& message,
                  const double& maxProgress)
{
    if(o_newSubtaskProgress > 0)
        startSubtask(message, maxProgress, o_newSubtaskProgress, o_newSubtaskPropagates);
    else
        startSubtask(message, maxProgress, 0, false);
};

///
void ProgressDisplay::startSubtask(const double& maxProgress)
{ 
    startSubtask("", maxProgress);
};
   
///
void ProgressDisplay::setSubtaskMessage(const std::string& message)
    { getCurrentSubtask().message = message; }

///
std::string ProgressDisplay::getSubtaskMessage() const
{
    return getCurrentSubtask().message;
}

///
double ProgressDisplay::getSubtaskMaxProgress() const
{
    assert(!noSubtasksAvailable()); //[TODO] make it nicer:)
    return getCurrentSubtask().maxProgress;
}
    
///
double ProgressDisplay::getSubtaskProgress() const
{
    assert(!noSubtasksAvailable()); //[TODO] make it nicer:)
    return getCurrentSubtask().progress;
}

///
void ProgressDisplay::updateSubtaskProgress(const double& newValue)
{
    if(noSubtasksAvailable())
    {
        //[TODO] debug
        return;
    }
    
    if(getCurrentSubtask().progress > newValue)
    {
        //[TODO] debug
        return;
    }
    
    getCurrentSubtask().progress = std::max(newValue, getSubtaskMaxProgress());
    updateProgressDisplay();
}

///
void ProgressDisplay::increaseSubtaskProgressBy(const double& deltaValue)
{
    updateSubtaskProgress(getSubtaskProgress() + deltaValue);
}

///
void ProgressDisplay::finishSubtask()
{
    subtaskFinished();
    
    if (!o_subtasks.back().measuresProgress() && o_subtasks.size()>1) {
        o_subtasks[o_subtasks.size()-2].progress += o_subtasks[o_subtasks.size()-1].progressForParentTask;
    }
    
    o_subtasks.pop_back();
    updateProgressDisplay();
}

///
bool ProgressDisplay::wasCanceled()
    { return false; }
    

///
void ProgressDisplay::propagateProgress(const double& newProgress)
{
    std::vector<ProgressSubtask>::reverse_iterator itr = o_subtasks.rbegin();
    
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
        
    } while(itr != o_subtasks.rend());
}

///
ProgressDisplay::ProgressSubtask& ProgressDisplay::getCurrentSubtask() const
    { return (ProgressSubtask& )o_subtasks.back(); }

///
bool ProgressDisplay::noSubtasksAvailable() const
    { return o_subtasks.empty(); }


}; //namespace
    