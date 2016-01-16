// -*- c-basic-offset: 4 -*-

/** @file AutoCtrlPointCreator.cpp
 *
 *  @brief implementation of AutoCtrlPointCreator Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  This program is free software; you can redistribute it and/or
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

#include <config.h>

#include "panoinc_WX.h"
#include "panoinc.h"

#include <fstream>
#if defined (__GNUC__) && !defined (__FreeBSD__)
#include <ext/stdio_filebuf.h>
#endif

#include "algorithms/optimizer/ImageGraph.h"

#include <hugin_utils/platform.h>
#include <wx/app.h>
#include "hugin/config_defaults.h"
#include "panodata/StandardImageVariableGroups.h"
#include "icpfind/AutoCtrlPointCreator.h"
#include <algorithms/optimizer/PTOptimizer.h>
#include <algorithms/basic/CalculateOverlap.h>
#include <algorithms/basic/CalculateOptimalScale.h>

#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/platform.h"
#include "base_wx/huginConfig.h"
#include "base_wx/wxPlatform.h"
#include <wx/utils.h>
#ifdef __WXMSW__
#include <wx/stdpaths.h>
#endif

// somewhere SetDesc gets defined.. this breaks wx/cmdline.h on OSX
#ifdef SetDesc
#undef SetDesc
#endif

#include <wx/cmdline.h>

#if defined MAC_SELF_CONTAINED_BUNDLE
  #include <wx/dir.h>
  #include <CoreFoundation/CFBundle.h>
#endif

void CPMessage(const wxString message,const wxString caption, wxWindow *parent)
{
    if(parent!=NULL)
    {
        wxMessageBox(message,caption,wxOK | wxICON_ERROR,parent);
    }
    else
    {
        std::cout << message << std::endl;
    }
};

int CPExecute(wxString prog, wxString args, wxString caption, wxWindow *parent)
{
    if(parent!=NULL)
    {
        return MyExecuteCommandOnDialog(prog, args, parent,  caption);
    }
    else
    {
        wxString cmdline=prog+wxT(" ")+args;
        return wxExecute(cmdline,wxEXEC_SYNC | wxEXEC_MAKE_GROUP_LEADER);
    };
};

HuginBase::CPVector AutoCtrlPointCreator::readUpdatedControlPoints(const std::string & file,
                                                    HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs, bool reordered)
{
    std::ifstream stream(file.c_str());
    if (! stream.is_open()) {
        DEBUG_ERROR("Could not open control point detector output: " << file);
        return HuginBase::CPVector();
    }

    HuginBase::Panorama tmpp;
    HuginBase::PanoramaMemento newPano;
    int ptoVersion = 0;
    newPano.loadPTScript(stream, ptoVersion, "");
    tmpp.setMemento(newPano);

    //check if sizes matches
    if(tmpp.getNrOfImages()!=imgs.size())
    {
        return HuginBase::CPVector();
    };

    // create mapping between the panorama images.
    std::map<unsigned int, unsigned int> imgMapping;

    // create mapping between the panorama images.
    if (reordered) {
        for (unsigned int ni = 0; ni < tmpp.getNrOfImages(); ni++) {
            std::string nname = hugin_utils::stripPath(tmpp.getImage(ni).getFilename());
            for(HuginBase::UIntSet::const_iterator it=imgs.begin();it!=imgs.end();++it) {
                std::string oname = hugin_utils::stripPath(pano.getImage(*it).getFilename());
                if (nname == oname) {
                    // insert image
                    imgMapping[ni] = *it;
                    break;
                }
            }
            if (! set_contains(imgMapping, ni)) {
                DEBUG_ERROR("Could not find image " << ni << ", name: " << tmpp.getImage(ni).getFilename() << " in autopano output");
                return HuginBase::CPVector();
            }
        }
    } else {
        size_t i=0;
        for(HuginBase::UIntSet::const_iterator it=imgs.begin();it!=imgs.end();++it)
        {
            imgMapping[i++]=*it;
        };
    }
    
    // get control points
    HuginBase::CPVector ctrlPoints = tmpp.getCtrlPoints();
    // make sure they are in correct order
    for (HuginBase::CPVector::iterator it = ctrlPoints.begin(); it != ctrlPoints.end(); ++it) {
        (*it).image1Nr = imgMapping[(*it).image1Nr];
        (*it).image2Nr = imgMapping[(*it).image2Nr];
    }

    return ctrlPoints;
}

#if defined MAC_SELF_CONTAINED_BUNDLE
wxString GetBundledProg(wxString progName)
{
    // First check inside the bundle for (AutoCP generator "without path"), e.g. binary name with path stripped off
    wxFileName file(progName);
    // if executable contains no path, look inside bundle, if program can be found there
    if(file.GetPath().IsEmpty())
        //return MacGetPathToBundledResourceFile(MacCreateCFStringWithWxString(progName));
        return MacGetPathToBundledExecutableFile(MacCreateCFStringWithWxString(progName));
    return wxEmptyString;
}
#endif

wxString GetProgPath(wxString progName)
{
#if defined MAC_SELF_CONTAINED_BUNDLE
    wxString bundled=GetBundledProg(progName);
    if(!bundled.IsEmpty())
        return bundled;
#else 
#ifdef __WXMSW__
    wxFileName prog(progName);
    if(prog.IsAbsolute())
    {
        return progName;
    }
    else
    {
        wxPathList pathlist;
        const wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        pathlist.Add(exePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
        pathlist.AddEnvList(wxT("PATH"));
        return pathlist.FindAbsoluteValidPath(progName);
    };
#endif
#endif
    return progName;
};

bool CanStartProg(wxString progName,wxWindow* parent)
{
#if defined MAC_SELF_CONTAINED_BUNDLE
    if(!GetBundledProg(progName).IsEmpty())
        return true;
#endif
    wxFileName prog(progName);
    bool canStart=false; 
    if(prog.IsAbsolute())
    {
        canStart=(prog.IsFileExecutable());
    }
    else
    {
        wxPathList pathlist;
#ifdef __WXMSW__
        const wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        pathlist.Add(exePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
#endif
        pathlist.AddEnvList(wxT("PATH"));
        wxString path = pathlist.FindAbsoluteValidPath(progName);
        if(path.IsEmpty())
            canStart=false;
        else
        {
            wxFileName prog2(path);
            canStart=(prog2.IsFileExecutable());
        };
    };
    if(!canStart)
        CPMessage(wxString::Format(
        _("Could not find \"%s\" in path.\nMaybe you have not installed it properly or given a wrong path in the settings."),progName.c_str()),
            _("Error"),parent);
    return canStart;
};

HuginBase::CPVector AutoCtrlPointCreator::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, wxWindow *parent)
{
    int return_value;
    return automatch(setting,pano,imgs,nFeatures,return_value,parent);
};

HuginBase::CPVector AutoCtrlPointCreator::automatch(CPDetectorSetting &setting,
                                         HuginBase::Panorama & pano,
                                         const HuginBase::UIntSet & imgs,
                                         int nFeatures,
                                         int & ret_value, 
                                         wxWindow *parent)
{
    HuginBase::CPVector cps;
    CPDetectorType t = setting.GetType();
    //check, if the cp generators exists
    if(!CanStartProg(setting.GetProg(),parent))
        return cps;
    if(setting.IsTwoStepDetector())
        if(!CanStartProg(setting.GetProgMatcher(),parent))
            return cps;
    if(t==CPDetector_AutoPanoSiftStack || t==CPDetector_AutoPanoSiftMultiRowStack)
        if(!setting.GetProgStack().IsEmpty())
            if(!CanStartProg(setting.GetProgStack(),parent))
                return cps;
    //change locale for correct numeric output
    char * p = setlocale(LC_NUMERIC,NULL);
    char * old_locale = strdup(p);
    setlocale(LC_NUMERIC,"C");
    switch (t) {
    case CPDetector_AutoPano:
	{
	    // autopano@kolor
	    AutoPanoKolor matcher;
	    cps = matcher.automatch(setting, pano, imgs, nFeatures, ret_value, parent);
	    break;
	}
    case CPDetector_AutoPanoSift:
	{
	    // autopano-sift
	    AutoPanoSift matcher;
	    cps = matcher.automatch(setting, pano, imgs, nFeatures, ret_value, parent);
	    break;
	}
    case CPDetector_AutoPanoSiftStack:
    {
        // autopano-sift with stacks
        AutoPanoSiftStack matcher;
        cps = matcher.automatch(setting, pano, imgs, nFeatures, ret_value, parent);
        break;
    }
    case CPDetector_AutoPanoSiftMultiRow:
    {
        // autopano-sift for multi-row panoramas
        AutoPanoSiftMultiRow matcher;
        cps = matcher.automatch(setting, pano, imgs, nFeatures, ret_value, parent);
        break;
    }
    case CPDetector_AutoPanoSiftMultiRowStack:
    {
        // autopano-sift for multi-row panoramas with stacks
        AutoPanoSiftMultiRowStack matcher;
        cps = matcher.automatch(setting, pano, imgs, nFeatures, ret_value, parent);
        break;
    }
    case CPDetector_AutoPanoSiftPreAlign:
    {
        // autopano-sift for panoramas with position information
        AutoPanoSiftPreAlign matcher;
        cps = matcher.automatch(setting, pano, imgs, nFeatures, ret_value, parent);
        break;
    }
	default:
	    DEBUG_ERROR("Invalid autopano type");
    }
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);
    return cps;
}

void AutoCtrlPointCreator::Cleanup(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           std::vector<wxString> &keyFiles, wxWindow *parent)
{
    if(setting.IsTwoStepDetector())
    {
        if(keyFiles.size()>0)
        {
            for(unsigned int i=0;i<keyFiles.size();i++)
            {
                if(wxFileExists(keyFiles[i]))
                {
                    if(!wxRemoveFile(keyFiles[i]))
                    {
                        DEBUG_DEBUG("could not remove temporary file: " << keyFiles[i].c_str());
                    };
                };
            };
        };
    }
    else
    {
        if(!setting.IsCleanupPossible())
        {
            return;
        };
        // create suitable command line..
        wxString cleanupExe = GetProgPath(setting.GetProg());
        wxString cleanupArgs = setting.GetArgsCleanup();
        if(cleanupArgs.IsEmpty())
        {
            return;
        };
    
        wxString ptoinfile_name = wxFileName::CreateTempFileName(wxT("ap_inproj"));
        cleanupArgs.Replace(wxT("%s"), ptoinfile_name);
        std::ofstream ptoinstream(ptoinfile_name.mb_str(wxConvFile));
        pano.printPanoramaScript(ptoinstream, pano.getOptimizeVector(), pano.getOptions(), imgs, false);

        int ret_value=CPExecute(cleanupExe, cleanupArgs, _("cleaning up temporary keypoint files"), parent);

        if(ret_value!=0)
        {
            DEBUG_DEBUG("could not cleanup temporary keypoint files");
        };
        if(!wxRemoveFile(ptoinfile_name))
        {
            DEBUG_DEBUG("could not remove temporary file: " << ptoinfile_name.c_str());
        };
    };
};
        
HuginBase::CPVector AutoPanoSift::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    HuginBase::CPVector cps;
    if (imgs.size() == 0) {
        return cps;
    }
    // create suitable command line..
    wxString autopanoExe = GetProgPath(setting.GetProg());
    if(setting.IsTwoStepDetector())
    {
        std::vector<wxString> keyFiles(pano.getNrOfImages());
        cps=automatch(setting, pano, imgs, nFeatures, keyFiles, ret_value, parent);
        Cleanup(setting, pano, imgs, keyFiles, parent);
        return cps;
    };
    wxString autopanoArgs = setting.GetArgs();
    
    // TODO: create a secure temporary filename here
    wxString ptofile = wxFileName::CreateTempFileName(wxT("ap_res"));
    autopanoArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    autopanoArgs.Replace(wxT("%p"), tmp);

    HuginBase::SrcPanoImage firstImg = pano.getSrcImage(*imgs.begin());
    tmp.Printf(wxT("%f"), firstImg.getHFOV());
    autopanoArgs.Replace(wxT("%v"), tmp);

    tmp.Printf(wxT("%d"), (int) firstImg.getProjection());
    autopanoArgs.Replace(wxT("%f"), tmp);

    long idx = autopanoArgs.Find(wxT("%namefile")) ;
    DEBUG_DEBUG("find %namefile in '"<< autopanoArgs.mb_str(wxConvLocal) << "' returned: " << idx);
    bool use_namefile = idx >=0;
    idx = autopanoArgs.Find(wxT("%i"));
    DEBUG_DEBUG("find %i in '"<< autopanoArgs.mb_str(wxConvLocal) << "' returned: " << idx);
    bool use_params = idx >=0;
    idx = autopanoArgs.Find(wxT("%s"));
    bool use_inputscript = idx >=0;

    if (! (use_namefile || use_params || use_inputscript)) {
        CPMessage(_("Please use %namefile, %i or %s to specify the input files for the control point detector"),
                     _("Error in control point detector command"), parent);
        return cps;
    }

    wxFile namefile;
    wxString namefile_name;
    if (use_namefile) {
        // create temporary file with image names.
        namefile_name = wxFileName::CreateTempFileName(wxT("ap_imgnames"), &namefile);
        DEBUG_DEBUG("before replace %namefile: " << autopanoArgs.mb_str(wxConvLocal));
        autopanoArgs.Replace(wxT("%namefile"), namefile_name);
        DEBUG_DEBUG("after replace %namefile: " << autopanoArgs.mb_str(wxConvLocal));
        for (HuginBase::UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
        {
            namefile.Write(wxString(pano.getImage(*it).getFilename().c_str(), HUGIN_CONV_FILENAME));
            namefile.Write(wxT("\r\n"));
        }
        // close namefile
        if (namefile_name != wxString(wxT(""))) {
            namefile.Close();
        }
    } else {
        std::string imgFiles;
        for (HuginBase::UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
        {
            imgFiles.append(" ").append(hugin_utils::quoteFilename(pano.getImage(*it).getFilename()));
        }
        autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), HUGIN_CONV_FILENAME));
    }

    wxString ptoinfile_name;
    if (use_inputscript) {
        wxFile ptoinfile;
        ptoinfile_name = wxFileName::CreateTempFileName(wxT("ap_inproj"));
        autopanoArgs.Replace(wxT("%s"), ptoinfile_name);

        std::ofstream ptoinstream(ptoinfile_name.mb_str(wxConvFile));
        //delete all existing control points in temp project
        //otherwise the existing control points will be loaded again
        HuginBase::Panorama tempPano = pano.duplicate();
        HuginBase::CPVector emptyCPV;
        tempPano.setCtrlPoints(emptyCPV);
        tempPano.printPanoramaScript(ptoinstream, tempPano.getOptimizeVector(), tempPano.getOptions(), imgs, false);
    }

#ifdef __WXMSW__
    if (autopanoArgs.size() > 32000) {
        CPMessage(_("Command line for control point detector too long.\nThis is a Windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"), parent );
        return cps;
    }
#endif

    wxString cmd = autopanoExe + wxT(" ") + autopanoArgs;
    DEBUG_DEBUG("Executing: " << autopanoExe.mb_str(wxConvLocal) << " " << autopanoArgs.mb_str(wxConvLocal));

    wxArrayString arguments = wxCmdLineParser::ConvertStringToArgs(autopanoArgs);
    if (arguments.GetCount() > 127) {
        DEBUG_ERROR("Too many arguments for call to wxExecute()");
        DEBUG_ERROR("Try using the %%s parameter in preferences");
        CPMessage(wxString::Format(_("Too many arguments (images). Try using the %%s parameter in preferences.\n\n Could not execute command: %s"), autopanoExe.c_str()), _("wxExecute Error"), parent);
        return cps;
    }

    ret_value = 0;
    // use MyExternalCmdExecDialog
    ret_value = CPExecute(autopanoExe, autopanoArgs, _("finding control points"), parent);

    if (ret_value == HUGIN_EXIT_CODE_CANCELLED) {
        return cps;
    } else if (ret_value == -1) {
        CPMessage( wxString::Format(_("Could not execute command: %s"),cmd.c_str()), _("wxExecute Error"), parent);
        return cps;
    } else if (ret_value > 0) {
        CPMessage(wxString::Format(_("Command: %s\nfailed with error code: %d"),cmd.c_str(),ret_value),
                     _("wxExecute Error"), parent);
        return cps;
    }

    if (! wxFileExists(ptofile.c_str())) {
        CPMessage(wxString::Format(_("Could not open %s for reading\nThis is an indicator that the control point detector call failed,\nor incorrect command line parameters have been used.\n\nExecuted command: %s"),ptofile.c_str(),cmd.c_str()),
                     _("Control point detector failure"), parent );
        return cps;
    }

    // read and update control points
    cps = readUpdatedControlPoints((const char*)ptofile.mb_str(HUGIN_CONV_FILENAME), pano, imgs, !use_inputscript);

    if (namefile_name != wxString(wxT(""))) {
        namefile.Close();
        wxRemoveFile(namefile_name);
    }

    if (ptoinfile_name != wxString(wxT(""))) {
        wxRemoveFile(ptoinfile_name);
    }

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }

    return cps;
}

HuginBase::CPVector AutoPanoSift::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, std::vector<wxString> &keyFiles, int & ret_value, wxWindow *parent)
{
    HuginBase::CPVector cps;
    if (imgs.size() == 0) 
    {
        return cps;
    }
    DEBUG_ASSERT(keyFiles.size()==pano.getNrOfImages());
    // create suitable command line..
    wxString generateKeysExe=GetProgPath(setting.GetProg());
    wxString matcherExe = GetProgPath(setting.GetProgMatcher());
    wxString generateKeysArgs=setting.GetArgs();
    wxString matcherArgs = setting.GetArgsMatcher();
    
    wxString tempDir= wxConfigBase::Get()->Read(wxT("tempDir"),wxT(""));
    if(!tempDir.IsEmpty())
        if(tempDir.Last()!=wxFileName::GetPathSeparator())
            tempDir.Append(wxFileName::GetPathSeparator());
    //check arguments
    if(generateKeysArgs.Find(wxT("%i"))==wxNOT_FOUND || generateKeysArgs.Find(wxT("%k"))==wxNOT_FOUND)
    {
        CPMessage(_("Please use %i to specify the input files and %k to specify the keypoint file for the generate keys step"),
                     _("Error in control point detector command"), parent);
        return cps;
    };
    if(matcherArgs.Find(wxT("%k"))==wxNOT_FOUND || matcherArgs.Find(wxT("%o"))==wxNOT_FOUND)
    {
        CPMessage(_("Please use %k to specify the keypoint files and %o to specify the output project file for the matching step"),
                     _("Error in control point detector command"), parent);
        return cps;
    };

    ret_value=0;
    for (HuginBase::UIntSet::const_iterator img = imgs.begin(); img != imgs.end(); ++img)
    {
        if(keyFiles[*img].IsEmpty())
        {
            //no key files exists, so generate it
            wxString keyfile=wxFileName::CreateTempFileName(tempDir+wxT("apk_"));
            keyFiles[*img]=keyfile;
            wxString cmd=generateKeysArgs;
            wxString tmp;
            tmp.Printf(wxT("%d"), nFeatures);
            cmd.Replace(wxT("%p"), tmp);

            HuginBase::SrcPanoImage srcImg = pano.getSrcImage(*img);
            tmp.Printf(wxT("%f"), srcImg.getHFOV());
            cmd.Replace(wxT("%v"), tmp);

            tmp.Printf(wxT("%d"), (int) srcImg.getProjection());
            cmd.Replace(wxT("%f"), tmp);
            
            cmd.Replace(wxT("%i"),hugin_utils::wxQuoteFilename(wxString(srcImg.getFilename().c_str(), HUGIN_CONV_FILENAME)));
            cmd.Replace(wxT("%k"),hugin_utils::wxQuoteFilename(keyfile));
            // use MyExternalCmdExecDialog
            ret_value = CPExecute(generateKeysExe, cmd, _("generating key file"), parent);
            cmd=generateKeysExe+wxT(" ")+cmd;
            if (ret_value == HUGIN_EXIT_CODE_CANCELLED) 
                return cps;
            else
                if (ret_value == -1) 
                {
                    CPMessage( wxString::Format(_("Could not execute command: %s"),cmd.c_str()), _("wxExecute Error"), parent);
                    return cps;
                } 
                else
                    if (ret_value > 0) 
                    {
                        CPMessage(wxString::Format(_("Command: %s\nfailed with error code: %d"),cmd.c_str(),ret_value),
                            _("wxExecute Error"), parent);
                        return cps;
                    };
        };
    };

    // TODO: create a secure temporary filename here
    wxString ptofile = wxFileName::CreateTempFileName(wxT("ap_res"));
    matcherArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    matcherArgs.Replace(wxT("%p"), tmp);

    HuginBase::SrcPanoImage firstImg = pano.getSrcImage(*imgs.begin());
    tmp.Printf(wxT("%f"), firstImg.getHFOV());
    matcherArgs.Replace(wxT("%v"), tmp);

    tmp.Printf(wxT("%d"), (int) firstImg.getProjection());
    matcherArgs.Replace(wxT("%f"), tmp);

    wxString imgFiles;
    for (HuginBase::UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
    {
        imgFiles.append(wxT(" ")).append(hugin_utils::wxQuoteFilename(keyFiles[*it]));
     };
     matcherArgs.Replace(wxT("%k"), wxString (imgFiles.wc_str(), HUGIN_CONV_FILENAME));

#ifdef __WXMSW__
    if (matcherArgs.size() > 32000) {
        CPMessage(_("Command line for control point detector too long.\nThis is a Windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"), parent );
        return cps;
    }
#endif

    wxString cmd = matcherExe + wxT(" ") + matcherArgs;
    DEBUG_DEBUG("Executing: " << matcherExe.mb_str(wxConvLocal) << " " << matcherArgs.mb_str(wxConvLocal));

    wxArrayString arguments = wxCmdLineParser::ConvertStringToArgs(matcherArgs);
    if (arguments.GetCount() > 127) {
        DEBUG_ERROR("Too many arguments for call to wxExecute()");
        CPMessage(wxString::Format(_("Too many arguments (images). Try using a cp generator setting which supports the %%s parameter in preferences.\n\n Could not execute command: %s"), matcherExe.c_str()), _("wxExecute Error"), parent);
        return cps;
    }

    // use MyExternalCmdExecDialog
    ret_value = CPExecute(matcherExe, matcherArgs, _("finding control points"), parent);

    if (ret_value == HUGIN_EXIT_CODE_CANCELLED) 
        return cps;
    else 
        if (ret_value == -1) 
        {
            CPMessage( wxString::Format(_("Could not execute command: %s"),cmd.c_str()), _("wxExecute Error"), parent);
            return cps;
        } 
        else
            if (ret_value > 0) 
            {
                CPMessage(wxString::Format(_("Command: %s\nfailed with error code: %d"),cmd.c_str(),ret_value),
                     _("wxExecute Error"), parent);
                return cps;
            };

    if (! wxFileExists(ptofile.c_str()))
    {
        CPMessage(wxString::Format(_("Could not open %s for reading\nThis is an indicator that the control point detector call failed,\nor incorrect command line parameters have been used.\n\nExecuted command: %s"),ptofile.c_str(),cmd.c_str()),
                     _("Control point detector failure"), parent );
        return cps;
    }

    // read and update control points
    cps = readUpdatedControlPoints((const char *)ptofile.mb_str(HUGIN_CONV_FILENAME), pano, imgs, true);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }

    return cps;
};

HuginBase::CPVector AutoPanoKolor::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                              int nFeatures, int & ret_value, wxWindow *parent)
{
    HuginBase::CPVector cps;
    wxString autopanoExe = setting.GetProg();

    // write default autopano.kolor.com flags
    wxString autopanoArgs = setting.GetArgs();

    std::string imgFiles;
    for (HuginBase::UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
    {
        imgFiles.append(" ").append(hugin_utils::quoteFilename(pano.getImage(*it).getFilename()));
    }

    wxString ptofilepath = wxFileName::CreateTempFileName(wxT("ap_res"));
    wxFileName ptofn(ptofilepath);
    wxString ptofile = ptofn.GetFullName();
    autopanoArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    autopanoArgs.Replace(wxT("%p"), tmp);
    HuginBase::SrcPanoImage firstImg = pano.getSrcImage(*imgs.begin());
    tmp.Printf(wxT("%f"), firstImg.getHFOV());
    autopanoArgs.Replace(wxT("%v"), tmp);

    tmp.Printf(wxT("%d"), (int) firstImg.getProjection());
    autopanoArgs.Replace(wxT("%f"), tmp);

    autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), HUGIN_CONV_FILENAME));

    wxString tempdir = ptofn.GetPath();
	autopanoArgs.Replace(wxT("%d"), ptofn.GetPath());
    wxString cmd;
    cmd.Printf(wxT("%s %s"), hugin_utils::wxQuoteFilename(autopanoExe).c_str(), autopanoArgs.c_str());
#ifdef __WXMSW__
    if (cmd.size() > 32766) {
        CPMessage(_("Command line for control point detector too long.\nThis is a Windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"), parent);
        return cps;
    }
#endif
    DEBUG_DEBUG("Executing: " << cmd.c_str());

    wxArrayString arguments = wxCmdLineParser::ConvertStringToArgs(cmd);
    if (arguments.GetCount() > 127) {
        DEBUG_ERROR("Too many arguments for call to wxExecute()");
        DEBUG_ERROR("Try using the %s parameter in preferences");
        CPMessage(wxString::Format(_("Too many arguments (images). Try using the %%s parameter in preferences.\n\n Could not execute command: %s"), autopanoExe.c_str()), _("wxExecute Error"), parent);
        return cps;
    }

    ret_value = 0;
    // use MyExternalCmdExecDialog
    ret_value = CPExecute(autopanoExe, autopanoArgs, _("finding control points"), parent);

    if (ret_value == HUGIN_EXIT_CODE_CANCELLED) {
        return cps;
    } else if (ret_value == -1) {
        CPMessage( wxString::Format(_("Could not execute command: %s"),cmd.c_str()), _("wxExecute Error"),  parent);
        return cps;
    } else if (ret_value > 0) {
        CPMessage(wxString::Format(_("Command: %s\nfailed with error code: %d"),cmd.c_str(),ret_value),
                     _("wxExecute Error"), parent);
        return cps;
    }

    ptofile = ptofn.GetFullPath();
    ptofile.append(wxT("0.oto"));
    if (! wxFileExists(ptofile.c_str()) ) {
        CPMessage(wxString::Format(_("Could not open %s for reading\nThis is an indicator that the control point detector call failed,\nor incorrect command line parameters have been used.\n\nExecuted command: %s"),ptofile.c_str(),cmd.c_str()),
                     _("Control point detector failure"), parent );
        return cps;
    }
    // read and update control points
    cps = readUpdatedControlPoints((const char *)ptofile.mb_str(HUGIN_CONV_FILENAME), pano, imgs, true);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
    return cps;
}

struct img_ev
{
    unsigned int img_nr;
    double ev;
};
struct stack_img
{
    unsigned int layer_nr;
    std::vector<img_ev> images;
};
bool sort_img_ev (img_ev i1, img_ev i2) { return (i1.ev<i2.ev); };

void AddControlPointsWithCheck(HuginBase::CPVector &cpv, HuginBase::CPVector &new_cp, HuginBase::Panorama *pano = NULL)
{
    for(unsigned int i=0;i<new_cp.size();i++)
    {
        HuginBase::ControlPoint cp=new_cp[i];
        bool duplicate=false;
        for(unsigned int j=0;j<cpv.size();j++)
        {
            if(cp==cpv[j])
            {
                duplicate=true;
                break;
            }
        };
        if(!duplicate)
        {
            cpv.push_back(cp);
            if(pano!=NULL)
                pano->addCtrlPoint(cp);
        };
    };
};

HuginBase::CPVector AutoPanoSiftStack::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    HuginBase::CPVector cps;
    if (imgs.size() == 0) {
        return cps;
    };
    std::vector<stack_img> stack_images;
    HuginBase::StandardImageVariableGroups* variable_groups = new HuginBase::StandardImageVariableGroups(pano);
    for (HuginBase::UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
    {
        unsigned int stack_nr=variable_groups->getStacks().getPartNumber(*it);
        //check, if this stack is already in list
        bool found=false;
        unsigned int index=0;
        for(index=0;index<stack_images.size();index++)
        {
            found=(stack_images[index].layer_nr==stack_nr);
            if(found)
                break;
        };
        if(!found)
        {
            //new stack
            stack_images.resize(stack_images.size()+1);
            index=stack_images.size()-1;
            //add new stack
            stack_images[index].layer_nr=stack_nr;
        };
        //add new image
        unsigned int new_image_index=stack_images[index].images.size();
        stack_images[index].images.resize(new_image_index+1);
        stack_images[index].images[new_image_index].img_nr=*it;
        stack_images[index].images[new_image_index].ev=pano.getImage(*it).getExposure();
    };
    delete variable_groups;
    //get image with median exposure for search with cp generator
    HuginBase::UIntSet images_layer;
    for(unsigned int i=0;i<stack_images.size();i++)
    {
        std::sort(stack_images[i].images.begin(),stack_images[i].images.end(),sort_img_ev);
        unsigned int index=0;
        if(stack_images[i].images[0].ev!=stack_images[i].images[stack_images[i].images.size()-1].ev)
        {
            index=stack_images[i].images.size() / 2;
        };
        images_layer.insert(stack_images[i].images[index].img_nr);
    };
    //generate cp for median exposure
    ret_value=0;
    if(images_layer.size()>1)
    {
        AutoPanoSift matcher;
        cps=matcher.automatch(setting, pano, images_layer, nFeatures, ret_value, parent);
        if(ret_value!=0)
            return cps;
    };
    //now work on all stacks
    if(!setting.GetProgStack().IsEmpty())
    {
        CPDetectorSetting stack_setting;
        stack_setting.SetType(CPDetector_AutoPanoSift);
        stack_setting.SetProg(setting.GetProgStack());
        stack_setting.SetArgs(setting.GetArgsStack());

        for(unsigned int i=0;i<stack_images.size();i++)
        {
            HuginBase::UIntSet images_stack;
            images_stack.clear();
            for(unsigned int j=0;j<stack_images[i].images.size();j++)
                images_stack.insert(stack_images[i].images[j].img_nr);
            if(images_stack.size()>1)
            {
                AutoPanoSift matcher;
                HuginBase::CPVector new_cps = matcher.automatch(stack_setting, pano, images_stack, nFeatures, ret_value, parent);
                if(new_cps.size()>0)
                    AddControlPointsWithCheck(cps,new_cps);
                if(ret_value!=0)
                    return cps;
            };
        };
    }
    return cps;
};

HuginBase::CPVector AutoPanoSiftMultiRow::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    HuginBase::CPVector cps;
    if (imgs.size() < 2) 
    {
        return cps;
    };
    std::vector<wxString> keyFiles(pano.getNrOfImages());
    //generate cp for every consecutive image pair
    unsigned int counter=0;
    for (HuginBase::UIntSet::const_iterator it = imgs.begin(); it != imgs.end();)
    {
        if(counter==imgs.size()-1)
            break;
        counter++;
        HuginBase::UIntSet ImagePair;
        ImagePair.clear();
        ImagePair.insert(*it);
        ++it;
        ImagePair.insert(*it);
        AutoPanoSift matcher;
        HuginBase::CPVector new_cps;
        new_cps.clear();
        if(setting.IsTwoStepDetector())
            new_cps=matcher.automatch(setting, pano, ImagePair, nFeatures, keyFiles, ret_value, parent);
        else
            new_cps=matcher.automatch(setting, pano, ImagePair, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps);
        if(ret_value!=0)
        {
            Cleanup(setting, pano, imgs, keyFiles, parent);
            return cps;
        };
    };
    // now connect all image groups
    // generate temporary panorama to add all found cps
    HuginBase::UIntSet allImgs;
    fill_set(allImgs, 0, pano.getNrOfImages()-1);
    HuginBase::Panorama optPano = pano.getSubset(allImgs);
    for (HuginBase::CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it)
        optPano.addCtrlPoint(*it);

    HuginGraph::ImageGraph graph(optPano);
    const HuginGraph::ImageGraph::Components comps=graph.GetComponents();
    size_t n = comps.size();
    if(n>1)
    {
        HuginBase::UIntSet ImagesGroups;
        for(size_t i=0;i<n;i++)
        {
            ImagesGroups.insert(*(comps[i].begin()));
            if(comps[i].size()>1)
                ImagesGroups.insert(*(comps[i].rbegin()));
        };
        AutoPanoSift matcher;
        HuginBase::CPVector new_cps;
        if(setting.IsTwoStepDetector())
            new_cps=matcher.automatch(setting, optPano, ImagesGroups, nFeatures, keyFiles, ret_value, parent);
        else
            new_cps=matcher.automatch(setting, optPano, ImagesGroups, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps,&optPano);
        if(ret_value!=0)
        {
            Cleanup(setting, pano, imgs, keyFiles, parent);
            return cps;
        };
        HuginGraph::ImageGraph graph2(optPano);
        n = graph2.IsConnected() ? 1 : 2;
    };
    if(n==1 && setting.GetOption())
    {
        //next steps happens only when all images are connected;
        //now optimize panorama
        HuginBase::PanoramaOptions opts = pano.getOptions();
        opts.setProjection(HuginBase::PanoramaOptions::EQUIRECTANGULAR);
        // calculate proper scaling, 1:1 resolution.
        // Otherwise optimizer distances are meaningless.
        opts.setWidth(30000, false);
        opts.setHeight(15000);

        optPano.setOptions(opts);
        int w = hugin_utils::roundi(HuginBase::CalculateOptimalScale::calcOptimalScale(optPano) * optPano.getOptions().getWidth());

        opts.setWidth(w);
        opts.setHeight(w/2);
        optPano.setOptions(opts);

        //generate optimize vector, optimize only yaw and pitch
        HuginBase::OptimizeVector optvars;
        const HuginBase::SrcPanoImage & anchorImage = optPano.getImage(opts.optimizeReferenceImage);
        for (unsigned i=0; i < optPano.getNrOfImages(); i++) 
        {
            std::set<std::string> imgopt;
            if(i==opts.optimizeReferenceImage)
            {
                //optimize only anchors pitch, not yaw
                imgopt.insert("p");
            }
            else
            {
                // do not optimize anchor image's stack for position.
                if(!optPano.getImage(i).YawisLinkedWith(anchorImage))
                {
                    imgopt.insert("p");
                    imgopt.insert("y");
                };
            };
            optvars.push_back(imgopt);
        }
        optPano.setOptimizeVector(optvars);

        // remove vertical and horizontal control points
        HuginBase::CPVector backupOldCPS = optPano.getCtrlPoints();
        HuginBase::CPVector backupNewCPS;
        for (HuginBase::CPVector::const_iterator it = backupOldCPS.begin(); it != backupOldCPS.end(); ++it) {
            if (it->mode == HuginBase::ControlPoint::X_Y)
            {
                backupNewCPS.push_back(*it);
            }
        }
        optPano.setCtrlPoints(backupNewCPS);
        // do a first pairwise optimisation step
        HuginBase::AutoOptimise::autoOptimise(optPano,false);
        HuginBase::PTools::optimize(optPano);
        optPano.setCtrlPoints(backupOldCPS);
        //and find cp on overlapping images
        //work only on image pairs, which are not yet connected
        AutoPanoSiftPreAlign matcher;
        CPDetectorSetting newSetting;
        newSetting.SetProg(setting.GetProg());
        newSetting.SetArgs(setting.GetArgs());
        if(setting.IsTwoStepDetector())
        {
            newSetting.SetProgMatcher(setting.GetProgMatcher());
            newSetting.SetArgsMatcher(setting.GetArgsMatcher());
        };
        newSetting.SetOption(true);
        HuginBase::CPVector new_cps;
        if(setting.IsTwoStepDetector())
            new_cps=matcher.automatch(newSetting, optPano, imgs, nFeatures, keyFiles, ret_value, parent);
        else
            new_cps=matcher.automatch(newSetting, optPano, imgs, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps);
    };
    Cleanup(setting, pano, imgs, keyFiles, parent);
    return cps;
};

HuginBase::CPVector AutoPanoSiftMultiRowStack::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    HuginBase::CPVector cps;
    if (imgs.size() == 0) {
        return cps;
    };
    std::vector<stack_img> stack_images;
    HuginBase::StandardImageVariableGroups* variable_groups = new HuginBase::StandardImageVariableGroups(pano);
    for (HuginBase::UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
    {
        unsigned int stack_nr=variable_groups->getStacks().getPartNumber(*it);
        //check, if this stack is already in list
        bool found=false;
        unsigned int index=0;
        for(index=0;index<stack_images.size();index++)
        {
            found=(stack_images[index].layer_nr==stack_nr);
            if(found)
                break;
        };
        if(!found)
        {
            //new stack
            stack_images.resize(stack_images.size()+1);
            index=stack_images.size()-1;
            //add new stack
            stack_images[index].layer_nr=stack_nr;
        };
        //add new image
        unsigned int new_image_index=stack_images[index].images.size();
        stack_images[index].images.resize(new_image_index+1);
        stack_images[index].images[new_image_index].img_nr=*it;
        stack_images[index].images[new_image_index].ev=pano.getImage(*it).getExposure();
    };
    delete variable_groups;
    //get image with median exposure for search with cp generator
    HuginBase::UIntSet images_layer;
    for(unsigned int i=0;i<stack_images.size();i++)
    {
        std::sort(stack_images[i].images.begin(),stack_images[i].images.end(),sort_img_ev);
        unsigned int index=0;
        if(stack_images[i].images[0].ev!=stack_images[i].images[stack_images[i].images.size()-1].ev)
        {
            index=stack_images[i].images.size() / 2;
        };
        images_layer.insert(stack_images[i].images[index].img_nr);
    };
    ret_value=0;
    //work on all stacks
    if(!setting.GetProgStack().IsEmpty())
    {
        CPDetectorSetting stack_setting;
        stack_setting.SetType(CPDetector_AutoPanoSift);
        stack_setting.SetProg(setting.GetProgStack());
        stack_setting.SetArgs(setting.GetArgsStack());

        for(unsigned int i=0;i<stack_images.size();i++)
        {
            HuginBase::UIntSet images_stack;
            images_stack.clear();
            for(unsigned int j=0;j<stack_images[i].images.size();j++)
                images_stack.insert(stack_images[i].images[j].img_nr);
            if(images_stack.size()>1)
            {
                AutoPanoSift matcher;
                HuginBase::CPVector new_cps = matcher.automatch(stack_setting, pano, images_stack, nFeatures, ret_value, parent);
                if(new_cps.size()>0)
                    AddControlPointsWithCheck(cps,new_cps);
                if(ret_value!=0)
                {
                    std::vector<wxString> emptyKeyfiles;
                    Cleanup(setting, pano, imgs, emptyKeyfiles, parent);
                    return cps;
                };
            };
        };
    }
    //generate cp for median exposure with multi-row algorithm
    if(images_layer.size()>1)
    {
        HuginBase::UIntSet allImgs;
        fill_set(allImgs, 0, pano.getNrOfImages()-1);
        HuginBase::Panorama newPano = pano.getSubset(allImgs);
        if(cps.size()>0)
            for (HuginBase::CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it)
                newPano.addCtrlPoint(*it);

        AutoPanoSiftMultiRow matcher;
        HuginBase::CPVector new_cps = matcher.automatch(setting, newPano, images_layer, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps);
    };
    return cps;
};

HuginBase::CPVector AutoPanoSiftPreAlign::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    std::vector<wxString> keyFiles(pano.getNrOfImages());
    return automatch(setting, pano, imgs, nFeatures, keyFiles, ret_value, parent);
};

HuginBase::CPVector AutoPanoSiftPreAlign::automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                                         int nFeatures, std::vector<wxString> &keyFiles, int & ret_value, wxWindow *parent)
{
    HuginBase::CPVector cps;
    if (imgs.size()<2) 
        return cps;
    DEBUG_ASSERT(keyFiles.size()==pano.getNrOfImages());

    std::vector<HuginBase::UIntSet> usedImages;
    usedImages.resize(pano.getNrOfImages());
    if(setting.GetOption())
    {
        //only work on not connected image pairs
        HuginBase::CPVector oldCps = pano.getCtrlPoints();
        for(unsigned i=0;i<oldCps.size();i++)
        {
            if (oldCps[i].mode == HuginBase::ControlPoint::X_Y)
            {
                usedImages[oldCps[i].image1Nr].insert(oldCps[i].image2Nr);
                usedImages[oldCps[i].image2Nr].insert(oldCps[i].image1Nr);
            };
        };
    };
    HuginBase::CalculateImageOverlap overlap(&pano);
    overlap.calculate(10);
    for (HuginBase::UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it)
    {
        HuginBase::UIntSet images;
        images.clear();
        images.insert(*it);
        HuginBase::UIntSet::const_iterator it2 = it;
        for(++it2;it2!=imgs.end();++it2)
        {
            //check if this image pair was yet used
            if(set_contains(usedImages[*it2],*it))
                continue;
            //now check position
            if(overlap.getOverlap(*it,*it2)>0)
            {
                images.insert(*it2);
            };
        };
        if(images.size()<2)
            continue;
        //remember image pairs for later
        for (HuginBase::UIntSet::const_iterator img_it = images.begin(); img_it != images.end(); ++img_it)
            for (HuginBase::UIntSet::const_iterator img_it2 = images.begin(); img_it2 != images.end(); ++img_it2)
                usedImages[*img_it].insert(*img_it2);
        AutoPanoSift matcher;
        HuginBase::CPVector new_cps;
        if(setting.IsTwoStepDetector())
            new_cps=matcher.automatch(setting, pano, images, nFeatures, keyFiles, ret_value, parent);
        else
            new_cps=matcher.automatch(setting, pano, images, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps);
        if(ret_value!=0)
        {
            Cleanup(setting, pano, imgs, keyFiles, parent);
            return cps;
        };
    };
    Cleanup(setting, pano, imgs, keyFiles, parent);
    return cps;
};
