/**
 * @file Comment.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Comment.h"
#include <boost/regex.hpp>

using namespace std;

namespace makefile
{

static const string prefix("\n# ");

/// \todo strip disallowed characters.
void Comment::print(ostream& os)
{
	static const boost::regex newline("[\n\r]");
	boost::regex_replace(text, newline, prefix);
	os << prefix << boost::regex_replace(text, newline, prefix) << endl;
}

}
