// -*- c-basic-offset: 4 -*-

/** @file CPDetectorConfig.cpp
 *
 *  @brief implementation of CPDetectorSetting and CPDetectorConfig classes, 
 *         which are for storing and changing settings of different CP detectors
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "icpfind/CPDetectorConfig.h"
#include <config.h>
#include "icpfind/CPDetectorConfig_default.h"

void CPDetectorConfig::Read(wxConfigBase *config)
{
    settings.Clear();
    int count=config->Read(wxT("/AutoPano/AutoPanoCount"),0l);
    default_generator=config->Read(wxT("/AutoPano/Default"),0l);
    if(count>0)
    {
        for(int i=0;i<count;i++)
            ReadIndex(config, i);
    };
    if(settings.GetCount()==0)
        ResetToDefault();
    if(default_generator>=settings.GetCount())
        default_generator=0;
};

void CPDetectorConfig::ReadIndex(wxConfigBase *config, int i)
{
    wxString path=wxString::Format(wxT("/AutoPano/AutoPano_%d"),i);
    if(config->HasGroup(path))
    {
        CPDetectorSetting* gen=new CPDetectorSetting;
        gen->Read(config,path);
        settings.Add(gen);
        if(i==default_generator)
            default_generator=settings.Index(*gen,true);
    };
};

void CPDetectorConfig::Write(wxConfigBase *config)
{
    int count=settings.Count();
    config->Write(wxT("/AutoPano/AutoPanoCount"),count);
    config->Write(wxT("/AutoPano/Default"),(int)default_generator);
    if(count>0)
    {
        for(int i=0;i<count;i++)
            WriteIndex(config, i);
    };
};

void CPDetectorConfig::WriteIndex(wxConfigBase *config, int i)
{
    wxString path=wxString::Format(wxT("/AutoPano/AutoPano_%d"),i);
    settings[i].Write(config,path);
};

void CPDetectorConfig::ResetToDefault()
{
    settings.Clear();
    int maxIndex=sizeof(default_cpdetectors)/sizeof(cpdetector_default)-1;
    for(int i=0;i<=maxIndex;i++)
        settings.Add(new CPDetectorSetting(i));
    default_generator=0;
};

void CPDetectorConfig::FillControl(wxControlWithItems *control,bool select_default,bool show_default)
{
    control->Clear();
    for(unsigned int i=0;i<settings.GetCount();i++)
    {
        wxString s=settings[i].GetCPDetectorDesc();
        if(show_default && i==default_generator)
            s=s+wxT(" (")+_("Default")+wxT(")");
        control->Append(s);
    };
    if(select_default)
        control->SetSelection(default_generator);
};

void CPDetectorConfig::Swap(int index)
{
    CPDetectorSetting* setting=settings.Detach(index);
    settings.Insert(setting,index+1);
    if(default_generator==index)
        default_generator=index+1;
    else 
        if(default_generator==index+1)
            default_generator=index;
};

void CPDetectorConfig::SetDefaultGenerator(unsigned int new_default_generator)
{
    if(new_default_generator<GetCount())
        default_generator=new_default_generator;
    else
        default_generator=0;
};

#include <wx/arrimpl.cpp> 
WX_DEFINE_OBJARRAY(ArraySettings);

CPDetectorSetting::CPDetectorSetting(int new_type)
{
    if(new_type>=0)
    {
        int maxIndex=sizeof(default_cpdetectors)/sizeof(cpdetector_default)-1;
        if (new_type<=maxIndex)
        {
            type=default_cpdetectors[new_type].type;
            desc=default_cpdetectors[new_type].desc;
            prog=default_cpdetectors[new_type].prog;
            args=default_cpdetectors[new_type].args;
            if(IsCleanupPossible())
            {
                args_cleanup=default_cpdetectors[new_type].args_cleanup;
            }
            else
            {
                args_cleanup=wxEmptyString;
            };
            prog_matcher=default_cpdetectors[new_type].prog_matcher;
            args_matcher=default_cpdetectors[new_type].args_matcher;
            if(ContainsStacks())
            {
                prog_stack=default_cpdetectors[new_type].prog_stack;
                args_stack=default_cpdetectors[new_type].args_stack;
            }
            else
            {
                prog_stack=wxEmptyString;
                args_stack=wxEmptyString;
            };
            option=default_cpdetectors[new_type].option;
        };
    }
    else
    {
        type=CPDetector_AutoPanoSift;
        desc=wxEmptyString;
        prog=wxEmptyString;
        args=wxEmptyString;
        args_cleanup=wxEmptyString;
        prog_matcher=wxEmptyString;
        args_matcher=wxEmptyString;
        prog_stack=wxEmptyString;
        args_stack=wxEmptyString;
        option=true;
    };
    CheckValues();
};

void CPDetectorSetting::CheckValues()
{
    if(type==CPDetector_AutoPano)
    {
        if(!prog_matcher.IsEmpty())
        {
            prog_matcher=wxEmptyString;
            args_matcher=wxEmptyString;
        };
    };
};

const bool CPDetectorSetting::IsCleanupPossible(CPDetectorType _type)
{
    return (_type==CPDetector_AutoPanoSiftMultiRow || 
            _type==CPDetector_AutoPanoSiftMultiRowStack);
};

const bool CPDetectorSetting::ContainsStacks(CPDetectorType _type)
{
    return (_type==CPDetector_AutoPanoSiftStack || _type==CPDetector_AutoPanoSiftMultiRowStack);
};

void CPDetectorSetting::Read(wxConfigBase *config, wxString path)
{
    type=(CPDetectorType)config->Read(path+wxT("/Type"),default_cpdetectors[0].type);
    desc=config->Read(path+wxT("/Description"),default_cpdetectors[0].desc);
    prog=config->Read(path+wxT("/Program"),default_cpdetectors[0].prog);
    args=config->Read(path+wxT("/Arguments"),default_cpdetectors[0].args);
    if(IsCleanupPossible())
    {
        args_cleanup=config->Read(path+wxT("/ArgumentsCleanup"),wxEmptyString);
    }
    else
    {
        args_cleanup=wxEmptyString;
    };
    prog_matcher=config->Read(path+wxT("/ProgramMatcher"),default_cpdetectors[0].prog_matcher);
    args_matcher=config->Read(path+wxT("/ArgumentsMatcher"),default_cpdetectors[0].args_matcher);
    if(ContainsStacks())
    {
        prog_stack=config->Read(path+wxT("/ProgramStack"),default_cpdetectors[0].prog_stack);
        args_stack=config->Read(path+wxT("/ArgumentsStack"),default_cpdetectors[0].args_stack);
    }
    else
    {
        prog_stack=wxEmptyString;
        args_stack=wxEmptyString;
    };
    config->Read(path+wxT("/Option"),&option,default_cpdetectors[0].option);
    CheckValues();
};

void CPDetectorSetting::Write(wxConfigBase *config, wxString path)
{
    config->Write(path+wxT("/Type"),type);
    config->Write(path+wxT("/Description"),desc);
    config->Write(path+wxT("/Program"),prog);
    config->Write(path+wxT("/Arguments"),args);
    config->Write(path+wxT("/ProgramMatcher"),prog_matcher);
    config->Write(path+wxT("/ArgumentsMatcher"),args_matcher);
    if(IsCleanupPossible())
    {
        config->Write(path+wxT("/ArgumentsCleanup"),args_cleanup);
    };
    if(ContainsStacks())
    {
        config->Write(path+wxT("/ProgramStack"),prog_stack);
        config->Write(path+wxT("/ArgumentsStack"),args_stack);
    };
    config->Write(path+wxT("/Option"),option);
};

