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
 * @file test_util.cpp
 * @brief Useful functions for testers.
 *  Created on: Jul 27, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstdio>
#include <sys/wait.h>
#include <iomanip>
#include "Makefile.h"
#include "test_util.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/code_converter.hpp>

using namespace makefile;
namespace fs = boost::filesystem;
namespace io = boost::iostreams;

namespace makefile { namespace tester {
/**
 * Executes make with capturing stderr and stdout and feeding the makefile via stdin.
 * @param argv as required by execvp (execvp takes a NULL-terminated array of null-terminated strings.) See manpage.
 * @param makeoutbuf stdout of make goes here.
 * @param makeerrbuf stderr of make goes here.
 * @return return value of make. Uses macros to extract the value from wait(). See man wait.
 */
int exec_make(std::stringbuf& makeoutbuf, std::stringbuf& makeerrbuf)
{
	// store 2 fd per pipe {read, write}
	int fdmakein[2];
	int fdmakeout[2];
	int fdmakeerr[2];
	if(pipe(fdmakein) || pipe(fdmakeout) || pipe(fdmakeerr))
	{
		std::cerr << "pipe() failed." << std::endl;
		return false;
	}


	if(fork())
	{ // parent
		close(fdmakein[0]);
		close(fdmakeout[1]);
		close(fdmakeerr[1]);

		// boost gives us a way to use those pipes in C++ style.
		// the code_converter allows wchar mode Makefile to be written to a char-pipe
#ifdef USE_WCHAR
		io::stream< io::code_converter<io::file_descriptor_sink> > makein(fdmakein[1]);
#else
		io::stream<io::file_descriptor_sink> makein(fdmakein[1]);
#endif
		io::stream<io::file_descriptor_source> makeout(fdmakeout[0]);
		io::stream<io::file_descriptor_source> makeerr(fdmakeerr[0]);

		// Write the makefile to make's stdin
		Makefile::getSingleton().writeMakefile(makein);
		Makefile::getSingleton().clean();
		makein.close();

		makeout.get(makeoutbuf, '\0');	// delimiter \0 should read until eof.
		makeerr.get(makeerrbuf, '\0');

		int status;
		wait(&status);			// status contains return value an some other flags
		if(WIFEXITED(status))	// that can be evaluated by these macros (man wait)
		{
			return WEXITSTATUS(status);
		}
		// If WIFEXITED is false, something complicated happened (signal etc..).
		std::cerr << "Command terminated abnormally. status=" << std::hex << status << std::endl;
		return status;


	} else { // child
		close(fdmakein[1]);
		close(fdmakeout[0]);
		close(fdmakeerr[0]);
		// replace stdin and stdout
		if(dup2(fdmakein[0], 0) == -1 || dup2(fdmakeout[1], 1) == -1 || dup2(fdmakeerr[1], 2) == -1)
		{
			std::cerr << "Failed to switch std stream" << std::endl;
			exit(-1);
		}

		// execvp takes a NULL-terminated array of null-terminated strings. cool ;)
		const char* const argv[] = {"make", "-f-", (char*) NULL};
		execvp(argv[0], (char* const*)argv);
		return -1;		// exec should never return
	}
}

bool Test::run()
{
	int status = exec_make(makeoutbuf, makeerrbuf);

	std::cout << std::setw(30) << std::left <<  name;
	if(eval())
	{
		std::cout << "PASS" << std::endl;
		return true;
	}
	std::cout << "FAIL" << std::endl;
	std::cout << "ret: " << status << std::endl;
	std::cout << "out: " << makeoutbuf.str() << std::endl;
	std::cout << "cmp: " << goodout << std::endl;
	std::cout << "err: " << makeerrbuf.str() << std::endl;
	return false;
}
}}
