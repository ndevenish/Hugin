/**
 * @file VariableDef.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Variable.h"
#include "VariableDef.h"


namespace makefile
{

string VariableDef::toString()
{
	return string(variable.getName() + cstr("=") + variable.getValue() + cstr('\n'));
}

}
