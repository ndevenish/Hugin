/**
 * @file test_makefilelib.cpp
 * @brief Tests all features of the lib.
 *  Created on: Jul 24, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "char_type.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstdio>
#include <sys/wait.h>
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

#ifdef USE_WCHAR
ostream& cout =  std::wcout;
ostream& cerr =  std::wcerr;
#else
ostream& cout =  std::cout;
ostream& cerr =  std::cerr;
#endif

bool run(const char* testname, const char* goodout)
{
	std::stringbuf makeoutbuf, makeerrbuf;
	const char* argv[] = {"make", "-f-", NULL};
	int status = exec_make(argv, makeoutbuf, makeerrbuf);

	// Compare output
	std::cout << std::setw(30) << std::left <<  testname;
	if(std::string(goodout) == makeoutbuf.str())
	{
		std::cout << "PASS" << std::endl;
		return true;
	}
	std::cout << "FAIL" << std::endl;
	std::cout << "ret: " << status << std::endl;
	std::cout << "out: " << makeoutbuf.str() << std::endl;
	std::cout << "err: " << makeerrbuf.str() << std::endl;
	return false;
}
bool test_Comment()
{
	Comment comment(cstr("First line"));
	comment.appendLine(cstr("second line"));
	comment.appendLine(cstr("third line\nfourth\r line"));
	comment.add();
	return run("Comment", "");
}

bool test_Rule()
{
	Rule rule;
	rule.addTarget(cstr("all"));
	rule.addCommand(cstr("@echo Hello Make"));
	rule.add();
	return run("Rule", "Hello Make\n");
}
int main(int argc, char *argv[])
{
	bool result = true;
	result &= test_Comment();
	result &= test_Rule();
	return !result;		// return 0 on success (= !true)
}
