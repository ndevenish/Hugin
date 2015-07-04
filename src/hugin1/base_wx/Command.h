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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _COMMAND_H
#define _COMMAND_H

#include <string>
#include <hugin_shared.h>
#include <panodata/Panorama.h>

namespace PanoCommand
{

/** Base class for all panorama commands.
 *
 *  see command pattern.
*/
class WXIMPEX PanoCommand
{
    public:
        /** constructor */
        explicit PanoCommand(HuginBase::Panorama& pano)
            : m_pano(pano), m_memento(NULL), m_redoMemento(NULL), m_clearDirty(false), m_successful(false)
        {};
        PanoCommand(HuginBase::Panorama& pano, const std::string& commandName)
            : m_pano(pano), m_memento(NULL), m_redoMemento(NULL), m_clearDirty(false), m_successful(false)
        {};

        /** destructor */
        virtual ~PanoCommand();

        /** execute the command. [virtual]
         *  Processes the panorama and saves the stateThe default implementation
         *   calls processPanorama() and saveMemento().
         *  Only override this method when you want to customize the undo
         *   behaviour.
         */
        virtual void execute();
        
        /** undo execute() [virtual]
         *
         *  the derived class must call PanoComand::execute() or saveMemento()
         *  in its execute() method to save the state.
         */
        virtual void undo();
        
        /** redo execute() [virtual]
         *
         *  the derived class must call PanoComand::execute() or saveRedoMemento()
         *  in its execute() method to save the state.
         */
        virtual void redo();
        /** returns the name of the command */
        virtual std::string getName() const;
        /** sets the name for the command */
        virtual void setName(const std::string& newName);
        ///
        virtual bool wasSuccessful();
        /** Called by execute(). The default implementation does nothing and
        *   returns true.
        *  Should return false when the processing was unsuccessful.
        */
        virtual bool processPanorama(HuginBase::Panorama& pano);

    protected:
        ///
        virtual void setSuccessful(bool success = true);
        
        /// saves the state for undo
        virtual void saveMemento();
        /// saves the state for redo
        virtual void saveRedoMemento();

        /// internal variables
        HuginBase::Panorama& m_pano;
        HuginBase::PanoramaDataMemento* m_memento;
        HuginBase::PanoramaDataMemento* m_redoMemento;

            // if true, the dirty tag is cleared, otherwise it is keep
        bool m_clearDirty;
    private:
        bool m_successful;
        std::string m_name;
};

} //namespace

#endif // _H
