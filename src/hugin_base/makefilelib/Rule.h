/**
 * @file Rule.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef RULE_H_
#define RULE_H_

#include "MakefileItem.h"

#include <vector>

namespace makefile
{

/**
 * Represents a makefile rule, including Prerequisite and Command.
 *
 */
class Rule : public PrimaryMakefileItem
{
	std::vector<string> targets;
	std::vector<string> prerequisites;
	std::vector<string> commands;

public:
	Rule()
	{}
	virtual ~Rule()
	{}

	virtual string toString();

	void addTarget(string t)
	{
		targets.push_back(t);
	}
	void addPrereq(string p)
	{
		prerequisites.push_back(p);
	}
	void addCommand(string c)
	{
		commands.push_back(c);
	}
};

}

#endif /* RULE_H_ */
