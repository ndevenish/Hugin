/**
 * @file main.cpp
 * @brief
 *  Created on: May 21, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include <iostream>
#include "Comment.h"

using namespace std;
using namespace makefile;

int main(int argc, char *argv[])
{
	Comment comment("First line");

	comment.appendLine("second line");
	comment.appendLine("third line\nfourth\r line");
	cout << comment;
	return 0;
}
