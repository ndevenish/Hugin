/**
 * @file test_filenames.cpp
 * @brief Tester that tries to feed a lot of possible characters
 * as filenames into the makefilelib, make, and the filesystem.
 *
 *  Created on: Jul 16, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

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

#include <iostream>
#include <boost/filesystem/fstream.hpp>
#include <locale>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

void printchars(std::wostream& out, wchar_t limit)
{
	wchar_t c;
	for(c = 0x20; c < limit; c++)
	{
		out << c;
	}

}

int createfiles_direct(const fs::wpath path, wchar_t limit)
{
	int err = 0;
	wchar_t c[] = L"X.1";
	for(*c = 0x20; *c < limit; (*c)++)
	{
		fs::wpath filename(c);
		fs::ofstream file(path / filename);
		file.close();
		if(!fs::exists(path / filename))
		{
			err++;
		}
	}
	return err;
}

int main(int argc, char *argv[])
{
	// set the environments locale.
	std::locale::global(std::locale(""));

	const wchar_t limit = 4096;
	fs::wpath basepath(L"/tmp/chartest");

	if(fs::is_directory(basepath))
		fs::remove_all(basepath);
	if(!fs::create_directories(basepath))
	{
		std::cerr << "Error creating directory " /*<< basepath.string()*/ << std::endl;
		return 1;
	}

	fs::wofstream outfile(basepath / L"tf.out");
	printchars(outfile, limit);
	int err = createfiles_direct(basepath, limit);
	std::cout << "Missing files " << err << std::endl;
	return 0;
}
