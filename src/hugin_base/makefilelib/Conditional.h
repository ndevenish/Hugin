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
	virtual string printif()=0;
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

	virtual string toString();
};

class ConditionalEQ: public makefile::Conditional
{
	string arg1, arg2;
	virtual string printif()
	{
		return cstr("ifeq (") + arg1 + cstr(",") + arg2 + cstr(")\n");
	}
public:
	ConditionalEQ(string arg1_, string arg2_)
	:arg1(arg1_), arg2(arg2_)
	{}
	virtual ~ConditionalEQ()
	{}

};

class ConditionalNEQ: public makefile::Conditional
{
	string arg1, arg2;
	virtual string printif()
	{
		return cstr("ifneq (") + arg1 + cstr(",") + arg2 + cstr(")\n");
	}
public:
	ConditionalNEQ(string arg1_, string arg2_)
	:arg1(arg1_), arg2(arg2_)
	{}
	virtual ~ConditionalNEQ()
	{}

};

class ConditionalDEF: public makefile::Conditional
{
	string varname;
	virtual string printif()
	{
		return cstr("ifdef ") + varname + cstr("\n");
	}
public:
	ConditionalDEF(string varname_)
	:varname(varname_)
	{}
	virtual ~ConditionalDEF()
	{}

};

class ConditionalNDEF: public makefile::Conditional
{
	string varname;
	virtual string printif()
	{
		return cstr("ifndef ") + varname + cstr("\n");
	}
public:
	ConditionalNDEF(string varname_)
	:varname(varname_)
	{}
	virtual ~ConditionalNDEF()
	{}

};

}

#endif /* CONDITIONAL_H_ */
