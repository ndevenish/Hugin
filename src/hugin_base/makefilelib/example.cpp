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
 * @file example.cpp
 * @brief
 *  Created on: Aug 15, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */
#include <iostream>
#include <fstream>
#include "Comment.h"
#include "Conditional.h"
#include "VariableDef.h"
#include "VariableRef.h"
#include "AutoVariable.h"
#include "Rule.h"
#include "Makefile.h"
#include "Manager.h"

using namespace makefile;

/**
 * @example example.cpp
 * A short example of how to use the Makefilelib.
 * It tries to create a Makefile to compile itself.
 *
 * This may not work, but thats not the goal of this program.
 *
 */

int main(int argc, char *argv[])
{
	// The Manager owns our MakefileItems and destructs them when
	// itself is destructed. Only heap-allocateed objects can be owned
	// by the Manager, otherwise delete would fail.
	Manager mgr;

	// own_add is a shortcut, if you don't need the pointer for anything else.
	mgr.own_add(new Comment("This example program creates a makefile to build itself"));

	// The other way is this:
	// We create a Variable.
	Variable* cc = mgr.own(new Variable("CXX", "g++"));

	// We create a Conditional that checks if CC is defined, and otherwise defines it.
	// Never forget to call mgr.own, or delete yourself, otherwise valgrind will tell you bad news ;)
	ConditionalNDEF* have_cc = mgr.own(new ConditionalNDEF("CXX"));
	// and add it.
	have_cc->add();

	// A Conditional can hold MakefileItems in it's if- and else-blocks
	// Variables have a VariableRef(erence) and a VariableDef(inition).
	// If CC is not defined, we want to define it.
	have_cc->addToIf(cc->getDef());

	// We want another Variable containing compiler flags
	// Variables quotemode defines how there value is quoted in the definition.
	Variable* cflags = mgr.own(new Variable("CFLAGS", "-Wall -c", Makefile::NONE));
	// ..and add the definition to the Makefile right here.
	cflags->getDef().add();

	// And another Var for the executable name
	Variable* executable = mgr.own(new Variable("OBJ", "example.o"));
	executable->getDef().add();

	// The default rule is the first that appears. We want to build the
	// executable.
	Rule* all = mgr.own(new Rule());
	all->add();
	all->addTarget("all");
	all->addPrereq(executable->getRef());

	// Now we need a rule to build all that. We create a rule, that says
	// how to get a .o file from a .cpp file. (Yes I know there are implicitrules..)
	Rule* build = mgr.own(new Rule());
	build->add();
	build->addTarget("%.o");
	build->addPrereq("%.cpp");

	// We use Automatic Variables in the command.
	// This is a special Varialbe having only a name.
	Variable* target = mgr.own(new AutoVariable("@"));
	Variable* prereq = mgr.own(new AutoVariable("<"));

	// Now we can add a command to the rule.
	build->addCommand(cc->getRef() +" "+ cflags->getRef() + " -o " + target->getRef() +" "+ prereq->getRef());

	// The Makefile singleton has pointers to our added objects. We open a file and
	// let it write our makefile into it.
	std::ofstream outfile("example.mk");
	Makefile::getSingleton().writeMakefile(outfile);
	outfile.close();

	// Now run make and look what it does. Maybe it won't work, because the cpp file is somewhere else
	// Run make -n -f example.mk to do a dryrun.
	// To get a running executable, this has to be linked against libmakefilelib.so.

	return 0;
}
