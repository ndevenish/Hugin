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
 * @file Manageable.h
 * @brief
 *  Created on: Aug 6, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef MANAGEABLE_H_
#define MANAGEABLE_H_
namespace makefile
{
/**
 * Marks classes that the Manager should handle.
 */

class MAKEIMPEX Manageable
{
public:
	Manageable() {}
	virtual ~Manageable() {}
};
}
#endif /* MANAGEABLE_H_ */
