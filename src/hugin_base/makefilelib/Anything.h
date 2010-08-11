/**
 * @file Anything.h
 * @brief
 *  Created on: Aug 11, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef ANYTHING_H_
#define ANYTHING_H_

/**
 *
 */
#include "MakefileItem.h"

namespace makefile
{
/**
 * This one only inserts some Text, not doing anything else.
 * It can be used to include make feautures not supported by the lib in a
 * makefile.
 */
class Anything: public PrimaryMakefileItem
{
	string text;
public:
	Anything()
	{
		// TODO Auto-generated constructor stub

	}
	Anything(const string& text_)
	: text(text_)
	{}

	virtual ~Anything() {}
	void setText(const string& text_)
	{
		text.assign(text_);
	}
	virtual string toString()
	{
		return text;
	}
};

}

#endif /* ANYTHING_H_ */
