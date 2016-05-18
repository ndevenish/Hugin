/**
* @file Executor.cpp
* @brief basic classes and function for queuing commands in wxWidgets
*
* @author T. Modes
*/

/*  This is free software; you can redistribute it and/or
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
*  License along with this software. If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "Executor.h"
#include "config.h"

#include <iostream>
#include <wx/utils.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/log.h>
#if wxCHECK_VERSION(3,0,0)
#include <wx/translation.h>
#else
#include <wx/intl.h>
#endif
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
#include "base_wx/platform.h"
#endif
#include "base_wx/wxPlatform.h"

namespace HuginQueue
{
    // build final command and execute it
    bool NormalCommand::Execute(bool dryRun)
    {
        if (dryRun)
        {
            std::cout << GetCommand().mb_str(wxConvLocal) << std::endl;
            return true;
        }
        if (!m_comment.IsEmpty())
        {
            std::cout << std::endl << m_comment.mb_str(wxConvLocal) << std::endl;
        };
        return wxExecute(GetCommand(), wxEXEC_SYNC | wxEXEC_MAKE_GROUP_LEADER) == 0l;
    };

    bool NormalCommand::CheckReturnCode() const
    {
        return true;
    };

    wxString NormalCommand::GetCommand() const
    {
        return wxEscapeFilename(m_prog) + wxT(" ") + m_args;
    };

    wxString NormalCommand::GetComment() const
    {
        return m_comment;
    };

    // optional command, returns always true, even if process failed
    bool OptionalCommand::Execute(bool dryRun)
    {
        NormalCommand::Execute(dryRun);
        return true;
    };

    bool OptionalCommand::CheckReturnCode() const
    {
        return false;
    };

    // execute the command queue
    bool RunCommandsQueue(CommandQueue* queue, size_t threads, bool dryRun)
    {
        // set OMP_NUM_THREADS to limit number of threads in OpenMP programs
        if (threads > 0)
        {
            wxString s;
            s << threads;
            wxSetEnv(wxT("OMP_NUM_THREADS"), s);
        };
        // set temp dir
        wxString tempDir = wxConfig::Get()->Read(wxT("tempDir"), wxT(""));
        if (!tempDir.IsEmpty())
        {
#ifdef UNIX_LIKE
            wxSetEnv(wxT("TMPDIR"), tempDir);
#else
            wxSetEnv(wxT("TMP"), tempDir);
#endif
        };
        bool isSuccessful = true;
        size_t i = 0;
        // prevent displaying message box if wxExecute failed
        wxLogStream log(&std::cerr);
        // final execute the commands
        while (isSuccessful && i < queue->size())
        {
            isSuccessful = (*queue)[i]->Execute(dryRun);
            ++i;
        };
        // clean up queue
        CleanQueue(queue);
        delete queue;
        return isSuccessful;
    };

    void CleanQueue(CommandQueue* queue)
    {
        while (!queue->empty())
        {
            delete queue->back();
            queue->pop_back();
        };
    };


    // return path in internal program (program that is shipped with Hugin)
    wxString GetInternalProgram(const wxString& bindir, const wxString& name)
    {
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
        CFStringRef filename = MacCreateCFStringWithWxString(name);
        wxString fn = MacGetPathToBundledExecutableFile(filename);
        CFRelease(filename);
        if (fn == wxT(""))
        {
            std::cerr << wxString::Format(_("External program %s not found in the bundle, reverting to system path"), name.c_str()) << std::endl;
            return name;
        }
        return fn;
#else
        return bindir + name;
#endif
    };

    // return name of external program (can be program bundeled with Hugin, or an external program 
    // as specified in preferences
    wxString GetExternalProgram(wxConfigBase * config, const wxString& bindir, const wxString& name)
    {
#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
        if (config->Read(name + wxT("/Custom"), 0l))
        {
            wxString fn = config->Read(name + wxT("/Exe"), wxT(""));
            if (wxFileName::FileExists(fn))
            {
                return fn;
            }
            else
            {
                std::cerr << wxString::Format(_("WARNING: External program %s not found as specified in preferences, reverting to bundled version"), fn.c_str()) << std::endl;
            };
        };
        if (name == wxT("exiftool"))
        {
            wxString exiftoolDirPath = MacGetPathToBundledResourceFile(CFSTR("ExifTool"));
            if (exiftoolDirPath != wxT(""))
            {
                return exiftoolDirPath + wxT("/exiftool");
            }
            else
            {
                std::cerr << wxString::Format(_("WARNING: External program %s not found in the bundle, reverting to system path"), name.c_str()) << std::endl;
                return wxT("exiftool");
            };
        };

        CFStringRef filename = MacCreateCFStringWithWxString(name);
        wxString fn = MacGetPathToBundledExecutableFile(filename);
        CFRelease(filename);
        if (fn == wxT(""))
        {
            std::cerr << wxString::Format(_("WARNING: External program %s not found in the bundle, reverting to system path"), name.c_str()) << std::endl;
            return name;
        };
        return fn;
#else
        if (config->Read(name + wxT("/Custom"), 0l))
        {
            wxString fn = config->Read(name + wxT("/Exe"), wxT(""));
            wxFileName prog(fn);
            if (prog.IsAbsolute())
            {
                if (prog.FileExists())
                {
                    return fn;
                };
            }
            else
            {
                // search in PATH
                wxPathList pathlist;
                pathlist.Add(bindir);
                pathlist.AddEnvList(wxT("PATH"));
                fn = pathlist.FindAbsoluteValidPath(fn);
                if (!fn.IsEmpty())
                {
                    return fn;
                };
            };
            std::cerr << wxString::Format(_("WARNING: External program %s not found as specified in preferences, reverting to bundled version"), name.c_str()) << std::endl;
        };
        // no user specified program or not found
#ifdef __WXMSW__
        // on Windows assume prog is bundled in program dir
        return bindir + name;
#else
        // on Unix simply return name, assume it is in the path
        return name;
#endif
#endif
    };

    wxString wxStringFromCDouble(double val, int precision)
    {
        wxString s = hugin_utils::doubleTowxString(val, precision);
        const wxString sep = wxLocale::GetInfo(wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER);
        s.Replace(sep, wxT("."));
        return s;
    };

}; // namespace
