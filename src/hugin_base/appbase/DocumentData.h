// -*- c-basic-offset: 4 -*-
/** @file 
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
*
*  $Id: $
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


namespace AppBase {

    
/**
 *
 */
class DocumentData
{
    
public:
    ///
    virtual void DocumentData() =0;
    virtual ~DocumentData();
    
    
public:
    virtual bool readData(std::istream dataInput, std::string documentType = "");
    virtual bool writeData(std::ostream dataOutput, std::string documentType = "");
                           
    virtual bool readDataOfType(std::istream dataInput, std::string documentType);
    virtual bool writeDataToType(std::ostream dataOutput, std::string documentType);
    
    
public:
    virtual bool isDirty()
        { return m_dirty; }
    virtual void clearDirty();
        { setDirty(false); };
protected:
    virtual void setDirty(bool dirty = true)
        { m_dirty = dirty; };
private:
    bool m_dirty;
    
};


    
};