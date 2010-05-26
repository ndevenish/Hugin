/**
 * @file Variable.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

#include <string>

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
public:
	Variable() {}
	Variable(std::string name_, std::string value_)
	: name(name_), value(value_) {}

	const std::string& getName()
	{
		return name;
	}

	const std::string& getValue()
	{
		return value;
	}

	virtual ~Variable() {}
};

}

#endif /* VARIABLE_H_ */
