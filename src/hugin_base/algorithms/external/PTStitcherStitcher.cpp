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

#include "PTStitcherStitcher.h"

#include <fstream>


using namespace AppBase;

namespace HuginBase {
    
        
bool ExternalFileOutputStitcherBase::runStitcher()
{
    if(!isCompatible())
    {
        // [TODO] DEBUG_WARNING()
        return false;
    }
    
    if(!writeScriptFile(o_scriptFile))
    {
        // [TODO] DEBUG_WARNING()
        return false;
    }
    
    if(!prepareExternalProgram(o_program))
    {
        // [TODO] DEBUG_WARNING()
        return false;
    }
    
    if(o_programExecutor == NULL)
    {
        // [TODO] DEBUG_WARNING("executor is NULL");
        return false;
    }
    
    int result = o_programExecutor->executeProgram(&o_program);
    if(result == ExternalProgramExecutor::INTERRUPTED)
        cancelAlgorithm();
    return (result == ExternalProgramExecutor::NORMAL);
}


bool ExternalFileOutputStitcherBase::writeScriptFile(const ExternalFileOutputStitcherBase::String& filepath)
{
    std::ofstream scriptfile(filepath.c_str());
    if (!scriptfile.good())
    {
        DEBUG_FATAL("could not open/create PTScript file");
        return false;
    }
    o_panorama.printStitcherScript(scriptfile, o_panoramaOptions, o_usedImages);
    scriptfile.close();
    return true;
}



PTStitcherProgramSetup::String PTStitcherProgramSetup::defaultCommand() const
{
    #ifdef WIN32
    return "PTStitcher.exe";
    #else
    return "PTStitcher";
    #endif
}


PTStitcherProgramSetup::String PTStitcherProgramSetup::defaultArgumentTemplate() const
    { return "-o {OUTPUT} {SCRIPT} {INPUT}"; }


PTStitcherProgramSetup::StringSet PTStitcherProgramSetup::getAvailableStringKeywords() const
{
    StringSet keywords;
    keywords.insert("OUTPUT");
    keywords.insert("SCRIPT");
    keywords.insert("INPUT");
    return keywords;
}

PTStitcherProgramSetup::String PTStitcherProgramSetup::getStringForKeyword(const String& keyword)
{
    if(keyword == "OUTPUT")
        return getStringForKeyword_OUTPUT();
        
    if(keyword == "SCRIPT")
        return getStringForKeyword_SCRIPT();
    
    if(keyword == "INPUT")
        return getStringForKeyword_INPUT();
        
    // [TODO] DEBUG_WARNING("unsupported keyword");
    return "";
}




bool PTStitcherFileOutputStitcher::isCompatible()
{
    if(hugin_utils::stripExtension(o_panoramaOptions.outfile).find('.') != String::npos)
    {
        //[TODO] debug
        return false;
    }
    
    if ( o_panoramaOptions.outputFormat == PanoramaOptions::QTVR ) {
        //[TODO] debug
        return false;
    }
    
    return true;
}

PTStitcherFileOutputStitcher::String PTStitcherFileOutputStitcher::getStringForKeyword_OUTPUT()
{
    return quoteFilename(hugin_utils::stripExtension(o_panoramaOptions.outfile));
}

PTStitcherFileOutputStitcher::String PTStitcherFileOutputStitcher::getStringForKeyword_SCRIPT()
{
    return quoteFilename(o_scriptFile);
}

PTStitcherFileOutputStitcher::String PTStitcherFileOutputStitcher::getStringForKeyword_INPUT()
{
    return "";
}

}//namespace
