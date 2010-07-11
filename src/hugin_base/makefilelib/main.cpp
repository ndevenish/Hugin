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
#include "Newline.h"
#include "Rule.h"
#include "Conditional.h"

#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>

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
	myfullname.setQuoteMode(Makefile::MAKE);
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
	try
	{
	Variable valuesucks("This_value_sucks", "any\nnewline");
	cout << valuesucks.getDef();
	}
	catch(std::exception& e)
	{
		cerr << e.what() << endl;
	}
	Variable namesucksless("This_name_sucks_less", "~~(bad:){\\value}");
	namesucksless.setQuoteMode(Makefile::SHELL);
	cout << namesucksless.getDef();
	namesucksless.setQuoteMode(Makefile::MAKE);
	cout << namesucksless.getDef();

	AutoVariable autovar("@");
//	cout << autovar.getDef(); causes an exception as it should.
	cout << autovar.getRef() << endl;

	return 0;
}

int tryreplace()
{
	boost::regex toescape;
//	boost::regex toescape("(p)|([Da])");
	std::string output;
//	std::string output("(?1--$&--)(?2__$&__)");
	toescape.assign("(\\$\\([^\\)]+\\))|(\\$[^\\(])|([\\\\ \\~\"\\|\\'\\`\\{\\}\\[\\]\\(\\)\\*\\#\\:\\=])");
	output.assign("(?1$&)(?2\\\\\\$$&)(?3\\\\$&)");
	std::string text("Ein_Dollar$ $ und_paar_andere (Sachen) werden $(richtig) escaped. backslash\\__ ein sternchen * doppelpunkt :=*~");
	cout << boost::regex_replace(text, toescape, output, boost::match_default | boost::format_all) << endl;
	return 0;

	cout << "SHELL mode" << endl << Makefile::quote(text, Makefile::SHELL) << endl;
	cout << "MAKE mode" << endl << Makefile::quote(text, Makefile::MAKE) << endl;

	return 0;
}

int trymakefile()
{
	Comment comment("First line");
	comment.appendLine("second line");
	comment.appendLine("third line\nfourth line\rfifth line");
	comment.add();

	Variable myname("MYNAME", "Flo");
	myname.getDef().add();

	Variable myfullname("MYFULLNAME", myname.getRef().toString() + " Achleitner");
	myfullname.getDef().add();
	myfullname.getRef().add();


	Newline nl1(2); nl1.add();
	Comment c1("Escaping modes:");
	Variable shellvar("SHELLVAR", "'has some special (char)s # [or] {not}", Makefile::SHELL);
	Variable makevar("MAKEVAR", "'has some special (char)s # [or] {not}", Makefile::MAKE);
	shellvar.getDef().add();
	makevar.getDef().add();
	Newline nl2; nl2.add();

	Makefile::getSingleton().writeMakefile(cout);
	Makefile::clean();
	return 0;
}

int tryrule()
{
	Comment c("Try how a rule looks like"); c.add();

	Variable t1("TARGET1", "t1.o"); t1.getDef().add();
	Variable t2("TARGET2", "t2.o"); t2.getDef().add();
	Variable p1("PRERE1", "t1.c"); p1.getDef().add();
	Variable p2("PRERE2", "t2.c"); p2.getDef().add();
	AutoVariable all("@");

	Rule r;
	r.addTarget(t1.getRef().toString());
	r.addTarget(t2.getRef().toString());
	r.addPrereq(p1.getRef().toString());
	r.addPrereq(p2.getRef().toString());
	r.addCommand("echo " + all.getRef().toString());
	r.add();

	r.toString();

	Makefile::getSingleton().writeMakefile(cout);
	Makefile::clean();

	return 0;
}
int trycond()
{
	Variable t1("TARGET1", "t1.o"); t1.getDef().add();
	Variable t2("TARGET2", "t2.o"); t2.getDef().add();
	Variable p1("PRERE1", "t1.c"); p1.getDef().add();
	Variable p2("PRERE2", "t2.c"); p2.getDef().add();
	AutoVariable all("@");
	Rule r;
	r.addTarget(t1.getRef().toString());
	r.addTarget(t2.getRef().toString());
	r.addPrereq(p1.getRef().toString());
	r.addPrereq(p2.getRef().toString());
	r.addCommand("echo " + all.getRef().toString());

	Variable iftrue("TRUE", "if_is_true");
	Variable iffales("FALSE", "if_is_false");

	ConditionalEQ cond1(iftrue.getRef().toString(), "if_is_true");
	cond1.addToIf(r);
	cond1.addToIf(iftrue.getDef());
	cond1.addToElse(iffales.getDef());
	cond1.addToElse(r);
	cond1.add();

	ConditionalNEQ cond2(iftrue.getRef().toString(), "if_is_true");
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

	Makefile::getSingleton().writeMakefile(cout);
	Makefile::clean();
	return 0;
}
int main(int argc, char *argv[])
{
//	return trymakefile();
//	return tryreplace();
//	return tryall();
//	return tryrule();
	return trycond();
	return 0;
}
