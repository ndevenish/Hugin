/**
 * @file VariableDef.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Variable.h"
#include "VariableDef.h"



void makefile::VariableDef::print(std::ostream & os)
{
	os << variable.getName() << "=" << variable.getValue() << std::endl;
}

