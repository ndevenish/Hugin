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

std::string VariableDef::toString()
{
	return std::string(variable.getName() + "=" + variable.getValue() + '\n');
}

}
