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
#include <wx/fileconf.h>

/** description of default cp generator, for fall back procedure */
wxString default_cpgenerator_desc(wxT("Hugin's Cpfind"));
/** program name of default cp generator, for fall back procedure */
#ifdef __WINDOWS__
wxString default_cpgenerator_prog(wxT("cpfind.exe"));
#else
wxString default_cpgenerator_prog(wxT("cpfind"));
#endif
/** arguments for default cp generator, for fall back procedure */
wxString default_cpgenerator_args(wxT("-o %o %s"));

void CPDetectorConfig::Read(wxConfigBase *config,wxString loadFromFile)
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
    {
        if(loadFromFile.IsEmpty())
        {
            ResetToDefault();
        }
        else
        {
            ReadFromFile(loadFromFile);
        };
    };
    if(default_generator>=settings.GetCount())
        default_generator=0;
};

void CPDetectorConfig::ReadFromFile(wxString filename)
{
    if(wxFile::Exists(filename))
    {
        wxFileConfig fconfig(wxT("hugin"),wxEmptyString,filename);
        Read(&fconfig);
    }
    else
    {
        ResetToDefault();
    };
};

void CPDetectorConfig::ReadIndex(wxConfigBase *config, int i)
{
    wxString path=wxString::Format(wxT("/AutoPano/AutoPano_%d"),i);
    if(config->HasGroup(path))
    {
        CPDetectorSetting* gen=new CPDetectorSetting;
        if(gen->Read(config,path))
        {
            settings.Add(gen);
            if(i==default_generator)
            {
                default_generator=settings.Index(*gen,true);
            };
        };
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

void CPDetectorConfig::WriteToFile(wxString filename)
{
    wxFileConfig fconfig(wxT("hugin"),wxEmptyString,filename);
    Write(&fconfig);
    fconfig.Flush();
};

void CPDetectorConfig::WriteIndex(wxConfigBase *config, int i)
{
    wxString path=wxString::Format(wxT("/AutoPano/AutoPano_%d"),i);
    settings[i].Write(config,path);
};

void CPDetectorConfig::ResetToDefault()
{
    settings.Clear();
    settings.Add(new CPDetectorSetting());
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

CPDetectorSetting::CPDetectorSetting()
{
    type=CPDetector_AutoPanoSift;
    desc=default_cpgenerator_desc;
    prog=default_cpgenerator_prog;
    args=default_cpgenerator_args;
    args_cleanup=wxEmptyString;
    prog_matcher=wxEmptyString;
    args_matcher=wxEmptyString;
    prog_stack=wxEmptyString;
    args_stack=wxEmptyString;
    option=true;
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
            _type==CPDetector_AutoPanoSiftMultiRowStack ||
            _type==CPDetector_AutoPanoSiftPreAlign);
};

const bool CPDetectorSetting::ContainsStacks(CPDetectorType _type)
{
    return (_type==CPDetector_AutoPanoSiftStack || _type==CPDetector_AutoPanoSiftMultiRowStack);
};

bool CPDetectorSetting::Read(wxConfigBase *config, wxString path)
{
    if(!config->Exists(path))
    {
        return false;
    }
    type=(CPDetectorType)config->Read(path+wxT("/Type"),CPDetector_AutoPanoSift);
    desc=config->Read(path+wxT("/Description"),default_cpgenerator_desc);
    prog=config->Read(path+wxT("/Program"),default_cpgenerator_prog);
    args=config->Read(path+wxT("/Arguments"),default_cpgenerator_args);
    if(IsCleanupPossible())
    {
        args_cleanup=config->Read(path+wxT("/ArgumentsCleanup"),wxEmptyString);
    }
    else
    {
        args_cleanup=wxEmptyString;
    };
    prog_matcher=config->Read(path+wxT("/ProgramMatcher"),wxEmptyString);
    args_matcher=config->Read(path+wxT("/ArgumentsMatcher"),wxEmptyString);
    if(ContainsStacks())
    {
        prog_stack=config->Read(path+wxT("/ProgramStack"),wxEmptyString);
        args_stack=config->Read(path+wxT("/ArgumentsStack"),wxEmptyString);
    }
    else
    {
        prog_stack=wxEmptyString;
        args_stack=wxEmptyString;
    };
    config->Read(path+wxT("/Option"),&option,true);
    CheckValues();
    return true;
};

void CPDetectorSetting::Write(wxConfigBase *config, wxString path)
{
    config->Write(path+wxT("/Type"),int(type));
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


