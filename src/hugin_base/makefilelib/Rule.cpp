/**
 * @file Rule.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Rule.h"

namespace makefile
{
std::string Rule::toString()
{
	std::string str("\n");
	std::vector<std::string>::iterator i;
	for(i = targets.begin(); i != targets.end(); i++)
	{
		str.append(*i + " ");
	}
	str.append(": ");

	for(i = prerequisites.begin(); i != prerequisites.end(); i++)
	{
		str.append(*i + " ");
	}
	str.append("\n");

	for(i = commands.begin(); i != commands.end(); i++)
	{
		str.append("\t" +*i + "\n");
	}

	return str;
}

}
