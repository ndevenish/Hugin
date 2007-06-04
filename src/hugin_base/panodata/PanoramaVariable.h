// -*- c-basic-offset: 4 -*-
/** @file PanoramaVariable.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoramaMemento.h 1970 2007-04-18 22:26:56Z dangelo $
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

#ifndef _PANORAMAMEMENTO_H
#define _PANORAMAMEMENTO_H


#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include <math.h>


namespace PT {

/** a variable has a value and a name.
 *
 *  linking is only supported by LinkedVariable, which
 *  is only used by Lens.
 */
class Variable
{
public:
    Variable(const std::string & name, double val = 0.0)
        : name(name), value(val)
        { };
    virtual ~Variable() {};

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
class LinkedVariable : public Variable
{
    LinkedVariable(const std::string & name, double val = 0.0, int link=-1)
    : Variable(name, val), m_link(link)
    {
    }

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
class LensVariable : public Variable
{
public:
    LensVariable(const std::string & name, double value, bool link=false)
        : Variable(name, value), linked(link)
        { };
    virtual ~LensVariable() {}
    virtual std::ostream & printLink(std::ostream & o, unsigned int link) const;

    bool isLinked() const
        { return linked; }
    void setLinked(bool l=true)
        { linked = l; }
private:
    bool linked;
};




/** functor to print a variable. */
struct PrintVar : public std::unary_function<Variable, void>
{
    PrintVar(std::ostream & o) : os(o) { };
    void operator() (Variable x) const { x.print(os) << " "; };
    std::ostream& os;
};


typedef std::map<std::string,Variable> VariableMap;
typedef std::vector<VariableMap> VariableMapVector;
typedef std::map<std::string,LensVariable> LensVarMap;

/** fill map with all image & lens variables */
void fillVariableMap(VariableMap & vars);

/** just lens variables */
void fillLensVarMap(LensVarMap & vars);

/** print a variable map to \p o */
void printVariableMap(std::ostream & o, const VariableMap & vars);




} // namespace
#endif // _PANORAMAMEMENTO_H
