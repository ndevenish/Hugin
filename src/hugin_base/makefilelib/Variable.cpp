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
	static const regex invalid(cstr("\\R"));
	if(boost::regex_search(value, invalid))
		throw std::invalid_argument("Bad Variable value: " + StringAdapter(value));
}

Variable::Variable(string name_, double value_, Makefile::QuoteMode quotemode_)
: name(name_), def(*this), ref(*this), quotemode(quotemode_), exported(false)
{
	std::ostringstream val;
	val.imbue(Makefile::locale);
	val << value_;
	value = val.str();
}
}
