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

    /** default panorama cmd, provides undo with mementos
     */
    class PanoCommand : public Command
    {
    public:
        PanoCommand(Panorama & p)
            : pano(p)
            { };

        virtual ~PanoCommand()
            {
            };

        /** save the state */
        virtual void execute()
            {
                memento = pano.getMemento();
            };
        /** set the saved state.
         *
         *  the derived class must call PanoComand::execute() in its
         *  execute() method to save the state.
         */
        virtual void undo()
            {
                pano.setMemento(memento);
                pano.changeFinished();
            }
        virtual std::string getName() const = 0;
        
    protected:
        Panorama & pano;
        PanoramaMemento memento;
    };

    
} // namespace PT

#endif // _PANOCOMMAND_H
