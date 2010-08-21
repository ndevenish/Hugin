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
 * @file test_makefilelib.cpp
 * @brief Tests all features of the lib.
 *  Created on: Jul 24, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "char_type.h"
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <fstream>
#include "Comment.h"
#include "Variable.h"
#include "VariableDef.h"
#include "VariableRef.h"
#include "MakefileItem.h"
#include "Makefile.h"
#include "AutoVariable.h"
#include "Newline.h"
#include "Rule.h"
#include "Conditional.h"
#include "StringAdapter.h"
#include "Manager.h"

#include "test_util.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

using namespace makefile;
namespace fs = boost::filesystem;
namespace io = boost::iostreams;

namespace makefile { namespace tester {

#ifdef USE_WCHAR
ostream& cout =  std::wcout;
ostream& cerr =  std::wcerr;
#else
ostream& cout =  std::cout;
ostream& cerr =  std::cerr;
#endif

struct TestComment : public Test
{
	Manager mgr;
	TestComment()
	:Test("Comment", "")
	{
		Comment* comment = mgr.own(new Comment(cstr("First line")));
		comment->appendLine(cstr("second line"));
		comment->appendLine(cstr("third line\nfourth\r line"));
		comment->add();
	}
};

struct TestRule : public Test
{
	Manager mgr;
	TestRule()
	:Test("Rule", "cp 1.in 1.out\ncp 2.in 2.out\nHello Make\n")
	{
		Rule& rule = *mgr.own(new Rule());
		Rule& rule2 = *mgr.own(new Rule());
		rule.addTarget(cstr("all"));
		rule.addPrereq(cstr("1.out"));
		rule.addPrereq(cstr("2.out"));
		rule.addCommand(cstr("@echo Hello Make"));
		rule.add();

		rule2.addTarget(cstr("%.out"));
		rule2.addPrereq(cstr("%.in"));
		rule2.addCommand(cstr("cp $*.in $@"));
		rule2.add();

		std::ofstream in1("1.in"); in1.close();
		std::ofstream in2("2.in"); in2.close();
	}

	bool precond()
	{
		return fs::exists("1.out") && fs::exists("2.out");
	}
	~TestRule()
	{
		fs::remove("1.in"); fs::remove("2.in"); fs::remove("1.out"); fs::remove("2.out");
	}
};

struct TestConditional : public Test
{
	Manager mgr;

	TestConditional()
	:Test("Conditional",
			"Results:\n"
			"cond1 is true\n"
			"cond2 is false\n"
			"cond3 is true\n"
			"cond4 is false\n")
	{
		Variable& var1 = *mgr.own(new Variable(cstr("VAR1"), cstr("equal")));
		Variable& var2 = *mgr.own(new Variable(cstr("VAR2"), cstr("equal")));
		Variable& var3 = *mgr.own(new Variable(cstr("VAR3"), cstr("nequal")));
		Variable& ifvar1 = *mgr.own(new Variable(cstr("TESTVAR1"), cstr("cond1 is true")));
		Variable& ifvar2 = *mgr.own(new Variable(cstr("TESTVAR2"), cstr("cond2 is true")));
		Variable& ifvar3 = *mgr.own(new Variable(cstr("TESTVAR3"), cstr("cond3 is true")));
		Variable& ifvar4 = *mgr.own(new Variable(cstr("TESTVAR4"), cstr("cond4 is true")));
		Variable& elsevar1 = *mgr.own(new Variable(ifvar1.getName(), cstr("cond1 is false")));
		Variable& elsevar2 = *mgr.own(new Variable(ifvar2.getName(), cstr("cond2 is false")));
		Variable& elsevar3 = *mgr.own(new Variable(ifvar3.getName(), cstr("cond3 is false")));
		Variable& elsevar4 = *mgr.own(new Variable(ifvar4.getName(), cstr("cond4 is false")));
		ConditionalDEF& cdef1 = *mgr.own(new ConditionalDEF(var1.getName()));
		ConditionalDEF& cdef2 = *mgr.own(new ConditionalDEF(cstr("THISISNOTDEFINED")));
		ConditionalEQ& ceq1 = *mgr.own(new ConditionalEQ(var1.getRef(), var2.getRef()));
		ConditionalEQ& ceq2 = *mgr.own(new ConditionalEQ(var1.getRef(), var3.getRef()));
		Rule& rule = *mgr.own(new Rule());

		var1.getDef().add();
		var2.getDef().add();
		var3.getDef().add();
		rule.addTarget(cstr("all"));
		rule.addCommand(cstr("@echo Results:"));
		rule.addCommand(cstr("@echo ") + ifvar1.getRef());
		rule.addCommand(cstr("@echo ") + ifvar2.getRef());
		rule.addCommand(cstr("@echo ") + ifvar3.getRef());
		rule.addCommand(cstr("@echo ") + ifvar4.getRef());

		cdef1.addToIf(ifvar1.getDef());
		cdef1.addToElse(elsevar1.getDef());
		cdef2.addToIf(ifvar2.getDef());
		cdef2.addToElse(elsevar2.getDef());

		ceq1.addToIf(ifvar3.getDef());
		ceq1.addToElse(elsevar3.getDef());
		ceq2.addToIf(ifvar4.getDef());
		ceq2.addToElse(elsevar4.getDef());

		cdef1.add();
		cdef2.add();
		ceq1.add();
		ceq2.add();
		rule.add();

//		ofstream mf("test.mk");
//		Makefile::getSingleton().writeMakefile(mf);
	}
};

struct TestVariable : public Test
{
	Manager mgr;
	const std::vector<string> values;
	TestVariable()
	: Test("Variable",
		"a single value\n"
		"3.14592\n"
		"a single word__another word__value3\n")
	{
		Variable* v1 = mgr.own(new Variable(cstr("VAR1"), cstr("a single value")));
		Variable* v2 = mgr.own(new Variable(cstr("VAR2"), 3.14592));
		std::vector<string> values;
		values.push_back("a single word");
		values.push_back("another word");
		values.push_back("value3");
		Variable* v3 = mgr.own(new Variable(cstr("VAR3"), values.begin(), values.end(),
				Makefile::SHELL, cstr("__")));

		v1->getDef().add(); v2->getDef().add(); v3->getDef().add();

		Rule* rule = mgr.own_add(new Rule());
		rule->addTarget(cstr("all"));
		rule->addCommand(cstr("@echo ") + v1->getRef());
		rule->addCommand(cstr("@echo ") + v2->getRef());
		rule->addCommand(cstr("@echo ") + v3->getRef());
	}
};

void do_test(bool& result, Test* test)
{
	result &= test->run();
	delete test;
}
}} // namespace
using namespace makefile::tester;

int main(int argc, char *argv[])
{
	bool result = true;
	do_test(result, new TestComment);
	do_test(result, new TestRule);
	do_test(result, new TestConditional);
	do_test(result, new TestVariable);
	return !result;		// return 0 on success (= !true)
}


