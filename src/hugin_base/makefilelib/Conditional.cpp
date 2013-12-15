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
 * @file Conditional.cpp
 * @brief
 *  Created on: Jul 10, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Conditional.h"

namespace makefile
{

string Conditional::toString()
{
	string str = printif();
	std::vector<MakefileItem*>::iterator i;
	for(i = ifblock.begin(); i != ifblock.end(); ++i)
		str.append((*i)->toString());
	str.append(cstr("\n"));

	// only if else block has contents.
	if(!elseblock.empty())
	{
		str.append(cstr("else\n"));
		for(i = elseblock.begin(); i != elseblock.end(); ++i)
			str.append((*i)->toString());
		str.append(cstr("\n"));
	}
	str.append(cstr("endif\n"));
	return str;
}

}
