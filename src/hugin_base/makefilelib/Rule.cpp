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
	for(i = targets.begin(); i != targets.end(); i++)
	{
		str.append(*i + cstr(" "));
	}
	str.append(cstr(": "));

	for(i = prerequisites.begin(); i != prerequisites.end(); i++)
	{
		str.append(*i + cstr(" "));
	}
	str.append(cstr("\n"));

	for(i = commands.begin(); i != commands.end(); i++)
	{
		str.append(cstr("\t") +*i + cstr("\n"));
	}

	return str;
}

}
