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
 * @file Anything.h
 * @brief
 *  Created on: Aug 11, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef ANYTHING_H_
#define ANYTHING_H_

/**
 *
 */
#include "MakefileItem.h"

namespace makefile
{
/**
 * This one only inserts some Text, not doing anything else.
 * It can be used to include make feautures not supported by the lib in a
 * makefile.
 */
class IMPEX Anything: public PrimaryMakefileItem
{
	string text;
public:
	Anything()
	{
		// TODO Auto-generated constructor stub

	}
	Anything(const string& text_)
	: text(text_)
	{}

	virtual ~Anything() {}
	void setText(const string& text_)
	{
		text.assign(text_);
	}
	virtual string toString()
	{
		return text;
	}
};

}

#endif /* ANYTHING_H_ */
