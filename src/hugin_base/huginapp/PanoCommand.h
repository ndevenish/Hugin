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

    /** Default panorama cmd, provides undo with mementos. 
     */
    template <typename StringType = std::string>
    class PanoCommand : public AppBase::Command<StringType>
    {
    public:
        
        ///
        PanoCommand(ManagedPanoramaData& panoData)
            : pano(panoData), memento(NULL), redoMemento(NULL), m_clearDirty(false)
        {};
        
        ///
        PanoCommand(ManagedPanoramaData& panoData, const StringType& commandName)
            : AppBase::Command<StringType>(commandName), pano(panoData), memento(NULL), redoMemento(NULL), m_clearDirty(false)
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
        virtual void execute()
        {
            saveMemento();
            
            bool success = processPanorama(pano);
            
            AppBase::Command<StringType>::setSuccessful(success);

            if(success)
            {
                // notify all observers about changes
                pano.changeFinished();
                if(m_clearDirty)
                {
                    pano.clearDirty();
                };
            }
            else
            {
                // [TODO] warning!
                pano.setMementoToCopyOf(memento);
            }
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
            if(redoMemento==NULL) {
                execute();
            } else {
                pano.setMementoToCopyOf(redoMemento);
                pano.changeFinished();
            }
        }

        
    protected:
        ///
        ManagedPanoramaData& pano;
        
        
    protected:
        ///
        PanoramaDataMemento* memento;
        
        ///
        PanoramaDataMemento* redoMemento;

        // if true, the dirty tag is cleared, otherwise it is keep
        bool m_clearDirty;
    };

} // namespace
#endif // _H
