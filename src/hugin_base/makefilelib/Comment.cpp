/**
 * @file Comment.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Comment.h"

namespace makefile
{

static const string prefix(cstr("\n# "));

/// \todo strip disallowed characters.

string Comment::toString()
{
	static const regex newline(cstr("\\R"));
	return string(prefix + boost::regex_replace(text, newline, prefix) + cstr('\n'));
}
}
