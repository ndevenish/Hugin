/**
 * @file Comment.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef COMMENT_H_
#define COMMENT_H_

#include "MakefileItem.h"

namespace makefile
{
/**
 * Encapsulates a Comment.
 * Makefile comments are prefixed with # for every line.
 * Currently only full line comments are supported.
 */
class Comment : public PrimaryMakefileItem
{
	string text;

public:
	Comment() {}

	Comment(string s) : text(s) {}

	virtual ~Comment() { }

	string& getText()
	{
		return text;
	}

	void setText(const string& s)
	{
		text = s;
	}

	void appendLine(const string& line)
	{
		text.append(cstr("\n"));
		text.append(line);
	}

	virtual string toString();
};

}

#endif /* COMMENT_H_ */
