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


void ProgressDisplay::startSubtask(const std::string& message,
                  const double& maxProgress,
                  const double& progressForParentTask,
                  const bool& propagatesProgress)
{
    ProgressSubtask newSubtask = ProgressSubtask(message, maxProgress, progressForParentTask, propagatesProgress);
    
    startSubtaskWithTask(newSubtask);
};


void ProgressDisplay::startSubtask(const std::string& message,
                  const double& maxProgress)
{
    if(o_newSubtaskProgress > 0)
        startSubtask(message, maxProgress, o_newSubtaskProgress, o_newSubtaskPropagates);
    else
        startSubtask(message, maxProgress, 0, false);
};


void ProgressDisplay::startSubtask(const double& maxProgress)
{ 
    startSubtask("", maxProgress);
};
   

void ProgressDisplay::setSubtaskMessage(const std::string& message)
    { getCurrentSubtask().message = message; }


std::string ProgressDisplay::getSubtaskMessage() const
{
    return getCurrentSubtask().message;
}


double ProgressDisplay::getSubtaskMaxProgress() const
{
    assert(!noSubtasksAvailable()); //[TODO] make it nicer:)
    return getCurrentSubtask().maxProgress;
}
    

double ProgressDisplay::getSubtaskProgress() const
{
    assert(!noSubtasksAvailable()); //[TODO] make it nicer:)
    return getCurrentSubtask().progress;
}


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


void ProgressDisplay::increaseSubtaskProgressBy(const double& deltaValue)
{
    updateSubtaskProgress(getSubtaskProgress() + deltaValue);
}


void ProgressDisplay::finishSubtask()
{
    subtaskFinished();
    
    if (!o_subtasks.back().measuresProgress() && o_subtasks.size()>1) {
        o_subtasks[o_subtasks.size()-2].progress += o_subtasks[o_subtasks.size()-1].progressForParentTask;
    }
    
    o_subtasks.pop_back();
    updateProgressDisplay();
}


void ProgressDisplay::cancelTask()
{
    o_canceled = true;
    // more to do?
}


bool ProgressDisplay::wasCancelled()
    { return o_canceled; }
    



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


ProgressDisplay::ProgressSubtask& ProgressDisplay::getCurrentSubtask() const
    { return (ProgressSubtask& )o_subtasks.back(); }


bool ProgressDisplay::noSubtasksAvailable() const
    { return o_subtasks.empty(); }



void StreamProgressDisplay::updateProgressDisplay()
{
    // [TODO] check for Ctrl-C then cancelTask()
    
    int lines = m_printedLines;
    // step back the line printed before.
    if (lines !=0) {
        m_stream << "\033[" << m_printedLines << "A" << "\r";
    }
    m_printedLines = 0;
    // build the message:
    for (std::vector<ProgressSubtask>::iterator it = o_subtasks.begin();
         it != o_subtasks.end(); ++it)
    {
        m_printedLines++;
        char tmp[81];
        tmp[80]=0;
        if (it->measuresProgress()) {
            snprintf(tmp,80,"%68s : %3.0f %%",
                     it->message.c_str(),
                     100 * it->progress / it->maxProgress);
        } else if (it+1 == o_subtasks.end()) {
            m_whizzCount = (++m_whizzCount) % (int)m_whizz.size();
            snprintf(tmp,80,"%72s :   %c ",
                     it->message.c_str(),
                     m_whizz[m_whizzCount]);
        } else {
            snprintf(tmp,80,"%72s :   - ",
                     it->message.c_str());
        }
        
        m_stream << tmp << std::endl;
    }
    // print empty lines..
    while (m_printedLines < lines) {
        m_stream << "                                                                               " << std::endl;
        m_printedLines++;
    }
}



}; //namespace
    