/**
 * @file Conditional.cpp
 * @brief
 *  Created on: Jul 10, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Conditional.h"

namespace makefile
{

std::string Conditional::toString()
{
	std::string str = printif();
	std::vector<MakefileItem*>::iterator i;
	for(i = ifblock.begin(); i != ifblock.end(); i++)
		str.append((*i)->toString());
	str.append("\n");

	// only if else block has contents.
	if(!elseblock.empty())
	{
		str.append("else\n");
		for(i = elseblock.begin(); i != elseblock.end(); i++)
			str.append((*i)->toString());
		str.append("\n");
	}
	str.append("endif\n");
	return str;
}

}
