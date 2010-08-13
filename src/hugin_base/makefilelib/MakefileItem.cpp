/**
 * @file MakefileItem.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "MakefileItem.h"

namespace makefile
{

ostream& operator<<(ostream& stream, MakefileItem& item)
{
	item.print(stream);
	return stream;
}

string operator+(const string& str, MakefileItem& item)
{
	string out(str);
	out.append(item.toString());
	return out;
}
string operator+(MakefileItem& item, const string& str)
{
	string out(item.toString());
	out.append(str);
	return out;
}

}
