/**
 * @file Makefile.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef MAKEFILE_H_
#define MAKEFILE_H_

#include "char_type.h"
#include <vector>
#include <iostream>
#include <stdexcept>

/**
 *
 */
namespace makefile
{
class MakefileItem;
/**
 * Container and Manager for all our \ref MakefileItem "MakefileItems".
 * It also contains some static utils and enums.
 */
class Makefile
{
	/// Holds pointers to every existing MakefileItem.
	std::vector<MakefileItem*> items;
	/// has the makefile been written?
	bool written;
	Makefile() : written(false) {}
	static Makefile* instance;
public:
	virtual ~Makefile() {}
	static Makefile& getSingleton();
	static void clean();

	/**
	 * Selects quoting modes.
	 */
	enum QuoteMode {MAKE, SHELL, NONE};
	static string quote(const string& in, Makefile::QuoteMode mode);


	/**
	 * Adds a MakefileItem to a list of existing Items.
	 * This adds a MakefileItem to the output order. Only items added are
	 * output.
	 * @param item pointer
	 */
	void add(MakefileItem* item)
	{
		items.push_back(item);
	}

	/**
	 * Removes a MakefileItem to a list of existing Items.
	 * MakefileItem::~MakefileItem removes itself using this.
	 * We have to make sure that we don't hold pointers to non-existing
	 * MakefileItems.
	 * @note The easiest way is to wipe the whole Makefile singleton, so
	 * if one MakefileItem is destructed, we loose the pointers to all the others
	 * too. This is ok if removing an item while needing the rest is not a use case.
	 * And if memory handling is not done by this class.
	 * @todo Should it be necessary, implement this remove method to not throw everyting away.
	 * @param item pointer
	 */
	static void remove(MakefileItem* item);

	/**
	 * Outputs all known MakefileItem to an ostream.
	 * @param out ostream to write the Makefile to.
	 * @return
	 */
	int writeMakefile(ostream& out);
};

}

#endif /* MAKEFILE_H_ */
