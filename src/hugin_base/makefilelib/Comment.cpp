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

std::string Comment::toString()
{
	static const boost::regex newline("[\n\r]");
	return string(prefix + boost::regex_replace(text, newline, prefix) + '\n');
}
}
