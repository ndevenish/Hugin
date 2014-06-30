// -*- c-basic-offset: 4 -*-

/** @file icpfind.cpp
 *
 *  @brief program to heuristic detection of control points in panoramas
 *  
 *  @author Thomas Modes
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <hugin_version.h>
#include "icpfind.h"

#include <fstream>
#include <sstream>

#include "AutoCtrlPointCreator.h"
#include <panodata/Panorama.h>
#include <base_wx/platform.h>
#include "hugin/config_defaults.h"
#include <vigra/impex.hxx>

using namespace std;
 
void iCPApp::ReadDetectorConfig()
{
    wxConfig config(wxT("hugin"));
    //read cp detectors settings
    CPDetectorConfig cpdetector_config;
    cpdetector_config.Read(&config);
    //write current cp detectors settings
    cpdetector_config.Write(&config);
    config.Flush();

    if(m_setting<0 || m_setting>=cpdetector_config.settings.size())
    {
        m_cpsetting=cpdetector_config.settings[cpdetector_config.GetDefaultGenerator()];
    }
    else
    {
        m_cpsetting=cpdetector_config.settings[m_setting];
    };

    if(m_matches==-1)
    {
        m_matches=config.Read(wxT("/Assistant/nControlPoints"), HUGIN_ASS_NCONTROLPOINTS);
    };
};

void iCPApp::OnInitCmdLine(wxCmdLineParser &parser)
{
    parser.AddSwitch(wxT("h"),wxT("help"),wxT("shows this help message"));
    parser.AddOption(wxT("s"),wxT("setting"),wxT("used setting"),wxCMD_LINE_VAL_NUMBER);
    parser.AddOption(wxT("m"),wxT("matches"),wxT("number of matches"),wxCMD_LINE_VAL_NUMBER);
    parser.AddOption(wxT("o"),wxT("output"),wxT("output project"),wxCMD_LINE_VAL_STRING,wxCMD_LINE_OPTION_MANDATORY);
    parser.AddParam(wxT("input.pto"),wxCMD_LINE_VAL_STRING,wxCMD_LINE_OPTION_MANDATORY);
};

bool iCPApp::OnCmdLineParsed(wxCmdLineParser &parser)
{
    // we don't call the parents method of OnCmdLineParse, this will pull in other options we don't want
    if(!parser.Found(wxT("s"),&m_setting))
    {
        m_setting=-1;
    };
    if(!parser.Found(wxT("m"),&m_matches))
    {
        m_matches=-1;
    };
    parser.Found(wxT("o"),&m_output);
    m_input=parser.GetParam();
    return true;
};

// dummy panotools progress functions
static int ptProgress( int command, char* argument )
{
	return 1;
}
static int ptinfoDlg( int command, char* argument )
{
	return 1;
}

int iCPApp::OnRun()
{
    ReadDetectorConfig();
    //read input project
    PT::PanoramaMemento newPano;
    int ptoVersion = 0;
    wxFileName file(m_input);
    file.MakeAbsolute();
    std::ifstream in((const char *)file.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
    if(!in.good())
    {
        cerr << "could not open script : " << file.GetFullPath().char_str() << endl;
        return 1;
    }
    if(!newPano.loadPTScript(in, ptoVersion,(std::string)file.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR).mb_str(HUGIN_CONV_FILENAME)))
    {
        cerr << "could not parse script: " << file.GetFullPath().char_str() << endl;
        return 1;
    };
    pano.setMemento(newPano);
 
    //match images
    AutoCtrlPointCreator matcher;
    HuginBase::UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    //deactivate libpano messages
    PT_setProgressFcn(ptProgress);
    PT_setInfoDlgFcn(ptinfoDlg);
    HuginBase::CPVector cps = matcher.automatch(m_cpsetting,pano,imgs,m_matches,NULL);
    PT_setProgressFcn(NULL);
    PT_setInfoDlgFcn(NULL);
    if(cps.size()==0)
    {
        return 1;
    };
    for(unsigned i=0;i<cps.size();i++)
    {
        pano.addCtrlPoint(cps[i]);
    };

    //write output
    HuginBase::OptimizeVector optvec = pano.getOptimizeVector();
    ofstream of((const char *)m_output.mb_str(HUGIN_CONV_FILENAME));
    wxFileName outputFile(m_output);
    outputFile.MakeAbsolute();
    string prefix(outputFile.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME).char_str());
    pano.printPanoramaScript(of, optvec, pano.getOptions(), imgs, false, prefix);
    
    cout << endl << "Written output to " << m_output.char_str() << endl;

    return 0;
};

IMPLEMENT_APP_CONSOLE(iCPApp)
