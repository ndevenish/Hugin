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
 * @file Newline.h
 * @brief
 *  Created on: Jul 8, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef NEWLINE_H_
#define NEWLINE_H_

/**
 *
 */
#include "MakefileItem.h"

namespace makefile
{
/**
 * Simply prints newlines. Can be used to give some structure.
 */
class IMPEX Newline: public PrimaryMakefileItem
{
	int newlines;
public:
	Newline(int newlines_ =1)
	:newlines(newlines_)
	{}
	virtual ~Newline()
	{}

	virtual string toString()
	{
		string n;
		for(int i=0; i<newlines; i++)
			n.append(cstr("\n"));
		return n;
	}
};

}

#endif /* NEWLINE_H_ */
