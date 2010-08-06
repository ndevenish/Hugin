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

class Manageable
{
public:
	Manageable() {}
	virtual ~Manageable() {}
};
}
#endif /* MANAGEABLE_H_ */
