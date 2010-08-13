/**
 * @file MakefileItem.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef MAKEFILEITEM_H_
#define MAKEFILEITEM_H_

#include "Makefile.h"
#include "Manageable.h"

namespace makefile
{

/**
 * The virtual baseclass for all objects that appear in the Makefile.
 * Subclasses must implement \ref print which allows us to write them to
 * an ostream, like a string, nice :).
 * The various implementations of \ref print have to take care of proper
 * makefile compatible output.
 */
class MakefileItem
{
public:
	MakefileItem()
	{
	}

	/// Removes the item from the Makefile
	virtual ~MakefileItem()
	{
		Makefile::remove(this);
	}

	/// @return A string representation of the MakefileItem.
	virtual string toString()=0;
	/// Write the items representation to an ostream in a makefile compatible way.
	void print(ostream& os)
	{
		os << toString();
	}
	/// Allow casts to string, very nice!
	operator string()
	{
		return toString();
	}

	/// Adds this to the Makefile item list.
	virtual void add()
	{
		Makefile::getSingleton().add(this);
	}

};

/// Allows writing to ostreams.
ostream& operator<<(ostream& stream, MakefileItem& item);

/// Allows adding strings an MakefileItems
string operator+(const string& str, MakefileItem& item);
/// Allows adding strings an MakefileItems
string operator+(MakefileItem& item, const string& str);

/**
 * This class is used to mark MakefileItems that can be used directly, like most can.
 * The only exception is currently VariableDef and VariableRef, they can only be used
 * together with their parent Variable.
 */
class PrimaryMakefileItem : public MakefileItem, public Manageable
{
public:
	PrimaryMakefileItem() {}
	virtual ~PrimaryMakefileItem() {}
};

}
#endif /* MAKEFILEITEM_H_ */
