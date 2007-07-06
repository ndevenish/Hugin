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

#ifndef _APPBASE_EXTERNALPROGRAM_H
#define _APPBASE_EXTERNALPROGRAM_H

#include <string>


namespace AppBase {

    
///
class ArgumentQuotator
{    
    public:
        ///
        ArgumentQuotator() {};
        ///
        virtual ~ArgumentQuotator() {};
    
    public:
        ///
        virtual std::string quoteArgument(const std::string& argument) =0;
        
        ///
        virtual std::string quoteFilename(const std::string& filename) =0;
};

///
class StandardArgumentQuotator : public ArgumentQuotator
{
    public:
        ///
        StandardArgumentQuotator() {};
        ///
        virtual ~StandardArgumentQuotator() {};
        
    public:
        ///
        virtual std::string quoteArgument(const std::string& argument);
        
        ///
        virtual std::string quoteFilename(const std::string& filename);
};

    
/**
 *
 */
class ExternalProgram : public virtual ArgumentQuotator
{
    public:
        typedef std::string String;
    
    public:
        ///
        ExternalProgram()
         : m_exitCode(-1), m_quotator(m_defaultQuotator)
        {}
    
        ///
        ExternalProgram(ArgumentQuotator& quotator)
         : m_exitCode(-1), m_quotator(quotator)
        {}
    
        ///
        virtual ~ExternalProgram() {};
        
        
        // -- accessors --
        
    public:
        ///
        virtual void setCommand(String command)
            { m_command = command; };
        
        ///
        virtual String getCommand()
            { return m_command; };
        
        ///
        virtual void setArguments(String arguments)
            { m_arguments = arguments; };    
        
        ///
        virtual String getArguments()
            { return m_arguments; };
        
        
        // -- argument utilities --
        
    public:
        ///
        virtual void addArgument(String argument)
            { m_arguments.append(" " + argument); };
        
        ///
        virtual void addArgumentSafely(String argument)
            { addArgument(quoteArgument(argument)); };
        
        ///
        virtual void addFilenameSafely(String filename)
            { addArgument(quoteFilename(filename)); };
        
        ///
        virtual void setCommandSafely(String command)
            { setCommand(quoteFilename(command)); };
        
        
    public:
        ///
        ArgumentQuotator& getQuotator() const
            {return m_quotator;}
            
        ///
        virtual String quoteArgument(const String& argument)
            {return getQuotator().quoteArgument(argument);}
        
        ///
        virtual String quoteFilename(const String& filename)
            {return getQuotator().quoteFilename(filename);}
        
        
        // -- executing --
        
    public:
        ///
        virtual int getExitCode()
            { return m_exitCode; };
        
        ///
        virtual void setExitCode(int exitCode)
            { m_exitCode = exitCode; };
        
        
        // -- variables --        
    private:
        String m_command;
        String m_arguments;
        int m_exitCode;
        
        ArgumentQuotator& m_quotator;
        StandardArgumentQuotator m_defaultQuotator;
};



/**
 *
 */
class ExternalProgramExecutor
{
        
    public:
        ///
        virtual ~ExternalProgramExecutor() {};
        
        
    public:
        ///
        enum ExecutionResult {INTERRUPTED = -1, NORMAL = 0, ERROR = 1};
        
        ///
        virtual ExecutionResult executeProgram(ExternalProgram* program) =0;
        
};



} //namespace

#endif //_H