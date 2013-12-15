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
 * @file Rule.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Rule.h"

namespace makefile
{
string Rule::toString()
{
	string str(cstr("\n"));
	std::vector<string>::iterator i;
	for(i = targets.begin(); i != targets.end(); ++i)
	{
		str.append(*i + cstr(" "));
	}
	str.append(cstr(": "));

	for(i = prerequisites.begin(); i != prerequisites.end(); ++i)
	{
		str.append(*i + cstr(" "));
	}
	str.append(cstr("\n"));

	for(i = commands.begin(); i != commands.end(); ++i)
	{
		str.append(cstr("\t") +*i + cstr("\n"));
	}

	return str;
}

}
