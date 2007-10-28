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

#ifndef _APPBASE_EXTERNALPROGRAMSETUP_H
#define _APPBASE_EXTERNALPROGRAMSETUP_H


#include <set>
#include <string>

#include <appbase/ExternalProgram.h>


namespace AppBase {

///
class ExternalProgramSetup : public virtual ArgumentQuotator
{
    public:
        typedef std::string String;
        typedef std::set<std::string> StringSet;
            
        
    public:
        ///
        ExternalProgramSetup()
            : m_quotator(m_defaultQuotator)
        {}
        
        ///
        ExternalProgramSetup(ArgumentQuotator& quotator)
            : m_quotator(quotator)
        {}
        
        ///
        virtual ~ExternalProgramSetup() {};
        
        
    public:
        ///
        virtual String defaultCommand() const =0;
        
        ///
        virtual String defaultArgumentTemplate() const =0;
        
        ///
        virtual void setCommand(const String& command)
        {
            m_command = command;
            m_defaultCommand = false;
        };
        
        ///
        virtual String getCommand() const
        {
            return (m_defaultCommand)? defaultCommand() : m_command;
        };
        
        ///
        virtual void setArgumentTemplate(const String& argumentTemplate)
        {
            m_argumentTemplate = argumentTemplate;
            m_defaultArg = false;
        };
        
        ///
        virtual String getArgumentTemplate() const
        {
            return (m_defaultArg)? defaultArgumentTemplate() : m_argumentTemplate;
        };
        
        
    public:
        ///
        virtual StringSet getAvailableStringKeywords() const =0;
        
        ///
        virtual String getStringKeywordPrefix() const
        { return "{"; };
        
        ///
        virtual String getStringKeywordSuffix() const
        { return "}"; };

    protected:
        ///
        virtual String getStringForKeyword(const String& keyword) =0;
        

    public:
        ///
        virtual bool setupExternalProgram(ExternalProgram* externalProgram);
        
        ///
        virtual String parseArgumentsFromTemplate(const String& argumentTemplate);
        
        ///
        virtual String parseArgumentsFromTemplate(const String& argumentTemplate,
                                                   ArgumentQuotator& quotator);
        
    protected:
        ///
        virtual String quoteArgument(const String& argument)
            {return m_quotator.quoteArgument(argument);}
        
        ///
        virtual String quoteFilename(const String& filename)
            {return m_quotator.quoteFilename(filename);}
        
        
    private:
        String m_command;
        String m_argumentTemplate;
        bool m_defaultCommand;
        bool m_defaultArg;
        
        ArgumentQuotator& m_quotator;
        StandardArgumentQuotator m_defaultQuotator;
};


} //namespace
#endif //_H
