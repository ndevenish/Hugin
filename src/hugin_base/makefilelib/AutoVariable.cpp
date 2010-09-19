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

const string AutoVariable::getValue()
{
	throw(std::runtime_error("AutoVariables have no predefined value."));
}
const string AutoVariable::getquotedValue()
{
	return getValue();
}
const std::vector<string>& AutoVariable::getValues()
{
	throw(std::runtime_error("AutoVariables have no predefined value."));
}

VariableDef& AutoVariable::getDef()
{
	throw(std::runtime_error("AutoVariables can not be defined."));
}

}
