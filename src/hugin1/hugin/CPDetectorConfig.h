// -*- c-basic-offset: 4 -*-
/**  @file CPDetectorConfig.h
 *
 *  @brief declaration of CPDetectorSetting, CPDetectorConfig and CPDetectorDialog classes,
 *         which are for storing and changing settings of different autopano generator 
 *
 *  @author Thomas Modes
 *
 *  $Id$
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _CPDETECTORCONFIG_H
#define _CPDETECTORCONFIG_H

#include "panoinc.h"
#include "panoinc_WX.h"

#include <wx/dynarray.h>

/** class, which stores all settings of one cp detector */
class CPDetectorSetting
{
public:
    /** constructor 
     *  @param new_type: -1: no settings, otherwise index of default_cpdetectors array */
    CPDetectorSetting(int new_type = -1);
    /** destructor */
    virtual ~CPDetectorSetting() {};
    /** read setting for this generator from config */
    void Read(wxConfigBase* config, wxString path);
    /** writes setting for this generator to config */
    void Write(wxConfigBase* config, wxString path);
    /** return description of this setting */
    const wxString GetCPDetectorDesc() {return desc; };
    /** sets description of this setting */
    void SetCPDetectorDesc(wxString new_desc) { desc=new_desc; };
    /** return program of this setting (program name) */
    const wxString GetProg() {return prog; };
    /** sets program of this setting */
    void SetProg(wxString new_prog) { prog=new_prog; };
    /** return arguments of this setting */
    const wxString GetArgs() {return args; };
    /** sets arguments of this setting */
    void SetArgs(wxString new_args) { args=new_args; };
    /** return type of this setting */
    const int GetType() {return type; };
    /** sets type of this setting */
    void SetType(int new_type) { type=new_type;};
private:
    int type;
    wxString desc,prog,args;
};

WX_DECLARE_OBJARRAY(CPDetectorSetting,ArraySettings);

/** class for storing settings of different control point generators */
class CPDetectorConfig
{
public:
    /** constructor */
    CPDetectorConfig() {};
    /** destructor */
    virtual ~CPDetectorConfig() {};
    /** read the settings of different cp generators from config */
    void Read(wxConfigBase* config=wxConfigBase::Get());
    /** writes the settings of different cp generators to config */
    void Write(wxConfigBase* config=wxConfigBase::Get());
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

/** dialog for input settings of one autopano generator */
class CPDetectorDialog : public wxDialog
{
public:
    /** constructor */
    CPDetectorDialog(wxWindow* parent);
    /** destructor, saves position and size */
    virtual ~CPDetectorDialog();
    /** updates edit fields with values from settings 
     *  @param cpdet_config CPDetectorConfig class, which stores the settings
     *  @param index index, from which the settings should be read */
    void UpdateFields(CPDetectorConfig* cpdet_config,int index);
    /** return inputed settings 
     *  @param cpdet_config CPDetectorConfig class, which stores the settings
     *  @param index index, to which the changed settings should be written */
    void UpdateSettings(CPDetectorConfig* cpdet_config,int index);
protected:
    /** check inputs */
    void OnOk(wxCommandEvent & e);
    /** select program with file open dialog */
    void OnSelectPath(wxCommandEvent &e);
private:
    wxTextCtrl *m_edit_desc, *m_edit_prog, *m_edit_args;
    wxChoice *m_cpdetector_type;

    DECLARE_EVENT_TABLE();
};

#endif
