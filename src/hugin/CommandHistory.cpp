// -*- c-basic-offset: 4 -*-

/** @file CommandHistory.cpp
 *
 *  @brief implementation of CommandHistory Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

// standard include
#include "panoinc_WX.h"
#include "panoinc.h"
#include "hugin/CommandHistory.h"

CommandHistory::CommandHistory()
    : nextCmd(0)
{
}

CommandHistory::~CommandHistory()
{
    std::vector<Command*>::iterator it;
    for (it = commands.begin(); it != commands.end(); ++it) {
        delete *it;
    }
}


void CommandHistory::clear()
{
    std::vector<Command*>::iterator it;
    for (it = commands.begin(); it != commands.end(); ++it) {
        delete *it;
    }
    commands.clear();
}




void CommandHistory::addCommand(Command *command, bool execute)
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
}



void CommandHistory::undo()
{
    if (nextCmd > 0) {
        // undo the current command
        DEBUG_DEBUG("undo: " << commands[nextCmd-1]->getName());
        commands[nextCmd-1]->undo();
        nextCmd--;
    } else {
        wxLogError("no command in undo history");
    }
}


void CommandHistory::redo()
{
    if (nextCmd < commands.size()) {
        DEBUG_DEBUG("redo: " << commands[nextCmd]->getName());
        commands[nextCmd]->execute();
        nextCmd++;
    } else {
        wxLogError("no command in redo history");
    }
}

// ======================================================================
// ======================================================================


GlobalCmdHist * GlobalCmdHist::instance = 0;

GlobalCmdHist::GlobalCmdHist()
{

}

GlobalCmdHist & GlobalCmdHist::getInstance()
{
    if (!instance) {
        instance = new GlobalCmdHist();
    }
    return *instance;
}
