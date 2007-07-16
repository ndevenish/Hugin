// -*- c-basic-offset: 4 -*-
/** @file utils.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: utils.h 1814 2006-12-31 14:37:05Z dangelo $
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

#ifndef _BASICALGORITHM_PANORAMAMAKEFILEEXPORT_H
#define _BASICALGORITHM_PANORAMAMAKEFILEEXPORT_H

#include <algorithm/PanoramaAlgorithm.h>

#include <iostream>


namespace HuginBase {
    
///
class PanoramaMakefileExport : public PanoramaAlgorithm
{
    public:
        ///
        typedef std::string String;
    
        ///
        struct PTPrograms
        {
            std::string nona;
            std::string PTStitcher;
            std::string PTmender;
            std::string PTblender;
            std::string PTmasker;
            std::string PTroller;
            std::string enblend;
            std::string enblend_opts;
            std::string smartblend;
            std::string smartblend_opts;
            
            PTPrograms()
            {
                    // default programs
                    nona = "nona";
                    PTStitcher = "PTStitcher";
                    PTmender = "PTmender";
                    PTblender = "PTblender";
                    PTmasker = "PTmasker";
                    PTroller = "PTroller";
                    enblend = "enblend";
                    smartblend = "smartblend.exe";
            }
        };
        
        ///
        PanoramaMakefileExport(PanoramaData& pano,
                               std::ostream& output,
                               const String& ptofile,
                               const String& outputPrefix,
                               const PTPrograms& progs,
                               const String& includePath)
         : PanoramaAlgorithm(pano), o_output(output),
           o_ptofile(ptofile), o_outputPrefix(outputPrefix), o_progs(progs), o_includePath(includePath)
        {};
        
        ///
        virtual ~PanoramaMakefileExport() {};
        
        
    public:
        ///
        static void createMakefile(const PanoramaData & pano,
                                   const std::string & ptofile,
                                   const std::string & outputPrefix,
                                   const PTPrograms & progs,
                                   const std::string & includePath,
                                   std::ostream & o);
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return false; }
        
        ///
        bool runAlgorithm()
        {
            createMakefile(o_panorama,
                           o_ptofile, o_outputPrefix, o_progs, o_includePath,
                           o_output);
            
            return true; // let's hope so.
        }
        
        
    protected:
            std::ostream& o_output;
        
            String o_ptofile;
            String o_outputPrefix;
            PTPrograms o_progs;
            String o_includePath;
};
        

}//namespace
#endif