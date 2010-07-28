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

#include "test_util.h"

#include <iostream>
#include <locale>
#include <vector>

using namespace makefile;
using namespace makefile::tester;
namespace fs = boost::filesystem;

#ifdef USE_WCHAR
ostream& cout =  std::wcout;
ostream& cerr =  std::wcerr;
#else
ostream& cout =  std::cout;
ostream& cerr =  std::cerr;
#endif

#define START 0x20

/**
 * Prints the tested characters.
 * @param out
 * @param limit
 */
void printchars(ostream& out, wchar_t limit)
{
	char_type c;
	for(c = 0x20; c < limit; c++)
	{
		out << c;
	}

}
/**
 * Tries to create files with names from 0x20 to limit using boost::filesystem.
 * @param dir
 * @param limit Upper limit for filenames
 * @return Filenames that couldn't be found after creation.
 */
std::vector<path> createfiles_direct(const path dir, uchar_type limit)
{
	std::vector<path> miss;
	char_type c[] = cstr("X.1");
	for(*c = START; static_cast<uchar_type>(*c) < limit; (*c)++)
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
/**
 * Tries to create files with names from 0x20 to limit by calling make.
 * It first writes a makefile of this structure:
@verbatim
FILENAME=X.1

all :
@touch $(FILENAME)
@endverbatim
 * Afterwards make is called with system() and creates the file by executing touch.
 *
 * @param dir
 * @param limit Upper limit for filenames
 * @return Filenames that couldn't be found after creation.
 */
std::vector<path> createfiles_make(const path dir, uchar_type limit)
{
	const path makefile(cstr("makefile"));
	std::vector<path> miss;
	char_type c[] = cstr("X.1");
	for(*c = START; static_cast<uchar_type>(*c) < limit; (*c)++)
	{
		path filename(c);

		// If the filename cannot be stored in a Variable, theres no chance to bring it through.
		try {
			makefile::Variable mffilename(cstr("FILENAME"), (dir / filename).string(), makefile::Makefile::SHELL);

			makefile::Rule touch;
			touch.addTarget(cstr("all"));
			touch.addCommand(cstr("@touch ") + mffilename.getRef().toString());

			mffilename.getDef().add();
			touch.add();

			string dirstring = dir.string();
			const char* argv[] = {"make", ("-C" + StringAdapter(dirstring)).c_str(), "-f-", NULL};
			std::stringbuf makeout, makeerr;
			int ret = exec_make(argv, makeout, makeerr);


			if(ret)
			{
				std::cerr << "make returned " << ret << std::endl;
				std::cout << makeout.str();
				std::cerr << makeerr.str();
			}
			}
		catch(std::exception& e) {
			std::cerr << "Variable exception: " << e.what() << std::endl;
		}

		if(!fs::exists(dir / filename))
		{
			miss.push_back(filename);
		}
	}
	return miss;
}
/**
 * Removes and recreates the output directories.
 * @param dir
 * @return 0 on success, 1 on error.
 */
int cleandir(path dir)
{
	if(fs::is_directory(dir))
		fs::remove_all(dir);
	if(!fs::create_directories(dir))
	{
		cerr << cstr("Error creating directory ") << dir.string() << std::endl;
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	// set the environments locale.
	std::locale::global(std::locale(""));

	uchar_type limit;
	if(argc != 2)
	{
		std::cerr << "Specify a limit as first argument" << std::endl;
		return 1;
	}else{
		limit = std::atoi(argv[1]);
	}
	std::cout << "Creating " << static_cast<unsigned long>(limit) - START << " files in" << std::endl;

	path basepath(fs::initial_path<path>() / cstr("chartest_direct"));
	path basepathmake(fs::initial_path<path>() / cstr("chartest_make"));

	cout << basepath.string() << std::endl;
	cout << basepathmake.string() << std::endl;

	if(cleandir(basepath) || cleandir(basepathmake))
		return 1;

//	ofstream outfile(basepath / cstr("tf.out"));
//	printchars(outfile, limit);

	std::vector<path> miss_direct = createfiles_direct(basepath, limit);
	cout << cstr("Direct: Missing files ") << miss_direct.size() << std::endl;
	for(std::vector<path>::iterator i = miss_direct.begin(); i != miss_direct.end(); i++)
		cout << i->string() << cstr('\n');
	cout << std::endl;

	std::vector<path> miss_make = createfiles_make(basepathmake, limit);
	cout << cstr("Make: Missing files ") << miss_make.size() << std::endl;
	for(std::vector<path>::iterator i = miss_make.begin(); i != miss_make.end(); i++)
			cout << i->string() << cstr('\n');
	cout << std::endl;
	return 0;
}
