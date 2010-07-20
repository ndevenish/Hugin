/**
 * @file test_filenames.cpp
 * @brief Tester that tries to feed a lot of possible characters
 * as filenames into the makefilelib, make, and the filesystem.
 *
 *  Created on: Jul 16, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "char_type.h"

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
#include <locale>

using namespace makefile;
namespace fs = boost::filesystem;

void printchars(ostream& out, wchar_t limit)
{
	char_type c;
	for(c = 0x20; c < limit; c++)
	{
		out << c;
	}

}

int createfiles_direct(const path dir, char_type limit)
{
	int err = 0;
	char_type c[] = cstr("X.1");
	for(*c = 0x20; *c < limit; (*c)++)
	{
		path filename(c);
		ofstream file(dir / filename);
		file.close();
		if(!fs::exists(dir / filename))
		{
			err++;
		}
	}
	return err;
}

int createfiles_make(const path dir, char_type limit)
{
	const path makefile(cstr("makefile"));
	int err = 0;
#if 0
	wchar_t c[] = L"X.1";
	for(*c = 0x20; *c < limit; (*c)++)
	{
		path filename(c);
		ofstream makefilefile(dir / makefile);

		makefile::Variable mffilename("FILENAME", (dir / filename).string());
		makefile::Rule touch;
		touch.addTarget(mffilename.getRef());
		touch.addCommand("touch " + mffilename.getRef());

		makefile::Makefile::getSingleton().writeMakefile(makefilefile);
		makefile::Makefile::clean();

		if(!fs::exists(dir / filename))
		{
			err++;
		}
	}
#endif
	return err;
}

int main(int argc, char *argv[])
{
	// set the environments locale.
	std::locale::global(std::locale(""));

	const char_type limit = 1024;
	path basepath(cstr("/tmp/chartest"));

	if(fs::is_directory(basepath))
		fs::remove_all(basepath);
	if(!fs::create_directories(basepath))
	{
		std::cerr << "Error creating directory " /*<< basepath.string()*/ << std::endl;
		return 1;
	}

	ofstream outfile(basepath / cstr("tf.out"));
	printchars(outfile, limit);
	int err = createfiles_direct(basepath, limit);
	std::cout << "Missing files " << err << std::endl;
	return 0;
}
