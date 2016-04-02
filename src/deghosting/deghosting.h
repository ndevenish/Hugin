
/**
 * Header file for base class for deghosting algorithms
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef DEGHOSTING_H_
#define DEGHOSTING_H_

#include <exception>
#include <vector>
#include <string>
#include <stdint.h>

#include <memory>
#include <vigra/stdimage.hxx>
#include <vigra/imageinfo.hxx>

#ifdef _MSC_VER
#define THROWNOIMAGESBADDIMENSION
#else
#define THROWNOIMAGESBADDIMENSION throw(NoImages, BadDimensions)
#endif

namespace deghosting {
    
    /** exception called when image dimensions differ
     */
    class BadDimensions : public std::exception {
        public:
            BadDimensions() : std::exception() {};
            virtual const char * what() const throw() {
                return "Input images must have the same dimensions";
            }
    };
    
    /** exception called when there are no input images
     */
    class NoImages : public std::exception {
        public:
            NoImages() : std::exception() {};
            virtual const char * what() const throw() {
               return "You must specify images";
            }
    };
    
    typedef std::shared_ptr<vigra::BImage> BImagePtr;
    typedef std::shared_ptr<vigra::FImage> FImagePtr;
    // type for camera response
    typedef std::vector<float> EMoR;

    // constants for advanced modes
    const uint16_t ADV_GAMMA         = 1;
    const uint16_t ADV_ONLYP         = 2;
    const uint16_t ADV_MULTIRES      = 4;

    // constants for debug modes
    const uint16_t SAVE_INITWEIGHTS   = 1;

    class Deghosting
    {
    public:
        Deghosting() : flags(0), debugFlags(0), iterations(0), verbosity(0) {}
        
        /** create weight masks
         * create weight masks for masking out ghosting regions
         */
        virtual std::vector<FImagePtr> createWeightMasks() = 0;

        /** load images for processing
         * @param inputFiles images to be processed
         */
        virtual void loadImages(std::vector<std::string>& inputFiles) THROWNOIMAGESBADDIMENSION;
        virtual void loadImages(std::vector<vigra::ImageImportInfo>& inputFiles) THROWNOIMAGESBADDIMENSION;

        /** set advanced flags
         * Allows to change behavior of used algorithm
         * @param flags one of the constants describing advanced mode
         */
        virtual void setFlags(const uint16_t flags);
        
        /** set flags for debugging purposes
         * @param debugFlags one of the constants describing action which should be done
         */
        virtual void setDebugFlags(const uint16_t debugFlags);
        
        /** set number of iterations
         */
        virtual void setIterationNum(const int iterations);
        
        /** set camera response function
         * set camera response function in EMoR format
         * @param response array of five floats representing response
         */
        virtual void setCameraResponse(EMoR response);
        
        /** set verbosity level
         * @param verbosity the higher the number is, the more verbose algorithm will be
         */
        virtual void setVerbosity(int verbosity);
        virtual ~Deghosting() {}
        
    protected:
        std::vector<vigra::ImageImportInfo> inputFiles;
        uint16_t flags;
        uint16_t debugFlags;
        int iterations;
        EMoR response;
        int verbosity;
    };

}

#endif /* DEGHOSTING_H_ */
