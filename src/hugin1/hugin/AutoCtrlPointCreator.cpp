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
#include "PT/ImageGraph.h"

#include "hugin/huginApp.h"
#include "hugin/config_defaults.h"
#include "hugin/AutoCtrlPointCreator.h"
#include "hugin/CommandHistory.h"
#include <algorithms/optimizer/PTOptimizer.h>

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


bool CanStartProg(wxString progName,wxWindow* parent)
{
    wxFileName prog(progName);
    bool canStart=false; 
    if(prog.IsAbsolute())
    {
        canStart=(prog.IsFileExecutable());
    }
    else
    {
        wxPathList pathlist;
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
        wxMessageBox(wxString::Format(
        _("Could not find \"%s\" in path.\nMaybe you have not installed it properly or given a wrong path in the settings."),progName.c_str()),
            _("Error"),wxOK | wxICON_INFORMATION,parent);
    return canStart;
};

CPVector AutoCtrlPointCreator::automatch(CPDetectorSetting &setting, PT::Panorama & pano, const PT::UIntSet & imgs,
                           int nFeatures, wxWindow *parent)
{
    int return_value;
    return automatch(setting,pano,imgs,nFeatures,return_value,parent);
};

CPVector AutoCtrlPointCreator::automatch(CPDetectorSetting &setting, 
                                         Panorama & pano,
                                         const UIntSet & imgs,
                                         int nFeatures,
                                         int & ret_value, 
                                         wxWindow *parent)
{
    CPVector cps;
    CPDetectorType t = setting.GetType();
    //check, if the cp generator exists
    if(!CanStartProg(setting.GetProg(),parent))
        return cps;
    if(t==CPDetector_AutoPanoSiftStack || t==CPDetector_AutoPanoSiftMultiRowStack)
        if(!setting.GetProgStack().IsEmpty())
            if(!CanStartProg(setting.GetProgStack(),parent))
                return cps;
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
    return cps;
}

CPVector AutoPanoSift::automatch(CPDetectorSetting &setting, Panorama & pano, const UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
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

    ret_value = 0;
    // use MyExternalCmdExecDialog
    ret_value = MyExecuteCommandOnDialog(autopanoExe, autopanoArgs, parent,  _("finding control points"));

    if (ret_value == HUGIN_EXIT_CODE_CANCELLED) {
        return cps;
    } else if (ret_value == -1) {
        wxMessageBox( wxString::Format(_("Could not execute command: %s"),cmd.c_str()), _("wxExecute Error"), 
            wxOK | wxICON_ERROR, parent);
        return cps;
    } else if (ret_value > 0) {
        wxMessageBox(wxString::Format(_("Command: %s\nfailed with error code: %d"),cmd.c_str(),ret_value),
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
                              int nFeatures, int & ret_value, wxWindow *parent)
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

    ret_value = 0;
    // use MyExternalCmdExecDialog
    ret_value = MyExecuteCommandOnDialog(autopanoExe, autopanoArgs, parent, _("finding control points"));

    if (ret_value == HUGIN_EXIT_CODE_CANCELLED) {
        return cps;
    } else if (ret_value == -1) {
        wxMessageBox( wxString::Format(_("Could not execute command: %s"),cmd.c_str()), _("wxExecute Error"), 
            wxOK | wxICON_ERROR, parent);
        return cps;
    } else if (ret_value > 0) {
        wxMessageBox(wxString::Format(_("Command: %s\nfailed with error code: %d"),cmd.c_str(),ret_value),
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

void AddControlPointsWithCheck(CPVector &cpv, CPVector &new_cp, Panorama *pano=NULL)
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

CPVector AutoPanoSiftStack::automatch(CPDetectorSetting &setting, Panorama & pano, const UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    CPVector cps;
    if (imgs.size() == 0) {
        return cps;
    };
    std::vector<stack_img> stack_images;
    HuginBase::StandardImageVariableGroups* variable_groups = new HuginBase::StandardImageVariableGroups(pano);
    for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
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
    UIntSet images_layer;
    for(unsigned int i=0;i<stack_images.size();i++)
    {
        std::sort(stack_images[i].images.begin(),stack_images[i].images.end(),sort_img_ev);
        unsigned int median=stack_images[i].images.size() / 2;
        images_layer.insert(stack_images[i].images[median].img_nr);
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
            UIntSet images_stack;
            images_stack.clear();
            for(unsigned int j=0;j<stack_images[i].images.size();j++)
                images_stack.insert(stack_images[i].images[j].img_nr);
            if(images_stack.size()>1)
            {
                AutoPanoSift matcher;
                CPVector new_cps=matcher.automatch(stack_setting, pano, images_stack, nFeatures, ret_value, parent);
                if(new_cps.size()>0)
                    AddControlPointsWithCheck(cps,new_cps);
                if(ret_value!=0)
                    return cps;
            };
        };
    }
    return cps;
};

CPVector AutoPanoSiftMultiRow::automatch(CPDetectorSetting &setting, Panorama & pano, const UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    CPVector cps;
    if (imgs.size() < 2) 
    {
        return cps;
    };
    //generate cp for every consecutive image pair
    unsigned int counter=0;
    for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); )
    {
        if(counter==imgs.size()-1)
            break;
        counter++;
        UIntSet ImagePair;
        ImagePair.clear();
        ImagePair.insert(*it);
        it++;
        ImagePair.insert(*it);
        AutoPanoSift matcher;
        CPVector new_cps;
        new_cps.clear();
        new_cps=matcher.automatch(setting, pano, ImagePair, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps);
        if(ret_value!=0)
            return cps;
    };
    // now connect all image groups
    // generate temporary panorama to add all found cps
    UIntSet allImgs;
    fill_set(allImgs, 0, pano.getNrOfImages()-1);
    Panorama optPano=pano.getSubset(allImgs);
    for (CPVector::const_iterator it=cps.begin();it!=cps.end();++it)
        optPano.addCtrlPoint(*it);

    CPGraph graph;
    createCPGraph(optPano, graph);
    CPComponents comps;
    int n = findCPComponents(graph, comps);
    if(n>1)
    {
        UIntSet ImagesGroups;
        for(unsigned int i=0;i<n;i++)
        {
            ImagesGroups.insert(*(comps[i].begin()));
            if(comps[i].size()>1)
                ImagesGroups.insert(*(comps[i].rbegin()));
        };
        AutoPanoSift matcher;
        CPVector new_cps;
        new_cps=matcher.automatch(setting, optPano, ImagesGroups, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps,&optPano);
        if(ret_value!=0)
            return cps;
        createCPGraph(optPano,graph);
        n=findCPComponents(graph, comps);
    };
    if(n==1 && setting.GetOption())
    {
        //next steps happens only when all images are connected;
        //now optimize panorama
        PanoramaOptions opts = pano.getOptions();
        opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
        // calculate proper scaling, 1:1 resolution.
        // Otherwise optimizer distances are meaningless.
        opts.setWidth(30000, false);
        opts.setHeight(15000);

        optPano.setOptions(opts);
        int w = optPano.calcOptimalWidth();
        opts.setWidth(w);
        opts.setHeight(w/2);
        optPano.setOptions(opts);

        //generate optimize vector, optimize only yaw and pitch
        OptimizeVector optvars;
        const SrcPanoImage & anchorImage = optPano.getImage(opts.optimizeReferenceImage);
        for (unsigned i=0; i < optPano.getNrOfImages(); i++) 
        {
            std::set<std::string> imgopt;
            // do not optimize anchor image's stack for position.
            if(!optPano.getImage(i).YawisLinkedWith(anchorImage))
            {
                imgopt.insert("p");
                imgopt.insert("y");
            }
            optvars.push_back(imgopt);
        }
        optPano.setOptimizeVector(optvars);
        HuginBase::PTools::optimize(optPano);
        //and find cp on overlapping images
        //work only on image pairs, which are not yet connected
        AutoPanoSiftPreAlign matcher;
        CPDetectorSetting newSetting;
        newSetting.SetProg(setting.GetProg());
        newSetting.SetArgs(setting.GetArgs());
        newSetting.SetOption(true);
        CPVector new_cps;
        new_cps=matcher.automatch(newSetting, optPano, imgs, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps);
    };
    return cps;
};

CPVector AutoPanoSiftMultiRowStack::automatch(CPDetectorSetting &setting, Panorama & pano, const UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    CPVector cps;
    if (imgs.size() == 0) {
        return cps;
    };
    std::vector<stack_img> stack_images;
    HuginBase::StandardImageVariableGroups* variable_groups = new HuginBase::StandardImageVariableGroups(pano);
    for(UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); it++)
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
    UIntSet images_layer;
    for(unsigned int i=0;i<stack_images.size();i++)
    {
        std::sort(stack_images[i].images.begin(),stack_images[i].images.end(),sort_img_ev);
        unsigned int median=stack_images[i].images.size() / 2;
        images_layer.insert(stack_images[i].images[median].img_nr);
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
            UIntSet images_stack;
            images_stack.clear();
            for(unsigned int j=0;j<stack_images[i].images.size();j++)
                images_stack.insert(stack_images[i].images[j].img_nr);
            if(images_stack.size()>1)
            {
                AutoPanoSift matcher;
                CPVector new_cps=matcher.automatch(stack_setting, pano, images_stack, nFeatures, ret_value, parent);
                if(new_cps.size()>0)
                    AddControlPointsWithCheck(cps,new_cps);
                if(ret_value!=0)
                    return cps;
            };
        };
    }
    //generate cp for median exposure with multi-row algorithm
    if(images_layer.size()>1)
    {
        UIntSet allImgs;
        fill_set(allImgs, 0, pano.getNrOfImages()-1);
        Panorama newPano=pano.getSubset(allImgs);
        if(cps.size()>0)
            for (CPVector::const_iterator it=cps.begin();it!=cps.end();++it)
                newPano.addCtrlPoint(*it);

        AutoPanoSiftMultiRow matcher;
        CPVector new_cps=matcher.automatch(setting, newPano, images_layer, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps);
    };
    return cps;
};

CPVector AutoPanoSiftPreAlign::automatch(CPDetectorSetting &setting, Panorama & pano, const UIntSet & imgs,
                                     int nFeatures, int & ret_value, wxWindow *parent)
{
    CPVector cps;
    if (imgs.size()<2) 
        return cps;
    vector<UIntSet> usedImages;
    usedImages.resize(pano.getNrOfImages());
    if(setting.GetOption())
    {
        //only work on not connected image pairs
        CPVector oldCps=pano.getCtrlPoints();
        for(unsigned i=0;i<oldCps.size();i++)
        {
            if(oldCps[i].mode==ControlPoint::X_Y)
            {
                usedImages[oldCps[i].image1Nr].insert(oldCps[i].image2Nr);
                usedImages[oldCps[i].image2Nr].insert(oldCps[i].image1Nr);
            };
        };
    };
    for(UIntSet::const_iterator it=imgs.begin();it!=imgs.end();it++)
    {
        UIntSet images;
        images.clear();
        images.insert(*it);
        SrcPanoImage simg = pano.getSrcImage(*it);
        double maxShift = simg.getHFOV();
        double minShiftYaw = 360.0 - maxShift;
        double minShiftPitch = 180.0 - maxShift;
        UIntSet::const_iterator it2=it;
        for(++it2;it2!=imgs.end();it2++)
        {
            //check if this image pair was yet used
            if(set_contains(usedImages[*it2],*it))
                continue;
            //now check position
            SrcPanoImage simg2 = pano.getSrcImage(*it2);
            double diffYaw=fabs(simg.getYaw()-simg2.getYaw());
            double diffPitch=fabs(simg.getPitch()-simg2.getPitch());
            if((diffYaw<maxShift || diffYaw>minShiftYaw) && (diffPitch<maxShift || diffPitch>minShiftPitch))
            {
                images.insert(*it2);
            };
        };
        if(images.size()<2)
            continue;
        //remember image pairs for later
        for(UIntSet::const_iterator img_it=images.begin();img_it!=images.end();img_it++)
            for(UIntSet::const_iterator img_it2=images.begin();img_it2!=images.end();img_it2++)
                usedImages[*img_it].insert(*img_it2);
        AutoPanoSift matcher;
        CPVector new_cps=matcher.automatch(setting, pano, images, nFeatures, ret_value, parent);
        if(new_cps.size()>0)
            AddControlPointsWithCheck(cps,new_cps);
        if(ret_value!=0)
            return cps;
    };
    return cps;
};
