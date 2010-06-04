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
class VariableDef : public MakefileItem
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
	virtual std::string toString();
};

}

#endif /* VARIABLEDEF_H_ */
