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
 * @file unicode.cpp
 * @brief Test program for locales and wide and narrow strings.
 *
 *  Created on: Jul 14, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include <cstring>
#include <iostream>
#include <string>
#include <fstream>
#include <locale>
#include <stdio.h>

/**
 * Tester that cleared up several things about wide and narrow character streams
 * and locales.
 *
 */
int main(int argc, char* argv[])
{
	std::locale de("de_DE.utf8");

	// the global locale is the default value for new streams and sets the C locale.
	std::cout << "C locale " << std::locale::global(std::locale("")).name() << std::endl;
	std::cout << "C locale " << std::locale::global(de).name() << std::endl;
	// This changes the C locale, but has no effect on the C++ streams.
	setlocale(LC_ALL, "en_US.utf8");

	std::cout << "C++ locale " << std::cout.getloc().name() << std::endl;
	std::cout.imbue(de);
	std::cout << "C++ locale " << std::cout.getloc().name() << std::endl;

	float comma = 15.45;

   const char text[] = "olé" ;
   const std::string strtext(text);
   std::ofstream file("/tmp/file");

   const wchar_t wtext[] = L"olé" ;
   const std::wstring wstrtext(wtext);
   std::wofstream wfile("/tmp/wfile", std::ios::binary);

   std::cout << "sizeof(char)    : " << sizeof(char) << std::endl ;
   std::cout << "text            : " << text << std::endl ;
   std::cout << "sizeof(text)    : " << sizeof(text) << std::endl ;
   std::cout << "strlen(text)    : " << strlen(text) << std::endl ;
   std::cout << "strtext.length(): " << strtext.length() << std::endl;

   std::cout << "text(binary)    :" ;

   for(size_t i = 0, iMax = strlen(text); i < iMax; ++i)
   {
      std::cout << " " << static_cast<unsigned int>(static_cast<unsigned char>(text[i])) ;
   }

   file << text;
   file << strtext;
   file << comma;

   std::cout << std::endl << comma << std::endl ; // shows the difference between C++ locale
   printf("%f\n", comma);	// and C locale

   std::cout << "sizeof(wchar_t) : " << sizeof(wchar_t) << std::endl ;
   std::cout << "wtext           : " << wtext << std::endl ;
//   std::cout << "wtext           : UNABLE TO CONVERT NATIVELY." << std::endl ;
   std::cout << "sizeof(wtext)   : " << sizeof(wtext) << std::endl ;
   std::cout << "wcslen(wtext)   : " << wcslen(wtext) << std::endl ;
   std::cout << "wstrtext.length():" << wstrtext.length() << std::endl;

   std::cout << "wtext(binary)   :" ;

   for(size_t i = 0, iMax = wcslen(wtext); i < iMax; ++i)
   {
      std::cout << " " << static_cast<unsigned int>(static_cast<unsigned short>(wtext[i])) ;
   }

   wfile << wtext;
   wfile << wstrtext;
   wfile << comma;

   std::cout << std::endl << std::endl ;

   return 0;
}
