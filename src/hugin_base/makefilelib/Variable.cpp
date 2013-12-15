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
 * @file Variable.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Variable.h"
#include "StringAdapter.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace makefile
{

/**
 * According to Gnu Make Manual http://www.gnu.org/software/make/manual/html_node/Using-Variables.html#Using-Variables
 * it's recommended to use only alphanumerics and _ in Variable name.
 *
 */
void Variable::checkName()
{
	static const regex validname(cstr("\\w+"));
	if( !boost::regex_match(name, validname))
		throw std::invalid_argument("Bad Variable name: " + StringAdapter(name));
}

/**
 * It's not allowed to have newline characters in Variable values, if they are defined like this. (There would be
 * an alternative).
 */
void Variable::checkValue()
{
	static const regex invalid(cstr("[^\\\\][\n\r]"));
	if(boost::regex_search(getValue(), invalid))
		throw std::invalid_argument("Bad Variable value: " + StringAdapter(getValue()));
}

void Variable::Create()
{
    def=new VariableDef(*this);
    ref=new VariableRef(*this);
};

Variable::Variable(string name_)
: name(name_), quotemode(Makefile::SHELL)
{
    Create();
}

Variable::Variable(string name_, string value_, Makefile::QuoteMode quotemode_)
: name(name_), quotemode(quotemode_), exported(false)
{
    Create();
	values.push_back(value_);
	checkName();
	checkValue();
}
Variable::Variable(string name_, double value_, Makefile::QuoteMode quotemode_)
: name(name_), quotemode(quotemode_), exported(false)
{
    Create();
	checkName();
	std::ostringstream val;
	val.imbue(Makefile::locale);
	val << value_;
	values.push_back(val.str());
}

Variable::Variable(string name_, std::vector<string>::iterator start, std::vector<string>::iterator end,
		Makefile::QuoteMode quotemode_, string separator_)
: name(name_), separator(separator_), quotemode(quotemode_), exported(false)
{
    Create();
	checkName();
	copy(start, end, std::back_inserter(values));
	checkValue();
}

Variable::~Variable()
{
    if(def)
        delete def;
    if(ref)
        delete ref;
};

const string Variable::getValue()
{
	string v;
	for(std::vector<string>::iterator it = values.begin(); it != values.end(); ++it)
	{
		if(it != values.begin()) v += separator;
		v += *it;
	}
	return v;
}
const string Variable::getquotedValue()
{
	string v;
	for(std::vector<string>::iterator it = values.begin(); it != values.end(); ++it)
	{
		if(it != values.begin()) v += separator;
		v += Makefile::quote(*it, quotemode);
	}
	return v;
}

}
