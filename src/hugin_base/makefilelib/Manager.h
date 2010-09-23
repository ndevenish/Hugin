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
 * @file Manager.h
 * @brief
 *  Created on: Aug 6, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef MANAGER_H_
#define MANAGER_H_
#ifdef __APPLE__
//for MacOX the macro "check" gets somewhere defined
//this will collide with some boost libs, to prevent conflicts
//we undef the macro
#undef check
#endif
#include <boost/ptr_container/ptr_vector.hpp>

#include "MakefileItem.h"
#include "Manageable.h"

namespace makefile
{


/**
 * Provides object management features for easy using of the lib.
 * It can work as an owner Manageable instances,
 * and is therefore responsible for their destruction afterwards.
 * The major advantage over holding MakefileItem s in auto-variables is that
 * they get out of scope easily.
 * before Makefile::writeMakefile and cause problems.
 * This class is independent from Makefile.
 * The usage:
 * - Create a Manager, and call Manager::own with all new Items.
 * - Finally after calling Makefile::writeMakefile, destroy the Manager and it
 *   will clean up all created Items.
 *
 * @note This can only work with heap allocated objects (new!).
 */

class MAKEIMPEX Manager
{
	// boost::ptr_vector is perfect here, and it handles deletion.
	boost::ptr_vector<Manageable> mitems;

	Manager(const Manager&); 	// cannot copy this object!
public:
	Manager();
	virtual ~Manager() {}

	/**
	 * Takes over ownership of a Manageable allocated with new. On destruction
	 * of the Manager item is deleted too.
	 * @param item
	 * @return item
	 */
	template<class I>
	I* own(I* item)
	{
		mitems.push_back(item);
		return item;
	}
	/**
	 * A shortcut. Calls Manager::own and Makefile::add for objects we don't need to deal
	 * with any further.
	 * @param item
	 */
	template<class MI>
	MI* own_add(MI* item)
	{
		own(item);
		item->add();
		return item;
	}

};

}

#endif /* MANAGER_H_ */
