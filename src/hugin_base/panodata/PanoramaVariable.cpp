// -*- c-basic-offset: 4 -*-
/** @file PanoramaVariable.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  !! from PanoramaMemento.h 1970
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

#include "PanoramaVariable.h"

#include <iostream>
#include <iomanip>
#include <utility>


namespace HuginBase {
    

std::ostream & Variable::print(std::ostream & o) const
{
    return o << name << std::setprecision(15) << value;
}

std::ostream & LensVariable::printLink(std::ostream & o,
                                       unsigned int linkImage) const
{
    return o << name << "=" << linkImage;
}


void fillVariableMap(VariableMap & vars)
{
    vars.insert(std::pair<const char*, Variable>("y",Variable("y",0)));
    vars.insert(std::pair<const char*, Variable>("r",Variable("r",0)));
    vars.insert(std::pair<const char*, Variable>("p",Variable("p",0)));

    vars.insert(std::pair<const char*, Variable>("TrX",Variable("TrX",0)));
    vars.insert(std::pair<const char*, Variable>("TrY",Variable("TrY",0)));
    vars.insert(std::pair<const char*, Variable>("TrZ",Variable("TrZ",0)));

    vars.insert(std::pair<const char*, Variable>("Tpy",Variable("Tpy",0)));
    vars.insert(std::pair<const char*, Variable>("Tpp",Variable("Tpp",0)));

    vars.insert(std::pair<const char*, Variable>("j",Variable("j",0)));
    
    // Lens variables
    vars.insert(std::pair<const char*, Variable>("v",Variable("v",51)));
    vars.insert(std::pair<const char*, Variable>("a",Variable("a",0.0)));
    vars.insert(std::pair<const char*, Variable>("b",Variable("b",0.0)));
    vars.insert(std::pair<const char*, Variable>("c",Variable("c",0.0)));
    vars.insert(std::pair<const char*, Variable>("d",Variable("d",0)));
    vars.insert(std::pair<const char*, Variable>("e",Variable("e",0)));
    vars.insert(std::pair<const char*, Variable>("g",Variable("g",0)));
    vars.insert(std::pair<const char*, Variable>("t",Variable("t",0)));
    
    vars.insert(std::pair<const char*, Variable>("Va",Variable("Va",1)));
    vars.insert(std::pair<const char*, Variable>("Vb",Variable("Vb",0)));
    vars.insert(std::pair<const char*, Variable>("Vc",Variable("Vc",0)));
    vars.insert(std::pair<const char*, Variable>("Vd",Variable("Vd",0)));
    vars.insert(std::pair<const char*, Variable>("Vx",Variable("Vx",0)));
    vars.insert(std::pair<const char*, Variable>("Vy",Variable("Vy",0)));
    
    // exposure value and white balance
    vars.insert(std::pair<const char*, Variable>("Eev",Variable("Eev",0.0)));
    vars.insert(std::pair<const char*, Variable>("Er",Variable("Er",1.0)));
    vars.insert(std::pair<const char*, Variable>("Eb",Variable("Eb",1.0)));
    
    // emor response variables
    vars.insert(std::pair<const char*, Variable>("Ra",Variable("Ra",0.0)));
    vars.insert(std::pair<const char*, Variable>("Rb",Variable("Rb",0.0)));
    vars.insert(std::pair<const char*, Variable>("Rc",Variable("Rc",0.0)));
    vars.insert(std::pair<const char*, Variable>("Rd",Variable("Rd",0.0)));
    vars.insert(std::pair<const char*, Variable>("Re",Variable("Re",0.0)));
}

void fillLensVarMap(LensVarMap & variables)
{
    variables.insert(std::pair<const char*, LensVariable>("v",LensVariable("v", 51, true)));
    variables.insert(std::pair<const char*, LensVariable>("a",LensVariable("a", 0.0, true )));
    variables.insert(std::pair<const char*, LensVariable>("b",LensVariable("b", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("c",LensVariable("c", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("d",LensVariable("d", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("e",LensVariable("e", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("g",LensVariable("g", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("t",LensVariable("t", 0.0, true)));
    
    // vignetting correction variables
    variables.insert(std::pair<const char*, LensVariable>("Va",LensVariable("Va", 1.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Vb",LensVariable("Vb", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Vc",LensVariable("Vc", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Vd",LensVariable("Vd", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Vx",LensVariable("Vx", 0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Vy",LensVariable("Vy", 0.0, true)));
    
    // exposure
    variables.insert(std::pair<const char*, LensVariable>("Eev",LensVariable("Eev", 0.0, false)));
    variables.insert(std::pair<const char*, LensVariable>("Er",LensVariable("Er", 1.0, false)));
    variables.insert(std::pair<const char*, LensVariable>("Eb",LensVariable("Eb", 1.0, false)));
    
    // emor response variables
    variables.insert(std::pair<const char*, LensVariable>("Ra",LensVariable("Ra",0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Rb",LensVariable("Rb",0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Rc",LensVariable("Rc",0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Rd",LensVariable("Rd",0.0, true)));
    variables.insert(std::pair<const char*, LensVariable>("Re",LensVariable("Re",0.0, true)));
}


void printVariableMap(std::ostream & o, const VariableMap & vars)
{
    for ( VariableMap::const_iterator it = vars.begin(); it != vars.end();++it)
    {
        it->second.print(o);
        o << " ";
    }
}


} // namespace
