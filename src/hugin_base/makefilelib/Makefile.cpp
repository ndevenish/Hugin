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
 * @file Makefile.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Makefile.h"
#include "MakefileItem.h"

namespace makefile
{

// intialize static singelton instance pointer
Makefile* Makefile::instance = NULL;

// static
/// Used for numeric output to get a decimal dot anyways.
const std::locale Makefile::locale(std::locale("C"));

const std::locale GetMakefileLocale()
{
    return Makefile::locale;
};

Makefile& Makefile::getSingleton()
{
	if(!instance)
		instance = new Makefile();
	return *instance;
}

void Makefile::clean()
{
	if(instance)
		delete instance;
	instance = NULL;
}

void Makefile::remove(MakefileItem* item)
{
	if(instance && !instance->written)
	{
		std::cerr <<
		"WARNING: A MakefileItem was removed before the Makefile::writeMakefile() was called.\n"
		"This is likely to be a programming error (out of scope?)" << std::endl;
	}
	clean();
}

//#define WIN32
/**
 * Quotes and escapes characters using regular expressions. Two modes are currently distinguished,
 * depending on the usage of the string.
 * The regular expressions need a lot of backslash escaping, eg. \\\\ means backslash. We need to get
 * through the compiler and the boost::regex library with the special characters
 *
 * The replacements in detail:
 * - Shell mode
 *   - WIN32: $ --> $$, \ --> /, # --> \#, and surround with quotes
 *   - others: $ --> \$$, [\ ~"|'`{}[]()*#:=&] --> escape with backslash
 * - Make mode
 *   - WIN32: $ --> $$, [ #=] --> escape with backslash
 *   - others: $ --> $$, [ #:=] --> escape with backslash
 *
 * All replacements take care of variable references and do not replace $(varname) patterns.
 *
 * @note src/hugin_base/hugin_utils/platform.h is where the information is from.
 * Unfortunately there are lots of comments in that code, which say that the correctness
 * of the escaping rules is rather unsure.
 *
 * @param in Input string.
 * @param mode switch.
 * @return A new string containing the processed content.
 */
string Makefile::quote(const string& in, Makefile::QuoteMode mode)
{
	regex toescape;
	string output;
	switch(mode)
	{
	case Makefile::SHELL:
#ifdef WIN32
		toescape.assign(cstr("(\\$[^\\(])|(\\\\)|(\\#)"));
		// uses a nice regex feature "recursive expressions" for doing it all in one (subexpression) cascade.
		output.assign(cstr("(?1\\$$&)(?2/)(?3\\\\$&)"));
		return string(cstr("\"") + boost::regex_replace(in, toescape, output, boost::match_default | boost::format_all) + cstr("\""));
#else
		// because parenthesis are replaced too, the first pattern detects variable references and passes them unchanged.
		toescape.assign(cstr("(\\$\\([^\\)]+\\))|(\\$)|([\\\\ \\~\"\\|\\'\\`\\{\\}\\[\\]\\(\\)\\*\\#\\:\\=\\&])"));
		output.assign(cstr("(?1$&)(?2\\\\\\$$&)(?3\\\\$&)"));
		return boost::regex_replace(in, toescape, output, boost::match_default | boost::format_all);
#endif
		break;
	case Makefile::MAKE:
#ifdef WIN32
		toescape.assign(cstr("(\\$[^\\(])|(\\\\)|([ \\#\\=])"));
		output.assign(cstr("(?1\\$$&)(?2/)(?3\\\\$&)"));
		return boost::regex_replace(in, toescape, output, boost::match_default | boost::format_all);
#else
		// do not replace $ if followed by a (. To allow variable references.
		toescape.assign(cstr("(\\$[^\\(])|([ \\#\\:\\=])"));
		output.assign(cstr("(?1\\$$&)(?2\\\\$&)"));
		return boost::regex_replace(in, toescape, output, boost::match_default | boost::format_all);
#endif
		break;
	default:
		return string(in);
	}
}

#ifdef WIN32
/** special quoting for environemnt variables on windows 
 *   - $ --> $$, # --> escape with backslash
 */
string Makefile::quoteEnvironment(const string& in)
{
    regex toescape;
    string output;
    toescape.assign(cstr("(\\$)|(\\#)"));
    output.assign(cstr("(?1\\$$&)(?2\\\\$&)"));
    return boost::regex_replace(in, toescape, output, boost::match_default | boost::format_all);
};
#endif

int Makefile::writeMakefile(ostream& out)
{
	for(std::vector<MakefileItem*>::iterator i = items.begin(); i != items.end(); i++)
	{
		out << **i;
	}
	written = true;
	return items.size();
}

}
