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
#include "StringAdapter.h"

#include <iostream>
#include <locale>
#include <vector>

using namespace makefile;
namespace fs = boost::filesystem;

#ifdef USE_WCHAR
ostream& cout =  std::wcout;
ostream& cerr =  std::wcerr;
#else
ostream& cout =  std::cout;
ostream& cerr =  std::cerr;
#endif

void printchars(ostream& out, wchar_t limit)
{
	char_type c;
	for(c = 0x20; c < limit; c++)
	{
		out << c;
	}

}

std::vector<path> createfiles_direct(const path dir, char_type limit)
{
	std::vector<path> miss;
	char_type c[] = cstr("X.1");
	for(*c = 0x20; *c < limit; (*c)++)
	{
		path filename(c);
		ofstream file(dir / filename);
		file.close();
		if(!fs::exists(dir / filename))
		{
			miss.push_back(filename);
		}
	}
	return miss;
}

std::vector<path> createfiles_make(const path dir, char_type limit)
{
	const path makefile(cstr("makefile"));
	std::vector<path> miss;
	char_type c[] = cstr("X.1");
	for(*c = 0x20; *c < limit; (*c)++)
	{
		path filename(c);
		ofstream makefilefile(dir / makefile);

		makefile::Variable mffilename(cstr("FILENAME"), (dir / filename).string(), makefile::Makefile::SHELL);
		makefile::Rule touch;
		touch.addTarget(cstr("all"));
		touch.addCommand(cstr("@touch ") + mffilename.getRef().toString());

		mffilename.getDef().add();
		touch.add();
		makefile::Makefile::getSingleton().writeMakefile(makefilefile);
		makefile::Makefile::clean();
		makefilefile.close();

		string dirstring = dir.string();
		std::string command("cd " + StringAdapter(dirstring) + " && make");
		int makeerr = std::system(command.c_str());

		if(makeerr)
			std::cerr << "make returned " << makeerr << std::endl;

		if(!fs::exists(dir / filename))
		{
			miss.push_back(filename);
		}
	}
	return miss;
}

int main(int argc, char *argv[])
{
	// set the environments locale.
	std::locale::global(std::locale(""));

	char_type limit;
	if(argc != 2)
	{
		std::cerr << "Specify a limit as first argument" << std::endl;
		return 1;
	}else{
		limit = std::atoi(argv[1]);
	}
	std::cout << "Creating " << static_cast<int>(limit) << " files, twice." << std::endl;

	path basepath(cstr("/tmp/chartest_direct"));
	path basepathmake(cstr("/tmp/chartest_make"));

	if(fs::is_directory(basepath))
		fs::remove_all(basepath);
	if(!fs::create_directories(basepath))
	{
		cerr << cstr("Error creating directory ") << basepath.string() << std::endl;
		return 1;
	}
	if(fs::is_directory(basepathmake))
			fs::remove_all(basepathmake);
	if(!fs::create_directories(basepathmake))
	{
		cerr << cstr("Error creating directory ") << basepathmake.string() << std::endl;
		return 1;
	}

//	ofstream outfile(basepath / cstr("tf.out"));
//	printchars(outfile, limit);

	std::vector<path> miss_direct = createfiles_direct(basepath, limit);
	cout << cstr("Direct: Missing files ") << miss_direct.size() << std::endl;
	for(std::vector<path>::iterator i = miss_direct.begin(); i != miss_direct.end(); i++)
		cout << i->string() << cstr('\n');


	std::vector<path> miss_make = createfiles_make(basepathmake, limit);
	cout << cstr("Make: Missing files ") << miss_make.size() << std::endl;
	for(std::vector<path>::iterator i = miss_make.begin(); i != miss_make.end(); i++)
			cout << i->string() << cstr('\n');
	return 0;
}
