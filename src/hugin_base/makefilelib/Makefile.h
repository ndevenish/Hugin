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
#include <locale>

#include <hugin_shared.h>

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
class MAKEIMPEX Makefile
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
	 * Prepare output streams to get number output right with this locale.
	 * We use the "C" locale for NUMERIC and the system's for everything else.
	 * @note: C++ locales are different from C. Calling setlocale doesn't influence C++ streams!
	 */
	static const std::locale locale;

	/**
	 * Selects quoting modes.
	 */
	enum QuoteMode {MAKE, SHELL, NONE};
	static string quote(const string& in, Makefile::QuoteMode mode);
#ifdef WIN32
    /** special quoting for environemnt variables on windows */
    static string quoteEnvironment(const string& in);
#endif

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

MAKEIMPEX const std::locale GetMakefileLocale();

}

#endif /* MAKEFILE_H_ */
