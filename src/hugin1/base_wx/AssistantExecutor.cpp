/**
* @file AssistantExecutor.cpp
* @brief implementation of CommandQueue for assistant
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
*  License along with this software; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#include "AssistantExecutor.h"

#include <wx/config.h>
#if wxCHECK_VERSION(3,0,0)
#include <wx/translation.h>
#else
#include <wx/intl.h>
#endif
#include "hugin_utils/utils.h"
#include "algorithms/optimizer/ImageGraph.h"
#include "base_wx/wxPlatform.h"
#include "hugin/config_defaults.h"


namespace HuginQueue
{
    CommandQueue* GetAssistantCommandQueue(const HuginBase::Panorama & pano, const wxString& ExePath, const wxString& project)
    {
        CommandQueue* commands=new CommandQueue();
        wxString quotedProject(wxEscapeFilename(project));

        //read main settings
        wxConfigBase* config = wxConfigBase::Get();
        const bool runCeleste = config->Read(wxT("/Celeste/Auto"), HUGIN_CELESTE_AUTO) != 0;
        double celesteThreshold;
        config->Read(wxT("/Celeste/Threshold"), &celesteThreshold, HUGIN_CELESTE_THRESHOLD);
        const bool celesteSmallRadius = config->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER) == 0;
        const bool runLinefind = config->Read(wxT("/Assistant/Linefind"), HUGIN_ASS_LINEFIND) != 0;
        const bool runCPClean = config->Read(wxT("/Assistant/AutoCPClean"), HUGIN_ASS_AUTO_CPCLEAN) != 0;
        double scale;
        config->Read(wxT("/Assistant/panoDownsizeFactor"), &scale, HUGIN_ASS_PANO_DOWNSIZE_FACTOR);

        bool runicp = (pano.getNrOfCtrlPoints() == 0);
        if (!runicp)
        {
            //we check, if all images are connected
            //if not, we run also icpfind
            HuginBase::CPGraph graph;
            createCPGraph(pano, graph);
            HuginBase::CPComponents comps;
            runicp = HuginBase::findCPComponents(graph, comps) > 1;
        };
        //build commandline for icpfind
        if (runicp)
        {
            //create cp find
            commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("icpfind")),
                wxT("-o ") + quotedProject + wxT(" ") + quotedProject, _("Searching for control points...")));
            //building celeste command
            if (runCeleste)
            {
                wxString args;
                args << wxT("-t ") << wxStringFromCDouble(celesteThreshold) << wxT(" ");
                if (celesteSmallRadius)
                {
                    args.Append(wxT("-r 1 "));
                }
                args.Append(wxT("-o ") + quotedProject + wxT(" ") + quotedProject);
                commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("celeste_standalone")),
                    args, _("Removing control points in clouds...")));
            };
            //building cpclean command
            if (runCPClean)
            {
                commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("cpclean")),
                    wxT("-o ") + quotedProject + wxT(" ") + quotedProject, _("Statistically cleaning of control points...")));
            };
        };
        //vertical line detector
        if (runLinefind)
        {
            const HuginBase::CPVector allCP = pano.getCtrlPoints();
            bool hasVerticalLines = false;
            if (allCP.size() > 0)
            {
                for (size_t i = 0; i < allCP.size() && !hasVerticalLines; i++)
                {
                    hasVerticalLines = (allCP[i].mode == HuginBase::ControlPoint::X);
                };
            };
            if (!hasVerticalLines)
            {
                commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("linefind")),
                    wxT("--output=") + quotedProject + wxT(" ") + quotedProject, _("Searching for vertical lines...")));
            };
        };
        //now optimise all
        commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("checkpto")), quotedProject));
        commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("autooptimiser")),
            wxT("-a -m -l -s -o ") + quotedProject + wxT(" ") + quotedProject, _("Optimizing...")));
        wxString panoModifyArgs;
        // if necessary scale down final pano
        if (scale <= 1.0)
        {
            panoModifyArgs << wxT("--canvas=") << hugin_utils::roundi(scale * 100) << wxT("% ");
        };
        panoModifyArgs.Append(wxT("--crop=AUTO --output=") + quotedProject + wxT(" ") + quotedProject);
        commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("pano_modify")),
            panoModifyArgs, _("Searching for best crop...")));
        return commands;
    };

}; // namespace