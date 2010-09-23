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
 * @file Comment.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef COMMENT_H_
#define COMMENT_H_

#include "MakefileItem.h"

namespace makefile
{
/**
 * Encapsulates a Comment.
 * Makefile comments are prefixed with # for every line.
 * Currently only full line comments are supported.
 */
class MAKEIMPEX Comment : public PrimaryMakefileItem
{
	string text;

public:
	Comment() {}

	Comment(string s) : text(s) {}

	virtual ~Comment() { }

	string& getText()
	{
		return text;
	}

	void setText(const string& s)
	{
		text = s;
	}

	void appendLine(const string& line)
	{
		text.append(cstr("\n"));
		text.append(line);
	}

	virtual string toString();
};

}

#endif /* COMMENT_H_ */
