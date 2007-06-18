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
class ExternalProgram
{

    typedef std::string String;
    
protected:
    ///
    ExternalProgram() : o_exitCode(-1) {};
public:
    ///
    virtual ~ExternalProgram();
    
    
    // -- accessors --
    
public:
    ///
    virtual void setCommand(String command)
        { o_command = command; };
    
    ///
    virtual String getCommand()
        { return o_command; };
    
    ///
    virtual void setArguments(String arguments)
        { o_arguments = arguments; };    
    
    ///
    virtual String getArguments()
        { return o_arguments; };
    
    
    // -- argument utilities --
    
public:
    ///
    virtual void addArgument(String argument)
        { o_arguments.append(argument + " "); };
    
    ///
    virtual void addArgumentSafely(String argument)
        { addArgument(quoteArgument(argument)); };
    
    ///
    virtual void addFilenameSafely(String filename)
        { addArgument(quoteFilename(argument)); };
    
    ///
    virtual void setCommandSafely(String command)
        { setCommand(quoteFilename(argument)); };
    
protected:
    ///
    virtual String quoteArgument(String argument)
        { return utils::quoteString(argument) };
    
    ///
    virtual String quoteFilename(String filename)
        { return utils::quoteFilename(argument) };
    
    
    // -- executing --
    
public:
    ///
    virtual int getExitCode()
        { return o_exitCode; };
    
    ///
    virtual void setExitCode(int exitCode)
        { o_exitCode = exitCode; };
    
    
    // -- variables --
    
private:
    String o_command;
    Strint o_arguments;
    int o_exitCode;
    
};


/**
 *
 */
class ExternalProgramExecutor
{
    
public:
    ///
    virtual void ExternalProgramExecutor() =0;
    ///
    virtual ~ExternalProgramExecutor();
    
    ///
    enum ExecutionResult {INTERRUPTED = -1, NORMAL = 0, ERROR = 1};
    
    ///
    virtual ExecutionResult executeProgram(ExternalProgram* program) =0;
    
};



}