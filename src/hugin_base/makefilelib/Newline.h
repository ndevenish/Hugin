/**
 * @file Newline.h
 * @brief
 *  Created on: Jul 8, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef NEWLINE_H_
#define NEWLINE_H_

/**
 *
 */
#include "MakefileItem.h"

namespace makefile
{
/**
 * Simply prints newlines. Can be used to give some structure.
 */
class Newline: public makefile::MakefileItem
{
	int newlines;
public:
	Newline(int newlines_ =1)
	:newlines(newlines_)
	{}
	virtual ~Newline()
	{}

	virtual std::string toString()
	{
		std::string n;
		for(int i=0; i<newlines; i++)
			n.append("\n");
		return n;
	}
};

}

#endif /* NEWLINE_H_ */
