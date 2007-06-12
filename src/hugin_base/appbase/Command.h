// -*- c-basic-offset: 4 -*-
/** @file History.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Command.h 164 2003-06-13 11:08:13Z dangelo $
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

#ifndef _COMMAND_H
#define _COMMAND_H

#include <string>


/** Base class for all panorama commands.
 *
 *  see command pattern.
 */
class Command
{
public:

    /** ctor.
     */
    Command() {};

    /** dtor.
         */
    virtual ~Command() {};

    /** execute the command.
     *
     *  should save information for undo().
     */
    virtual void execute() = 0;

    /** undo execute()
     *
     *  must restore the model to the state before execute().
     *  execute() may be called later to redo the undo.
     */
    virtual void undo() = 0;
    
    /** redo execute()
     *
     *  for special optimisation; the default implementation calls execute();
     */
    virtual void redo()
        { execute(); };
    
    /** provides names for mainly debugs etc.
     *  The default implementation returns some dummy string.
     */
    virtual std::string getNameStdString() const
        { return "(command name empty)" };

};


#endif // _COMMAND_H
