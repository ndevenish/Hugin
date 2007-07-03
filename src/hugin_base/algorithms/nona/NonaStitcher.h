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


using namespace AppBase;
using namespace Nona;

namespace HuginBase {
    
    /**
     *
     */
    class NonaImageStitcher : ImageStitcherAlgorithm
    {
        public:
            ///
            NonaImageStitcher(const PanoramaData& panoramaData,
                                 ProgressDisplay* progressDisplay,
                                 const PanoramaOptions& options,
                                 const UIntSet& usedImages,
                                 DestImage& panoImage, DestAlpha& alpha
                                 SingleImageRemapper& remapper)
             : ImageStitcherAlgorithm(panoramaData, progressDisplay, options, usedImages, panoImage, alpha),
               o_remapper(remapper) 
            {};

                
        protected:
            ///
            virtual bool runStitcher()
            {
                MultiProgressDisplayAdaptor* progDisp
                    = MultiProgressDisplayAdaptor::newMultiProgressDisplay(getProgressDisplay());
                
                StackingBlender blender;
                SimpleStitcher<DestImage, DestAlpha> stitcher(o_panorama, *progDisp);
                stitcher.stitch(opts, usedImages,
                                destImageRange(o_panoImage), destImage(o_alpha),
                                o_remapper,
                                blender);
                
                delete progDisp;
                
                return true;
            }
            
            
        protected:
            SingleImageRemapper& o_remapper;
        
    };
    
    
    /**
     *
     */
    class NonaDifferenceImageStitcher : ImageStitcherAlgorithm
    {
    public:
        ///
        NonaDifferenceImageStitcher(const PanoramaData& panoramaData,
                             ProgressDisplay* progressDisplay,
                             const PanoramaOptions& options,
                             const UIntSet& usedImages,
                             DestImage& panoImage, DestAlpha& alpha, SingleImageRemapper& remapper)
         : NonaImageStitcher(panoramaData, progressDisplay, options, usedImages, panoImage, alpha, remapper)
        {};
        
        ///
        ~NonaDifferenceImageStitcher();
        
            
    protected:
        ///
        virtual bool runStitcher()
        {
            MultiProgressDisplayAdaptor* progDisp
                = MultiProgressDisplayAdaptor::newMultiProgressDisplay(getProgressDisplay());
            
            ReduceToDifferenceFunctor<RGBValue<float>> func;
            ReduceStitcher<DestImage, DestAlpha> stitcher(o_panorama, *progDisp);
            stitcher.stitch(opts, usedImages,
                            destImageRange(o_panoImage), destImage(o_alpha),
                            o_remapper,
                            func);
            
            delete progDisp;
            
            return true;
        }
        
    };
    
    
    /**
     *
     */
    class NonaHDRImageStitcher : NonaImageStitcher
    {
    public:
        ///
        NonaHDRImageStitcher(const PanoramaData& panoramaData,
                             ProgressDisplay* progressDisplay,
                             const PanoramaOptions& options,
                             const UIntSet& usedImages,
                             DestImage& panoImage, DestAlpha& alpha, SingleImageRemapper& remapper)
         : NonaImageStitcher(panoramaData, progressDisplay, options, usedImages, panoImage, alpha, remapper)
        {};
        
        ///
        ~NonaHDRImageStitcher();
        
            
    protected:
        ///
        virtual bool runStitcher()
        {
            MultiProgressDisplayAdaptor* progDisp
                = MultiProgressDisplayAdaptor::newMultiProgressDisplay(getProgressDisplay());
            
            ReduceToHDRFunctor<RGBValue<float>> hdrmerge;
            ReduceStitcher<DestImage, DestAlpha> stitcher(o_panorama, *progDisp);
            stitcher.stitch(opts, usedImages,
                            destImageRange(o_panoImage), destImage(o_alpha),
                            o_remapper,
                            hdrmerge);
            
            delete progDisp;
            
            return true;
        }
        
    };
    
    
    
    
    
    
    /** This class will use the stitchPanorama function of nona. The filename
     *  may be automatically modified preserving only the basename. 
     */
    class NonaFileOutputStitcher : FileOutputStitcherAlgorithm
    {
        
    public:
        NonaFileOutputStitcher(const PanoramaData& panoramaData,
                                    ProgressDisplay* progressDisplay,
                                    const PanoramaOptions& options,
                                    const UIntSet& usedImages,
                                    const String& filename)
            : FileOutputStitcherAlgorithm(panoramaData, options, usedImages, progressDisplay, filename, true)
        {};
        
        ///
        ~NonaFileOutputStitcher();
        
        
    protected:
        ///
        virtual bool runStitcher();  // uses Nona::stitchPanorama()
        
    };
    
    
};