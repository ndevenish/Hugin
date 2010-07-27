/**
 * @file test_util.cpp
 * @brief
 *  Created on: Jul 27, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstdio>
#include <sys/wait.h>
#include "Makefile.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

using namespace makefile;
namespace fs = boost::filesystem;
namespace io = boost::iostreams;

int exec_make(const char* const argv[], std::stringbuf& makeoutbuf, std::stringbuf& makeerrbuf)
{
	// store 2 fd {read, write}
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

		io::stream<io::file_descriptor_sink> makein(fdmakein[1]);
		io::stream<io::file_descriptor_source> makeout(fdmakeout[0]);
		io::stream<io::file_descriptor_source> makeerr(fdmakeerr[0]);
		// Write the makefile to make's stdin
		Makefile::getSingleton().writeMakefile(makein);
		Makefile::getSingleton().clean();
		makein.close();
		makeout.get(makeoutbuf, '\0');	// delimiter \0 should read until eof.
		makeerr.get(makeerrbuf, '\0');
		int status;
		wait(&status);
		if(WIFEXITED(status))
		{
			return WEXITSTATUS(status);
		}
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
		// like this const char* const argv[] = {"make", "-f-", (char*) NULL};
		execvp(argv[0], (char* const*)argv);
		return -1;		// exec should never return
	}
}
