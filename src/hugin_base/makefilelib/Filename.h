/**
 * @file Filename.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef FILENAME_H_
#define FILENAME_H_

/**
 *
 */
#include "MakefileItem.h"

namespace makefile
{

/**
 * Encapsulates a filename handling platform specific issues like allowed characters.
 */
class Filename: public makefile::MakefileItem
{
public:
	Filename();
	virtual ~Filename();

	virtual void print(std::ostream& os);
};

}

#endif /* FILENAME_H_ */
