// -*- c-basic-offset: 4 -*-
/** @file ProgressReporterOld.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $id: $
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

#ifndef _APPBASE_POGRESSREPORTEROLD_H
#define _APPBASE_POGRESSREPORTEROLD_H

#include <hugin_shared.h>
#include <string>
#include <vector>
#include <iostream>

#include <appbase/ProgressDisplay.h>

namespace AppBase
{
    
    /**
     *
     */
    class IMPEX ProgressReporter
    {
    public:
        ProgressReporter(double maxProgress = 1.0);
        virtual ~ProgressReporter() {};
        
        virtual void setMessage(const std::string & msg) {};
        virtual bool increaseProgress(double delta) =0;
        
        virtual bool increaseProgress(double delta, const std::string& msg)
        {
            setMessage(msg);
            return increaseProgress(delta);
        }
        
    protected:
        double m_progress;
        double m_maxProgress;
    };
    
    
    /**
     *
     */
    class DummyProgressReporter : public ProgressReporter
    {
    public:
        DummyProgressReporter(double maxProgress = 1.0)
        : ProgressReporter(maxProgress)
        {};
        
        ~DummyProgressReporter() {};
        
        void setMessage(const std::string& msg) {};
        bool increaseProgress(double delta) { return true; };
    };
    

    /**
     *
     */
    class ProgressReporterAdaptor : public ProgressReporter
    {
    public:
        ///
        ProgressReporterAdaptor(ProgressDisplay& myProgressDisplay, const double& maxProgress);
           
        ///
        virtual ~ProgressReporterAdaptor();
        
        ///
        static ProgressReporter* newProgressReporter(ProgressDisplay* myProgressDisplay, const double& maxProgress);
        
    public:
        ///
        bool increaseProgress(double delta);
        
        ///
        void setMessage(const std::string & msg);
        
        
    protected:
        ProgressDisplay& o_progressDisplay;
    };
    
    
    
    ///
    class StreamProgressReporter : public ProgressReporter
    {
    public:
        ///
        StreamProgressReporter(double maxProgress, std::ostream & out=std::cout);
        
        ///
        virtual ~StreamProgressReporter();
        
    public:
        ///
        virtual bool increaseProgress(double delta);
        
        ///
        virtual bool increaseProgress(double delta, const std::string & msg);
        
        ///
        virtual void setMessage(const std::string & msg);

        ///
        void print();
        
    private:
        std::string m_message;
        std::ostream & m_stream;
    };
    
    
} // namespace


#endif // _H
