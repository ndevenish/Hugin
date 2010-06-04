/**
 * @file VariableRef.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Variable.h"
#include "VariableRef.h"


namespace makefile
{

std::string VariableRef::toString()
{
	return std::string("$(" + variable.getName() + ")");
}
}
