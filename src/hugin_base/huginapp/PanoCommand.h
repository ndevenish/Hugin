// -*- c-basic-offset: 4 -*-
/** @file PanoCommand.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoCommand.h 1951 2007-04-15 20:54:49Z dangelo $
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

#ifndef _PANOCOMMAND_H
#define _PANOCOMMAND_H


#include <vector>

#include <common/Command.h>
#include <common/utils.h>
#include <common/stl_utils.h>

#include "PanoImage.h"
#include "Panorama.h"
#include "PanoToolsInterface.h"


namespace PT {

    /** Default panorama cmd, provides undo with mementos. 
     */
    template <StringType = std::string>
    class PanoCommand : public Command<StringType>
    {
    public:
        
        ///
        PanoCommand(ManagedPanoramaData& panoData)
            : pano(panoData)
        {};
        
        ///
        PanoCommand(ManagedPanoramaData& panoData, const StringType& commandName)
            : Command(commandName), pano(panoData)
        {};
        
        ///
        virtual ~PanoCommand()
        {};
        
        
    protected:
        /// saves the state for undo
        virtual void saveMemento()
        {
            memento = pano.getMemento();
        };
        
        /// saves the state for redo
        virtual void saveRedoMemento()
        {
            redoMemento = pano.getMemento();
        }
        
        
    public:
        /** Processes the panorama and saves the stateThe default implementation
         *   calls processPanorama() and saveMemento().
         *  Only override this method when you want to customize the undo
         *   behaviour. 
         */
        virtual bool execute()
        {
            saveMemento();
            
            bool success = processPanorama(pano);
            
            setSuccessful(success);
            
            if(!success)
            {
                // [TODO] warning!
                pano.setMemento(memento);
            }
            
            return success;
        };
        

        
    protected:
            
        /** Called by execute(). The default implementation does nothing and
         *   returns true.
         *  Should return false when the processing was unsuccessful.
         */
        virtual bool processPanorama(ManagedPanoramaData& panoramaData)
        {
            // [TODO] maybe a warning
            return true;
        }
        
    public:
        
        /** undoes from saved state
         *
         *  the derived class must call PanoComand::execute() or saveMemento()
         *  in its execute() method to save the state.
         */
        virtual void undo()
        {
            saveRedoMemento();
            pano.setMemento(memento);
            pano.changeFinished();
        }
        
        /** redoes from saved state
         *
         *  the derived class must call PanoComand::execute() or saveRedoMemento()
         *  in its execute() method to save the state.
         */
        virtual void redo()
        {
            pano.setMemento(redoMemento);
            pano.changeFinished();
        }

        
    protected:
        ///
        ManagedPanoramaData& pano;
        
        
    protected:
        ///
        PanoramaMemento memento;
        
        ///
        PanoramaMemento redoMemento;
        
    };

    
} // namespace

#endif // _H
