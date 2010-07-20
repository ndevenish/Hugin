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
	for(i = ifblock.begin(); i != ifblock.end(); i++)
		str.append((*i)->toString());
	str.append(cstr("\n"));

	// only if else block has contents.
	if(!elseblock.empty())
	{
		str.append(cstr("else\n"));
		for(i = elseblock.begin(); i != elseblock.end(); i++)
			str.append((*i)->toString());
		str.append(cstr("\n"));
	}
	str.append(cstr("endif\n"));
	return str;
}

}
