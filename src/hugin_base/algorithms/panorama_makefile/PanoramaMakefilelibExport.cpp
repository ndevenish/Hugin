/**
 * @file PanoramaMakefilelibExport.cpp
 * @brief
 *  Created on: Aug 5, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "PanoramaMakefilelibExport.h"

#include <makefilelib/char_type.h>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <locale>
#include <makefilelib/Comment.h>
#include <makefilelib/Variable.h>
#include <makefilelib/VariableDef.h>
#include <makefilelib/VariableRef.h>
#include <makefilelib/MakefileItem.h>
#include <makefilelib/Makefile.h>
#include <makefilelib/AutoVariable.h>
#include <makefilelib/Newline.h>
#include <makefilelib/Rule.h>
#include <makefilelib/Conditional.h>

namespace HuginBase
{
using namespace makefile;
bool PanoramaMakefilelibExport::create()
{
	Comment header(cstr("First line of the new makefilelib's work!"));
	header.add();

	// prepare output stream to get number output right.
	// We use the "C" locale for NUMERIC and the system's for everything else.
	// Note: C++ locales are different from C. Calling setlocale doesn't influence C++ streams!
	std::locale makefilelocale(std::locale(""), "C", std::locale::numeric);
	makefile.imbue(makefilelocale);

	Makefile::getSingleton().writeMakefile(makefile);
	Makefile::getSingleton().clean();
	return true;
}

}
