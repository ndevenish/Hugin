/**
 * @file MakefileItem.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef MAKEFILEITEM_H_
#define MAKEFILEITEM_H_

#include "Makefile.h"

#include <ostream>
#include <string>

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
		Makefile::getSingleton().add(this);
	}

	virtual ~MakefileItem()
	{
		Makefile::getSingleton().remove(this);
	}

	/// @return A string representation of the MakefileItem.
	virtual std::string toString()=0;
	/// Write the items representation to an ostream in a makefile compatible way.
	void print(std::ostream& os)
	{
		os << toString();
	}
	/// Allow casts to string, very nice!
	operator std::string()
	{
		return toString();
	}

};

std::ostream& operator<<(std::ostream& stream, MakefileItem& item);

}

#endif /* MAKEFILEITEM_H_ */
