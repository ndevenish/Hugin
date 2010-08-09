/**
 * @file AutoVariable.cpp
 * @brief
 *  Created on: Jun 5, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "AutoVariable.h"
#include <stdexcept>

namespace makefile
{
int AutoVariable::checkStrings()
{
	return 0;
}

inline const string AutoVariable::getValue()
{
	throw(std::runtime_error("AutoVariables have no predefined value."));
}
inline const string AutoVariable::getquotedValue()
{
	return getValue();
}

inline VariableDef& AutoVariable::getDef()
{
	throw(std::runtime_error("AutoVariables can not be defined."));
}

}
