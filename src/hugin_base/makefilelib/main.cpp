/**
 * @file main.cpp
 * @brief
 *  Created on: May 21, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include <iostream>
#include "Comment.h"
#include "Variable.h"
#include "VariableDef.h"
#include "VariableRef.h"
#include "MakefileItem.h"
#include "Makefile.h"
using namespace std;
using namespace makefile;

int main(int argc, char *argv[])
{
	Comment comment("First line");

	comment.appendLine("second line");
	comment.appendLine("third line\nfourth\r line");
	cout << comment;

	Variable myname("MYNAME", "Flo");
	VariableDef mynamedef(myname);
	VariableRef mynameref(myname);
	cout << myname.getName() << endl;
	cout << myname.getValue() << endl;
	cout << mynamedef << endl;
	cout << mynameref << endl;
	cout << myname.getDef();
	cout << myname.getRef() << endl;
	return 0;
}
