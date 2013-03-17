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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "PTScriptParsing.h"

#include <hugin_utils/utils.h>
#include <stdio.h>


namespace HuginBase {
namespace PTScriptParsing {



/// helper functions for parsing of a script line
bool getPTParam(std::string & output, const std::string & line, const std::string & parameter)
{
    int len = line.size();
    for (int i=1; i < len; i++) {
        if (line[i-1] == ' ' && line[i] != ' ') {
            // beginning of a parameter
            std::string par = line.substr(i, parameter.length());
            if (par == parameter) {
                // found, skip to value
                i = i+parameter.length();
                if ( i >= len ) {
                    // parameter without value, not valid.
                    output = "";
                    return true;
                }
                if (line[i]== '"') {
                    i++;
                    if ( i >= len ) {
                        return false;
                    }
                    // this is a string parameter, skip to next "
                    size_t end = line.find('"', i);
                    if (end == std::string::npos) {
                        // unclosed string found
                        return false;
                    }
                    output = line.substr(i,end-i);
                    return true;
                } else {
                    // ordinary parameter, skip to next space
                    size_t end = line.find_first_of(" \t\n", i);
                    output = line.substr(i, end-i);
                    return true;
                }
            } else {
                // this is another parameter, skip it
                i++;
                if ( i >= len ) {
                    return false;
                }
                if (line[i]== '"') {
                    i++;
                    // this is a string parameter, skip to next "
                    size_t end = line.find('"', i);
                    if (end == std::string::npos) {
                        // unclosed string found
                        return false;
                    }
                    i = end;
                    if ( i >= len ) {
                        return false;
                    }
                } else {
                    // this is an ordinary parameter, skip to next space
                    size_t end = line.find_first_of(" \t\n", i);
                    if (end == std::string::npos) {
                        // not found, last parameter
                        return false;
                    }
                    i = end;
                }
            }
        }
    }
    return false;
}

#if 0
bool getPTParam(std::string & output, const std::string & line, const std::string & parameter)
{
    std::string::size_type p;
    if ((p=line.find(std::string(" ") + parameter)) == std::string::npos) {
        DEBUG_INFO("could not find param " << parameter
                   << " in line: " << line);
        return false;
    }
    p += parameter.length() + 1;
    std::string::size_type p2 = line.find(' ',p);
    output = line.substr(p, p2-p);
    //    DEBUG_DEBUG("string idex: " << p <<"," << p2 << "  string: \"" << output << "\"");
    return true;
}

bool getPTStringParam(std::string & output, const std::string & line, const std::string & parameter)
{
    std::string::size_type p;
    if ((p=line.find(std::string(" ") + parameter + "\"")) == std::string::npos) {
        DEBUG_INFO("could not find string param " << parameter
                   << " in line: " << line);
        return false;
    }
    p += parameter.length() + 2;
    std::string::size_type e = line.find("\"",p);
    DEBUG_DEBUG("p:" << p << " e:" << e);
    output = line.substr(p,e-p);
    DEBUG_DEBUG("output: ##" << output << "##");
    return true;
}

bool getPTStringParamColon(std::string & output, const std::string & line, const std::string & parameter)
{
    std::string::size_type p;
    if ((p=line.find(std::string(" ") + parameter + ":")) == std::string::npos) {
        DEBUG_INFO("could not find string param " << parameter
                   << " in line: " << line);
        return false;
    }
    p += parameter.length() + 2;
    std::string::size_type e = line.find(" ",p);
    DEBUG_DEBUG("p:" << p << " e:" << e);
    output = line.substr(p,e-p);
    DEBUG_DEBUG("output: ##" << output << "##");
    return true;
}
#endif

bool getDoubleParam(double & d, const std::string & line, const std::string & name)
{
    std::string s;
    if (!getPTParam(s, line, name)) {
        return false;
    }
    return hugin_utils::stringToDouble(s, d);
}

bool getPTDoubleParam(double & value, int & link,
                          const std::string & line, const std::string & var)
{
    std::string val;
    if (getPTParam(val,line, var)) {
        DEBUG_ASSERT(line.size() > 0);
        DEBUG_DEBUG(var << ":" <<val);
        if (val[0] == '=') {
            link = hugin_utils::lexical_cast<int>(val.substr(1));
        } else {
            link = -1;
            if (!hugin_utils::stringToDouble(val, value)) {
                return false;
            }
        }
    } else {
        return false;
    }
    return true;
}

bool readVar(Variable & var, int & link, const std::string & line)
{
    std::string val;
    if (getPTParam(val,line, var.getName())) {
        DEBUG_ASSERT(line.size() > 0);
        DEBUG_DEBUG(var.getName() << ":" <<val);
        if (val[0] == '=') {
            link = hugin_utils::lexical_cast<int>(val.substr(1));
        } else {
            link = -1;
            double dest = 0;
            if (!hugin_utils::stringToDouble(val, dest)) {
                return false;
            }
            var.setValue(dest);
        }
    } else {
        return false;
    }
    return true;
}





// cannot use Lens::variableNames here, because r,p,v,j need to be included
/// @todo Use information from image_variables.h and ImageVariableTranslate.h instead?
const char * ImgInfo::varnames[] = {"v", "a","b","c", "d","e", "g","t", "r","p","y","j","TrX", "TrY", "TrZ", "Tpy", "Tpp",
				    "Va", "Vb", "Vc", "Vd",  "Vx", "Vy",
				    "Eev", "Er",  "Eb",
				    "Ra", "Rb", "Rc", "Rd", "Re",  0};

double ImgInfo::defaultValues[] = {51.0,  0.0, 0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
				   1.0, 0.0, 0.0, 0.0,   0.0, 0.0, 
				   0.0, 1.0,  1.0, 
				   0.0, 0.0, 0.0, 0.0, 0.0};

    
void ImgInfo:: init()
{
    // blend_radius = 0;
    width = -1;
    height = -1;
    f = -2;
    vigcorrMode = 0;  // default to no correction
                      // is transformed to correction by division later
    responseType = 0; // default to EMOR
    for (const char ** v = varnames; *v != 0; v++) {
        vars[*v] = 0;
        links[*v] = -2;
    }
    autoCenterCrop = true;
    cropFactor = 1;
    enabled = true;
}


void ImgInfo::parse(const std::string & line)
{
    double * val = defaultValues;
    for (const char ** v = varnames; *v; v++, val++) {
        vars[*v] = *val;
        links[*v] = -1;
        std::string name;
        name = *v;
        getPTDoubleParam(vars[*v], links[*v], line, name);
    }
    
    // getIntParam(blend_radius, line, "u");
    
    // read lens type and hfov
    getIntParam(f, line, "f");
    
    getPTParam(filename,line,"n");
    getIntParam(width, line, "w");
    getIntParam(height, line, "h");
    
    getIntParam(vigcorrMode, line, "Vm");
    // HACK: force Va1, for all images that use the a polynomial vig correction mode.
    // reset to vignetting correction by division.
    if (vigcorrMode != 5) {
        vigcorrMode = 5;
        vars["Va"] = 1.0;
        vars["Vb"] = 0.0;
        vars["Vc"] = 0.0;
        vars["Vd"] = 0.0;
    }
    
    getIntParam(responseType, line, "Rt");
    getPTParam(flatfieldname,line,"Vf");
    
    std::string crop_str;
    if ( getPTParam(crop_str, line, "C") ) {
        int left, right, top, bottom;
        int n = sscanf(crop_str.c_str(), "%d,%d,%d,%d", &left, &right, &top, &bottom);
        if (n == 4) {
            crop = vigra::Rect2D(left, top, right, bottom);
        } else {
            DEBUG_WARN("Could not parse crop string: " << crop_str);
        }
    }
    if ( getPTParam(crop_str, line, "S") ) {
        int left, right, top, bottom;
        int n = sscanf(crop_str.c_str(), "%d,%d,%d,%d", &left, &right, &top, &bottom);
        if (n == 4) {
            crop = vigra::Rect2D(left, top, right, bottom);
        } else {
            DEBUG_WARN("Could not parse crop string: " << crop_str);
        }
    }
    
    
}



}
} // namespace
