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
	Comment comment;
	TestComment()
	:Test("Comment", ""),
	 comment(cstr("First line"))
	{
		comment.appendLine(cstr("second line"));
		comment.appendLine(cstr("third line\nfourth\r line"));
		comment.add();
	}
};

struct TestRule : public Test
{
	Rule rule, rule2;

	TestRule()
	:Test("Rule", "cp 1.in 1.out\ncp 2.in 2.out\nHello Make\n")
	{
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
	Variable var1, var2, var3, ifvar1, ifvar2, ifvar3, ifvar4, elsevar1, elsevar2, elsevar3, elsevar4;
	ConditionalDEF cdef1, cdef2;
	ConditionalEQ ceq1, ceq2;
	Rule rule;
	TestConditional()
	:Test("Conditional",
			"Results:\n"
			"cond1 is true\n"
			"cond2 is false\n"
			"cond3 is true\n"
			"cond4 is false\n"),
	var1(cstr("VAR1"), cstr("equal")),
	var2(cstr("VAR2"), cstr("equal")),
	var3(cstr("VAR3"), cstr("nequal")),
	ifvar1(cstr("TESTVAR1"), cstr("cond1 is true")),
	ifvar2(cstr("TESTVAR2"), cstr("cond2 is true")),
	ifvar3(cstr("TESTVAR3"), cstr("cond3 is true")),
	ifvar4(cstr("TESTVAR4"), cstr("cond4 is true")),
	elsevar1(ifvar1.getName(), cstr("cond1 is false")),
	elsevar2(ifvar2.getName(), cstr("cond2 is false")),
	elsevar3(ifvar3.getName(), cstr("cond3 is false")),
	elsevar4(ifvar4.getName(), cstr("cond4 is false")),
	cdef1(var1.getName()),
	cdef2(cstr("THISISNOTDEFINED")),
	ceq1(var1.getRef(), var2.getRef()),
	ceq2(var1.getRef(), var3.getRef())
	{
		var1.getDef().add();
		var2.getDef().add();
		var3.getDef().add();
		rule.addTarget(cstr("all"));
		rule.addCommand(cstr("@echo Results:"));
		rule.addCommand(cstr("@echo ") + ifvar1.getRef().toString());
		rule.addCommand(cstr("@echo ") + ifvar2.getRef().toString());
		rule.addCommand(cstr("@echo ") + ifvar3.getRef().toString());
		rule.addCommand(cstr("@echo ") + ifvar4.getRef().toString());

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
	return !result;		// return 0 on success (= !true)
}


