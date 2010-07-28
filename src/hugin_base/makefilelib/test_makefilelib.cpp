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
	return !result;		// return 0 on success (= !true)
}


