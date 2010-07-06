/**
 * @file Makefile.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Makefile.h"
#include <boost/regex.hpp>

namespace makefile
{

Makefile::Makefile()
{
	// TODO Auto-generated constructor stub

}

Makefile::~Makefile()
{
	// TODO Auto-generated destructor stub
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
 *   - others: $ --> \$$, [\ ~"|'`{}[]()*#:=] --> escape with backslash
 * - Make mode
 *   - WIN32: $ --> $$, [ #=] --> escape with backslash
 *   - others: $ --> $$, [ #:=] --> escape with backslash
 *
 * @note src/hugin_base/hugin_utils/platform.h is where the information is from.
 * Unfortunately there are lots of comments in that code, which say that the correctness
 * of the escaping rules is rather unsure.
 *
 * @param in Input string.
 * @param mode switch.
 * @return A new string containing the processed content.
 */
std::string Makefile::quote(const std::string& in, Makefile::QuoteMode mode)
{
	boost::regex toescape;
	std::string output;
	switch(mode)
	{
	case Makefile::SHELL:
#ifdef WIN32
		toescape.assign("(\\$)|(\\\\)|(\\#)");
		// uses a nice regex feature "recursive expressions" for doing it all in one (subexpression) cascade.
		output.assign("\\\"(?1\\$$&)(?2/)(?3\\\\$&)\\\"");
#else
		toescape.assign("(\\$)|([\\\\ \\~\"\\|\\'\\`\\{\\}\\[\\]\\(\\)\\*\\#\\:\\=])");
		output.assign("(?1\\\\\\$$&)(?2\\\\$&)");
#endif
		break;
	case Makefile::MAKE:
#ifdef WIN32
		toescape.assign("(\\$)|([ \\#\\=])");
		output.assign("(?1\\$$&)(?2\\\\$&)");
#else
		toescape.assign("(\\$)|([ \\#\\:\\=])");
		output.assign("(?1\\$$&)(?2\\\\$&)");
#endif
		break;
	default:
		toescape.assign(".*");
		output.assign("$&");
	}
	return boost::regex_replace(in, toescape, output, boost::match_default | boost::format_all);
}

}
