// -*- c-basic-offset: 4 -*-
/** @file History.h
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

#ifndef _COMMAND_H
#define _COMMAND_H

#include <vector>

#include <qstring.h>
#include <qobject.h>

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

    /** is used to provide names for the undo menu */
    virtual QString getName() const = 0;

private:

};


/** A history for Command, provides undo/redo functionality.
 *
 *  To use this, all modifications to the model have to be done
 *  through commands that are executed with addCommand();
 *
 *  I'd like to use KCommandHistory, wich automatically provides
 *  nice undo/redo buttons, but I want to be platform independant.
 */
class CommandHistory : public QObject
{
    Q_OBJECT
public:

    /** ctor.
     */
    CommandHistory();

    /** dtor.
     */
    virtual ~CommandHistory();

    /**
     * Erases all the undo/redo history.
     * Use this when reloading the data, for instance, since this invalidates
     * all the commands.
     */
    void clear();

    /**
     * Adds a command to the history. Call this for each @p command you create.
     * Unless you set @p execute to false, this will also execute the command.
     * This means, most of the application's code will look like
     *    MyCommand * cmd = new MyCommand(i18n("The name"), parameters);
     *    m_historyCommand.addCommand( cmd );
     */
    void addCommand(Command *command, bool execute=true);

public slots:

    /**
     * Undoes the last action.
     */
    virtual void undo();
    /**
     * Redoes the last undone action.
     */
    virtual void redo();

private:
    // our commands
    std::vector<Command*> commands;
    size_t nextCmd;

};


/** Singleton CommandHistory
 *
 *  for application that have only one subject that will receive
 *  commands
 */
class GlobalCmdHist : public CommandHistory
{
public:
    static GlobalCmdHist & getInstance();
protected:
    GlobalCmdHist();
private:
    static GlobalCmdHist * instance;
};

#endif // _COMMAND_H
