// -*- c-basic-offset: 4 -*-
/** @file PTScriptParsing.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 * !! Panorama.h 1947 
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PANODATA_PTSCRIPTPARSING_H
#define _PANODATA_PTSCRIPTPARSING_H

#include <hugin_shared.h>
#include <string>
#include <vigra/diff2d.hxx>

#include <panodata/PanoramaVariable.h>



namespace HuginBase {
namespace PTScriptParsing {

        
    /// helper functions for parsing a script line
    IMPEX bool getPTParam(std::string & output, const std::string & line, const std::string & parameter);

//  template <class T>
//  bool getParam(T & value, const std::string & line, const std::string & name);

    ///
    template <class T>
    bool getIntParam(T & value, const std::string & line, const std::string & name);
    

    bool readVar(Variable & var, int & link, const std::string & line);
/*
    bool getPTStringParam(std::string & output, const std::string & line,
                          const std::string & parameter);

    bool getPTStringParamColon(std::string & output, const std::string & line, const std::string & parameter);
*/

    bool getDoubleParam(double & d, const std::string & line, const std::string & name);

    bool getPTDoubleParam(double & value, int & link,
                          const std::string & line, const std::string & var);
    ///
    struct ImgInfo
    {        
        std::string filename;
        std::string flatfieldname;
        std::map<std::string, double> vars;
        std::map<std::string, int> links;
        int f;
        // int blend_radius;
        int width, height;
        int vigcorrMode;
        int responseType;
        vigra::Rect2D crop;
        bool autoCenterCrop;
        double cropFactor;
        bool enabled;


    public:
        ImgInfo()
        {
            init();
        }

        explicit ImgInfo(const std::string & line)
        {
            init();
            this->parse(line);
        }

    protected:
        void init();

    public:
        void parse(const std::string & line);

    public:
        static const char *varnames[];
        static double defaultValues[];

    };


//==============================================================================
// template implementation
    
    
#if 0
    template <class T>
    bool getParam(T & value, const std::string & line, const std::string & name)
    {
            std::string s;
            if (!getPTParam(s, line, name)) {
                return false;
            }
            std::istringstream is(s);
            is >> value;
            return true;
    }
#endif
    
    template <class T>
    bool getIntParam(T & value, const std::string & line, const std::string & name)
    {
            std::string s;
            if (!getPTParam(s, line, name)) {
                return false;
            }
            std::istringstream is(s);
            is >> value;
            return true;
    }
    
} // namespace
} // namespace
#endif //_H
