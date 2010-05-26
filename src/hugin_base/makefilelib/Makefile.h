/**
 * @file Makefile.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef MAKEFILE_H_
#define MAKEFILE_H_

/**
 *
 */
namespace makefile
{

/**
 * Container and Manager for all our \ref MakefileItem "MakefileItems".
 */
class Makefile
{
public:
	Makefile();
	virtual ~Makefile();
};

}

#endif /* MAKEFILE_H_ */
