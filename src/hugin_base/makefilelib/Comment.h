/**
 * @file Comment.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef COMMENT_H_
#define COMMENT_H_

#include "MakefileItem.h"
#include <string>

namespace makefile
{
/**
 * Encapsulates a Comment.
 * Makefile comments are prefixed with # for every line.
 * Currently only full line comments are supported.
 */
class Comment : public MakefileItem
{
	std::string text;

public:
	Comment() {}

	Comment(std::string s) : text(s) {}

	virtual ~Comment() { }

	std::string& getText()
	{
		return text;
	}

	void setText(const std::string& s)
	{
		text = s;
	}

	void appendLine(const std::string& line)
	{
		text.append("\n");
		text.append(line);
	}

	virtual std::string toString();
};

}

#endif /* COMMENT_H_ */
