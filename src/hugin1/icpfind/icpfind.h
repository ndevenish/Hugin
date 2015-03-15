// -*- c-basic-offset: 4 -*-

/** @file icpfind.cpp
 *
 *  @brief program to heuristic detection of control points in panoramas
 *  
 *  @author Thomas Modes
 *
 */

#include "panoinc_WX.h"
#include <wx/cmdline.h>
#include "CPDetectorConfig.h"
#include <set>

#ifdef __WXMAC__
class iCPApp : public wxApp
#else
class iCPApp : public wxAppConsole
#endif
{
    /** the main procedure of iCPApp */
    virtual int OnRun();
    /** set the parameters for the command line parser */
    virtual void OnInitCmdLine(wxCmdLineParser &parser);
    /** processes the command line parameters */
    virtual bool OnCmdLineParsed(wxCmdLineParser &parser);
private:
    /** read the CPDetectorConfig from file/registry */
    void ReadDetectorConfig();

    long m_setting;
    long m_matches;
    wxString m_input;
    wxString m_output;
    CPDetectorSetting m_cpsetting;
    HuginBase::Panorama pano;
};

DECLARE_APP(iCPApp)
