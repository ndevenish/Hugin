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
 * @file main.cpp
 * @brief This file was only used for testing during development.
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
#include "Newline.h"
#include "Rule.h"
#include "Conditional.h"
#include "StringAdapter.h"

#include <boost/scoped_ptr.hpp>

using namespace makefile;

#ifdef USE_WCHAR
ostream& out =  std::wcout;
#else
ostream& out =  std::cout;
#endif

int tryall()
{
	Comment comment(cstr("First line"));
	comment.appendLine(cstr("second line"));
	comment.appendLine(cstr("third line\nfourth\r line"));
	out << comment;

	Variable myname(cstr("MYNAME"), cstr("Flo"));
	out << myname.getName() << std::endl;
	out << myname.getValue() << std::endl;
	out << myname.getDef();
	out << myname.getRef() << std::endl;

	Variable myfullname(cstr("MYFULLNAME"), myname.getRef().toString() + cstr(" Achleitner"));
	out << myfullname.getDef() << myfullname.getRef() << std::endl;
	myfullname.setQuoteMode(Makefile::MAKE);
	out << myfullname.getDef() << myfullname.getRef() << std::endl;

	try
	{
	Variable namesucks(cstr("This name sucks"), cstr("anyvalue"));
	out << namesucks.getDef();
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	try
	{
	Variable valuesucks(cstr("This_value_sucks"), cstr("any\nnewline"));
	out << valuesucks.getDef();
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	try
	{
	Variable valuesucks(cstr("This_value_sucks_not"), cstr("any escaped\\\nnewline"));
	out << valuesucks.getDef();
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	Variable namesucksless(cstr("This_name_sucks_less"), cstr("~~(bad:){\\value}"));
	namesucksless.setQuoteMode(Makefile::SHELL);
	out << namesucksless.getDef();
	namesucksless.setQuoteMode(Makefile::MAKE);
	out << namesucksless.getDef();

	AutoVariable autovar(cstr("@"));
//	out << autovar.getDef(); causes an exception as it should.
	out << autovar.getRef() << std::endl;

	return 0;
}

int tryreplace()
{
	regex toescape;
//	regex toescape(cstr("(p)|([Da])"));
	string output;
//	string output(cstr("(?1--$&--)(?2__$&__)"));
	toescape.assign(cstr("(\\$\\([^\\)]+\\))|(\\$[^\\(])|([\\\\ \\~\"\\|\\'\\`\\{\\}\\[\\]\\(\\)\\*\\#\\:\\=])"));
	output.assign(cstr("(?1$&)(?2\\\\\\$$&)(?3\\\\$&)"));
	string text(cstr("Ein_Dollar$ $ und_paar_andere (Sachen) werden $(richtig) escaped. backslash\\__ ein sternchen * doppelpunkt :=*~"));
	out << boost::regex_replace(text, toescape, output, boost::match_default | boost::format_all) << std::endl;
	return 0;

	out << "SHELL mode" << std::endl << Makefile::quote(text, Makefile::SHELL) << std::endl;
	out << "MAKE mode" << std::endl << Makefile::quote(text, Makefile::MAKE) << std::endl;

	return 0;
}

int trymakefile()
{
	Comment comment(cstr("First line"));
	comment.appendLine(cstr("second line"));
	comment.appendLine(cstr("third line\nfourth line\rfifth line"));
	comment.add();

	Variable myname(cstr("MYNAME"), cstr("Flo"));
	myname.getDef().add();

	Variable myfullname(cstr("MYFULLNAME"), myname.getRef().toString() + cstr(" Achleitner"));
	myfullname.getDef().add();
	myfullname.getRef().add();


	Newline nl1(2); nl1.add();
	Comment c1(cstr("Escaping modes:"));
	Variable shellvar(cstr("SHELLVAR"), cstr("'has some special (char)s # [or] {not}"), Makefile::SHELL);
	Variable makevar(cstr("MAKEVAR"), cstr("'has some special (char)s # [or] {not}"), Makefile::MAKE);
	shellvar.getDef().add();
	makevar.getDef().add();
	Newline nl2; nl2.add();

	Makefile::getSingleton().writeMakefile(out);
	Makefile::clean();
	return 0;
}

int tryrule()
{
	Comment c(cstr("Try how a rule looks like")); c.add();

	Variable t1(cstr("TARGET1"), cstr("t1.o")); t1.getDef().add();
	Variable t2(cstr("TARGET2"), cstr("t2.o")); t2.getDef().add();
	Variable p1(cstr("PRERE1"), cstr("t1.c")); p1.getDef().add();
	Variable p2(cstr("PRERE2"), cstr("t2.c")); p2.getDef().add();
	AutoVariable all(cstr("@"));

	Rule r;
	r.addTarget(t1.getRef().toString());
	r.addTarget(t2.getRef().toString());
	r.addPrereq(p1.getRef().toString());
	r.addPrereq(p2.getRef().toString());
	r.addCommand(cstr("echo ") + all.getRef().toString());
	r.add();

	r.toString();

	Makefile::getSingleton().writeMakefile(out);
	Makefile::clean();

	return 0;
}
int trycond()
{
	Variable t1(cstr("TARGET1"), cstr("t1.o")); t1.getDef().add();
	Variable t2(cstr("TARGET2"), cstr("t2.o")); t2.getDef().add();
	Variable p1(cstr("PRERE1"), cstr("t1.c")); p1.getDef().add();
	Variable p2(cstr("PRERE2"), cstr("t2.c")); p2.getDef().add();
	AutoVariable all(cstr("@"));
	Rule r;
	r.addTarget(t1.getRef().toString());
	r.addTarget(t2.getRef().toString());
	r.addPrereq(p1.getRef().toString());
	r.addPrereq(p2.getRef().toString());
	r.addCommand(cstr("echo ") + all.getRef().toString());

	Variable iftrue(cstr("TRUE"), cstr("if_is_true"));
	Variable iffales(cstr("FALSE"), cstr("if_is_false"));

	ConditionalEQ cond1(iftrue.getRef().toString(), cstr("if_is_true"));
	cond1.addToIf(r);
	cond1.addToIf(iftrue.getDef());
	cond1.addToElse(iffales.getDef());
	cond1.addToElse(r);
	cond1.add();

	ConditionalNEQ cond2(iftrue.getRef().toString(), cstr("if_is_true"));
	cond2.addToIf(r);
	cond2.addToIf(iftrue.getDef());
	cond2.addToElse(iffales.getDef());
	cond2.addToElse(r);
	cond2.add();

	ConditionalDEF cond3(iftrue.getName());
	cond3.addToIf(r);
	cond3.addToIf(iftrue.getDef());
	cond3.addToElse(iffales.getDef());
	cond3.addToElse(r);
	cond3.add();

	ConditionalNDEF cond4(iftrue.getName());
	cond4.addToIf(r);
	cond4.addToIf(iftrue.getDef());
	cond4.add();

	Makefile::getSingleton().writeMakefile(out);
	Makefile::clean();
	return 0;
}
int main(int argc, char *argv[])
{
	return
//	trymakefile() ||
//	tryreplace() ||
	tryall() ||
//	tryrule() ||
//	trycond() ||
	0;
}
