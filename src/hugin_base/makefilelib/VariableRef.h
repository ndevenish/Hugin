/**
 * @file VariableRef.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef VARIABLEREF_H_
#define VARIABLEREF_H_

#include "MakefileItem.h"

namespace makefile
{
class Variable;

/**
 * Represents the Reference of a Variable in the makefile. On creation the object
 * is bounded to a Variable. If an object of this type is \ref print ed
 * it produces a Reference to a Variable like \verbatim $(VAR) \endverbatim
 */
class VariableRef : public MakefileItem
{
	Variable& variable;
	VariableRef(const VariableRef&);	// no implicite copies!
public:
	VariableRef(Variable& var)
	: variable(var)
	{
		// TODO Auto-generated constructor stub

	}
	virtual ~VariableRef()
	{
		// TODO Auto-generated destructor stub
	}

	virtual string toString();

};

}

#endif /* VARIABLEREF_H_ */
