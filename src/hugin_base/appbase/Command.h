// -*- c-basic-offset: 4 -*-
/** @file Command.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _APPBASE_COMMAND_H
#define _APPBASE_COMMAND_H

#include <string>


namespace AppBase {
    

/** Base class for all panorama commands.
 *
 *  see command pattern.
*/
template <class StringType = std::string>
class Command
{
    
    public:
        //
        Command()
            : m_successful(false)
        {};
        
        ///
        Command(const StringType& commandName)
            : m_name(commandName), m_successful(false)
        {};

        ///
        virtual ~Command() {};

        
    public:
        /** execute the command. [pure virtual]
         *
         *  should save information for undo().
         */
        virtual void execute() = 0;
        
        /** undo execute() [pure virtual]
         *
         *  must restore the model to the state before execute().
         *  execute() may be called later to redo the undo.
         */
        virtual void undo() = 0;
        
        /** redo execute() [pure virtual]
         *
         *  for special optimisation; the default implementation calls execute();
         */
        virtual void redo()
            { execute(); };
        
        
    public:
        ///
        virtual StringType getName() const 
            { return m_name; }
        
        ///
        virtual void setName(const StringType& newName)
            { m_name = newName; }
        
        
    public:
        ///
        virtual bool wasSuccessful()
            { return m_successful;}
        
    protected:
        ///
        virtual void setSuccessful(bool success = true)
            { m_successful = success; }
        
        
    private:
        bool m_successful;
        StringType m_name;
};


} //namespace

typedef AppBase::Command<> Command;

#endif // _H
