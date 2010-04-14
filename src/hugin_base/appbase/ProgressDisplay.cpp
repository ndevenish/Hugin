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

#include <cmath>

#include <iostream>
#include <iomanip>
#include <hugin_math/hugin_math.h>

#include "ProgressDisplay.h"

using namespace std;

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
        DEBUG_INFO("No subtask available");
        return;
    }
    
    if(getCurrentSubtask().progress > newValue)
    {
        DEBUG_INFO("Progress has already reached its max.");
        return;
    }
    
    propagateProgress(newValue);
    //getCurrentSubtask().progress = std::min(newValue, getSubtaskMaxProgress());
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
        
        DEBUG_INFO("Propagating progress:+" << diffFromPrev*100 << "%");
        
        itr->progress += diffFromPrev;
        
        if(!itr->propagatesProgress)
        {
            DEBUG_INFO("Propagation stopped.");
            return;
        }
        
        // scale previous change for higher level
        diffFromPrev *= itr->progressForParentTask / itr->maxProgress;
        ++itr;
        
    } while(itr != o_subtasks.rend());
}


ProgressDisplay::ProgressSubtask& ProgressDisplay::getCurrentSubtask() const
    { return (ProgressSubtask& )o_subtasks.back(); }


bool ProgressDisplay::noSubtasksAvailable() const
    { return o_subtasks.empty(); }



void StreamProgressDisplay::updateProgressDisplay()
{
    // TODO: check for Ctrl-C then cancelTask() ?

    // step back the line printed before.
    if (m_printedLines == 0 )
    {
        m_stream << endl;
    }
    m_printedLines = 1;
    // build the message

    int strlen=0;
    ostringstream stream;
#if defined _WINDOWS
    stream << setfill('\b') << setw(81);
#else
    stream << "\r";
#endif
    for (std::vector<ProgressSubtask>::iterator it = o_subtasks.begin();
         it != o_subtasks.end(); ++it)
    {
        if (stream.str().size() + it->message.size() > 70) {
            break;
        }
        if (it != o_subtasks.begin()) {
            stream << ", ";
        }
        stream << it->message;

        std::vector<ProgressSubtask>::iterator next = it;
        ++next;
    }
    bool showProgress=false;
    if(o_subtasks.size()>0)
        showProgress=o_subtasks[0].measuresProgress();
    if(showProgress){
        stream << ": " << setw(3) << hugin_utils::roundi( 100.0 * o_subtasks[0].progress / o_subtasks[0].maxProgress) << "%";
    } else {
        m_whizzCount = (++m_whizzCount) % (int)m_whizz.size();
        stream << ": "  << m_whizz[m_whizzCount] << "   ";
    }

    int fill = 81-stream.str().size();
    stream << setw(fill) << " ";
    m_stream << stream.str() << flush;
}



}; //namespace
