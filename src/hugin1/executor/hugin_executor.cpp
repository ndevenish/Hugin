// -*- c-basic-offset: 4 -*-

/** @file hugin_executor.cpp
 *
 *  @brief program for assistant and stitching execution
 *
 *
 *  @author T. Modes
 *
 */

/*  This program is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <hugin_config.h>

#include <fstream>
#include <sstream>

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/arrstr.h>
#include <wx/stdpaths.h>
#if defined _WIN32
// undefine DIFFERENCE defined in windows.h (included by wx/app.h)
#undef DIFFERENCE
#endif

#include "base_wx/huginConfig.h"
#include "base_wx/platform.h"
#include "panodata/Panorama.h"
#include "hugin/config_defaults.h"

#include "base_wx/Executor.h"
#include "base_wx/AssistantExecutor.h"
#include "base_wx/StitchingExecutor.h"

#ifdef __WXMAC__
// on wxMac wxAppConsole stores the settings in another folder, so we have to use wxApp to
// retrieve our settings which are stored by Hugin
// wxAppConsole is accessing preferences in $HOME/appname Preferences
// wxApp        is accessing preferences in  $HOME/Library/Preferences/appname Preferences
#define APP wxApp
#else
#define APP wxAppConsole
#endif

class HuginExecutor : public APP
{
    /** init translation settings */
    virtual bool OnInit()
    {
        wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        m_utilsBinDir=exePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
#if defined __WXMSW__
        // locale setup
        exePath.RemoveLastDir();
        m_locale.AddCatalogLookupPathPrefix(exePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + wxT("share\\locale"));
#elif defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
        // initialize paths
        {
            wxString thePath = MacGetPathToBundledResourceFile(CFSTR("locale"));
            if (thePath != wxT(""))
            {
                m_locale.AddCatalogLookupPathPrefix(thePath);
            }
            else
            {
                wxMessageBox(_("Translations not found in bundle"), _("Fatal Error"));
                return false;
            }
        };
#else
        // add the locale directory specified during configure
        m_locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
#endif
        // init our config settings
        wxConfig* config = new wxConfig(wxT("hugin"));
        wxConfigBase::Set(config);

        // initialize i18n
#if defined _MSC_VER && defined Hugin_shared
        std::locale::global(std::locale(""));
#endif
        int localeID = config->Read(wxT("language"), (long)HUGIN_LANGUAGE);
        m_locale.Init(localeID);
        // set the name of locale recource to look for
        m_locale.AddCatalog(wxT("hugin"));
        
        return APP::OnInit();
    };
    /** the main procedure of Executor app */
    virtual int OnRun()
    {
        HuginBase::Panorama pano;
        wxFileName inputFile(m_input);
        inputFile.Normalize();
        std::string input(inputFile.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
        std::ifstream prjfile(input.c_str());
        if (!prjfile.good())
        {
            std::cerr << "could not open script : " << input << std::endl;
            return -1;
        }
        pano.setFilePrefix(hugin_utils::getPathPrefix(input));
        AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
        if (err != AppBase::DocumentData::SUCCESSFUL)
        {
            std::cerr << "error while parsing panos tool script: " << input << std::endl;
            std::cerr << "DocumentData::ReadWriteError code: " << err << std::endl;
            return -1;
        }
        prjfile.close();

        HuginQueue::CommandQueue* commands;
        wxArrayString tempfiles;
        wxString oldCwd;
        if (m_runAssistant)
        {
            commands = HuginQueue::GetAssistantCommandQueue(pano, m_utilsBinDir, m_input);
        }
        else
        {
            wxFileName outputPrefix;
            if (m_prefix.IsEmpty())
            {
                outputPrefix = getDefaultOutputName(m_input, pano);
                outputPrefix.Normalize();
            }
            else
            {
                outputPrefix = m_prefix;
                outputPrefix.MakeAbsolute();
            };
            oldCwd = wxFileName::GetCwd();
            wxFileName::SetCwd(outputPrefix.GetPath());
            wxString statusText;
            wxArrayString outputFiles;
            if (m_userOutput.IsEmpty())
            {
                commands = HuginQueue::GetStitchingCommandQueue(pano, m_utilsBinDir, inputFile.GetFullPath(), outputPrefix.GetName(), statusText, outputFiles, tempfiles);
            }
            else
            {
                commands = HuginQueue::GetStitchingCommandQueueUserOutput(pano, m_utilsBinDir, inputFile.GetFullPath(), outputPrefix.GetName(), m_userOutput, statusText, outputFiles, tempfiles);
            };
            if (!m_dryRun)
            {
                std::cout << statusText.mb_str(wxConvLocal) << std::endl;
            }
        };

        if (commands->empty())
        {
            std::cout << "ERROR: Queue is empty." << std::endl;
            return 1;
        };

        if (m_threads == -1)
        {
            m_threads = wxConfigBase::Get()->Read(wxT("/output/NumberOfThreads"), 0l);
        };

        const bool success = HuginQueue::RunCommandsQueue(commands, m_threads, m_dryRun);
        if (!tempfiles.IsEmpty())
        {
            if (m_dryRun)
            {
#ifdef __WXMSW__
                std::cout << "del ";
#else
                std::cout << "rm ";
#endif
                std::cout << HuginQueue::GetQuotedFilenamesString(tempfiles).mb_str(wxConvLocal) << std::endl;
            }
            else
            {
                // short waiting time to write all files to disc
                wxMilliSleep(100);
                std::cout << _("Removing temporary files...") << std::endl;
                for (size_t i = 0; i < tempfiles.size(); ++i)
                {
                    wxRemoveFile(tempfiles[i]);
                };
            };
        };
        // restore current working dir
        if (!oldCwd.IsEmpty())
        {
            wxFileName::SetCwd(oldCwd);
        }
        return success ? 0 : 1;
    };

    /** set the parameters for the command line parser */
    virtual void OnInitCmdLine(wxCmdLineParser &parser)
    {
        parser.AddSwitch(wxT("h"), wxT("help"), _("shows this help message"), wxCMD_LINE_OPTION_HELP);
        parser.AddSwitch(wxT("a"), wxT("assistant"), _("execute assistant"));
        parser.AddSwitch(wxT("s"), wxT("stitching"), _("execute stitching with given project"));
        parser.AddOption(wxT("t"), wxT("threads"), _("number of used threads"), wxCMD_LINE_VAL_NUMBER);
        parser.AddOption(wxT("p"), wxT("prefix"), _("prefix used for stitching"), wxCMD_LINE_VAL_STRING);
        parser.AddOption(wxT("u"), wxT("user-defined-output"), _("use user defined commands in given file"), wxCMD_LINE_VAL_STRING);
        parser.AddSwitch(wxT("d"), wxT("dry-run"), _("only print commands"));
        parser.AddParam(wxT("input.pto"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY);
        m_runAssistant = false;
        m_runStitching = false;
        m_dryRun = false;
        m_threads = -1;
    }

    /** processes the command line parameters */
    virtual bool OnCmdLineParsed(wxCmdLineParser &parser)
    {
        // we don't call the parents method of OnCmdLineParse, this will pull in other options we don't want
        m_runAssistant = parser.Found(wxT("a"));
        m_runStitching = parser.Found(wxT("s"));
        m_dryRun = parser.Found(wxT("d"));
        long threads;
        if (parser.Found(wxT("t"), &threads))
        {
            m_threads = threads;
        };
        parser.Found(wxT("p"), &m_prefix);
        parser.Found(wxT("u"), &m_userOutput);
        if (!m_userOutput.IsEmpty())
        {
            wxFileName userOutputFile(m_userOutput);
            if (!userOutputFile.FileExists())
            {
                if (userOutputFile.GetDirCount() == 0)
                {
                    // file not found, search now in data dir
                    userOutputFile.SetPath(wxString(hugin_utils::GetDataDir().c_str(), HUGIN_CONV_FILENAME));
                    if (!userOutputFile.FileExists())
                    {
                        std::cerr << "ERROR: File \"" << m_userOutput.mb_str(wxConvLocal) << "\" does not exists." << std::endl
                            << "       Also tried file \"" << userOutputFile.GetFullPath().mb_str(wxConvLocal) << "\", which does also not exists." << std::endl;
                        return false;
                    }
                    m_userOutput = userOutputFile.GetFullPath();
                }
                else
                {
                    std::cerr << "ERROR: File \"" << m_userOutput.mb_str(wxConvLocal) << "\" does not exists." << std::endl;
                    return false;
                };
            };
        }
        m_input = parser.GetParam();
        if (!m_runAssistant && !m_runStitching)
        {
            std::cerr << "ERROR: Switch --assistant or --stitching is required." << std::endl;
            return false;
        };
        if (m_runAssistant == m_runStitching)
        {
            std::cerr << "ERROR: Switches --assistant and --stitching are mutually excluse." << std::endl;
            return false;
        }
        return true;
    };

private:
    /** flag, if assistant should started */
    bool m_runAssistant;
    /** flag, if stitching should started */
    bool m_runStitching;
    /** input file for userdefined output */
    wxString m_userOutput;
    /** flag, if commands should only be printed */
    bool m_dryRun;
    /** input project file */
    wxString m_input;
    /** stitching prefix */
    wxString m_prefix;
    /** number of threads used for assistant or stitching */
    long m_threads;
    /** path to utils */
    wxString m_utilsBinDir;
    /** locale for internationalisation */
    wxLocale m_locale;
};

DECLARE_APP(HuginExecutor)

IMPLEMENT_APP_CONSOLE(HuginExecutor)
