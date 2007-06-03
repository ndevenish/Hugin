// -*- c-basic-offset: 4 -*-
/** @file PickerApp.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: CPEditorTest.h 99 2003-05-22 23:47:10Z dangelo $
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

#ifndef _PICKERAPP_H
#define _PICKERAPP_H

class CPImageCtrl;


/** brief description.
 *
 *  What this does
 */
class PickerApp : public wxApp
{
public:

    /** ctor.
     */
    PickerApp();

    /** dtor.
     */
    virtual ~PickerApp();


    virtual bool OnInit();

private:
    wxScrolledWindow * scrollview;
};

DECLARE_APP(PickerApp)


#endif // _PICKERAPP_H
