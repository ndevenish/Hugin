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

#include "PT/Panorama.h"

#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "hugin/AutoCtrlPointCreator.h"
#include "hugin/CommandHistory.h"

#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/platform.h"
#include "common/wxPlatform.h"
#include <wx/utils.h>

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
    newPano.loadPTScript(stream, "");
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


CPVector AutoCtrlPointCreator::automatch(Panorama & pano,
                                         const UIntSet & imgs,
                                         int nFeatures)
{
    CPVector cps;
    int t = wxConfigBase::Get()->Read(wxT("/AutoPano/Type"),HUGIN_AP_TYPE);
    if (t < 0) {

        wxString tmp[2];
    	tmp[0] = _("Autopano (version 1.03 or greater), from http://autopano.kolor.com");
    	tmp[1] = _("Autopano-Sift, from http://user.cs.tu-berlin.de/~nowozin/autopano-sift/");
    	// determine autopano type
    	wxSingleChoiceDialog d(NULL,  _("Choose which autopano program should be used\n"), _("Select autopano type"),
	   	 	       2, tmp, NULL);
        
        if (d.ShowModal() == wxID_OK) {
            t = d.GetSelection();
        } else {
            return cps;
        }
    }
    
#ifdef __WXMAC__
    if(t==0)
    {
        if(wxMessageBox(_("Autopano from http://autopano.kolor.com is not available for OSX"), 
                        _("Would you like to use Autopano-Sift instead?"),
                        wxOK|wxCANCEL|wxICON_EXCLAMATION)
           == wxOK) t=1;
        else return cps;
    }
#endif
    switch (t) {
	case 0:
	{
	    // autopano@kolor
	    AutoPanoKolor matcher;
	    cps = matcher.automatch(pano, imgs, nFeatures);
	    break;
	}
	case 1:
	{
	    // autopano-sift
	    AutoPanoSift matcher;
	    cps = matcher.automatch(pano, imgs, nFeatures);
	    break;
	}
	default:
	    DEBUG_ERROR("Invalid autopano type");
    }
    wxConfigBase::Get()->Write(wxT("/AutoPano/Type"),t);
    return cps;
}

CPVector AutoPanoSift::automatch(Panorama & pano, const UIntSet & imgs,
                                     int nFeatures)
{
    CPVector cps;
    if (imgs.size() == 0) {
        return cps;
    }
    // create suitable command line..

#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"), wxT(HUGIN_APSIFT_EXE));
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano program / frontend script"),
                         wxT(""), wxT("autopano-win32.exe"),
                         _("Executables (*.exe)|*.exe"),
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write(wxT("/AutopanoSift/AutopanoExe"),autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return cps;
        }
    }
#elif (defined __WXMAC__) && defined MAC_SELF_CONTAINED_BUNDLE
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"), wxT(HUGIN_APSIFT_EXE));
    
    //if the autopano-sift front end specified in preference does not exist:
    if (autopanoExe == wxT(HUGIN_APSIFT_EXE))
    {
        autopanoExe = MacGetPathToBundledResourceFile(CFSTR("autopano-complete-mac.sh"));

        //if the script exists inside the bundle (which should), then check if there is autopano-sift files inside bundle:
        if(autopanoExe != wxT(""))
        {
            wxString autopanoExeDir = MacGetPathToBundledResourceFile(CFSTR("autopano-sift"));

            //if they are not in bundle, then do not use the script included in the bundle
            if( autopanoExeDir == wxT("")
                || !wxFileExists(autopanoExeDir+wxT("/autopano.exe"))
                || !wxFileExists(autopanoExeDir+wxT("/generatekeys-sd.exe"))
                || !wxFileExists(autopanoExeDir+wxT("/libsift.dll")) )
            {
                wxMessageBox(wxT(""), _("Autopano-SIFT is not installed."));
                return cps;
            }
        }
    } else if(!wxFileExists(autopanoExe)) {
        wxFileDialog dlg(0,_("Select autopano frontend script"),
                         wxT(""), wxT(""),
                         _("Shell Scripts (*.sh)|*.sh"),
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write(wxT("/AutopanoSift/AutopanoExe"), autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return cps;
        }
    }
#else
    // autopano should be in the path on linux
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/AutopanoExe"),wxT(HUGIN_APSIFT_EXE));
#endif

    wxString autopanoArgs = wxConfigBase::Get()->Read(wxT("/AutoPanoSift/Args"),
                                                      wxT(HUGIN_APSIFT_ARGS));

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
    DEBUG_DEBUG("find %namefile in '"<< autopanoArgs.mb_str(*wxConvCurrent) << "' returned: " << idx);
    bool use_namefile = idx >=0;
    idx = autopanoArgs.Find(wxT("%i"));
    DEBUG_DEBUG("find %i in '"<< autopanoArgs.mb_str(*wxConvCurrent) << "' returned: " << idx);
    bool use_params = idx >=0;
    if (use_namefile && use_params) {
        wxMessageBox(_("Please use either %namefile or %i in the autopano-sift command line."),
                     _("Error in Autopano command"), wxOK | wxICON_ERROR);
        return cps;
    }
    if ((! use_namefile) && (! use_params)) {
        wxMessageBox(_("Please use  %namefile or %i to specify the input files for autopano-sift"),
                     _("Error in Autopano command"), wxOK | wxICON_ERROR);
        return cps;
    }

    wxFile namefile;
    wxString namefile_name;
    if (use_namefile) {
        // create temporary file with image names.
        namefile_name = wxFileName::CreateTempFileName(wxT("ap_imgnames"), &namefile);
        DEBUG_DEBUG("before replace %namefile: " << autopanoArgs.mb_str(*wxConvCurrent));
        autopanoArgs.Replace(wxT("%namefile"), namefile_name);
        DEBUG_DEBUG("after replace %namefile: " << autopanoArgs.mb_str(*wxConvCurrent));
        int imgNr=0;
        for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
        {
            imgMapping[imgNr] = *it;
            namefile.Write(wxString(pano.getImage(*it).getFilename().c_str(), *wxConvCurrent));
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
        autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), *wxConvCurrent));
    }
    
#ifdef __WXMSW__
    if (autopanoArgs.size() > 32000) {
        wxMessageBox(_("autopano command line too long.\nThis is a windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"),
                     wxCANCEL | wxICON_ERROR );
        return cps;
    }
#endif

    wxString cmd = autopanoExe + wxT(" ") + autopanoArgs;
    DEBUG_DEBUG("Executing: " << autopanoExe.mb_str(*wxConvCurrent) << " " << autopanoArgs.mb_str(*wxConvCurrent));

    int ret = 0;

    // use MyExternalCmdExecDialog
    ret = MyExecuteCommandOnDialog(autopanoExe, autopanoArgs, 0,  _("finding control points"));

    if (ret == -1) {
        wxMessageBox( _("Could not execute command: " + cmd), _("wxExecute Error"), wxOK | wxICON_ERROR);
        return cps;
    } else if (ret > 0) {
        wxMessageBox(_("command: ") + cmd +
                     _("\nfailed with error code: ") + wxString::Format(wxT("%d"),ret),
		     _("wxExecute Error"),
                     wxOK | wxICON_ERROR);
        return cps;
    }

    if (! wxFileExists(ptofile.c_str())) {
        wxMessageBox(wxString(_("Could not open ")) + ptofile + _(" for reading\nThis is an indicator that the autopano call failed,\nor wrong command line parameters have been used.\n\nAutopano command: ")
                     + cmd, _("autopano failure"), wxOK | wxICON_ERROR );
        return cps;
    }

    // read and update control points
    cps = readUpdatedControlPoints((const char *)ptofile.mb_str(*wxConvCurrent), pano);

#ifdef __WXMSW__
	// set old cwd.
	wxSetWorkingDirectory(cwd);
#endif

    if (namefile_name != wxString(wxT(""))) {
        namefile.Close();
        wxRemoveFile(namefile_name);
    }

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }

    return cps;
}


CPVector AutoPanoKolor::automatch(Panorama & pano, const UIntSet & imgs,
                              int nFeatures)
{
    CPVector cps;
#ifdef __WXMSW__
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/AutopanoExe"), wxT(HUGIN_APKOLOR_EXE));
    if (!wxFile::Exists(autopanoExe)){
        wxFileDialog dlg(0,_("Select autopano program"),
                         wxT(""), wxT("autopano.exe"),
                         _("Executables (*.exe)|*.exe"),
                         wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            autopanoExe = dlg.GetPath();
            wxConfigBase::Get()->Write(wxT("/AutoPanoKolor/AutopanoExe"),autopanoExe);
        } else {
            wxLogError(_("No autopano selected"));
            return cps;
        }
    }
#else
    // todo: selection of autopano on linux..
    wxString autopanoExe = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/AutopanoExe"),wxT(HUGIN_APKOLOR_EXE));
#endif

    // write default autopano.kolor.com flags
    wxString autopanoArgs = wxConfigBase::Get()->Read(wxT("/AutoPanoKolor/Args"),
                                                      wxT(HUGIN_APKOLOR_ARGS));

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

    autopanoArgs.Replace(wxT("%i"), wxString (imgFiles.c_str(), *wxConvCurrent));

    wxString tempdir = ptofn.GetPath();
	autopanoArgs.Replace(wxT("%d"), ptofn.GetPath());
    wxString cmd;
    cmd.Printf(wxT("%s %s"), utils::wxQuoteFilename(autopanoExe).c_str(), autopanoArgs.c_str());
#ifdef __WXMSW__
    if (cmd.size() > 32766) {
        wxMessageBox(_("autopano command line too long.\nThis is a windows limitation\nPlease select less images, or place the images in a folder with\na shorter pathname"),
                     _("Too many images selected"),
                     wxCANCEL );
        return cps;
    }
#endif
    DEBUG_DEBUG("Executing: " << cmd.c_str());

    int ret = 0;
    // use MyExternalCmdExecDialog
    ret = MyExecuteCommandOnDialog(autopanoExe, autopanoArgs, 0, _("finding control points"));

    if (ret == -1) {
        wxMessageBox( _("Could not execute command: " + cmd), _("wxExecute Error"),
                      wxOK | wxICON_ERROR);
        return cps;
    } else if (ret > 0) {
        wxMessageBox(_("command: ") + cmd +
                     _("\nfailed with error code: ") + wxString::Format(wxT("%d"),ret),
		     _("wxExecute Error"),
                     wxOK | wxICON_ERROR);
        return cps;
    }

    ptofile = ptofn.GetFullPath();
    ptofile.append(wxT("0.oto"));
    if (! wxFileExists(ptofile.c_str()) ) {
        wxMessageBox(wxString(_("Could not open ")) + ptofile + _(" for reading\nThis is an indicator that the autopano call failed,\nor wrong command line parameters have been used.\n\nAutopano command: ")
                     + cmd + _("\n current directory:") +
			         wxGetCwd(),
		             _("autopano failure"), wxCANCEL );
        return cps;
    }
    // read and update control points
    cps = readUpdatedControlPoints((const char *)ptofile.mb_str(*wxConvCurrent), pano);

    if (!wxRemoveFile(ptofile)) {
        DEBUG_DEBUG("could not remove temporary file: " << ptofile.c_str());
    }
    return cps;
}


