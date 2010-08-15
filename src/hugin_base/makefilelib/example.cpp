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
	ConditionalNDEF* have_cc = mgr.own(new ConditionalNDEF("CXX"));
	// and add it.
	have_cc->add();

	// A Conditional can hold MakefileItems in it's if- and else-blocks
	// Variables have a VariableRef(erence) and a VariableDef(inition).
	// If CC is not defined, we want to define it.
	have_cc->addToIf(cc->getDef());

	// We want another Variable containing compiler flags
	// Variables quotemode defines how there value is quoted in the definition.
	Variable* cflags = mgr.own(new Variable("CFLAGS", "-Wall"));
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
	// how to get a .out file from a .cpp file.
	Rule* build = mgr.own(new Rule());
	build->add();
	build->addTarget("%.o");
	build->addPrereq("%.cpp");

	// We use Automatic Variables in the command.
	Variable* target = mgr.own(new AutoVariable("@"));
	Variable* prereq = mgr.own(new AutoVariable("<"));

	// Now we can add a command to the rule.
	build->addCommand(cc->getRef() +" "+ cflags->getRef() + " -o " + target->getRef() +" "+ prereq->getRef());

	// The Makefile singleton has pointers to our added objects. We open a file and
	// let it write our makefile into it.
	std::ofstream outfile("example.mk");
	Makefile::getSingleton().writeMakefile(outfile);
	outfile.close();

	return 0;
}
