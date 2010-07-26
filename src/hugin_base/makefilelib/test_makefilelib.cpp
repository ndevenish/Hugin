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
	// store 2 fd {read, write}
	int fdmakein[2];
	int fdmakeout[2];
	if(pipe(fdmakein) || pipe(fdmakeout))
	{
		std::cerr << "pipe() failed." << std::endl;
		return false;
	}
	std::stringbuf makeoutbuf;

	if(fork())
	{ // parent
		close(fdmakein[0]);
		close(fdmakeout[1]);

		io::stream<io::file_descriptor_sink> makein(fdmakein[1]);
		io::stream<io::file_descriptor_source> makeout(fdmakeout[0]);
		// Write the makefile to make's stdin
		Makefile::getSingleton().writeMakefile(makein);
		Makefile::getSingleton().clean();
		makein.close();
		makeout.get(makeoutbuf, '\0');	// delimiter \0 should read until eof.
		wait(NULL);


	} else { // child
		close(fdmakein[1]);
		close(fdmakeout[0]);
		// replace stdin and stdout
		if(dup2(fdmakein[0], 0) == -1 || dup2(fdmakeout[1], 1) == -1)
		{
			std::cerr << "Failed to switch std stream" << std::endl;
			exit(-1);
		}

		// execvp takes a NULL-terminated array of null-terminated strings. cool ;)
		const char* const argv[] = {"make", "-f-", (char*) NULL};
		execvp(argv[0], (char* const*)argv);
	}

	// Compare output
	std::cout << "make says: " << makeoutbuf.str() << std::endl;
	std::cout << testname << "   ";
	if(std::string(goodout) == makeoutbuf.str())
	{
		std::cout << "PASS" << std::endl;
		return true;
	}
	std::cout << "FAIL" << std::endl;
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
