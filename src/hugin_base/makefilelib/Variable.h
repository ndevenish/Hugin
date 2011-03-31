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
#include <vector>

namespace makefile
{

/**
 * Holds name and value of a makefile variable.
 * The \ref MakefileItem "MakefileItems" VariableRef and VariableDef refer are linked to one of
 * these to know what they represent.
 */
class MAKEIMPEX Variable : public Manageable
{
protected:
	string name;
	/// holds a list of values
	std::vector<string> values;
	/// separator for output of the value list
	string separator;
	/// A VariableDef connected to the Variable.
	VariableDef* def;
	/// A VariableDef connected to the Variable.
	VariableRef* ref;
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

    void Create();

	/// To be used only by subclasses, like AutoVariable.
	Variable(string name_);

private:
	Variable(const Variable&);	// no implicite copies!
public:
	/// Takes a string value
	Variable(string name_, string value_, Makefile::QuoteMode quotemode_ = Makefile::SHELL);
	/// Takes a numeric value
	Variable(string name_, double value_, Makefile::QuoteMode quotemode_ = Makefile::NONE);
	/** Takes values and appends them using seperator. The seperators will not be quoted
	 * so using this, it's possible to have a list of filenames with spaces in them, quote
	 * those correctly, but have a space seperated list of filenames.
	 */
	Variable(string name_,std::vector<string>::iterator start, std::vector<string>::iterator end,
			Makefile::QuoteMode quotemode_ = Makefile::SHELL, string separator_ = " ");

	virtual ~Variable();

	virtual const string getName()
	{
		return name;
	}
	/**
	 * @return The assembled value, unquoted.
	 */
	virtual const string getValue();
	/**
	 * @return The assembled value, quoted.
	 */
	virtual const string getquotedValue();

	/**
	 * @return The value vector.
	 */
	virtual const std::vector<string>& getValues()
	{
		return values;
	}

	virtual VariableDef& getDef()
	{
		return *def;
	}

	virtual VariableRef& getRef()
	{
		return *ref;
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
