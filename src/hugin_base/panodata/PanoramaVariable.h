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

#ifndef _PANODATA_PANORAMAVARIABLE_H
#define _PANODATA_PANORAMAVARIABLE_H


#include <hugin_shared.h>
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>

#include <hugin_utils/stl_utils.h>


namespace HuginBase {

/** a variable has a value and a name.
 *
 *  linking is only supported by LinkedVariable, which
 *  is only used by Lens.
 */

class IMPEX Variable
{
public :
// in the hsi version, class Variable needs a default Ctor    
#ifdef HUGIN_HSI
        Variable(const std::string & name = "" ,
                 double val = 0.0)
                 : name(name), value(val)
        {};
#else
        Variable(const std::string & name, double val = 0.0)
            : name(name), value(val)
        {};
#endif        
        virtual ~Variable()
        {};
        

        /// print this variable
        virtual std::ostream & print(std::ostream & o) const;

        
        const std::string & getName() const
            { return name; }
        
        void setValue(double v)
            { value = v; }
        
        double getValue() const
            { return value; }

    protected:
        std::string name;
        double value;
};



// a linked variable (which contains the link target explicitly
class IMPEX LinkedVariable : public Variable
{
    
    public:
        LinkedVariable(const std::string & name = "",
                       double val = 0.0,
                       int link=-1)
        : Variable(name, val), m_link(link)
        {}

        bool isLinked() const
        { return m_link >= 0; }
        
        int getLink()  const
        { return m_link; }
        
        void setLink(int link)
        { m_link = link; }

    protected:
        int m_link;
};



/** A lens variable can be linked.
 *
 *  It is only used in the lens class, not directly in the images.
 */

// KFJ 2011-01-12 added default for name and value parameter
// in order to have a default contructor

class IMPEX LensVariable : public Variable
{
    public:

// in the hsi version, class LensVariable needs a default Ctor
#ifdef HUGIN_HSI
        LensVariable(const std::string & name = "" ,
                     double value = 0.0 ,
                     bool link=false)
            : Variable(name, value), linked(link)
        {};
#else
        LensVariable(const std::string & name, double value, bool link=false)
            : Variable(name, value), linked(link)
        {};
#endif
        virtual ~LensVariable()
            {};
        
        
        ///
        virtual std::ostream& printLink(std::ostream & o, unsigned int link) const;

        
        ///
        bool isLinked() const
            { return linked; }
        ///
        void setLinked(bool l=true)
            { linked = l; }
        
    private:
        bool linked;
    
};

/** functor to print a variable. */
#ifndef SWIG
/* this gave me trouble in hsi, currently deactivated
 * TODO: find out how it can be made to work */
struct PrintVar : public std::unary_function<Variable, void>
{
    PrintVar(std::ostream & o)
        : os(o)
    {};
    
    void operator()(Variable x) const
        { x.print(os) << " "; };
    
    std::ostream& os;
};
#endif

///
typedef std::map<std::string,Variable> VariableMap;

/** fill map with all image & lens variables */
IMPEX void fillVariableMap(VariableMap & vars);

/** print a variable map to \p o */
IMPEX void printVariableMap(std::ostream & o, const VariableMap & vars);

///
typedef std::vector<VariableMap> VariableMapVector;


///
typedef std::map<std::string,LensVariable> LensVarMap;

/** just lens variables */
IMPEX void fillLensVarMap(LensVarMap & vars);


///
typedef std::vector<std::set<std::string> > OptimizeVector;


} // namespace
#endif // _H
