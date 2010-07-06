/**
 * @file Variable.cpp
 * @brief
 *  Created on: May 25, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "Variable.h"
#include <boost/regex.hpp>
#include <stdexcept>


namespace makefile
{

/// @todo Checks nothing yet.
/**
 * According to Gnu Make Manual http://www.gnu.org/software/make/manual/html_node/Using-Variables.html#Using-Variables
 * it's recommended to use only alphanumerics and _ in Variable name.
 *
 * @return
 */
int Variable::checkStrings()
{
	static const boost::regex validname("\\w+");
	if( !boost::regex_match(name, validname))
		throw std::invalid_argument("Bad Variable name: " + name);


	return 0;
}
}
