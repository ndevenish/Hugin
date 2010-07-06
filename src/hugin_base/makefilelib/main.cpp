/**
 * @file main.cpp
 * @brief
 *  Created on: May 21, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include <iostream>
#include <stdexcept>
#include "Comment.h"
#include "Variable.h"
#include "VariableDef.h"
#include "VariableRef.h"
#include "MakefileItem.h"
#include "Makefile.h"
#include "AutoVariable.h"

#include <boost/regex.hpp>

using namespace std;
using namespace makefile;

int tryall()
{
	Comment comment("First line");
	comment.appendLine("second line");
	comment.appendLine("third line\nfourth\r line");
	cout << comment;

	Variable myname("MYNAME", "Flo");
	cout << myname.getName() << endl;
	cout << myname.getValue() << endl;
	cout << myname.getDef();
	cout << myname.getRef() << endl;

	Variable myfullname("MYFULLNAME", myname.getRef().toString() + " Achleitner");
	cout << myfullname.getDef() << myfullname.getRef() << endl;

	try
	{
	Variable namesucks("This name sucks", "anyvalue");
	cout << namesucks.getDef();
	}
	catch(std::exception& e)
	{
		cerr << e.what() << endl;
	}
	Variable namesucksless("This_name_sucks_less", "badvalue");
	cout << namesucksless.getDef();

	AutoVariable autovar("@");
//	cout << autovar.getDef(); causes an exception as it should.
	cout << autovar.getRef() << endl;
}

int tryreplace()
{
	boost::regex toescape("(\\$)|([\\\\ \\~\\$\"\\|\\'\\`\\{\\}\\[\\]\\(\\)\\*\\#\\:\\=])");
//	boost::regex toescape("(p)|([Da])");
	std::string output("(?1\\$$&)(?2\\\\$&)");
//	std::string output("(?1--$&--)(?2__$&__)");
	std::string text("Ein_Dollar$ $ und_paar_andere Sachen werden richtig escaped. backslash\\__ ein sternchen * doppelpunkt :=*~");
	cout << boost::regex_replace(text, toescape, output, boost::match_default | boost::format_all) << endl;

	cout << "SHELL mode" << endl << Makefile::quote(text, Makefile::SHELL) << endl;
	cout << "MAKE mode" << endl << Makefile::quote(text, Makefile::MAKE) << endl;
}
int main(int argc, char *argv[])
{
	return tryreplace();
	return tryall();
	return 0;
}
