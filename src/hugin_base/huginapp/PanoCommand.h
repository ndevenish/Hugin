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

#ifndef _HUGINAPP_PANOCOMMAND_H
#define _HUGINAPP_PANOCOMMAND_H

#include <appbase/Command.h>

#include <panodata/PanoramaData.h>


namespace HuginBase
{
    using namespace AppBase;

    /** Default panorama cmd, provides undo with mementos. 
     */
    template <typename StringType = std::string>
    class PanoCommand : public Command<StringType>
    {
    public:
        
        ///
        PanoCommand(ManagedPanoramaData& panoData)
            : pano(panoData)
        {};
        
        ///
        PanoCommand(ManagedPanoramaData& panoData, const StringType& commandName)
            : Command<StringType>(commandName), pano(panoData)
        {};
        
        ///
        virtual ~PanoCommand()
        {
            if(memento != NULL)
                delete memento;
            if(redoMemento != NULL)
                delete redoMemento;
        };
        
        
    protected:
        /// saves the state for undo
        virtual void saveMemento()
        {
            if(memento != NULL)
                delete memento;
            memento = pano.getNewMemento();
        };
        
        /// saves the state for redo
        virtual void saveRedoMemento()
        {
            if(redoMemento != NULL)
                delete redoMemento;
            redoMemento = pano.getNewMemento();
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
            
            Command<StringType>::setSuccessful(success);
            
            if(!success)
            {
                // [TODO] warning!
                pano.setMementoToCopyOf(memento);
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
            DEBUG_ASSERT(memento!=NULL);
            saveRedoMemento();
            pano.setMementoToCopyOf(memento);
            pano.changeFinished();
        }
        
        /** redoes from saved state
         *
         *  the derived class must call PanoComand::execute() or saveRedoMemento()
         *  in its execute() method to save the state.
         */
        virtual void redo()
        {
            DEBUG_ASSERT(redoMemento!=NULL);
            pano.setMementoToCopyOf(redoMemento);
            pano.changeFinished();
        }

        
    protected:
        ///
        ManagedPanoramaData& pano;
        
        
    protected:
        ///
        PanoramaDataMemento* memento;
        
        ///
        PanoramaDataMemento* redoMemento;
        
    };

    
} // namespace
#endif // _H