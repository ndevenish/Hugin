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

#include <cmath>

#include "ProgressReporterOld.h"


namespace AppBase {

    
ProgressReporter::ProgressReporter(double maxProgress) 
  : m_progress(0), m_maxProgress(maxProgress)
{    
}




///
ProgressReporterAdaptor::ProgressReporterAdaptor(ProgressDisplay& myProgressDisplay, const double& maxProgress)
  : ProgressReporter(maxProgress), o_progressDisplay(myProgressDisplay)
{
    o_progressDisplay.startSubtask(maxProgress);
    o_progressDisplay.startSubtask("", 0.0, 0.0, false);
};


///
ProgressReporterAdaptor::~ProgressReporterAdaptor()
{
    o_progressDisplay.finishSubtask();
    o_progressDisplay.finishSubtask();
};


///
bool ProgressReporterAdaptor::increaseProgress(double delta)
{
    std::string msg = o_progressDisplay.getSubtaskMessage();
    o_progressDisplay.finishSubtask();
    o_progressDisplay.increaseSubtaskProgressBy(delta);
    o_progressDisplay.startSubtask(msg, 0.0, 0.0, false);
    
    return !o_progressDisplay.wasCancelled();
}


///
void ProgressReporterAdaptor::setMessage(const std::string & msg)
{
    o_progressDisplay.setSubtaskMessage(msg);
}


ProgressReporter* ProgressReporterAdaptor::newProgressReporter(ProgressDisplay* myProgressDisplay, const double& maxProgress)
{
    if(myProgressDisplay != NULL)
        return new ProgressReporterAdaptor(*myProgressDisplay, maxProgress);
    else
        return new DummyProgressReporter(maxProgress);
}

    



StreamProgressReporter::StreamProgressReporter(double maxProgress, std::ostream & out)
: ProgressReporter(maxProgress), m_stream(out)
{
    
}

StreamProgressReporter::~StreamProgressReporter()
{
    m_stream << "\r" << std::flush;
}


bool StreamProgressReporter::increaseProgress(double delta) 
{
    m_progress += delta;
    print();
    // check for Ctrl-C ?
    return true;
}


bool StreamProgressReporter::increaseProgress(double delta, const std::string & msg) 
{
    m_message = msg;
    m_progress += delta;
    print();
    // check for Ctrl-C ?
    return true;
}


void StreamProgressReporter::setMessage(const std::string & msg)
{
    m_message = msg;
    print();
}


void StreamProgressReporter::print()
{
    double prog = floor(m_progress/m_maxProgress*100);
    if (prog > 100) prog = 100;
    m_stream << "\r" << m_message << ": " << prog << "%" << std::flush;
}


} //namespace

