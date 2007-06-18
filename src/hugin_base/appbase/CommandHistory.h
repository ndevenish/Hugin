// -*- c-basic-offset: 4 -*-
/** @file CommandHistory.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: CommandHistory.h 606 2004-06-13 13:48:51Z dangelo $
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

#ifndef _COMMANDHISTORY_H
#define _COMMANDHISTORY_H

class Command;

/** A history for Command, provides undo/redo functionality.
 *
 *  To use this, all modifications to the model have to be done
 *  through commands that are executed with addCommand();
 */
template <CommandClass = Command<std::string>>
class CommandHistory
{
public:

    /** ctor.
     */
    CommandHistory()
        : nextCmd(0)
    {};

    /** dtor.
     */
    virtual ~CommandHistory()
    {
        std::vector<CommandClass*>::iterator it;
        for (it = commands.begin(); it != commands.end(); ++it) {
            delete *it;
        }
    };

    /**
     * Erases all the undo/redo history.
     * Use this when reloading the data, for instance, since this invalidates
     * all the commands.
     */
    void clear()
    {
        std::vector<CommandClass*>::iterator it;
        for (it = commands.begin(); it != commands.end(); ++it) {
            delete *it;
        }
        commands.clear();
    };

    /**
     * Adds a command to the history. Call this for each @p command you create.
     * Unless you set @p execute to false, this will also execute the command.
     * This means, most of the application's code will look like
     * \code
     *    MyCommand * cmd = new MyCommand(...);
     *    m_historyCommand.addCommand( cmd );
     * \endcode
     *
     * Ownership of @p command is transfered to CommandHistory
     */
    void addCommand(CommandClass* command, bool execute=true)
    {
        assert(command);
        
        if (execute) {
            // execute command
            command->execute();
        }
        
        if (nextCmd > commands.size()) {
            DEBUG_FATAL("Invalid state in Command History: nextCmd:" << nextCmd
                        << " size:" << commands.size());
        } else if (nextCmd < (commands.size())) {
            // case: there were redoable commands, remove them now, the
            // current command has invalidated them.
            size_t nrDelete = commands.size()  - nextCmd;
            for (size_t i=0; i < nrDelete; i++) {
                delete commands.back();
                commands.pop_back();
            }
        }
        commands.push_back(command);
        nextCmd++;
    };

    /**
     * Undoes the last action.
     */
    virtual void undo()
    {
        if (canUndo()) {
            // undo the current command
            DEBUG_DEBUG("undo: " << commands[nextCmd-1]->getName());
            commands[nextCmd-1]->undo();
            nextCmd--;
        } else {
            // [TODO] exception
        }
    };
    
    /**
     * Redoes the last undone action.
     */
    virtual void redo()
    {
        if (canRedo()) {
            DEBUG_DEBUG("redo: " << commands[nextCmd]->getName());
            commands[nextCmd]->execute();
            nextCmd++;
        } else {
            // [TODO] exception
        }
    };
    
    ///
    virtual bool canUndo()
        { return nextCmd > 0; };
    
    ///
    virtual bool canRedo()
        { return nextCmd < commands.size(); };
    

private:
    // our commands
    std::vector<CommandClass*> commands;
    size_t nextCmd;

};

#endif // _COMMANDHISTORY_H
