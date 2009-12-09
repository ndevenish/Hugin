// -*- c-basic-offset: 4 -*-

/** @file AutoCtrlPointCreator.cpp
 *
 *  @brief implementation of AutoCtrlPointCreator Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>

#include "panoinc_WX.h"
#include "panoinc.h"

#include <fstream>
#ifdef __GNUC__
#include <ext/stdio_filebuf.h>
#endif

#include "PT/Panorama.h"

#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "hugin/AutoCtrlPointCreator.h"
#include "hugin/CommandHistory.h"

#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/platform.h"
#include "base_wx/huginConfig.h"
#include "common/wxPlatform.h"
#include <wx/utils.h>

// somewhere SetDesc gets defined.. this breaks wx/cmdline.h on OSX
#ifdef SetDesc
#undef SetDesc
#endif

#include <wx/cmdline.h>


#if defined MAC_SELF_CONTAINED_BUNDLE
  #include <wx/dir.h>
  #include <CoreFoundation/CFBundle.h>
#endif

using namespace std;
using namespace PT;
using namespace utils;

CPVector AutoCtrlPointCreator::readUpdatedControlPoints(const std::string & file,
                                                    PT::Panorama & pano)
{
    ifstream stream(file.c_str());
    if (! stream.is_open()) {
        DEBUG_ERROR("Could not open autopano output: " << file);
        return CPVector();
    }

    Panorama tmpp;
    PanoramaMemento newPano;
    int ptoVersion = 0;
    newPano.loadPTScript(stream, ptoVersion, "");
    tmpp.setMemento(newPano);

    // create mapping between the panorama images.
    map<unsigned int, unsigned int> imgMapping;
    for (unsigned int ni = 0; ni < tmpp.getNrOfImages(); ni++) {
        std::string nname = stripPath(tmpp.getImage(ni).getFilename());
        for (unsigned int oi=0; oi < pano.getNrOfImages(); oi++) {
            std::string oname = stripPath(pano.getImage(oi).getFilename());
            if (nname == oname) {
                // insert image
                imgMapping[ni] = oi;
                break;
            }
        }
        if (! set_contains(imgMapping, ni)) {
            DEBUG_ERROR("Could not find image " << ni << ", name: " << tmpp.getImage(ni).getFilename() << " in autopano output");
            return CPVector();
        }
    }


    // get control points
    CPVector ctrlPoints = tmpp.getCtrlPoints();
    // make sure they are in correct order
    for (CPVector::iterator it= ctrlPoints.begin(); it != ctrlPoints.end(); ++it) {
        (*it).image1Nr = imgMapping[(*it).image1Nr];
        (*it).image2Nr = imgMapping[(*it).image2Nr];
    }

    return ctrlPoints;
}


CPVector AutoCtrlPointCreator::automatch(CPDetectorSetting &setting, 
                                         Panorama & pano,
                                         const UIntSet & imgs,
                                         int nFeatures,
                                         wxWindow *parent)
{
    CPVector cps;
    int t = setting.GetType();
    //check, if the cp generator exists
    wxFileName prog(setting.GetProg());
    bool canStart=false; 
    if(prog.IsAbsolute())
    {
        canStart=(prog.IsFileExecutable());
    }
    else
    {
        wxPathList pathlist;
        pathlist.AddEnvList(wxT("PATH"));
        wxString path = pathlist.FindAbsoluteValidPath(setting.GetProg());
        if(path.IsEmpty())
            canStart=false;
        else
        {
            wxFileName prog2(path);
            canStart=(prog2.IsFileExecutable());
        };
    };
    if(!canStart)
    {
        wxMessageBox(wxString::Format(
            _("Could not find \"%s\" in path.\nMaybe you have not installed it properly or given a wrong path in the settings."),setting.GetProg().c_str()),
            _("Error"),wxOK | wxICON_INFORMATION,parent);
        return cps;
    };
    switch (t) {
	case 0:
	{
	    // autopano@kolor
	    AutoPanoKolor matcher;
	    cps = matcher.automatch(setting, pano, imgs, nFeatures, parent);
	    break;
	}
	case 1:
	{
	    // autopano-sift
	    AutoPanoSift matcher;
	    cps = matcher.automatch(setting, pano, imgs, nFeatures, parent);
	    break;
	}
	default:
	    DEBUG_ERROR("Invalid autopano type");
    }
    return cps;
}

CPVector AutoPanoSift::automatch(CPDetectorSetting &setting, Panorama & pano, const UIntSet & imgs,
                                     int nFeatures, wxWindow *parent)
{
    CPVector cps;
    if (imgs.size() == 0) {
        return cps;
    }
    // create suitable command line..
    wxString autopanoExe = setting.GetProg();
    wxString autopanoArgs = setting.GetArgs();
    
#ifdef __WXMSW__
    // remember cwd.
    wxString cwd = wxGetCwd();
    wxString apDir = wxPathOnly(autopanoExe);
    if (apDir.Length() > 0) {
        wxSetWorkingDirectory(apDir);
    }
#endif

    // TODO: create a secure temporary filename here
    wxString ptofile = wxFileName::CreateTempFileName(wxT("ap_res"));
    autopanoArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    autopanoArgs.Replace(wxT("%p"), tmp);

    SrcPanoImage firstImg = pano.getSrcImage(*imgs.begin());
    tmp.Printf(wxT("%f"), firstImg.getHFOV());
    autopanoArgs.Replace(wxT("%v"), tmp);

    tmp.Printf(wxT("%d"), (int) firstImg.getProjection());
    autopanoArgs.Replace(wxT("%f"), tmp);

    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;

    long idx = autopanoArgs.Find(wxT("%namefile")) ;
    DEBUG_DEBUG("find %namefile in '"<< autopanoArgs.mb_str(wxConvLocal) << "' returned: " << idx);
    bool use_namefile = idx >=0;
    idx = autopanoArgs.Find(wxT("%i"));
    DEBUG_DEBUG("find %i in '"<< autopanoArgs.mb_str(wxConvLocal) << "' returned: " << idx);
    bool use_params = idx >=0;
    idx = autopanoArgs.Find(wxT("%s"));
    bool use_inputscript = idx >=0;

    if (! (use_namefile || use_params || use_inputscript)) {
        wxMessageBox(_("Please use  %namefile, %i or %s to specify the input files for control point detector"),
                     _("Error in control point detector command"), wxOK | wxICON_ERROR,parent);
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
        int imgNr=0;
        for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
        {
            imgMapping[imgNr] = *it;
            namefile.Write(wxString(pano.getImage(*it).getFilename().c_str(), HUGIN_CONV_FILENAME));
            namefile.Write(wxT("\r\n"));
            imgNr++;
        }
        // close namefile
        if (namefile_name != wxString(wxT(""))) {
            namefile.Close();
        }
    } else {
        string imgFiles;
        int imgNr=0;
        for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
        {
            imgMapping[imgNr] = *it;
            imgFiles.append(" ").append(quoteFilename(pano.getImage(*it).getFilename()));
            imgNr++;
        }
        autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), HUGIN_CONV_FILENAME));
    }

    wxString ptoinfile_name;
    if (use_inputscript) {
        wxFile ptoinfile;
        ptoinfile_name = wxFileName::CreateTempFileName(wxT("ap_inproj"));
        autopanoArgs.Replace(wxT("%s"), ptoinfile_name);

        ofstream ptoinstream(ptoinfile_name.mb_str(wxConvFile));
        pano.printPanoramaScript(ptoinstream, pano.getOptimizeVector(), pano.getOptions(), imgs, false);
    }

#ifdef __WXMSW__
    if (autopanoArgs.size() > 32000) {
        wxMessageBox(_("Command line for control point detector too long.\nThis is a windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"),
                     wxCANCEL | wxICON_ERROR, parent );
        return cps;
    }
#endif

    wxString cmd = autopanoExe + wxT(" ") + autopanoArgs;
    DEBUG_DEBUG("Executing: " << autopanoExe.mb_str(wxConvLocal) << " " << autopanoArgs.mb_str(wxConvLocal));

    wxArrayString arguments = wxCmdLineParser::ConvertStringToArgs(autopanoArgs);
    if (arguments.GetCount() > 127) {
        DEBUG_ERROR("Too many arguments for call to wxExecute()");
        DEBUG_ERROR("Try using the %s parameter in preferences");
        wxMessageBox(wxString::Format(_("Too many arguments (images). Try using the %%s parameter in preferences.\n\n Could not execute command: %s"), autopanoExe.c_str()), _("wxExecute Error"), wxOK | wxICON_ERROR, parent);
        return cps;
    }

    int ret = 0;
    // use MyExternalCmdExecDialog
    ret = MyExecuteCommandOnDialog(autopanoExe, autopanoArgs, parent,  _("finding control points"));

    if (ret == -1) {
        wxMessageBox( wxString::Format(_("Could not execute command: %s"),cmd.c_str()), _("wxExecute Error"), 
            wxOK | wxICON_ERROR, parent);
        return cps;
    } else if (ret > 0) {
        wxMessageBox(wxString::Format(_("Command: %s\nfailed with error code: %d"),cmd.c_str(),ret),
                     _("wxExecute Error"), wxOK | wxICON_ERROR, parent);
        return cps;
    }

    if (! wxFileExists(ptofile.c_str())) {
        wxMessageBox(wxString::Format(_("Could not open %s for reading\nThis is an indicator that the control point detector call failed,\nor wrong command line parameters have been used.\n\nExecuted command: %s"),ptofile.c_str(),cmd.c_str()),
                     _("Control point detector failure"), wxOK | wxICON_ERROR, parent );
        return cps;
    }

    // read and update control points
    cps = readUpdatedControlPoints((const char *)ptofile.mb_str(HUGIN_CONV_FILENAME), pano);

#ifdef __WXMSW__
	// set old cwd.
	wxSetWorkingDirectory(cwd);
#endif

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


CPVector AutoPanoKolor::automatch(CPDetectorSetting &setting, Panorama & pano, const UIntSet & imgs,
                              int nFeatures, wxWindow *parent)
{
    CPVector cps;
    wxString autopanoExe = setting.GetProg();

    // write default autopano.kolor.com flags
    wxString autopanoArgs = setting.GetArgs();

    // build a list of all image files, and a corrosponding connection map.
    // local img nr -> global (panorama) img number
    std::map<int,int> imgMapping;
    string imgFiles;
    int imgNr=0;
    for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
    {
        imgMapping[imgNr] = *it;
        imgFiles.append(" ").append(quoteFilename(pano.getImage(*it).getFilename()));
        imgNr++;
    }

    wxString ptofilepath = wxFileName::CreateTempFileName(wxT("ap_res"));
    wxFileName ptofn(ptofilepath);
    wxString ptofile = ptofn.GetFullName();
    autopanoArgs.Replace(wxT("%o"), ptofile);
    wxString tmp;
    tmp.Printf(wxT("%d"), nFeatures);
    autopanoArgs.Replace(wxT("%p"), tmp);
    SrcPanoImage firstImg = pano.getSrcImage(*imgs.begin());
    tmp.Printf(wxT("%f"), firstImg.getHFOV());
    autopanoArgs.Replace(wxT("%v"), tmp);

    tmp.Printf(wxT("%d"), (int) firstImg.getProjection());
    autopanoArgs.Replace(wxT("%f"), tmp);

    autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), HUGIN_CONV_FILENAME));

    wxString tempdir = ptofn.GetPath();
	autopanoArgs.Replace(wxT("%d"), ptofn.GetPath());
    wxString cmd;
    cmd.Printf(wxT("%s %s"), utils::wxQuoteFilename(autopanoExe).c_str(), autopanoArgs.c_str());
#ifdef __WXMSW__
    if (cmd.size() > 32766) {
        wxMessageBox(_("Command line for control point detector too long.\nThis is a windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"),
                     wxCANCEL, parent);
        return cps;
    }
#endif
    DEBUG_DEBUG("Executing: " << cmd.c_str());

    wxArrayString arguments = wxCmdLineParser::ConvertStringToArgs(cmd);
    if (arguments.GetCount() > 127) {
        DEBUG_ERROR("Too many arguments for call to wxExecute()");
        DEBUG_ERROR("Try using the %s parameter in preferences");
        wxMessageBox(wxString::Format(_("Too many arguments (images). Try using the %%s parameter in preferences.\n\n Could not execute command: %s"), autopanoExe.c_str()), _("wxExecute Error"), wxOK | wxICON_ERROR, parent);
        return cps;
    }

    int ret = 0;
    // use MyExternalCmdExecDialog
    ret = MyExecuteCommandOnDialog(autopanoExe, autopanoArgs, parent, _("finding control points"));

    if (ret == HUGIN_EXIT_CODE_CANCELLED) {
        return cps;
    } else if (ret == -1) {
        wxMessageBox( wxString::Format(_("Could not execute command: %s"),cmd.c_str()), _("wxExecute Error"), 
            wxOK | wxICON_ERROR, parent);
        return cps;
    } else if (ret > 0) {
        wxMessageBox(wxString::Format(_("Command: %s\nfailed with error code: %d"),cmd.c_str(),ret),
                     _("wxExecute Error"), wxOK | wxICON_ERROR, parent);
        return cps;
    }

    ptofile = ptofn.GetFullPath();
    ptofile.append(wxT("0.oto"));
    if (! wxFileExists(ptofile.c_str()) ) {
        wxMessageBox(wxString::Format(_("Could not open %s for reading\nThis is an indicator that the control point detector call failed,\nor wrong command line parameters have been used.\n\nExecuted command: %s"),ptofile.c_str(),cmd.c_str()),
                     _("Control point detector failure"), wxOK | wxICON_ERROR, parent );
        return cps;
    }
    // read and update control points
    cps = readUpdatedControlPoints((const char *)ptofile.mb_str(HUGIN_CONV_FILENAME), pano);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
    return cps;
}


