// -*- c-basic-offset: 4 -*-
/**  @file CPDetectorConfig.h
 *
 *  @brief declaration of CPDetectorSetting and CPDetectorConfig classes,
 *         which are for storing and changing settings of different autopano generator 
 *
 *  @author Thomas Modes
 *
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

#ifndef _CPDETECTORCONFIG_H
#define _CPDETECTORCONFIG_H

#include <hugin_shared.h>
#include "panoinc.h"
#include "panoinc_WX.h"

#include <wx/dynarray.h>

enum CPDetectorType
{
    CPDetector_AutoPano=0,
    CPDetector_AutoPanoSift=1,
    CPDetector_AutoPanoSiftStack=2,
    CPDetector_AutoPanoSiftMultiRow=3,
    CPDetector_AutoPanoSiftMultiRowStack=4,
    CPDetector_AutoPanoSiftPreAlign=5
};

/** class, which stores all settings of one cp detector */
class ICPIMPEX CPDetectorSetting
{
public:
    /** constructor */
    CPDetectorSetting();
    /** destructor */
    virtual ~CPDetectorSetting() {};
    /** read setting for this generator from config 
     *  @return true on success */
    bool Read(wxConfigBase* config, wxString path);
    /** writes setting for this generator to config */
    void Write(wxConfigBase* config, wxString path);
    /** return description of this setting */
    const wxString GetCPDetectorDesc() {return desc; };
    /** sets description of this setting */
    void SetCPDetectorDesc(wxString new_desc) { desc=new_desc; };
    /** return type of this setting */
    const CPDetectorType GetType() {return type; };
    /** sets type of this setting */
    void SetType(CPDetectorType new_type) { type=new_type;};
    /** return program for one step detector or feature descriptor */
    const wxString GetProg() {return prog; };
    /** sets program for one step detector or feature descriptor */
    void SetProg(wxString new_prog) { prog=new_prog; };
    /** return arguments of one step detector or feature descriptor */
    const wxString GetArgs() {return args; };
    /** sets arguments of one step detector or feature descriptor */
    void SetArgs(wxString new_args) { args=new_args; };
    /** return arguments for the cleanup step */
    const wxString GetArgsCleanup() {return args_cleanup; };
    /** sets arguments for the cleanup step */
    void SetArgsCleanup(wxString new_args) { args_cleanup=new_args; };
    /** return program for feature matcher */
    const wxString GetProgMatcher() {return prog_matcher; };
    /** sets program for feature matcher */
    void SetProgMatcher(wxString new_prog) { prog_matcher=new_prog; };
    /** return arguments for feature matcher */
    const wxString GetArgsMatcher() {return args_matcher; };
    /** sets arguments for feature matcher */
    void SetArgsMatcher(wxString new_args) { args_matcher=new_args; };
    /** return program name, which works on stacks */
    const wxString GetProgStack() {return prog_stack; };
    /** sets program for detecting cp in stacks */
    void SetProgStack(wxString new_prog) { prog_stack=new_prog; };
    /** return arguments of program for detection of cp in stacks */
    const wxString GetArgsStack() {return args_stack; };
    /** sets arguments of program for detection of cp in stacks */
    void SetArgsStack(wxString new_args) { args_stack=new_args; };
    /** gets options, used in multi-row and prealigned cp detectors */
    const bool GetOption() { return option; }
    /** set options, used in multi-row cp and prealigned detectors */
    void SetOption(bool new_option) { option=new_option; };
    /** returns true, if setting is suitable for two step detector otherwise false */
    const bool IsTwoStepDetector() { return !prog_matcher.IsEmpty(); };
    /** return true, if setting allows a final cleanup run */
    static const bool IsCleanupPossible(CPDetectorType _type);
    const bool IsCleanupPossible() { return IsCleanupPossible(type); };
    /** return true, if setting contains a stack detector, even it is empty */
    static const bool ContainsStacks(CPDetectorType _type);
    const bool ContainsStacks() { return ContainsStacks(type); };
private:
    void CheckValues();
    CPDetectorType type;
    wxString desc;
    wxString prog;
    wxString args;
    wxString args_cleanup;
    wxString prog_matcher;
    wxString args_matcher;
    wxString prog_stack;
    wxString args_stack;
    bool option;
};

#if defined _WIN32 && defined Hugin_shared 
WX_DECLARE_USER_EXPORTED_OBJARRAY(CPDetectorSetting,ArraySettings,ICPIMPEX);
#else
WX_DECLARE_OBJARRAY(CPDetectorSetting,ArraySettings);
#endif

/** class for storing settings of different control point generators */
class ICPIMPEX CPDetectorConfig
{
public:
    /** constructor */
    CPDetectorConfig() {};
    /** destructor */
    virtual ~CPDetectorConfig() {};
    /** read the settings of different cp generators from config 
     *  @param config wxConfigBase, from which the settings are loaded, if empty the default wxConfigBase is used 
     *  @param loadFromFile file, from which the settings are loaded, if no settings are stored by hugin itself
     */
    void Read(wxConfigBase* config=wxConfigBase::Get(),wxString loadFromFile=wxEmptyString);
    /** writes the settings of different cp generators to config */
    void Write(wxConfigBase* config=wxConfigBase::Get());
    /** import the cp detector settings from external file */
    void ReadFromFile(wxString filename);
    /** exporth the cp detector settings to external file */
    void WriteToFile(wxString filename);
    /** reset values to default */
    void ResetToDefault();
    /** fills a wxControlWithItems with the available generators 
     *  @param control control, which should show the generators 
     *    @param select_default should default generator be selected
     *  @param show_default should default generator get a "(Default)" suffix */
    void FillControl(wxControlWithItems *control, bool select_default = false, bool show_default = false);
    /** return counts of cp detector settings */
    unsigned int GetCount() { return settings.GetCount(); };
    /** swaps setting which index and index+1 */
    void Swap(int index);
    /** return index of default generator (this one is used for assistent) */
    unsigned int GetDefaultGenerator() { return default_generator; };
    /** sets new default generator, which is used by assistent */
    void SetDefaultGenerator(unsigned int new_default_generator); 
    /** array which stores the different autopano settings */
    ArraySettings settings;
private:
    unsigned int default_generator;
    void ReadIndex(wxConfigBase* config, int i);
    void WriteIndex(wxConfigBase* config, int i);
};

#endif
