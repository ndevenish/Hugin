/**
 * @file MakefileItem.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef MAKEFILEITEM_H_
#define MAKEFILEITEM_H_

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
	MakefileItem() {}
	virtual ~MakefileItem() {}

	/// Write the items representation to an ostream in a makefile compatible way.
	virtual void print(std::ostream& os)=0;
	/// @return A string representation of the MakefileItem.
	virtual std::string toString()=0;

};

std::ostream& operator<<(std::ostream& stream, MakefileItem& item);

}

#endif /* MAKEFILEITEM_H_ */
