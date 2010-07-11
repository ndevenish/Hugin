/**
 * @file Conditional.h
 * @brief
 *  Created on: Jul 10, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef CONDITIONAL_H_
#define CONDITIONAL_H_

/**
 *
 */
#include "MakefileItem.h"

#include <vector>

namespace makefile
{

/**
 * Makefiles can have conditional parts, that are only seen by make if the condition
 * is true. Therefore the conditional blocks can contain any one or more MakefileItem.
 * There are four kinds of conditionals, they can start with ifeq, ifneq, ifdef and ifndef.
 * The first two test for equality or non-equality, the latter two are true if a Variable is
 * defined or not.
 * This is the abstract baseclass for all those.
 * @note The output does intentionally not have intentions. Maybe that would be good and unproblematic
 * but I'm not sure, and intention matters in makefiles (see rules).
 */
class Conditional: public makefile::MakefileItem
{
	/// Output the header line, like ifeq (arg1, arg2).
	virtual std::string printif()=0;
	/// MakefileItem active if the condition is true.
	std::vector<MakefileItem*> ifblock;
	/// MakefileItem in the else block. The else-block is ommited if this is empty.
	std::vector<MakefileItem*> elseblock;
public:
	Conditional()
	{}
	virtual ~Conditional()
	{}

	void addToIf(MakefileItem& item)
	{
		ifblock.push_back(&item);
	}
	void addToElse(MakefileItem& item)
	{
		elseblock.push_back(&item);
	}

	virtual std::string toString();
};

class ConditionalEQ: public makefile::Conditional
{
	std::string arg1, arg2;
	virtual std::string printif()
	{
		return "ifeq (" + arg1 + "," + arg2 + ")\n";
	}
public:
	ConditionalEQ(std::string arg1_, std::string arg2_)
	:arg1(arg1_), arg2(arg2_)
	{}
	virtual ~ConditionalEQ()
	{}

};

class ConditionalNEQ: public makefile::Conditional
{
	std::string arg1, arg2;
	virtual std::string printif()
	{
		return "ifneq (" + arg1 + "," + arg2 + ")\n";
	}
public:
	ConditionalNEQ(std::string arg1_, std::string arg2_)
	:arg1(arg1_), arg2(arg2_)
	{}
	virtual ~ConditionalNEQ()
	{}

};

class ConditionalDEF: public makefile::Conditional
{
	std::string varname;
	virtual std::string printif()
	{
		return "ifdef " + varname + "\n";
	}
public:
	ConditionalDEF(std::string varname_)
	:varname(varname_)
	{}
	virtual ~ConditionalDEF()
	{}

};

class ConditionalNDEF: public makefile::Conditional
{
	std::string varname;
	virtual std::string printif()
	{
		return "ifndef " + varname + "\n";
	}
public:
	ConditionalNDEF(std::string varname_)
	:varname(varname_)
	{}
	virtual ~ConditionalNDEF()
	{}

};

}

#endif /* CONDITIONAL_H_ */
