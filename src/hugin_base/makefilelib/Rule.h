/*
This file is part of hugin.

hugin is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

hugin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with hugin.  If not, see <http://www.gnu.org/licenses/>.
*/

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
class MAKEIMPEX Rule : public PrimaryMakefileItem
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
    /** Adds an other rule as a prerequisite for this rule */
    void addPrereq(Rule& r)
    {
        for(std::vector<string>::iterator i = r.targets.begin(); i != r.targets.end(); ++i)
        {
            addPrereq(*i);
        }
    };
    void addPrereq(Rule* r)
    {
        addPrereq(*r);
    };
	/**
	 * Adds a string as a command to the Rule.
	 * @param c
	 */
	void addCommand(string c,bool doEcho=true,bool ignoreErrors=false)
	{
        string command;
        if(!doEcho)
        {
            command="@";
        };
        if(ignoreErrors)
        {
            command.append("-");
        };
        command.append(c);
        commands.push_back(command);
	}
};

}

#endif /* RULE_H_ */
