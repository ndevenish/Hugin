// -*- c-basic-offset: 4 -*-
/** @file huginapp/PanoCommand.h
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Command.h"

namespace PanoCommand
{

PanoCommand::~PanoCommand()
{
    if (m_memento != NULL)
    {
        delete m_memento;
    };
    if (m_redoMemento != NULL)
    {
        delete m_redoMemento;
    };
}

void PanoCommand::execute()
{
    saveMemento();
    bool success = processPanorama(m_pano);
    setSuccessful(success);
    if (success)
    {
        // notify all observers about changes
        m_pano.changeFinished();
        if (m_clearDirty)
        {
            m_pano.clearDirty();
        };
    }
    else
    {
        // [TODO] warning!
        m_pano.setMementoToCopyOf(m_memento);
    };
}

void PanoCommand::undo()
{
    DEBUG_ASSERT(m_memento != NULL);
    saveRedoMemento();
    m_pano.setMementoToCopyOf(m_memento);
    m_pano.changeFinished();
}

void PanoCommand::redo()
{
    if (m_redoMemento == NULL)
    {
        execute();
    }
    else
    {
        m_pano.setMementoToCopyOf(m_redoMemento);
        m_pano.changeFinished();
    };
}

std::string PanoCommand::getName() const
{
    return m_name;
}

void PanoCommand::setName(const std::string& newName)
{
    m_name = newName;
}

bool PanoCommand::wasSuccessful()
{
    return m_successful;
}

void PanoCommand::setSuccessful(bool success)
{
    m_successful = success;
}

void PanoCommand::saveMemento()
{
    if (m_memento != NULL)
    {
        delete m_memento;
    };
    m_memento = m_pano.getNewMemento();
};

void PanoCommand::saveRedoMemento()
{
    if (m_redoMemento != NULL)
    {
        delete m_redoMemento;
    };
    m_redoMemento = m_pano.getNewMemento();
}

bool PanoCommand::processPanorama(HuginBase::Panorama& panoramaData)
{
    return true;
}

} // namespace
