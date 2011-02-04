// -*- c-basic-offset: 4 -*-
/** @file 
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

#ifndef _NONAIMAGESTITCHER_H
#define _NONAIMAGESTITCHER_H

#include <algorithm/StitcherAlgorithm.h>

#include <nona/Stitcher.h>
#include <nona/ImageRemapper.h>


namespace HuginBase {
    

    /**
     *
     */
    class NonaImageStitcher : public ImageStitcherAlgorithm
    {
#ifdef HUGIN_HSI
        public:
#else
        protected:
#endif
            typedef Nona::SingleImageRemapper<DestImage,DestAlpha> ImageMapper;
        
        public:
            ///
            NonaImageStitcher(PanoramaData& panoramaData,
                              AppBase::ProgressDisplay* progressDisplay,
                              const PanoramaOptions& options,
                              const UIntSet& usedImages,
                              DestImage& panoImage, DestAlpha& alpha,
                              ImageMapper& remapper)
             : ImageStitcherAlgorithm(panoramaData, progressDisplay, options, usedImages, panoImage, alpha),
               o_remapper(remapper) 
            {};
        
            ///
            ~NonaImageStitcher() {};

                
        protected:
            ///
            virtual bool runStitcher();
            
            
        protected:
            ImageMapper& o_remapper;
        
    };
    
    
    /**
     *
     */
    class NonaDifferenceImageStitcher : public NonaImageStitcher
    {
        public:
            ///
            NonaDifferenceImageStitcher(PanoramaData& panoramaData,
                                 AppBase::ProgressDisplay* progressDisplay,
                                 const PanoramaOptions& options,
                                 const UIntSet& usedImages,
                                 DestImage& panoImage, DestAlpha& alpha,
                                 ImageMapper& remapper)
             : NonaImageStitcher(panoramaData, progressDisplay, options, usedImages, panoImage, alpha, remapper)
            {};
            
            ///
            ~NonaDifferenceImageStitcher() {};
            
                
        protected:
            ///
            virtual bool runStitcher();
            
    };
    
    
    /**
     *
     */
    class NonaHDRImageStitcher : public NonaImageStitcher
    {
        public:
            ///
            NonaHDRImageStitcher(PanoramaData& panoramaData,
                                 AppBase::ProgressDisplay* progressDisplay,
                                 const PanoramaOptions& options,
                                 const UIntSet& usedImages,
                                 DestImage& panoImage, DestAlpha& alpha, 
                                 ImageMapper& remapper)
             : NonaImageStitcher(panoramaData, progressDisplay, options, usedImages, panoImage, alpha, remapper)
            {};
            
            ///
            ~NonaHDRImageStitcher() {};
            
                
        protected:
            ///
            virtual bool runStitcher();
        
    };
    
    
} // namespace
#endif // _H
