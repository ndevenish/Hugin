// -*- c-basic-offset: 4 -*-
/** @file ProgressDisplayOld.h
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

#include "ProgressDisplayOld.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

namespace AppBase {
    
    
ProgressTask::ProgressTask(std::string shortMessage, std::string message,
                           double subStepProgress, double progress)
  : shortMessage(shortMessage), message(message),
    subStepProgress(subStepProgress), progress(progress),
    measureProgress(true), last_displayed_progress(-1)
{

}

ProgressTask::ProgressTask(std::string shortMessage, std::string message)
  : shortMessage(shortMessage), message(message),
    subStepProgress(0), progress(0),
    measureProgress(false), last_displayed_progress(-1)
{
    
}




MultiProgressDisplay::MultiProgressDisplay(double minPrintStep)
: m_minProgressStep(minPrintStep) 
{

}

void MultiProgressDisplay::pushTask(const ProgressTask & task)
{
    tasks.push_back(task);
    taskAdded();
    updateProgressDisplay();
}


/** remove a task from the progress display */
void MultiProgressDisplay::popTask()
{
    taskRemove();
    if (!tasks.back().measureProgress && tasks.size()>1) {
        tasks[tasks.size()-2].progress += tasks[tasks.size()-2].subStepProgress;
    }
    tasks.pop_back();
    updateProgressDisplay();
}


/** change the message text of the current task */
void MultiProgressDisplay::setShortMessage(const std::string & msg)
{
    tasks.back().shortMessage = msg;
    updateProgressDisplay();
}


/** change the message text of the current task */
void MultiProgressDisplay::setMessage(const std::string & msg)
{
    tasks.back().message = msg;
    updateProgressDisplay();
}

/** set progress (affects this task and all tasks above it) */
void MultiProgressDisplay::setProgress(double progress)
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
void MultiProgressDisplay::increase()
{
    // substep progress.
    setProgress(tasks.back().progress + tasks.back().subStepProgress);
}


void MultiProgressDisplay::propagateProgress(double progress)
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




///
MultiProgressDisplayAdaptor::MultiProgressDisplayAdaptor(ProgressDisplay& myProgressDisplay)
  : MultiProgressDisplay(0.0), o_progressDisplay(myProgressDisplay)
{};

///
MultiProgressDisplay* MultiProgressDisplayAdaptor::newMultiProgressDisplay(ProgressDisplay* myProgressDisplay)
{
    if(myProgressDisplay != NULL)
        return new MultiProgressDisplayAdaptor(*myProgressDisplay);
    else
        return new DummyMultiProgressDisplay();
}


///
void MultiProgressDisplayAdaptor::taskAdded()
{
    assert(tasks.size() > 0);
    if (tasks.size() > 1) {
        o_progressDisplay.setParentProgressOfNewSubtasks(( ++(tasks.rbegin()) )->subStepProgress, true);
    }
    o_progressDisplay.startSubtask(tasks.back().getMessage(), 1.0);
};

///
void MultiProgressDisplayAdaptor::taskRemove()
{
    o_progressDisplay.finishSubtask();
};

///
void MultiProgressDisplayAdaptor::updateProgressDisplay()
{
    if(tasks.empty())
    {
        DEBUG_INFO("There are no tasks.");
        return;
    }
    
    if(tasks.back().getMessage().length() == 0)
        o_progressDisplay.setSubtaskMessage(tasks.back().getShortMessage());
    else if(tasks.back().getShortMessage().length() == 0)
        o_progressDisplay.setSubtaskMessage(tasks.back().getMessage());
    else
        o_progressDisplay.setSubtaskMessage(tasks.back().getShortMessage() + " ("+tasks.back().getMessage()+")");
    
    o_progressDisplay.updateSubtaskProgress(tasks.back().getProgress());
}





StreamMultiProgressDisplay::StreamMultiProgressDisplay(std::ostream& o, double minPrintStep)
  : MultiProgressDisplay(minPrintStep),
    m_stream(o), m_printedLines(0),
    m_whizz("-\\|/"), m_whizzCount(0)
{

}


void StreamMultiProgressDisplay::updateProgressDisplay()
{
    int lines = m_printedLines;
    // step back the line printed before.
    if (lines !=0) {
        m_stream << "\033[" << m_printedLines << "A" << "\r";
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
            snprintf(tmp,80,"%20s: %-50s : %3.0f %%",
                     it->getShortMessage().c_str(),
                     it->getMessage().c_str(),
                     100 * it->getProgress());
        } else if (! it->measureProgress && it+1 == tasks.end()) {
            m_whizzCount = (++m_whizzCount) % (int)m_whizz.size();
            snprintf(tmp,80,"%20s: %-50s :   %c  ",
                     it->getShortMessage().c_str(),
                     it->getMessage().c_str(),
                     m_whizz[m_whizzCount]);
        } else {
            snprintf(tmp,80,"%20s: %-50s :   -  ",
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



} //namespace

