/**
 * @file Variable.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

#include "VariableDef.h"
#include "VariableRef.h"
#include "Makefile.h"
#include "Manageable.h"

namespace makefile
{

/**
 * Holds name and value of a makefile variable.
 * The \ref MakefileItem "MakefileItems" VariableRef and VariableDef refer are linked to one of
 * these to know what they represent.
 */
class Variable : public Manageable
{
protected:
	string name, value;
	/// A VariableDef connected to the Variable.
	VariableDef def;
	/// A VariableDef connected to the Variable.
	VariableRef ref;
	Makefile::QuoteMode quotemode;
	/// Decides wether this Variable is defined with export.
	bool exported;

	/**
	 * Checks the name Strings and throws on forbidden characters.
	 */
	virtual void checkName();
	/**
	 * Checks value and throws on forbidden characters (newlines).
	 */
	virtual void checkValue();

	/// To be used only by subclasses, like AutoVariable.
	Variable(string name_)
	: name(name_), value(cstr("")), def(*this), ref(*this), quotemode(Makefile::SHELL)
	{

	}
private:
	Variable(const Variable&);	// no implicite copies!
public:
	Variable(string name_, string value_, Makefile::QuoteMode quotemode_ = Makefile::SHELL)
	: name(name_), value(value_), def(*this), ref(*this), quotemode(quotemode_), exported(false)
	{
		checkName();
		checkValue();
	}
	Variable(string name_, double value_, Makefile::QuoteMode quotemode_ = Makefile::SHELL);
	virtual ~Variable() {}

	virtual const string getName()
	{
		return name;
	}

	virtual const string getValue()
	{
		return value;
	}
	virtual const string getquotedValue()
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

	void setExport(bool ex)
	{
		exported = ex;
	}
	bool getExport()
	{
		return exported;
	}

};

}

#endif /* VARIABLE_H_ */
