/**
 * @file Rule.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef RULE_H_
#define RULE_H_

#include "MakefileItem.h"

namespace makefile
{

/**
 * Represents a makefile rule, including Prerequisite and Command.
 *
 */
class Rule : public MakefileItem
{
public:
	Rule()
	{
		// TODO Auto-generated constructor stub

	}
	virtual ~Rule()
	{
		// TODO Auto-generated destructor stub
	}

	virtual std::string toString();
};

}

#endif /* RULE_H_ */
