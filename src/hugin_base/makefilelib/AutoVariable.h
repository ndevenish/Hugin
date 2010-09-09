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
 * @file AutoVariable.h
 * @brief
 *  Created on: Jun 5, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef AUTOVARIABLE_H_
#define AUTOVARIABLE_H_


#include "Variable.h"

namespace makefile
{
/**
 * GNU Make has automatic Variables, which can not be defined and have names
 * consisting of one special character.
 * Use this class to refer to such variables.
 * http://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html#Automatic-Variables
 */
class MAKEIMPEX AutoVariable: public makefile::Variable
{
	/// Automatic Variables have special names, so we omit checking.
	virtual int checkStrings();
public:
	AutoVariable(string name)
	: Variable(name)
	{
		checkStrings();
	}
	virtual ~AutoVariable()
	{};

	/// Has no value, exception.
	virtual const string getValue();
	virtual const string getquotedValue();
	virtual const std::vector<string>& getValues();

	/// Has no definition, exception.
	virtual VariableDef& getDef();
};

}

#endif /* AUTOVARIABLE_H_ */
