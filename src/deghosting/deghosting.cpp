
/**
 * Implementation of basic routines for deghosting algorithms
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

#include "deghosting.h"

using namespace vigra;

namespace deghosting {
    
    void Deghosting::loadImages(std::vector<std::string>& newInputFiles) throw(NoImages, BadDimensions) {
        if (newInputFiles.empty())
            throw NoImages();
        const ImageImportInfo firstInfo = ImageImportInfo(newInputFiles[0].c_str());
        const int width = firstInfo.width();
        const int height = firstInfo.height();
        for (unsigned int i = 0; i< newInputFiles.size(); i++) {
            ImageImportInfo tmpInfo = ImageImportInfo(newInputFiles[i].c_str());
            if ((width != tmpInfo.width()) || (height != tmpInfo.height()))
                throw BadDimensions();
            inputFiles.push_back(tmpInfo);
        }
     }
    
    void Deghosting::loadImages(std::vector<vigra::ImageImportInfo>& newInputFiles) throw(NoImages, BadDimensions) {
        if (newInputFiles.empty())
            throw NoImages();
        const int width = newInputFiles[0].width();
        const int height = newInputFiles[0].height();
        for (unsigned int i = 0; i< newInputFiles.size(); i++) {
            if ((width != newInputFiles[i].width()) || (height != newInputFiles[i].height()))
                throw BadDimensions();
        }
        inputFiles = newInputFiles;
    }

    void Deghosting::setFlags(const uint16_t newFlags) {
        flags = newFlags;
    }

    void Deghosting::setDebugFlags(const uint16_t newFlags) {
        debugFlags = newFlags;
    }

    void Deghosting::setIterationNum(const int newIterations) {
        iterations = newIterations;
    }

    void Deghosting::setCameraResponse(EMoR newResponse) {
        response = newResponse;
    }

    void Deghosting::setVerbosity(int newVerbosity) {
        verbosity = newVerbosity;
    }

}

