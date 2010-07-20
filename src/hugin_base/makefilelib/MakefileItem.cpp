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


}
