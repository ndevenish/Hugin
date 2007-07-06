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
#ifndef _PTSTITCHERSTITCHER_H
#define _PTSTITCHERSTITCHER_H

#include <algorithm/StitcherAlgorithm.h>
#include <appbase/ExternalProgramSetup.h>



using namespace AppBase;

namespace HuginBase {
    
        
    ///
    class ExternalFileOutputStitcherBase : public FileOutputStitcherAlgorithm
    {
        public:
            ///
            ExternalFileOutputStitcherBase(PanoramaData& panoramaData,
                                           ExternalProgramExecutor* executor,
                                           const PanoramaOptions& options,
                                           const UIntSet& usedImages,
                                           const String& scriptFilePath,
                                           const String& filename, const bool& addExtension = true)
                : FileOutputStitcherAlgorithm(panoramaData, NULL, options, usedImages, filename, addExtension),
                  o_scriptFile(scriptFilePath), o_programExecutor(executor)
            {};
            
            ///
            virtual ~ExternalFileOutputStitcherBase() {};
        
        
        public:
            ///
            virtual bool runStitcher();
            
        protected:
            /// preliminary checking; interface that can handle error message would be more desireable. 
            virtual bool isCompatible() =0;
        
            ///
            virtual bool prepareExternalProgram(ExternalProgram& program) =0;
            
            /// 
            virtual bool writeScriptFile(const String& filepath);
            
            
        public:
            ///
            const ExternalProgram& getExternalProgram() const
                { return o_program; }
            
            
        protected:
            ExternalProgramExecutor* o_programExecutor;
            ExternalProgram o_program;
            String o_scriptFile;
    };
    
    
    
    ///
    class PTStitcherProgramSetup : public ExternalProgramSetup
    {
        public:
            PTStitcherProgramSetup()
                : ExternalProgramSetup()
            {};
         
            virtual ~PTStitcherProgramSetup() {};
        
        public:
            ///
            virtual String defaultCommand() const;
        
            ///
            virtual String defaultArgumentTemplate() const;
            
            ///
            virtual StringSet getAvailableStringKeywords() const;     
            
        protected:
            ///
            virtual String getStringForKeyword(const String& keyword);
            
            ///
            virtual String getStringForKeyword_OUTPUT() =0;
            
            ///
            virtual String getStringForKeyword_SCRIPT() =0;
            
            ///
            virtual String getStringForKeyword_INPUT() =0;
            
    };
    
    
    ///
    class PTStitcherFileOutputStitcher : public ExternalFileOutputStitcherBase,
                                         public PTStitcherProgramSetup
    {
        public:
            
            typedef ExternalFileOutputStitcherBase::String String;
        
            ///
            PTStitcherFileOutputStitcher(PanoramaData& panoramaData,
                                         ExternalProgramExecutor* executor,
                                         const PanoramaOptions& options,
                                         const UIntSet& usedImages,
                                         const String& scriptFilePath,
                                         const String& filename, const bool& addExtension = true)
              : PTStitcherProgramSetup(),
                ExternalFileOutputStitcherBase(panoramaData,
                                               executor,
                                               options,
                                               usedImages,
                                               scriptFilePath,
                                               filename, addExtension)
            {};
        
            ///
            virtual ~PTStitcherFileOutputStitcher() {};
        
        
        protected:
            ///
            virtual bool isCompatible();
            
            ///
            virtual bool prepareExternalProgram(ExternalProgram& program)
                { return setupExternalProgram(&program); }
        
        
        protected:
            ///
            virtual String getStringForKeyword_OUTPUT();
            
            ///
            virtual String getStringForKeyword_SCRIPT();
            
            ///
            virtual String getStringForKeyword_INPUT();
    };
    
} //namespace
#endif //_H