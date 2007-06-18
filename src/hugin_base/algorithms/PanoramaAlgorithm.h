// -*- c-basic-offset: 4 -*-
/** @file PanoramaAlgorithm.h
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
*
*  $Id: $
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

#include <exception>

using namespace AppBase;

namespace HuginBase {
    
    class PanoramaData;

    
    /**
     *
     */
    class PanoramaAlgorithm
    {

    public:
        
        PanoramaAlgorithm(PanoramaData& panorama)
            : m_panorama(panorama)
            { };
        
        virtual ~PanoramaAlgorithm();
        
        /// returns true if the algorithm changes the PanoramaData.
        virtual bool modifiesPanoramaData() { return false; };
        
        /// mainly for debug; are all the required data there?
        virtual bool isParametersValid() { return true; };
        
        /* runs the algorithm.
        *   You should override with your algorithm's implementiation.
        */
        virtual bool runAlgorithm() { return true; };
        
        
        /*
         * Here is the informal interface guidelines that you should follow when
         * you subclass from this class:
         *
         *  1. You should have [ void setSomeParameter(some parameter) ] methods
         *   for all parameters of the algorithms if any.
         *
         *  2. You should provide [ bool call(all parameters) ] method for
         *   convenience.
         *
         *  3. You should provide [ SomeType getSomeResult() ] methods if there 
         *   is any result to the algorithm.
         *
         *  4. For complicated algorithms, you can have [ MyErrorEnum getError() ]
         *   that returns error.
         *
         *  5. You can optionaly provide
         *   [ static SomeType executeMyAlgorithm(PanoramaData& panorama, all parameters) ]
         *   with appropriate exception as well.
         *
         */
        

    protected:
        
        ///
        PanoramaData& getPanorama() const
            { return m_panorama; };

        ///
        void setPanorama( PanoramaData& panoramaData )
            { m_panorama = panoramaData; };
        
    private:
            
        PanoramaData& m_panorama;
        
    };



    /**

    */
    class TimeConsumingPanoramaAlgorithm : public PanoramaAlgorithm
    {
        
    public:
        
        /// [Warning! it keeps the reference to the panorama data!]
        TimeConsumingPanoramaAlgorithm(PanoramaData& panorama, ProgressReport* progressReoport = NULL)
            : m_panorama(panorama),
              m_progressReport(progressDisplay), m_wasCancelled(false)
        { };
        
        
        /*
         * Please follow the same guideline as the PanoramaAlgorithm class, but with:
         *
         *  5. should now be 
         *   [ static SomeType executeMyAlgorithm(PanoramaData& panorama, ProgressDisplay* progressDisplay, all parameters) ]
         *   instead.
         *
         */
        
        
    // -- access to the ProgressDisplay --        
        
    protected:
        
        ///
        ProgressReport* getProgressReport() const
            { return m_progressReport; };
        
        ///
        bool hasProgressDisplay() const
            { m_progressReport != NULL; };
        
        
    // -- cancelling process --
        
    public:
            
        ///
        bool wasCancelled()
            { return m_wasCancelled; };
        
    protected:
            
        /** Call this when the algorithm is cancelled. This method sets
         *  wasCancelled() to return true, and calls algorithmCancelled()
         */ 
        void cancelAlgorithm()
        {
            m_wasCancelled = true;
            AlgorithmCancelled();
        }
    
        /** Called when the algorithm got cancelled;
         *  override with cleaning up process etc.
         *  the default implementation does nothing.
         */
        virtual void algorithmCancelled() {};
        
        
    // --private variables --
        
    private:
            
        ProgressDisplay* m_progressReport;
        bool m_wasCancelled;
        
    };
    

} // namespace

