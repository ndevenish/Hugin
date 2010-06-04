/**
 * @file Variable.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

#include <string>
#include "VariableDef.h"
#include "VariableRef.h"
namespace makefile
{

/**
 * Holds name and value of a makefile variable.
 * The \ref MakefileItem "MakefileItems" VariableRef and VariableDef refer are linked to one of
 * these to know what they represent.
 */
class Variable
{
	std::string name, value;
	/// A VariableDef connected to the Variable.
	VariableDef def;
	/// A VariableDef connected to the Variable.
	VariableRef ref;

	/**
	 * Checks the name and value Strings and replaces forbidden characters.
	 * @return Number of replaced chars.
	 */
	int checkStrings();
public:
	Variable(std::string name_, std::string value_)
	: name(name_), value(value_), def(*this), ref(*this)
	{
		checkStrings();
	}

	std::string& getName()
	{
		return name;
	}

	std::string& getValue()
	{
		return value;
	}

	VariableDef& getDef()
	{
		return def;
	}

	VariableRef& getRef()
	{
		return ref;
	}

	virtual ~Variable() {}
};

}

#endif /* VARIABLE_H_ */
