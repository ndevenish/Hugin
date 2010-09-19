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
 * @file test_util_win32.cpp
 * @brief Useful functions for testers. This is the WIN32 Version.
 * It uses black WIN32-API magic to to the same as the original.
 *  Created on: Jul 27, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstdio>
#include <iomanip>
#include "Makefile.h"
#include "test_util.h"

#include <windows.h>
#include <tchar.h>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/code_converter.hpp>

using namespace makefile;
namespace fs = boost::filesystem;
namespace io = boost::iostreams;

namespace makefile { namespace tester {
/**
 * Executes make with capturing stderr and stdout and feeding the makefile via stdin.
 * This is the WIN32 Version.
 * It uses black WIN32-API magic to to the same as the original.
 * @param argv as required by execvp (execvp takes a NULL-terminated array of null-terminated strings.) See manpage.
 * @param makeoutbuf stdout of make goes here.
 * @param makeerrbuf stderr of make goes here.
 * @return return value of make. Uses macros to extract the value from wait(). See man wait.
 */
int exec_make(std::stringbuf& makeoutbuf, std::stringbuf& makeerrbuf)
{
	// store 2 HANDLE per pipe {read, write}
	HANDLE hmakein_rd = NULL;
	HANDLE hmakein_wr = NULL;
	HANDLE hmakeout_rd = NULL;
	HANDLE hmakeout_wr = NULL;
	HANDLE hmakeerr_rd = NULL;
	HANDLE hmakeerr_wr = NULL;

	// this is taken from an example in msdn
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES); // seems that windows doesn't know it's own structs ;)
	sa.bInheritHandle = true;
	sa.lpSecurityDescriptor = NULL;

	if( !CreatePipe(&hmakeout_rd, &hmakeout_wr, &sa, 0) ||	// create pipes  
		!CreatePipe(&hmakein_rd, &hmakein_wr, &sa, 0) ||
		!CreatePipe(&hmakeerr_rd, &hmakeerr_wr, &sa, 0) ||
		!SetHandleInformation(hmakeout_rd, HANDLE_FLAG_INHERIT, 0) || // and set inheritance handle information
		!SetHandleInformation(hmakein_wr, HANDLE_FLAG_INHERIT, 0) ||
		!SetHandleInformation(hmakeerr_rd, HANDLE_FLAG_INHERIT, 0))
	{
		std::cerr << "CreatePipe() failed." << std::endl;
		return -1;
	}


	// parent creates child
	PROCESS_INFORMATION pi; 
	STARTUPINFO si;
	 
	// Set up members of the PROCESS_INFORMATION structure. 
 
	ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );
 
	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
 
	ZeroMemory( &si, sizeof(STARTUPINFO) );
	si.cb = sizeof(STARTUPINFO); 
	si.hStdError = hmakeerr_wr;
	si.hStdOutput = hmakeout_wr;
	si.hStdInput = hmakein_rd;
	si.dwFlags |= STARTF_USESTDHANDLES;
 
	// Create the child process. 
    
	if( !CreateProcess(NULL, 
	"make -f-",     // command line
	NULL,          // process security attributes 
	NULL,          // primary thread security attributes 
	TRUE,          // handles are inherited 
	0,             // creation flags 
	NULL,          // use parent's environment 
	NULL,          // use parent's current directory 
	&si,  // STARTUPINFO pointer 
	&pi))  // receives PROCESS_INFORMATION 
	{
		std::cerr << "CreateProcess failed" << std::endl;
		return -1;
	}
	// Important, close unused pipe ends.
	CloseHandle(hmakeout_wr);
	CloseHandle(hmakeerr_wr);
	CloseHandle(hmakein_rd);

		
		// boost gives us a way to use those pipes in C++ style.
		// the code_converter allows wchar mode Makefile to be written to a char-pipe
#ifdef USE_WCHAR
	io::stream< io::code_converter<io::file_descriptor_sink> > makein(fdmakein[1]);
#else
	io::stream<io::file_descriptor_sink> makein(hmakein_wr);
#endif
	io::stream<io::file_descriptor_source> makeout(hmakeout_rd);
	io::stream<io::file_descriptor_source> makeerr(hmakeerr_rd);

	// Write the makefile to make's stdin
	Makefile::getSingleton().writeMakefile(makein);
	Makefile::getSingleton().clean();
	makein.close();

	makeout.get(makeoutbuf, '\0');	// delimiter \0 should read until eof.
	makeerr.get(makeerrbuf, '\0');

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD status;
	GetExitCodeProcess(pi.hProcess, &status);
	return status;
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
