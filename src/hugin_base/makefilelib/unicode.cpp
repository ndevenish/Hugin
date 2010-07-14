/**
 * @file unicode.cpp
 * @brief
 *  Created on: Jul 14, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
   const char text[] = "olé" ;
   const std::string strtext(text);
   const wchar_t wtext[] = L"olé" ;
   const std::wstring wstrtext(wtext);

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

   std::cout << std::endl << std::endl ;

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

   std::cout << std::endl << std::endl ;


   return 0;
}
