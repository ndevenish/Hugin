/**
 * @file StitchingExecutor.h
 * @brief interface of CommandQueue creating for stitching engine
 *  
 * @author T. Modes
 */

/*  This is free software; you can redistribute it and/or
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

#ifndef STITCHINGEXECUTOR_H
#define STITCHINGEXECUTOR_H

#include <hugin_shared.h>
#include <panodata/Panorama.h>
#include <wx/arrstr.h>
#include "Executor.h"

namespace HuginQueue
{
    /** generates the command queue for stitching a pano 
        it will also generate the necessary exiftool argfiles 
        @param[in] pano panorama structure containing the input project
        @param[in] ExePath ExePath base path to all used utilities
        @param[in] project name of the project file, which contains the pano data (should by in sync with pano, to prevent double loading of the data)
        @param[in] prefix prefix of the output, should only contain the filename part
        @param[out] statusText contains a short status text, can be printed before the queue is actually executed, useful for bug reports
        @param[out] outputFiles array of all output files, contains also the temporary files created during stitching (used for detecting of overwritting files)
        @param[out] tempFilesDelete array with all temporary files which should be deleted at the end
        @return pointer to CommandQueue
        */
    WXIMPEX CommandQueue* GetStitchingCommandQueue(const HuginBase::Panorama & pano, const wxString& ExePath, const wxString& project, const wxString& prefix, wxString& statusText, wxArrayString& outputFiles, wxArrayString& tempFilesDelete);

}; // namespace 

#endif
