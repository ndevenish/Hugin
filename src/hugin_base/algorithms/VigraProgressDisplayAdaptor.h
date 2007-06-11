// -*- c-basic-offset: 4 -*-
/** @file VigraProgressDisplayAdaptor.h
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
*  Hereby the author, Ippei UKAI, grant the license of this particular file to
*  be relaxed to the GNU Lesser General Public License as published by the Free
*  Software Foundation; either version 2 of the License, or (at your option)
*  any later version. Please note however that when the file is linked to or
*  compiled with other files in this library, the GNU General Public License as
*  mentioned above is likely to restrict the terms of use further.
*
*/

#include <vigra_ext/ProgressDisplay.h>

using namespace AppBase;

namespace HuginBase {

    
    /**
     *
     */
    template <class StringType = std::string>
    class ProgressDisplayAdaptor : vigra_ext::MultiProgressDisplay
    {
    public:
        
        ProgressDisplayAdaptor(ProgressDisplay<StringType>& myProgressDisplay)
        : vigra_ext::MultiProgressDisplay(0.0), m_progressDisplay(myProgressDisplay)
        {};
            
        virtual ~ProgressDisplayAdaptor() {};
    
        ///
        void taskAdded()
        {
            m_progressDisplay.startSubtask(StringType(tasks.back().subStepProgress), 1.0, tasks.back().subStepProgress, true);
        };
        
        ///
        void taskRemove()
        {
            m_progressDisplay.finishSubtask();
        };
        
        ///
        void updateProgressDisplay()
        {
            m_progressDisplay.updateSubtaskProgress(tasks.back().getProgress());
        };
                
        
    protected:
            
        ProgressDisplay<StringType>& m_progressDisplay;
        
    };
    
    
    /**
     *
     */
    class DummyMultiProgressDispaly : vigra_ext::MultiProgressDisplay
    {
        void pushTask(const vigra_ext::ProgressTask & task) {};
        void popTask() {};
        void setShortMessage(const std::string & msg) {};
        void setMessage(const std::string & msg) {};
        void setProgress(double progress) {};
        void increase() {};
        
        virtual void updateProgressDisplay() {};
        virtual void taskAdded() {};
        virtual void taskRemove() {};
    };
    
}; //namespace
    