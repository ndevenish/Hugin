/**
 * @file Rule.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef RULE_H_
#define RULE_H_

#include "MakefileItem.h"
#include "Variable.h"
#include "VariableRef.h"

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

	/**
	 * Adds a target string.
	 * @param t
	 */
	void addTarget(const string& t)
	{
		targets.push_back(t);
	}
	/**
	 * Adds a Variable as a Target. A VariableRef is taken from the Variable and added.
	 * @param t
	 */
	void addTarget(Variable& t)
	{
		addTarget(t.getRef().toString());
	}
	void addTarget(Variable* t)
	{
		addTarget(t->getRef().toString());
	}
	/**
	 * Adds a string as a prerequisite.
	 * @param p
	 */
	void addPrereq(string p)
	{
		prerequisites.push_back(p);
	}
	/**
	 * Adds a Variable as a prerequisite. A VariableRef is taken from the Variable and added.
	 * @param p
	 */
	void addPrereq(Variable& p)
	{
		addPrereq(p.getRef().toString());
	}
	void addPrereq(Variable* p)
	{
		addPrereq(p->getRef().toString());
	}
	/**
	 * Adds a string as a command to the Rule.
	 * @param c
	 */
	void addCommand(string c)
	{
		commands.push_back(c);
	}
};

}

#endif /* RULE_H_ */
