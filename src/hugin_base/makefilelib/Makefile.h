/**
 * @file Makefile.h
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef MAKEFILE_H_
#define MAKEFILE_H_

#include <string>
#include <vector>
#include <iostream>

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
	std::vector<MakefileItem*> items;
public:
	Makefile();
	virtual ~Makefile();

	enum QuoteMode {MAKE, SHELL};
	static std::string quote(const std::string& in, Makefile::QuoteMode mode);

	void add(MakefileItem& item)
	{
		items.push_back(&item);
	}
	void add(MakefileItem* item)
	{
		items.push_back(item);
	}

	int writeMakefile(std::ostream& out);
};

}

#endif /* MAKEFILE_H_ */
