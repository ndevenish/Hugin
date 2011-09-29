// -*- c-basic-offset: 4 -*-

/** @file PTBatcher.h
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: PTBatcher.h 3322 2008-08-16 5:00:07Z mkuder $
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

#include "Batch.h"

//Host application needed to use wxWidgets frame objects in console
class HostApp : public wxApp
{
public:
    //Pseudo constructor
    virtual bool OnInit();

    //Initializes batch object
    void InitBatch(wxString path)
    {
        batch = new Batch(NULL,path,false);
    };
    Batch* batch;
};

IMPLEMENT_APP(HostApp)
