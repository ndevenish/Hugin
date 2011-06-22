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
#include "icpfind/AutoCtrlPointCreator.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/PTWXDlg.h"
#include "algorithms/control_points/CleanCP.h"
#include "celeste/Celeste.h"
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#ifdef HUGIN_HSI
#include "hugin/PythonProgress.h"
#endif

using namespace HuginBase;

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
        wxArrayString Filenames;
        dlg.GetPaths(Pathnames);
        dlg.GetFilenames(Filenames);

        // remember path for later
        config->Write(wxT("/actualPath"), dlg.GetDirectory());
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
        bool foundForbiddenChars=false;
        for(unsigned int i=0;i<Pathnames.GetCount() && !foundForbiddenChars; i++)
        {
           foundForbiddenChars=foundForbiddenChars || containsInvalidCharacters(Pathnames[i]);
        };
        if(foundForbiddenChars)
        {
            wxMessageBox(wxString::Format(_("The filename(s) contains one of the following invalid characters: %s\nHugin can not work with these filenames. Please rename your file(s) and try again."),getInvalidCharacters().c_str()),
                _("Error"),wxOK | wxICON_EXCLAMATION);
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

    // sort files by date
    sortbytime spred(timeMap);
    sort(files.begin(), files.end(), spred);
    return new PT::wxAddImagesCmd(pano,files);
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

PT::PanoCommand* ChangeAnchorImageOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    PanoramaOptions opt = pano.getOptions();
    opt.optimizeReferenceImage = *(images.begin());
    return new PT::SetPanoOptionsCmd(pano,opt);
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

PT::PanoCommand* NewStackOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    return new PT::NewPartCmd(pano,images,HuginBase::StandardImageVariableGroups::getStackVariables());
};

bool ChangeStackOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    if(images.size()==0)
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

PT::PanoCommand* ChangeStackOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    HuginBase::StandardImageVariableGroups variableGroups(pano);
    long nr = wxGetNumberFromUser(
                            _("Enter new stack number"),
                            _("stack number"),
                            _("Change stack number"), 0, 0,
                            variableGroups.getStacks().getNumberOfParts()-1
                                 );
    if (nr >= 0)
    {
        // user accepted
        return new PT::ChangePartNumberCmd(pano,images,nr, HuginBase::StandardImageVariableGroups::getStackVariables());
    }
    else
    {
        return NULL;
    };
};

bool RemoveControlPointsOperation::IsEnabled(PT::Panorama& pano,HuginBase::UIntSet images)
{
    return pano.getNrOfImages()>0;
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

PT::PanoCommand* GenerateControlPointsOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    UIntSet selImages;
    if(images.size()<2)
    {
        fill_set(selImages,0,pano.getNrOfImages()-1);
    }
    else
    {
        selImages=images;
    };
    long nFeatures=wxConfigBase::Get()->Read(wxT("/Assistant/nControlPoints"), HUGIN_ASS_NCONTROLPOINTS);
    AutoCtrlPointCreator matcher;
    CPVector cps = matcher.automatch(m_setting, pano, selImages, nFeatures,parent);
    wxMessageBox(wxString::Format(_("Added %lu control points"), (unsigned long) cps.size()), _("Control point detector result"),wxOK|wxICON_INFORMATION);
    return new PT::AddCtrlPointsCmd(pano, cps);
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

#ifdef HUGIN_HSI
PT::PanoCommand* PythonOperation::GetInternalCommand(wxWindow* parent, PT::Panorama& pano, HuginBase::UIntSet images)
{
    if(m_filename.FileExists())
    {
        PythonWithImagesProgress pythonDlg(parent,pano,images,m_filename.GetFullPath());
        if(pythonDlg.RunScript())
        {
            if(pythonDlg.ShowModal()==wxID_OK)
            {
                return new PT::UpdateProjectCmd(pano,pythonDlg.GetPanoramaMemento());
            };
        };
    }
    return NULL;
};

#endif