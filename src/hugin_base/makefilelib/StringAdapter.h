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
 * @file StringAdapter.h
 * @brief
 *  Created on: Jul 23, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef STRINGADAPTER_H_
#define STRINGADAPTER_H_

#include <string>
#include <sstream>


namespace makefile
{
/**
 * Adapts a string of wide or narrow characters to a narrow character std::string.
 * It uses the narrow method of standard iostreams, it doesn't do any code conversion.
 * It's purpose is to allow exception texts to contain wide strings, regardless of
 * USE_WCHAR.
 */
class StringAdapter : public std::string
{
	void use_narrow(const wchar_t* ws)
	{
		std::ostringstream ostr;
		for(const wchar_t* i = ws; *i; i++)
		{
			ostr.put(ostr.narrow(*i, '?'));
		}
		assign(ostr.str());
	}
public:
	StringAdapter(std::wstring& ws)
	{
		use_narrow(ws.c_str());
	}
	StringAdapter(const std::string& s)
	: std::string(s)
	{}
	StringAdapter(const wchar_t* ws)
	{
		use_narrow(ws);
	}
	StringAdapter(const char* s)
	: std::string(s)
	{}
	virtual ~StringAdapter()
	{
	}
};

}

#endif /* STRINGADAPTER_H_ */
