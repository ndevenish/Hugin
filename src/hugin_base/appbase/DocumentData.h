// -*- c-basic-offset: 4 -*-
/** @file 
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
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
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this software; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.
*
*  Hereby the author, Ippei UKAI, grant the license of this particular file to
*  be relaxed to the GNU Lesser General Public License as published by the Free
*  Software Foundation; either version 2 of the License, or (at your option)
*  any later version. Please note however that when the file is linked to or
*  compiled with other files in this library, the GNU General Public License as
*  mentioned above is likely to restrict the terms of use further.
*
*/

#ifndef _APPBASE_DOCUMENTDATA_H
#define _APPBASE_DOCUMENTDATA_H

#include <string>
#include <hugin_shared.h>

namespace AppBase { 
    
/**
 *
 */
class IMPEX DocumentData
{
        
    public:
        ///
        virtual ~DocumentData() {};
        

    public:
        enum ReadWriteError { SUCCESSFUL=-1, UNKNOWN_ERROR, INCOMPATIBLE_TYPE, INVALID_DATA, PARCER_ERROR };
            
        virtual ReadWriteError readData(std::istream& dataInput, std::string documentType = "") =0;
        virtual ReadWriteError writeData(std::ostream& dataOutput, std::string documentType = "") =0;
        
        
    public:
        virtual bool isDirty() const
            { return m_dirty; }
        
        virtual void clearDirty()
            { setDirty(false); };
            
    protected:
        virtual void setDirty(const bool& dirty = true)
            { m_dirty = dirty; };
            
            
    private:
        bool m_dirty;
        
};


    
}; //namespace
#endif //_H
