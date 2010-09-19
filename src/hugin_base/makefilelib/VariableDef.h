/*
This file is part of hugin.

hugin is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

hugin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with hugin.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file VariableDef.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef VARIABLEDEF_H_
#define VARIABLEDEF_H_

#include "MakefileItem.h"

namespace makefile
{
class Variable;

/**
 * Represents the Definition of a Variable in the makefile. On creation the object
 * is bounded to a Variable. If an object of this type is \ref print ed
 * it produces a Declaration to a Variable like \verbatim VAR=value \endverbatim
 */
class MAKEIMPEX VariableDef : public MakefileItem
{
	Variable& variable;
	VariableDef(const VariableDef&);	// no implicite copies!
public:
	VariableDef(Variable& var)
	: variable(var)
	{
		// TODO Auto-generated constructor stub

	}
	virtual ~VariableDef()
	{
		// TODO Auto-generated destructor stub
	}
	virtual string toString();
};

}

#endif /* VARIABLEDEF_H_ */
