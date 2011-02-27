// -*- c-basic-offset: 4 -*-
/** @file PanoramaAlgorithm.h
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
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
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this software; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.
*
*  Hereby the author, Ippei UKAI, grant the license of this particular file to
*  be relaxed to the GNU Lesser General Public License as published by the Free
*  Software Foundation; either version 2 of the License, or (at your option)
*  any later version. Please note however that when the file is linked to or
*  compiled with other files in this library, the GNU General Public License as
*  mentioned above is likely to restrict the terms of use further.
*
*/

#ifndef _ALGORITHM_PANORAMAALGORITHM_H
#define _ALGORITHM_PANORAMAALGORITHM_H

#include <hugin_shared.h>
#include <appbase/ProgressDisplay.h>


namespace HuginBase {
    
    class PanoramaData;

    
    /**
     *
     */
    class IMPEX PanoramaAlgorithm
    {

    protected:
        ///
        PanoramaAlgorithm(PanoramaData& panorama)
            : o_panorama(panorama), o_successful(false)
            { };

    public:
        ///
        virtual ~PanoramaAlgorithm() {};
        

    public:
        /// returns true if the algorithm changes the PanoramaData.
        virtual bool modifiesPanoramaData() const =0;
        
        ///
        virtual bool hasRunSuccessfully()
        {
            return o_successful;
        }
        
        /// runs the algorithm.
        virtual void run()
        {
            o_successful = runAlgorithm();
        }
        
#if 0
        /// runs the algorithm.
        template<class AlgorithmClass>
        AlgorithmClass& runMe()
        {
            AlgorithmClass& THIS = static_cast<AlgorithmClass&>(*this);
            THIS.run();
            return THIS;
        }
#endif
        
        /** implementation of the algorithm.
        *   You should override with your algorithm's implementiation.
        */
        virtual bool runAlgorithm() =0; 
        
        /*
         * Here is the informal interface guidelines that you should follow when
         * you subclass from this class:
         *
         *  1. You should provide [ SomeType getSomeResult() ] methods if there 
         *   is any result from the algorithm.
         *
         *  2. For complicated algorithms, you can have [ MyErrorEnum getError() ]
         *   that returns error.
         *
         *  3. You can optionaly implement your algorithm as
         *   [ static SomeType executeMyAlgorithm(PanoramaData& panorama, all parameters) ].
         *
         */
        
    protected:
        PanoramaData& o_panorama;
        bool o_successful;
        
    };



    /**
    *
    */
    class IMPEX TimeConsumingPanoramaAlgorithm : public PanoramaAlgorithm
    {
        
    protected:
        /// [Warning! it keeps the reference to the panorama data!]
        TimeConsumingPanoramaAlgorithm(PanoramaData& panorama, AppBase::ProgressDisplay* progressDisplay = NULL)
            : PanoramaAlgorithm(panorama),
              m_progressDisplay(progressDisplay), m_wasCancelled(false)
        { };
        
    public:
        ///
        virtual ~TimeConsumingPanoramaAlgorithm() {};
        
        
    public:
        /*
         * Please follow the same guideline as the PanoramaAlgorithm class, but with:
         *
         *  3. should now be 
         *   [ static SomeType executeMyAlgorithm(PanoramaData& panorama, ProgressDisplay* progressDisplay, all parameters) ]
         *   instead.
         *
         */
        
        
    // -- access to the ProgressDisplay --        
        
    protected:
        ///
        virtual AppBase::ProgressDisplay* getProgressDisplay() const
            { return m_progressDisplay; };
        
        ///
        virtual bool hasProgressDisplay() const
            { return getProgressDisplay() != NULL; };
        
        
    // -- cancelling process --
        
    public:
        ///
        virtual bool wasCancelled() const
            { return m_wasCancelled; };
        
    protected:
        /** Call this when the algorithm is cancelled. This method sets
         *  wasCancelled() to return true, and calls algorithmCancelled()
         */ 
        virtual void cancelAlgorithm()
        {
            m_wasCancelled = true;
            algorithmCancelled();
        }
    
        /** Called when the algorithm got cancelled;
         *  override with cleaning up process etc.
         *  the default implementation does nothing.
         */
        virtual void algorithmCancelled() {};
        
        
    // -- private variables --
        
    private:
        AppBase::ProgressDisplay* m_progressDisplay;
        bool m_wasCancelled;
        
    };
    

}; // namespace
#endif // _H
