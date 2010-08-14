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
class AutoVariable: public makefile::Variable
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
