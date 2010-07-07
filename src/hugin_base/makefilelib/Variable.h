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
#include "Makefile.h"

namespace makefile
{

/**
 * Holds name and value of a makefile variable.
 * The \ref MakefileItem "MakefileItems" VariableRef and VariableDef refer are linked to one of
 * these to know what they represent.
 */
class Variable
{
protected:
	std::string name, value;
	/// A VariableDef connected to the Variable.
	VariableDef def;
	/// A VariableDef connected to the Variable.
	VariableRef ref;
	Makefile::QuoteMode quotemode;

	/**
	 * Checks the name Strings and replaces forbidden characters.
	 * @return Number of replaced chars.
	 */
	virtual int checkName();

	/// To be used only by subclasses, like AutoVariable.
	Variable(std::string name_)
	: name(name_), value(""), def(*this), ref(*this), quotemode(Makefile::SHELL)
	{

	}
private:
	Variable(const Variable&);	// no implicite copies!
public:
	Variable(std::string name_, std::string value_)
	: name(name_), value(value_), def(*this), ref(*this), quotemode(Makefile::SHELL)
	{
		checkName();
	}
	virtual ~Variable() {}

	virtual std::string getName()
	{
		return name;
	}

	virtual std::string getValue()
	{
		return Makefile::quote(value, quotemode);
	}

	virtual VariableDef& getDef()
	{
		return def;
	}

	virtual VariableRef& getRef()
	{
		return ref;
	}

	void setQuoteMode(Makefile::QuoteMode mode)
	{
		quotemode = mode;
	}


};

}

#endif /* VARIABLE_H_ */
