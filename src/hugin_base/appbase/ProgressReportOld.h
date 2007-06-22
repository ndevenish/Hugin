// -*- c-basic-offset: 4 -*-
/** @file ProgressDisplayOld.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.h 1952 2007-04-15 20:57:55Z dangelo $
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

#ifndef POGRESSDISPLAYOLD_H
#define POGRESSDISPLAYOLD_H

#include <string>
#include <vector>


namespace AppBase
{
    
    /**
     *
     */
    class ProgressReporter
    {
    public:
        ProgressReporter(double maxProgress = 1.0);
        virtual ~ProgressReporter() {};
        virtual void setMessage(const std::string & msg) = 0;
        virtual bool increaseProgress(double delta) = 0;
        
        virtual bool increaseProgress(double delta, const std::string & msg)
        {
            setMessage(msg);
            return increaseProgress(double delta);
        }
    };
    
    
    /**
     *
     */
    class DummyProgressReport : public ProgressReport
    {
    public:
        DummyProgressReport(double maxProgress = 1.0)
        : ProgressReport(maxProgress);
        
        ~ProgressReporter() {};
        
        void setMessage(const std::string & msg) {};
        bool increaseProgress(double delta) { return true; };
    };
    

    /**
     *
     */
    class ProgressReporterAdaptor : ProgressReporter
    {
    public:
        ///
        ProgressReporterAdaptor(ProgressDisplay& myProgressDisplay, const double& maxProgress)
         : ProgressReporter(maxProgress), o_progressDisplay(myProgressDisplay)
        {
             o_progressDisplay.startSubtask(maxProgress);
        };
           
        ///
        virtual ~ProgressReporterAdaptor()
        {
            o_progressDisplay.subtaskFinished();
        };
        
        ///
        static ProgressReporter newProgressReporter(ProgressDisplay* myProgressDisplay, const double& maxProgress)
        {
            if(myProgressDisplay != NULL)
                return new ProgressReporterAdaptor(*myProgressDisplay, maxProgress);
            else
                return new DummyProgressReport(maxProgress);
        }
            
        
    public:
        ///
        bool increaseProgress(double delta)
        {
            o_progressDisplay.increaseSubtaskProgressBy(delta);
            return !o_progressDisplay.wasCanceled();
         };
        
        ///
        void setMessage(const std::string & msg)
        {
            o_progressDisplay.setSubtaskMessage(msg);
        }
        
        
    protected:
        ProgressDisplay& o_progressDisplay;
    };
    
    
} // namespace


#endif // POGRESSDISPLAYOLD_H
