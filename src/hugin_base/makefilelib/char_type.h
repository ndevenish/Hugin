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
 * @file char_type.h
 * @brief This file contains several typedefs similar to those of the
 * standard headers and allows easy switching between char and wchar_t usage
 * if only the types defined here are used in the hole library.
 *
 * This pulls the type names into the namespace makefile. To use those defined
 * here make sure to use this namespace.
 * It also pulls in all the include files, if maybe only one of them is needed.
 * This will increase compile time a little more than necessary but keeps things
 * simple.
 *
 *  Created on: Jul 14, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef CHARTYPE_H_
#define CHARTYPE_H_

#include <string>
#include <boost/regex.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>

//#define USE_WCHAR

namespace makefile
{
#ifdef USE_WCHAR
	typedef wchar_t char_type;
	typedef unsigned wchar_t uchar_type;
#else
	typedef char char_type;
	typedef unsigned char uchar_type;
#endif

/// streams, use the extended fstream versions from boost::filesystem.
typedef std::basic_filebuf<char_type> filebuf;
typedef boost::filesystem::basic_ifstream<char_type> ifstream;
typedef std::basic_istream<char_type> istream;
typedef boost::filesystem::basic_fstream<char_type> fstream;
typedef boost::filesystem::basic_ofstream<char_type> ofstream;
typedef std::basic_ostream<char_type> ostream;

/// string
typedef std::basic_string<char_type>    string;

#ifdef USE_WCHAR
	/// paths from boost::filesystem
	typedef boost::filesystem::wpath path;
	/// boost::regex
	typedef boost::wregex regex;
	/// Prepend literal strings with L for wchar use.
	#define cstr(x) L ## x
#else
	/// paths from boost::filesystem
	typedef boost::filesystem::path path;
	/// boost::regex
	typedef boost::regex regex;
	/// Let literal strings unchanged for char use.
	#define cstr(x) x
#endif

}

#endif /* CHARTYPE_H_ */
