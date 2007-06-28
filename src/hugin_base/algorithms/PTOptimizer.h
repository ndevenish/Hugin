// -*- c-basic-offset: 4 -*-
/** @file 
 *
* !! from PTOptimise.h 1951
 *
 *  functions to call the optimizer of panotools.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PTOptimise.h 1951 2007-04-15 20:54:49Z dangelo $
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




namespace HuginBase {
    
    
    /// conceptual
    class PTOptimizer : PanoramaAlgorithm
    {
    
        public:
            ///
            PTOptimizer(PanoramaData& panorama)
             : PanoramaAlgorithm(panorama)
            {};
        
            ///
            virtual ~PTOptimizer();
    
            
        public:
            ///
            virtual bool modifiesPanoramaData()
                { return true; }
    };
    
    
    ///
    class AutoOptimise : PTOptimizer
    {
        
        public:
            ///
            AutoOptimise(PanoramaData& panorama)
             : PTOptimizer(panorama)
            {};
        
            ///
            virtual ~AutoOptimise();
            
            
        public:
            ///
            virtual bool runAlgorithm()
            {
                autoOptimise(o_panorama);
                return true; // let's hope so.
            }

    };
    
    
    class SmartOptimise : PTOptimizer
    {
        
        public:
            ///
            SmartOptimise(PanoramaData& panorama)
             : PTOptimizer(panorama)
            {};
        
            ///
            virtual ~SmartOptimise();
            
            
        public:
            ///
            virtual bool runAlgorithm()
            {
                smartOptimise(o_panorama);
                return true; // let's hope so.
            }

    }