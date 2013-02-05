// -*- c-basic-offset: 4 -*-

/** @file PanoOperation.cpp
 *
 *  @brief Implementation of PanoOperation class
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

#include "hugin/PanoOperation.h"
#include "hugin/config_defaults.h"
#include "hugin/wxPanoCommand.h"
#include "huginapp/ImageCache.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/PTWXDlg.h"
#include "algorithms/optimizer/ImageGraph.h"
#include "algorithms/control_points/CleanCP.h"
#include "celeste/Celeste.h"
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include "base_wx/LensTools.h"
#include "base_wx/wxLensDB.h"
#include "hugin/ResetDialog.h"
#include "hugin/MainFrame.h"

using namespace HuginBase;

namespace PanoOperation
{

wxString PanoOperation::GetLabel()
{
    return wxEmptyString;
};

bool PanoOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    return true;
};

PT::PanoCommand* PanoOperation::GetCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    if(IsEnabled(pano,images))
    {
        return GetInternalCommand(parent,pano,images);
    }
    else
    {
        return NULL;
    };
};

bool PanoSingleImageOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    return images.size()==1;
};

bool PanoMultiImageOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    return images.size()>0;
};

/** small function to show add image dialog
  * @param parent pointer to window, for showing dialog
  * @param files vector, to which the selected valid filenames will be added
  * @returns true, if a valid image was selected, otherwise false
  */
bool AddImageDialog(wxWindow* parent, std::vector<std::string>& files)
{
    // get stored path
    wxConfigBase* config = wxConfigBase::Get();
    wxString path = config->Read(wxT("/actualPath"), wxT(""));
    wxFileDialog dlg(parent,_("Add images"),
                     path, wxT(""),
                     HUGIN_WX_FILE_IMG_FILTER,
                     wxFD_OPEN | wxFD_MULTIPLE, wxDefaultPosition);
    dlg.SetDirectory(path);

    // remember the image extension
    wxString img_ext;
    if (config->HasEntry(wxT("lastImageType")))
    {
      img_ext = config->Read(wxT("lastImageType")).c_str();
    }
    if (img_ext == wxT("all images"))
      dlg.SetFilterIndex(0);
    else if (img_ext == wxT("jpg"))
      dlg.SetFilterIndex(1);
    else if (img_ext == wxT("tiff"))
      dlg.SetFilterIndex(2);
    else if (img_ext == wxT("png"))
      dlg.SetFilterIndex(3);
    else if (img_ext == wxT("hdr"))
      dlg.SetFilterIndex(4);
    else if (img_ext == wxT("exr"))
      dlg.SetFilterIndex(5);
    else if (img_ext == wxT("all files"))
      dlg.SetFilterIndex(6);

    // call the file dialog
    if (dlg.ShowModal() == wxID_OK)
    {
        // get the selections
        wxArrayString Pathnames;
        dlg.GetPaths(Pathnames);

        // remember path for later
#ifdef __WXGTK__
        //workaround a bug in GTK, see https://bugzilla.redhat.com/show_bug.cgi?id=849692 and http://trac.wxwidgets.org/ticket/14525
        config->Write(wxT("/actualPath"), wxPathOnly(Pathnames[0]));
#else
        config->Write(wxT("/actualPath"), dlg.GetDirectory());
#endif
        // save the image extension
        switch (dlg.GetFilterIndex())
        {
            case 0: config->Write(wxT("lastImageType"), wxT("all images")); break;
            case 1: config->Write(wxT("lastImageType"), wxT("jpg")); break;
            case 2: config->Write(wxT("lastImageType"), wxT("tiff")); break;
            case 3: config->Write(wxT("lastImageType"), wxT("png")); break;
            case 4: config->Write(wxT("lastImageType"), wxT("hdr")); break;
            case 5: config->Write(wxT("lastImageType"), wxT("exr")); break;
            case 6: config->Write(wxT("lastImageType"), wxT("all files")); break;
        }

        //check for forbidden/non working chars
        wxArrayString invalidFiles;
        for(unsigned int i=0;i<Pathnames.GetCount(); i++)
        {
           if(containsInvalidCharacters(Pathnames[i]))
           {
               invalidFiles.Add(Pathnames[i]);
           };
        };
        if(invalidFiles.size()>0)
        {
            ShowFilenameWarning(parent, invalidFiles);
            return false;
        }
        for (unsigned int i=0; i<Pathnames.GetCount(); i++)
        {
            files.push_back((const char *)Pathnames[i].mb_str(HUGIN_CONV_FILENAME));
        };
        return true;
    };
    return false;
};

wxString AddImageOperation::GetLabel()
{
    return _("Add individual images...");
};

PT::PanoCommand* AddImageOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    std::vector<std::string> files;
    if(AddImageDialog(parent, files))
    {
        if(files.size()>0)
        {
            return new PT::wxAddImagesCmd(pano,files);
        };
    };
    return NULL;
};

WX_DECLARE_STRING_HASH_MAP(time_t, StringToPointerHash);
WX_DECLARE_STRING_HASH_MAP(int, StringToFlagHash);

time_t ReadExifTime(const char* filename)
{
    Exiv2::Image::AutoPtr image;
    try
    {
        image = Exiv2::ImageFactory::open(filename);
    }
    catch(...)
    {
        return 0;
    }
    if (image.get() == 0)
    {
        return 0;
    }

    image->readMetadata();
    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty())
    {
        return 0;
    }

    Exiv2::Exifdatum& tag = exifData["Exif.Image.DateTime"];
    const std::string date_time = tag.toString();

    // Remember the file and a shutter timestamp.
    struct tm when;
    memset(&when, 0, sizeof(when));
    when.tm_wday = -1;

    // parse into the tm_structure
    const int a = sscanf(date_time.c_str(), "%d:%d:%d %d:%d:%d",
            &when.tm_year, &when.tm_mon, &when.tm_mday,
            &when.tm_hour, &when.tm_min, &when.tm_sec);

    if (a == 6)
    {
        when.tm_isdst = -1;
        when.tm_mon -= 1;      // Adjust for unix zero-based months
        when.tm_year -= 1900;  // Adjust for year starting at 1900
    }
    else
    {
        // Not in EXIF format
        return 0;
    }

    time_t stamp;
    stamp = mktime(&when);
    if (stamp == (time_t)(-1))
        return 0;

    return stamp;
}

struct sortbytime
{
    sortbytime(map<string, time_t> & h) : m_time(h) {};
    bool operator()(const std::string & s1, const std::string & s2)
    {
        time_t t1 = m_time[s1];
        time_t t2 = m_time[s2];
        return t1 < t2;
    };
    map<string, time_t> & m_time;
};

wxString AddImagesSeriesOperation::GetLabel()
{
    return _("Add time-series of images...");
};

PT::PanoCommand* AddImagesSeriesOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    //load image if pano contains no images
    std::vector<std::string> files;
    if(pano.getNrOfImages()==0)
    {
        if(!AddImageDialog(parent,files))
        {
            return NULL;
        };
        //just in case
        if(files.size()==0)
        {
            return NULL;
        };
    }
    else
    {
        for(size_t i=0;i<pano.getNrOfImages();i++)
        {
            files.push_back(pano.getImage(i).getFilename());
        };
    };

    DEBUG_TRACE("seeking similarly timed images");

    // Collect potential image-mates.
    StringToPointerHash filenames;
    StringToFlagHash preloaded;
    for(size_t i=0;i<files.size();i++)
    {
        wxString file(files[i].c_str(), HUGIN_CONV_FILENAME);
        preloaded[file] = 1;

        // Glob for all files of same type in same directory.
        wxString path = ::wxPathOnly(file) + wxT("/*");
        file = ::wxFindFirstFile(path);
        while (!file.IsEmpty())
        {
            // Associated with a NULL dummy timestamp for now.
            if(vigra::isImage(files[i].c_str()))
            {
                filenames[file] = 0;
            };
            file = ::wxFindNextFile();
        }
    }

    DEBUG_INFO("found " << filenames.size() << " candidate files to search.");

    // For each globbed or loaded file,
    StringToPointerHash::iterator found;
    std::map<std::string, time_t> timeMap;
    for (found = filenames.begin(); found != filenames.end(); found++)
    {
        wxString file = found->first;
        // Check the time if it's got a camera EXIF timestamp.
        time_t stamp = ReadExifTime(file.mb_str(HUGIN_CONV_FILENAME));
        if (stamp)
        {
            filenames[file] = stamp;
            timeMap[(const char *)file.mb_str(HUGIN_CONV_FILENAME)] = stamp;
        }
    }

    //TODO: sorting the filenames keys by timestamp would be useful
    int maxtimediff = wxConfigBase::Get()->Read(wxT("CaptureTimeSpan"), HUGIN_CAPTURE_TIMESPAN);
    // For each timestamped file,
    for (found = filenames.begin(); found != filenames.end(); found++)
    {
        wxString recruit = found->first;
        if (preloaded[recruit] == 1)
            continue;
        time_t pledge = filenames[recruit];
        if (!pledge)
            continue;

        // For each other image already loaded,
        for(size_t i=0;i<files.size();i++)
        {
            wxString file(files[i].c_str(), HUGIN_CONV_FILENAME);
            if (file == recruit)
                continue;

            // If it is within threshold time,
            time_t stamp = filenames[file];
            if (abs((int)(pledge - stamp)) < maxtimediff)
            {
                // Load this file, and remember it.
                DEBUG_TRACE("Recruited " << recruit.mb_str(wxConvLocal));
                std::string file = (const char *)recruit.mb_str(HUGIN_CONV_FILENAME);
                files.push_back(file);
                // Don't recruit it again.
                filenames[recruit] = 0;
                break;
            }
        }
    }

    if(files.size()>0)
    {
        // sort files by date
        sortbytime spred(timeMap);
        sort(files.begin(), files.end(), spred);
        // Load all of the named files.
        return new PT::wxAddImagesCmd(pano,files);
    }
    else
    {
        wxMessageBox(
            _("No matching images found."),
#ifdef _WINDOWS
            _("Hugin"),
#else
            wxT(""),
#endif
            wxOK | wxICON_INFORMATION, parent);
        return NULL;
    };
};

wxString RemoveImageOperation::GetLabel()
{
    return _("Remove selected image(s)");
};

PT::PanoCommand* RemoveImageOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    //remove images from cache
    for (UIntSet::iterator it = images.begin(); it != images.end(); ++it)
    {
        ImageCache::getInstance().removeImage(pano.getImage(*it).getFilename());
    }
    return new PT::RemoveImagesCmd(pano, images);
};

wxString ChangeAnchorImageOperation::GetLabel()
{
    return _("Anchor this image for position");
};

PT::PanoCommand* ChangeAnchorImageOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    PanoramaOptions opt = pano.getOptions();
    opt.optimizeReferenceImage = *(images.begin());
    return new PT::SetPanoOptionsCmd(pano,opt);
};

wxString ChangeColorAnchorImageOperation::GetLabel()
{
    return _("Anchor this image for exposure");
};

PT::PanoCommand* ChangeColorAnchorImageOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    PanoramaOptions opt = pano.getOptions();
    opt.colorReferenceImage = *(images.begin());
    // Set the color correction mode so that the anchor image is persisted
    if (opt.colorCorrection == 0)
    {
        opt.colorCorrection = (PanoramaOptions::ColorCorrection) 1;
    }
    return new PT::SetPanoOptionsCmd(pano, opt);
};

bool NewLensOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    if(pano.getNrOfImages()==0 || images.size()==0)
    {
        return false;
    }
    else
    {
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        return variable_groups.getLenses().getNumberOfParts()<pano.getNrOfImages();
    };
};

wxString NewLensOperation::GetLabel()
{
    return _("New lens");
};

PT::PanoCommand* NewLensOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    return new PT::NewPartCmd(pano, images, HuginBase::StandardImageVariableGroups::getLensVariables());
};

bool ChangeLensOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    if(pano.getNrOfImages()==0 || images.size()==0)
    {
        return false;
    }
    else
    {
        //project must have more than 1 lens before you can assign an other lens number
        HuginBase::StandardImageVariableGroups variableGroups(pano);
        return variableGroups.getLenses().getNumberOfParts() > 1;
    };
};

wxString ChangeLensOperation::GetLabel()
{
    return _("Change lens...");
};

PT::PanoCommand* ChangeLensOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    HuginBase::StandardImageVariableGroups variable_groups(pano);
    long nr = wxGetNumberFromUser(
                            _("Enter new lens number"),
                            _("Lens number"),
                            _("Change lens number"), 0, 0,
                            variable_groups.getLenses().getNumberOfParts()-1
                                 );
    if (nr >= 0)
    {
        // user accepted
        return new PT::ChangePartNumberCmd(pano, images, nr, HuginBase::StandardImageVariableGroups::getLensVariables());
    }
    else
    {
        return NULL;
    };
};

LoadLensOperation::LoadLensOperation(bool fromLensfunDB)
{
    m_fromLensfunDB=fromLensfunDB;
};

wxString LoadLensOperation::GetLabel()
{
    if(m_fromLensfunDB)
    {
        return _("Load lens from Lensfun database");
    }
    else
    {
        return _("Load lens from ini file");
    };
};

PT::PanoCommand* LoadLensOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    HuginBase::StandardImageVariableGroups variable_groups(pano);
    if(images.size()==1)
    {
        if(wxMessageBox(_("You selected only one image.\nShould the loaded parameters be applied to all images with the same lens?"),_("Question"), wxICON_QUESTION | wxYES_NO)==wxYES)
        {
            unsigned int lensNr = variable_groups.getLenses().getPartNumber(*images.begin());
            // get all images with the current lens.
            for (size_t i = 0; i < pano.getNrOfImages(); i++)
            {
                if (variable_groups.getLenses().getPartNumber(i) == lensNr)
                {
                    images.insert(i);
                };
            };
        };
    };
    vigra::Size2D sizeImg0=pano.getImage(*(images.begin())).getSize();
    //check if all images have the same size
    bool differentImageSize=false;
    for(UIntSet::const_iterator it=images.begin();it!=images.end() && !differentImageSize;it++)
    {
        differentImageSize=(pano.getImage(*it).getSize()!=sizeImg0);
    };
    if(differentImageSize)
    {
        if(wxMessageBox(_("You selected images with different sizes.\nApply lens parameter file can result in unwanted results.\nApply settings anyway?"), _("Error"), wxICON_QUESTION |wxYES_NO)==wxID_NO)
        {
            return NULL;
        };
    };
    PT::PanoCommand* cmd=NULL;
    bool isLoaded=false;
    if(m_fromLensfunDB)
    {
        isLoaded=ApplyLensDBParameters(parent,&pano,images,cmd);
    }
    else
    {
        isLoaded=ApplyLensParameters(parent,&pano,images,cmd);
    };
    if(isLoaded)
    {
        return cmd;
    }
    else
    {
        return NULL;
    }
};

SaveLensOperation::SaveLensOperation(int lensInfo)
{
    m_lensInfo=lensInfo;
};

wxString SaveLensOperation::GetLabel()
{
    switch(m_lensInfo)
    {
        case 0:
            return _("Save lens to ini file");
            break;
        case 1:
            return _("Save lens parameters to lensfun database");
            break;
        case 2:
            return _("Save camera parameters to lensfun database");
            break;
    }
    return wxEmptyString;
};

PT::PanoCommand* SaveLensOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    unsigned int imgNr = *(images.begin());
    switch(m_lensInfo)
    {
        case 1:
            SaveLensParameters(parent,pano.getImage(imgNr));
            break;
        case 2:
            SaveCameraCropFactor(parent,pano.getImage(imgNr));
            break;
        case 0:
        default:
            SaveLensParametersToIni(parent, &pano, images);
            break;
    };
    return NULL;
};

wxString RemoveControlPointsOperation::GetLabel()
{
    return _("Remove control points");
};

bool RemoveControlPointsOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    return pano.getNrOfImages()>0 && pano.getNrOfCtrlPoints()>0;
};

PT::PanoCommand* RemoveControlPointsOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    UIntSet selImages;
    if(images.size()==0)
    {
        fill_set(selImages,0,pano.getNrOfCtrlPoints()-1);
    }
    else
    {
        selImages=images;
    };
    UIntSet cpsToDelete;
    const CPVector & cps = pano.getCtrlPoints();
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it)
    {
        if (set_contains(selImages, (*it).image1Nr) && set_contains(selImages, (*it).image2Nr) )
        {
            cpsToDelete.insert(it - cps.begin());
        }
    }
    if(cpsToDelete.size()==0)
    {
        wxMessageBox(_("Selected images have no control points."),
#ifdef __WXMSW__
            wxT("Hugin"),
#else
            wxT(""),
#endif
            wxICON_EXCLAMATION | wxOK);
        return NULL;
    };
    int r =wxMessageBox(wxString::Format(_("Really delete %lu control points?"),
                                         (unsigned long int) cpsToDelete.size()),
                        _("Delete Control Points"),
                        wxICON_QUESTION | wxYES_NO);
    if (r == wxYES)
    {
        return new PT::RemoveCtrlPointsCmd(pano, cpsToDelete );
    }
    else
    {
        return NULL;
    };
};

wxString CleanControlPointsOperation::GetLabel()
{
    return _("Clean control points");
};

bool CleanControlPointsOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    return pano.getNrOfCtrlPoints()>2;
};

PT::PanoCommand* CleanControlPointsOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    deregisterPTWXDlgFcn();
    // work around a flaw in wxProgresDialog that results in incorrect layout
    // by pre-allocting sufficient horizontal space
    ProgressReporterDialog progress(2, _("Cleaning Control points"), _("Checking pairwise")+wxString((wxChar)' ',10),parent, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME);
    UIntSet removedCPs=getCPoutsideLimit_pair(pano,2.0);

    //create a copy to work with
    //we copy remaining control points to new pano object for running second step
    HuginBase::Panorama newPano=pano.duplicate();
    std::map<size_t,size_t> cpMap;
    CPVector allCPs=newPano.getCtrlPoints();
    CPVector firstCleanedCP;
    size_t j=0;
    for(size_t i=0;i<allCPs.size();i++)
    {
        ControlPoint cp=allCPs[i];
        if(cp.mode==ControlPoint::X_Y && !set_contains(removedCPs,i))
        {
            firstCleanedCP.push_back(cp);
            cpMap[j++]=i;
        };
    };
    newPano.setCtrlPoints(firstCleanedCP);

    //check for unconnected images
    CPGraph graph;
    createCPGraph(newPano, graph);
    CPComponents comps;
    int n=findCPComponents(graph, comps);
    progress.increaseProgress(1, std::wstring(wxString(_("Checking whole project")).wc_str(wxConvLocal)));
    if (n <= 1)
    {
        //now run the second step
        UIntSet removedCP2=getCPoutsideLimit(newPano,2.0);
        if(removedCP2.size()>0)
        {
            for(UIntSet::const_iterator it=removedCP2.begin();it!=removedCP2.end();it++)
            {
                removedCPs.insert(cpMap[*it]);
            };
        };
    }
    progress.increaseProgress(1, std::wstring(wxString(_("Finished cleaning")).wc_str(wxConvLocal)));
    registerPTWXDlgFcn(MainFrame::Get());
    if(removedCPs.size()>0)
    {
        wxMessageBox(wxString::Format(_("Removed %lu control points"), removedCPs.size()), _("Cleaning"),wxOK|wxICON_INFORMATION,parent);
        return new PT::RemoveCtrlPointsCmd(pano,removedCPs);
    };
    return NULL;
};

wxString CelesteOperation::GetLabel()
{
    return _("Remove control points on clouds");
};

PT::PanoCommand* CelesteOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    ProgressReporterDialog progress(images.size()+2, _("Running Celeste"), _("Running Celeste"),parent);
    MainFrame::Get()->SetStatusText(_("searching for cloud-like control points..."),0);
    progress.increaseProgress(1.0, std::wstring(wxString(_("Loading model file")).wc_str(wxConvLocal)));

    struct celeste::svm_model* model=MainFrame::Get()->GetSVMModel();
    if(model==NULL)
    {
        MainFrame::Get()->SetStatusText(wxT(""),0);
        return NULL;
    };

    // Get Celeste parameters
    wxConfigBase *cfg = wxConfigBase::Get();
    // SVM threshold
    double threshold = HUGIN_CELESTE_THRESHOLD;
    cfg->Read(wxT("/Celeste/Threshold"), &threshold, HUGIN_CELESTE_THRESHOLD);

    // Mask resolution - 1 sets it to fine
    bool t = (cfg->Read(wxT("/Celeste/Filter"), HUGIN_CELESTE_FILTER) == 0);
    int radius=(t)?10:20;
    DEBUG_TRACE("Running Celeste");

    UIntSet cpsToRemove;
    for (UIntSet::const_iterator it=images.begin(); it!=images.end(); it++)
    {
        // Image to analyse
        HuginBase::CPointVector cps=pano.getCtrlPointsVectorForImage(*it);
        if(cps.size()==0)
        {
            progress.increaseProgress(1.0, std::wstring(wxString(_("Running Celeste")).wc_str(wxConvLocal)));
            continue;
        };
        ImageCache::EntryPtr img=ImageCache::getInstance().getImage(pano.getImage(*it).getFilename());
        vigra::UInt16RGBImage in;
        if(img->image16->width()>0)
        {
            in.resize(img->image16->size());
            vigra::copyImage(srcImageRange(*(img->image16)),destImage(in));
        }
        else
        {
            ImageCache::ImageCacheRGB8Ptr im8=img->get8BitImage();
            in.resize(im8->size());
            vigra::transformImage(srcImageRange(*im8),destImage(in),vigra::functor::Arg1()*vigra::functor::Param(65535/255));
        };
        UIntSet cloudCP=celeste::getCelesteControlPoints(model,in,cps,radius,threshold,800);
        in.resize(0,0);
        if(cloudCP.size()>0)
        {
            for(UIntSet::const_iterator it2=cloudCP.begin();it2!=cloudCP.end();it2++)
            {
                cpsToRemove.insert(*it2);
            };
        };
        progress.increaseProgress(1.0, std::wstring(wxString(_("Running Celeste")).wc_str(wxConvLocal)));
    };

    progress.increaseProgress(1.0, std::wstring(wxString(_("Running Celeste")).wc_str(wxConvLocal)));
    if(cpsToRemove.size()>0)
    {
        wxMessageBox(wxString::Format(_("Removed %lu control points"), (unsigned long int) cpsToRemove.size()), _("Celeste result"),wxOK|wxICON_INFORMATION);
        MainFrame::Get()->SetStatusText(wxT(""),0);
        return new PT::RemoveCtrlPointsCmd(pano,cpsToRemove);
    }
    else
    {
        MainFrame::Get()->SetStatusText(wxT(""),0);
        return NULL;
    };
};

ResetOperation::ResetOperation(ResetMode newResetMode)
{
    m_resetMode=newResetMode;
    m_resetPos=(m_resetMode==RESET_POSITION);
    m_resetHFOV=(m_resetMode==RESET_LENS);
    m_resetLens=(m_resetMode==RESET_LENS);
    m_resetExposure=0;
    if(m_resetMode==RESET_PHOTOMETRICS)
    {
        m_resetExposure=1;
    };
    m_resetVignetting=(m_resetMode==RESET_PHOTOMETRICS);
    m_resetColor=(m_resetMode==RESET_PHOTOMETRICS);
    m_resetCameraResponse=(m_resetMode==RESET_PHOTOMETRICS);
};

wxString ResetOperation::GetLabel()
{
    switch(m_resetMode)
    {
        case RESET_DIALOG:
        case RESET_DIALOG_LENS:
        case RESET_DIALOG_PHOTOMETRICS:
            return _("Reset user defined...");
            break;
        case RESET_POSITION:
            return _("Reset positions");
            break;
        case RESET_LENS:
            return _("Reset lens parameters");
            break;
        case RESET_PHOTOMETRICS:
            return _("Reset photometric parameters");
    };
    return wxEmptyString;
};

bool ResetOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    return pano.getNrOfImages()>0;
};

PT::PanoCommand* ResetOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    if(m_resetMode==RESET_DIALOG || m_resetMode==RESET_DIALOG_LENS || m_resetMode==RESET_DIALOG_PHOTOMETRICS)
    {
        if(!ShowDialog(parent))
        {
            return NULL;
        };
    };
    if(images.size()==0)
    {
        fill_set(images,0,pano.getNrOfImages()-1);
    };
    // If we should unlink exposure value (to load it from EXIF)
    bool needs_unlink = false;
    VariableMapVector vars;
    for(UIntSet::const_iterator it = images.begin(); it != images.end(); it++)
    {
        unsigned int imgNr = *it;
        VariableMap ImgVars=pano.getImageVariables(imgNr);
        if(m_resetPos)
        {
            map_get(ImgVars,"y").setValue(0);
            map_get(ImgVars,"p").setValue(0);
            map_get(ImgVars,"r").setValue(pano.getSrcImage(imgNr).getExifOrientation());
            map_get(ImgVars,"TrX").setValue(0);
            map_get(ImgVars,"TrY").setValue(0);
            map_get(ImgVars,"TrZ").setValue(0);
        };
        double cropFactor = 0;
        double focalLength = 0;
        double eV = 0;
        SrcPanoImage srcImg = pano.getSrcImage(imgNr);
        if(m_resetHFOV || m_resetExposure>0)
        {
            srcImg.readEXIF(focalLength,cropFactor,eV,false,false);
        };
        if(m_resetHFOV)
        {
            if(focalLength!=0&&cropFactor!=0)
            {
                double newHFOV=calcHFOV(srcImg.getProjection(),focalLength,cropFactor,srcImg.getSize());
                if(newHFOV!=0)
                {
                    map_get(ImgVars,"v").setValue(newHFOV);
                };
            };
        };
        if(m_resetLens)
        {
            map_get(ImgVars,"a").setValue(0);
            map_get(ImgVars,"b").setValue(0);
            map_get(ImgVars,"c").setValue(0);
            map_get(ImgVars,"d").setValue(0);
            map_get(ImgVars,"e").setValue(0);
            map_get(ImgVars,"g").setValue(0);
            map_get(ImgVars,"t").setValue(0);
        };
        if(m_resetExposure>0)
        {
            if(m_resetExposure==1)
            {
                //reset to exif value
                
                if (pano.getImage(*it).ExposureValueisLinked())
                {
                    /* Unlink exposure value variable so the EXIF values can be
                     * independant. */
                    needs_unlink = true;
                }
                if(eV!=0)
                    map_get(ImgVars,"Eev").setValue(eV);
            }
            else
            {
                //reset to zero
                map_get(ImgVars,"Eev").setValue(0);
            };
        };
        if(m_resetColor)
        {
            map_get(ImgVars,"Er").setValue(1);
            map_get(ImgVars,"Eb").setValue(1);
        };
        if(m_resetVignetting)
        {
            map_get(ImgVars,"Vb").setValue(0);
            map_get(ImgVars,"Vc").setValue(0);
            map_get(ImgVars,"Vd").setValue(0);
            map_get(ImgVars,"Vx").setValue(0);
            map_get(ImgVars,"Vy").setValue(0);

        };
        if(m_resetCameraResponse)
        {
            map_get(ImgVars,"Ra").setValue(0);
            map_get(ImgVars,"Rb").setValue(0);
            map_get(ImgVars,"Rc").setValue(0);
            map_get(ImgVars,"Rd").setValue(0);
            map_get(ImgVars,"Re").setValue(0);
        };
        vars.push_back(ImgVars);
    };
    std::vector<PT::PanoCommand *> reset_commands;
    if (needs_unlink)
    {
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
        variables.insert(HuginBase::ImageVariableGroup::IVE_ExposureValue);
        
        reset_commands.push_back(
                new ChangePartImagesLinkingCmd(
                            pano,
                            images,
                            variables,
                            false,
                            HuginBase::StandardImageVariableGroups::getLensVariables())
                );
    }
    reset_commands.push_back(
                            new PT::UpdateImagesVariablesCmd(pano, images, vars)
                                           );
    if(m_resetExposure>0)
    {
        //reset panorama output exposure value
        reset_commands.push_back(new PT::ResetToMeanExposure(pano));
    };
    return new PT::CombinedPanoCommand(pano, reset_commands);
};

bool ResetOperation::ShowDialog(wxWindow* parent)
{
    ResetDialog reset_dlg(parent);
    bool checkGeometric;
    bool checkPhotometric;
    switch(m_resetMode)
    {
        case RESET_DIALOG:
            checkGeometric=true;
            checkPhotometric=true;
            break;
        case RESET_DIALOG_LENS:
            reset_dlg.LimitToGeometric();
            checkGeometric=true;
            checkPhotometric=false;
            break;
        case RESET_DIALOG_PHOTOMETRICS:
            reset_dlg.LimitToPhotometric();
            checkGeometric=false;
            checkPhotometric=true;
    };
    if(reset_dlg.ShowModal()==wxID_OK)
    {
        if(checkGeometric)
        {
            m_resetPos=reset_dlg.GetResetPos();
            m_resetHFOV=reset_dlg.GetResetFOV();
            m_resetLens=reset_dlg.GetResetLens();
        };
        if(checkPhotometric)
        {
            if(reset_dlg.GetResetExposure())
            {
                if(reset_dlg.GetResetExposureToExif())
                {
                    m_resetExposure=1;
                }
                else
                {
                    m_resetExposure=2;
                };
            }
            else
            {
                m_resetExposure=0;
            };
            m_resetVignetting=reset_dlg.GetResetVignetting();
            m_resetColor=reset_dlg.GetResetColor();
            m_resetCameraResponse=reset_dlg.GetResetResponse();
        };
        return true;
    }
    else
    {
        return false;
    };
};

bool NewStackOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    if(pano.getNrOfImages()==0 || images.size()==0)
    {
        return false;
    }
    else
    {
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        return variable_groups.getStacks().getNumberOfParts()<pano.getNrOfImages();
    };
};

wxString NewStackOperation::GetLabel()
{
    return _("New stack");
};

PT::PanoCommand* NewStackOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    return new PT::NewPartCmd(pano, images, HuginBase::StandardImageVariableGroups::getStackVariables());
};

bool ChangeStackOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    if(pano.getNrOfImages()==0 || images.size()==0)
    {
        return false;
    }
    else
    {
        //project must have more than 1 stack before you can assign an other stack number
        HuginBase::StandardImageVariableGroups variableGroups(pano);
        return variableGroups.getStacks().getNumberOfParts() > 1;
    };
};

wxString ChangeStackOperation::GetLabel()
{
    return _("Change stack...");
};

PT::PanoCommand* ChangeStackOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    HuginBase::StandardImageVariableGroups variable_groups(pano);
    long nr = wxGetNumberFromUser(
                            _("Enter new stack number"),
                            _("stack number"),
                            _("Change stack number"), 0, 0,
                            variable_groups.getStacks().getNumberOfParts()-1
                                 );
    if (nr >= 0)
    {
        // user accepted
        return new PT::ChangePartNumberCmd(pano, images, nr, HuginBase::StandardImageVariableGroups::getStackVariables());
    }
    else
    {
        return NULL;
    };
};

bool AssignStacksOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    return pano.getNrOfImages()>1;
};

wxString AssignStacksOperation::GetLabel()
{
    return _("Set stack size...");
};

PT::PanoCommand* AssignStacksOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    long stackSize = wxGetNumberFromUser(
                            _("Enter image count per stack"),
                            _("stack size"),
                            _("Images per stack"), 3, 1,
                            pano.getNrOfImages()
                                 );
    if(stackSize<0)
    {
        return NULL;
    };
    std::vector<PT::PanoCommand *> commands;
    HuginBase::StandardImageVariableGroups variable_groups(pano);
    if(variable_groups.getStacks().getNumberOfParts()<pano.getNrOfImages())
    {
        // first remove all existing stacks
        for(size_t i=1; i<pano.getNrOfImages(); i++)
        {
            UIntSet imgs;
            imgs.insert(i);
            commands.push_back(new PT::NewPartCmd(pano, imgs, HuginBase::StandardImageVariableGroups::getStackVariables()));
        };
    };

    if (stackSize > 1)
    {
        size_t stackNr=0;
        size_t imgNr=0;
        while(imgNr<pano.getNrOfImages())
        {
            UIntSet imgs;
            for(size_t i=0; i<stackSize && imgNr<pano.getNrOfImages(); i++)
            {
                imgs.insert(imgNr);
                imgNr++;
            };
            commands.push_back(new PT::ChangePartNumberCmd(pano, imgs, stackNr, HuginBase::StandardImageVariableGroups::getStackVariables()));
            stackNr++;
        };
    };
    return new PT::CombinedPanoCommand(pano, commands);
};

static PanoOperationVector PanoOpImages;
static PanoOperationVector PanoOpLens;
static PanoOperationVector PanoOpStacks;
static PanoOperationVector PanoOpControlPoints;
static PanoOperationVector PanoOpReset;

PanoOperationVector* GetImagesOperationVector()
{
    return &PanoOpImages;
};

PanoOperationVector* GetLensesOperationVector()
{
    return &PanoOpLens;
};

PanoOperationVector* GetStacksOperationVector()
{
    return &PanoOpStacks;
};

PanoOperationVector* GetControlPointsOperationVector()
{
    return &PanoOpControlPoints;
};

PanoOperationVector* GetResetOperationVector()
{
    return &PanoOpReset;
};

void GeneratePanoOperationVector()
{
    PanoOpImages.push_back(new AddImageOperation());
    PanoOpImages.push_back(new AddImagesSeriesOperation());
    PanoOpImages.push_back(new RemoveImageOperation());
    PanoOpImages.push_back(new ChangeAnchorImageOperation());
    PanoOpImages.push_back(new ChangeColorAnchorImageOperation());

    PanoOpLens.push_back(new NewLensOperation());
    PanoOpLens.push_back(new ChangeLensOperation());
    PanoOpLens.push_back(new LoadLensOperation(false));
    PanoOpLens.push_back(new LoadLensOperation(true));
    PanoOpLens.push_back(new SaveLensOperation(0));
    PanoOpLens.push_back(new SaveLensOperation(1));
    PanoOpLens.push_back(new SaveLensOperation(2));

    PanoOpStacks.push_back(new NewStackOperation());
    PanoOpStacks.push_back(new ChangeStackOperation());
    PanoOpStacks.push_back(new AssignStacksOperation());

    PanoOpControlPoints.push_back(new RemoveControlPointsOperation());
    PanoOpControlPoints.push_back(new CelesteOperation());
    PanoOpControlPoints.push_back(new CleanControlPointsOperation());

    PanoOpReset.push_back(new ResetOperation(ResetOperation::RESET_POSITION));
    PanoOpReset.push_back(new ResetOperation(ResetOperation::RESET_LENS));
    PanoOpReset.push_back(new ResetOperation(ResetOperation::RESET_PHOTOMETRICS));
    PanoOpReset.push_back(new ResetOperation(ResetOperation::RESET_DIALOG));

};


void _CleanPanoOperationVector(PanoOperationVector& vec)
{
    for(size_t i=0; i<vec.size(); i++)
    {
        delete vec[i];
    }
    vec.clear();
};

void CleanPanoOperationVector()
{
    _CleanPanoOperationVector(PanoOpImages);
    _CleanPanoOperationVector(PanoOpLens);
    _CleanPanoOperationVector(PanoOpStacks);
    _CleanPanoOperationVector(PanoOpControlPoints);
    _CleanPanoOperationVector(PanoOpReset);
};

} //namespace
