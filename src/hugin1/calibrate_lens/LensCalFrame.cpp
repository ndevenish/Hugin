// -*- c-basic-offset: 4 -*-

/** @file LensCalFrame.cpp
 *
 *  @brief implementation of LensCal main frame class
 *
 *  @author T. Modes
 *
 */

/* 
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

#include "panoinc_WX.h"
#include "panoinc.h"

#include "base_wx/platform.h"
#include "base_wx/wxPlatform.h"
#include "base_wx/LensTools.h"
#include "huginapp/ImageCache.h"
#include "LensCalFrame.h"
#include <wx/app.h>
#include "LensCalApp.h"
#include "hugin/config_defaults.h"
#include <algorithms/optimizer/PTOptimizer.h>
#include "lensdb/LensDB.h"
#include "base_wx/wxLensDB.h"

using namespace HuginBase;

const unsigned int cps_per_line=10;

#define DEFAULT_LENSCAL_SCALE 2.0
#define DEFAULT_LENSCAL_THRESHOLD 4.0
#define DEFAULT_RESIZE_DIMENSION 1600
#define DEFAULT_MINLINELENGTH 0.3

// utility functions
bool str2double(wxString s, double & d)
{
    if (!hugin_utils::stringToDouble(std::string(s.mb_str(wxConvLocal)), d)) 
    {
        return false;
    }
    return true;
}

/** file drag and drop handler method */
bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    DEBUG_TRACE("OnDropFiles");
    LensCalFrame* frame=wxGetApp().GetLensCalFrame();
    if (!frame) 
        return false;

    // try to add as images
    wxArrayString files;
    wxArrayString invalidFiles;
    for (unsigned int i=0; i< filenames.GetCount(); i++)
    {
        wxFileName file(filenames[i]);
        if (file.GetExt().CmpNoCase(wxT("jpg")) == 0 ||
            file.GetExt().CmpNoCase(wxT("jpeg")) == 0 ||
            file.GetExt().CmpNoCase(wxT("tif")) == 0 ||
            file.GetExt().CmpNoCase(wxT("tiff")) == 0 ||
            file.GetExt().CmpNoCase(wxT("png")) == 0 ||
            file.GetExt().CmpNoCase(wxT("bmp")) == 0 ||
            file.GetExt().CmpNoCase(wxT("gif")) == 0 ||
            file.GetExt().CmpNoCase(wxT("pnm")) == 0 ||
            file.GetExt().CmpNoCase(wxT("sun")) == 0 ||
            file.GetExt().CmpNoCase(wxT("hdr")) == 0 ||
            file.GetExt().CmpNoCase(wxT("viff")) == 0 )
        {
            if(containsInvalidCharacters(file.GetFullPath()))
            {
                invalidFiles.push_back(file.GetFullPath());
            }
            else
            {
                files.push_back(file.GetFullPath());
            };
        }
    }
    if(invalidFiles.size()>0)
    {
        ShowFilenameWarning(frame, invalidFiles);
    }
    // we got some images to add.
    if (files.size() > 0)
    {
        // use a Command to ensure proper undo and updating of GUI parts
        frame->AddImages(files);
    }
    return true;
}

// event table. this frame will recieve mostly global commands.
BEGIN_EVENT_TABLE(LensCalFrame, wxFrame)
    EVT_LISTBOX(XRCID("lenscal_images_list"), LensCalFrame::OnImageSelected)
    EVT_MENU(XRCID("menu_save"), LensCalFrame::OnSaveProject)
    EVT_MENU(XRCID("menu_quit"), LensCalFrame::OnExit)
    EVT_BUTTON(XRCID("lenscal_add_image"), LensCalFrame::OnAddImage)
    EVT_BUTTON(XRCID("lenscal_remove_image"), LensCalFrame::OnRemoveImage)
    EVT_BUTTON(XRCID("lenscal_find_lines"), LensCalFrame::OnFindLines)
    EVT_BUTTON(XRCID("lenscal_reset"), LensCalFrame::OnReset)
    EVT_BUTTON(XRCID("lenscal_opt"), LensCalFrame::OnOptimize)
    EVT_BUTTON(XRCID("lenscal_save_lens"), LensCalFrame::OnSaveLens)
    EVT_BUTTON(XRCID("lenscal_refresh"), LensCalFrame::OnRefresh)
    EVT_CHOICE(XRCID("lenscal_preview_content"), LensCalFrame::OnSelectPreviewContent)
    EVT_CHECKBOX(XRCID("lenscal_show_lines"), LensCalFrame::OnShowLines)
END_EVENT_TABLE()

LensCalFrame::LensCalFrame(wxWindow* parent)
{
    DEBUG_TRACE("");
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("lenscal_frame"));
    DEBUG_TRACE("");

    // load our menu bar
#ifdef __WXMAC__
    wxApp::s_macExitMenuItemId = XRCID("menu_quit");
#endif
    SetMenuBar(wxXmlResource::Get()->LoadMenuBar(this, wxT("lenscal_menubar")));

    m_choice_projection=XRCCTRL(*this,"lenscal_proj_choice",wxChoice);
    FillLensProjectionList(m_choice_projection);
    m_images_list=XRCCTRL(*this,"lenscal_images_list",wxListBox);
    m_preview=XRCCTRL(*this,"lenscal_preview",LensCalImageCtrl);

    wxConfigBase* config = wxConfigBase::Get();
    config->Read(wxT("/LensCalFrame/EdgeScale"),&m_edge_scale,DEFAULT_LENSCAL_SCALE);
    config->Read(wxT("/LensCalFrame/EdgeThreshold"),&m_edge_threshold,DEFAULT_LENSCAL_THRESHOLD);
    m_resize_dimension=config->Read(wxT("/LensCalFrame/ResizeDimension"),DEFAULT_RESIZE_DIMENSION);
    config->Read(wxT("/LensCalFrame/MinLineLength"),&m_minlinelength,DEFAULT_MINLINELENGTH);
    ParametersToDisplay();

    bool selected;
    config->Read(wxT("/LensCalFrame/Optimize_a"),&selected,false);
    XRCCTRL(*this,"lenscal_opt_a",wxCheckBox)->SetValue(selected);
    config->Read(wxT("/LensCalFrame/Optimize_b"),&selected,true);
    XRCCTRL(*this,"lenscal_opt_b",wxCheckBox)->SetValue(selected);
    config->Read(wxT("/LensCalFrame/Optimize_c"),&selected,false);
    XRCCTRL(*this,"lenscal_opt_c",wxCheckBox)->SetValue(selected);
    config->Read(wxT("/LensCalFrame/Optimize_de"),&selected,false);
    XRCCTRL(*this,"lenscal_opt_de",wxCheckBox)->SetValue(selected);

    // set the minimize icon
#ifdef __WXMSW__
    wxIcon myIcon(GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    SetTitle(_("Hugin Lens calibration GUI"));

    // create a new drop handler. wxwindows deletes the automatically
    SetDropTarget(new FileDropTarget());

    // create a status bar
    const int fields (2);
    CreateStatusBar(fields);
    int widths[fields] = {-1, 85};
    SetStatusWidths( fields, &widths[0]);

    // Set sizing characteristics
    //set minumum size
#if defined __WXMAC__ || defined __WXMSW__
    // a minimum nice looking size; smaller than this would clutter the layout.
    SetSizeHints(900, 675);
#else
    // For ASUS eeePc
    SetSizeHints(780, 455); //set minumum size
#endif

    // set progress display for image cache.
    ImageCache::getInstance().setProgressDisplay(this);
#if defined __WXMSW__
    unsigned long long mem = HUGIN_IMGCACHE_UPPERBOUND;
    unsigned long mem_low = config->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND);
    unsigned long mem_high = config->Read(wxT("/ImageCache/UpperBoundHigh"), (long) 0);
    if (mem_high > 0) {
      mem = ((unsigned long long) mem_high << 32) + mem_low;
    }
    else {
      mem = mem_low;
    }
    ImageCache::getInstance().SetUpperLimit(mem);
#else
    ImageCache::getInstance().SetUpperLimit(config->Read(wxT("/ImageCache/UpperBound"), HUGIN_IMGCACHE_UPPERBOUND));
#endif
    //disable buttons
    EnableButtons();
    XRCCTRL(*this,"lenscal_remove_image",wxButton)->Enable(false);
}

LensCalFrame::~LensCalFrame()
{
    DEBUG_TRACE("dtor");
    m_preview->SetEmptyImage();
    ImageCache::getInstance().setProgressDisplay(0);
    delete & ImageCache::getInstance();
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();
    if(ReadInputs(false,true,false))
    {
        config->Write(wxT("/LensCalFrame/EdgeScale"),m_edge_scale);
        config->Write(wxT("/LensCalFrame/EdgeThreshold"),m_edge_threshold);
        config->Write(wxT("/LensCalFrame/ResizeDimension"),(int)m_resize_dimension);
        config->Write(wxT("/LensCalFrame/MinLineLength"),m_minlinelength);
    };
    config->Write(wxT("/LensCalFrame/Optimize_a"),XRCCTRL(*this,"lenscal_opt_a",wxCheckBox)->GetValue());
    config->Write(wxT("/LensCalFrame/Optimize_b"),XRCCTRL(*this,"lenscal_opt_b",wxCheckBox)->GetValue());
    config->Write(wxT("/LensCalFrame/Optimize_c"),XRCCTRL(*this,"lenscal_opt_c",wxCheckBox)->GetValue());
    config->Write(wxT("/LensCalFrame/Optimize_de"),XRCCTRL(*this,"lenscal_opt_de",wxCheckBox)->GetValue());
    StoreFramePosition(this, wxT("LensCalFrame"));
    config->Flush();
    //cleanup
    for(unsigned int i=0;i<m_images.size();i++)
    {
        delete m_images[i];
    };
    m_images.clear();
    LensDB::LensDB::Clean();
    DEBUG_TRACE("dtor end");
}

void LensCalFrame::ParametersToDisplay()
{
    XRCCTRL(*this,"lenscal_scale",wxTextCtrl)->SetValue(doubleTowxString(m_edge_scale,2));
    XRCCTRL(*this,"lenscal_threshold",wxTextCtrl)->SetValue(doubleTowxString(m_edge_threshold,2));
    XRCCTRL(*this,"lenscal_resizedim",wxTextCtrl)->SetValue(wxString::Format(wxT("%d"),m_resize_dimension));
    XRCCTRL(*this,"lenscal_minlinelength",wxTextCtrl)->SetValue(doubleTowxString(m_minlinelength,2));
};

const wxString & LensCalFrame::GetXRCPath()
{
     return wxGetApp().GetXRCPath();
};

void LensCalFrame::resetProgress(double max)
{
    m_progressMax = max;
    m_progress = 0;
    m_progressMsg = wxT("");
}

bool LensCalFrame::increaseProgress(double delta)
{
    m_progress += delta;

    // build the message:
    int percentage = (int) floor(m_progress/m_progressMax*100);
    if (percentage > 100) percentage = 100;

    return displayProgress();
}

bool LensCalFrame::increaseProgress(double delta, const std::string & msg)
{
    m_progress += delta;
    m_progressMsg = wxString(msg.c_str(), wxConvLocal);

    return displayProgress();
}


void LensCalFrame::setMessage(const std::string & msg)
{
    m_progressMsg = wxString(msg.c_str(), wxConvLocal);
}

bool LensCalFrame::displayProgress()
{
    // build the message:
    int percentage = (int) floor(m_progress/m_progressMax*100);
    if (percentage > 100) percentage = 100;

    wxStatusBar *statbar = GetStatusBar();
    statbar->SetStatusText(wxString::Format(wxT("%s: %d%%"),m_progressMsg.c_str(), percentage),0);
#ifdef __WXMSW__
    UpdateWindow(NULL);
#endif
    return true;
}

/** update the display */
void LensCalFrame::updateProgressDisplay()
{
    wxString msg;
    // build the message:
    for (std::vector<AppBase::ProgressTask>::reverse_iterator it = tasks.rbegin();
                 it != tasks.rend(); ++it)
    {
        wxString cMsg;
        if (it->getProgress() > 0) {
            cMsg.Printf(wxT("%s %s [%3.0f%%]"),
                        wxString(it->getShortMessage().c_str(), wxConvLocal).c_str(),
                        wxString(it->getMessage().c_str(), wxConvLocal).c_str(),
                        100 * it->getProgress());
        } else {
            cMsg.Printf(wxT("%s %s"),wxString(it->getShortMessage().c_str(), wxConvLocal).c_str(),
                        wxString(it->getMessage().c_str(), wxConvLocal).c_str());
        }
        // append to main message
        if (it == tasks.rbegin()) {
            msg = cMsg;
        } else {
            msg.Append(wxT(" | "));
            msg.Append(cMsg);
        }
    }
    wxStatusBar *m_statbar = GetStatusBar();
    DEBUG_TRACE("Statusmb : " << msg.mb_str(wxConvLocal));
    m_statbar->SetStatusText(msg,0);

#ifdef __WXMSW__
    UpdateWindow(NULL);
#endif
}

void LensCalFrame::OnExit(wxCommandEvent &e)
{
    Close();
};

void LensCalFrame::AddImages(wxArrayString files)
{
    wxArrayString wrongSize;
    wxArrayString wrongExif;
    for (unsigned int i=0; i<files.GetCount(); i++)
    {
        ImageLineList* image=new ImageLineList(files[i]);
        //check input
        if(m_images.size()>0)
        {
            const HuginBase::SrcPanoImage* image0=m_images[0]->GetPanoImage();
            const HuginBase::SrcPanoImage* image1=image->GetPanoImage();
            if(image0->getSize()!=image1->getSize())
            {
                wrongSize.push_back(files[i]);
                delete image;
                continue;
            };
            if(image0->hasEXIFread() && image1->hasEXIFread())
            {
                if(image0->getExifMake()!=image1->getExifMake() ||
                    image0->getExifModel()!=image1->getExifModel() ||
                    image0->getExifFocalLength()!=image1->getExifFocalLength() ||
                    image0->getExifCropFactor()!=image1->getExifCropFactor())
                {
                    //only show a warning, but continue processing
                    wrongExif.push_back(files[i]);
                };
            };
        };
        m_images.push_back(image);
        SetStatusText(wxString::Format(_("Added %s"),image->GetFilename().c_str()));
        if(image->GetPanoImage()->hasEXIFread())
        {
            XRCCTRL(*this,"lenscal_focallength",wxTextCtrl)->SetValue( 
                hugin_utils::doubleTowxString(image->GetPanoImage()->getExifFocalLength(),2)
                );
            XRCCTRL(*this,"lenscal_cropfactor",wxTextCtrl)->SetValue(
                hugin_utils::doubleTowxString(image->GetPanoImage()->getExifCropFactor(),2)
                );
            SelectProjection(m_choice_projection,image->GetPanoImage()->getProjection());
        };
    }
    UpdateList(false);
    m_images_list->SetSelection(m_images_list->GetCount()-1);
    wxCommandEvent e;
    OnImageSelected(e);
    EnableButtons();
    if(wrongSize.size()>0)
    {
        wxString fileText;
        for(unsigned int i=0;i<wrongSize.size();i++)
        {
            wxFileName filename(wrongSize[i]);
            fileText.Append(filename.GetFullName());
            if(i<wrongSize.size()-1)
                fileText.Append(wxT(", "));
        };
        wxMessageBox(wxString::Format(_("The size of the images (%s) does not match the already added image(s)."),fileText.c_str()),
            _("Error"),wxOK|wxICON_EXCLAMATION,this);
    };
    if(wrongExif.size()>0)
    {
        wxString fileText;
        for(unsigned int i=0;i<wrongExif.size();i++)
        {
            wxFileName filename(wrongExif[i]);
            fileText.Append(filename.GetFullName());
            if(i<wrongExif.size()-1)
                fileText.Append(wxT(", "));
        };
        wxMessageBox(wxString::Format(_("The EXIF information of the added images (%s) is not consistent with the already added image(s).\nPlease check the image again, if you selected the correct images."),fileText.c_str()),
            _("Warning"),wxOK|wxICON_EXCLAMATION,this);
    };
};

void LensCalFrame::OnAddImage(wxCommandEvent &e)
{
    wxConfigBase* config = wxConfigBase::Get();
    wxString path = config->Read(wxT("/actualPath"), wxT(""));
    wxFileDialog dlg(this,_("Add images"),
                     path, wxT(""),
                     HUGIN_WX_FILE_IMG_FILTER,
                     wxFD_OPEN | wxFD_MULTIPLE, wxDefaultPosition);
    dlg.SetDirectory(path);

    // remember the image extension
    wxString img_ext;
    if (config->HasEntry(wxT("lastImageType"))){
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
    DEBUG_INFO ( "Image extention: " << img_ext.mb_str(wxConvLocal) )

    // call the file dialog
    if (dlg.ShowModal() == wxID_OK)
    {
        // get the selections
        wxArrayString Pathnames;
        dlg.GetPaths(Pathnames);

        // save the current path to config
#ifdef __WXGTK__
        //workaround a bug in GTK, see https://bugzilla.redhat.com/show_bug.cgi?id=849692 and http://trac.wxwidgets.org/ticket/14525
        config->Write(wxT("/actualPath"), wxPathOnly(Pathnames[0]));
#else
        config->Write(wxT("/actualPath"), dlg.GetDirectory());
#endif

        wxArrayString invalidFiles;
        for(unsigned int i=0;i<Pathnames.GetCount(); i++)
        {
            if(containsInvalidCharacters(Pathnames[i]))
            {
                invalidFiles.push_back(Pathnames[i]);
            };
        };
        if(invalidFiles.size()>0)
        {
            ShowFilenameWarning(this, invalidFiles);
        }
        else
        {
            AddImages(Pathnames);
        };
        DEBUG_INFO ( wxString::Format(wxT("img_ext: %d"), dlg.GetFilterIndex()).mb_str(wxConvLocal) )
        // save the image extension
        switch ( dlg.GetFilterIndex() )
        {
            case 0: config->Write(wxT("lastImageType"), wxT("all images")); break;
            case 1: config->Write(wxT("lastImageType"), wxT("jpg")); break;
            case 2: config->Write(wxT("lastImageType"), wxT("tiff")); break;
            case 3: config->Write(wxT("lastImageType"), wxT("png")); break;
            case 4: config->Write(wxT("lastImageType"), wxT("hdr")); break;
            case 5: config->Write(wxT("lastImageType"), wxT("exr")); break;
            case 6: config->Write(wxT("lastImageType"), wxT("all files")); break;
        }
    }
    else
    {
        // nothing to open
        SetStatusText( _("Add Image: cancel"));
    }
    EnableButtons();
};

void LensCalFrame::UpdateListString(unsigned int index)
{
    wxFileName file(m_images[index]->GetFilename());
    m_images_list->SetString(index,wxString::Format(_("%s (%d lines)"),file.GetFullName().c_str(),m_images[index]->GetNrOfValidLines()));
};

void LensCalFrame::UpdateList(bool restoreSelection)
{
    int oldSelection=m_images_list->GetSelection();
    m_images_list->Clear();
    for(unsigned int i=0;i<m_images.size();i++)
    {
        wxFileName file(m_images[i]->GetFilename());
        wxString text=wxString::Format(_("%s (%d lines)"),file.GetFullName().c_str(),m_images[i]->GetNrOfValidLines());
        m_images_list->Append(text);
    };
    if(oldSelection!=wxNOT_FOUND && restoreSelection)
    {
        m_images_list->SetSelection(oldSelection);
    };
    wxCommandEvent e;
    OnImageSelected(e);
};

void LensCalFrame::OnRemoveImage(wxCommandEvent &e)
{
    int selection=m_images_list->GetSelection();
    if(selection!=wxNOT_FOUND)
    {
        delete m_images[selection];
        m_images.erase(m_images.begin()+selection);
        ImageCache::getInstance().softFlush();
        UpdateList(false);
    }
    else
    {
        wxBell();
    };
    EnableButtons();
};

void LensCalFrame::EnableButtons()
{
    bool enabling=m_images.size()>0;
    XRCCTRL(*this,"lenscal_find_lines",wxButton)->Enable(enabling);
    XRCCTRL(*this,"lenscal_opt",wxButton)->Enable(enabling);
    XRCCTRL(*this,"lenscal_save_lens",wxButton)->Enable(enabling);
    GetMenuBar()->Enable(XRCID("menu_save"),enabling);
};

bool LensCalFrame::ReadInputs(bool readFocalLength,bool readOptions,bool readLensParameter)
{
    if(readFocalLength)
    {
        m_projection=(SrcPanoImage::Projection)GetSelectedProjection(m_choice_projection);
        if(!str2double(XRCCTRL(*this,"lenscal_focallength",wxTextCtrl)->GetValue(),m_focallength))
            return false;
        if(m_focallength<1)
            return false;
        if(!str2double(XRCCTRL(*this,"lenscal_cropfactor",wxTextCtrl)->GetValue(),m_cropfactor))
            return false;
        if(m_cropfactor<0.1)
            return false;
    }
    if(readOptions)
    {
        if(!str2double(XRCCTRL(*this,"lenscal_scale",wxTextCtrl)->GetValue(),m_edge_scale))
            return false;
        if(!str2double(XRCCTRL(*this,"lenscal_threshold",wxTextCtrl)->GetValue(),m_edge_threshold))
            return false;
        double resize_dim;
        if(!str2double(XRCCTRL(*this,"lenscal_resizedim",wxTextCtrl)->GetValue(),resize_dim))
            return false;
        if(resize_dim<100)
            return false;
        m_resize_dimension=(unsigned int)resize_dim;
        if(!str2double(XRCCTRL(*this,"lenscal_minlinelength",wxTextCtrl)->GetValue(),m_minlinelength))
            return false;
        if(m_minlinelength<=0 || m_minlinelength>1)
            return false;
    };
    if(readLensParameter)
    {
        if(!str2double(XRCCTRL(*this,"lenscal_a",wxTextCtrl)->GetValue(),m_a))
            return false;
        if(!str2double(XRCCTRL(*this,"lenscal_b",wxTextCtrl)->GetValue(),m_b))
            return false;
        if(!str2double(XRCCTRL(*this,"lenscal_c",wxTextCtrl)->GetValue(),m_c))
            return false;
        if(!str2double(XRCCTRL(*this,"lenscal_d",wxTextCtrl)->GetValue(),m_d))
            return false;
        if(!str2double(XRCCTRL(*this,"lenscal_e",wxTextCtrl)->GetValue(),m_e))
            return false;
    }
    return true;
};

void LensCalFrame::OnFindLines(wxCommandEvent &e)
{
    if(!ReadInputs(true,true,false))
    {
        wxMessageBox(_("There are invalid values in the input boxes.\nPlease check your inputs."),_("Warning"),wxOK | wxICON_INFORMATION, this);
        return;
    }
    m_preview->SetLens(m_projection,m_focallength,m_cropfactor);
    for(unsigned int i=0;i<m_images.size();i++)
    {
        std::string filename(m_images[i]->GetFilename().mb_str(HUGIN_CONV_FILENAME));
        ImageCache::EntryPtr img=ImageCache::getInstance().getImage(filename);
        double scale;
        SetStatusText(_("Detecting edges..."));
        m_images[i]->SetEdgeImage(HuginLines::detectEdges(*(img->get8BitImage()),m_edge_scale,m_edge_threshold,m_resize_dimension,scale));
        SetStatusText(_("Finding lines..."));
        m_images[i]->SetLines(HuginLines::findLines(*(m_images[i]->GetEdgeImage()),m_minlinelength,m_focallength,m_cropfactor));
        m_images[i]->ScaleLines(scale);
    };
    SetStatusText(_("Finished"));
    UpdateList(true);
};

void LensCalFrame::OnOptimize(wxCommandEvent &e)
{
    if(!ReadInputs(true,false,true))
    {
        wxMessageBox(_("There are invalid values in the input boxes.\nPlease check your inputs."),_("Warning"),wxOK | wxICON_INFORMATION, this);
        return;
    }
    unsigned int count=0;
    for(unsigned int i=0;i<m_images.size();i++)
        count+=m_images[i]->GetNrOfValidLines();
    if(count==0)
    {
        wxMessageBox(_("There are no detected lines.\nPlease run \"Find lines\" first. If there are no lines found, change the parameters."),_("Warning"),wxOK  | wxICON_INFORMATION, this);
        return;
    };
    Optimize();
};

Panorama LensCalFrame::GetPanorama()
{
    HuginBase::Panorama pano;
    OptimizeVector optvec;
    unsigned int line_number=3; 
    for(unsigned int i=0;i<m_images.size();i++)
    {
        SrcPanoImage image(*(m_images[i]->GetPanoImage()));
        image.setProjection(m_projection);
        image.setExifFocalLength(m_focallength);
        image.setExifCropFactor(m_cropfactor);
        image.setVar("a",m_a);
        image.setVar("b",m_b);
        image.setVar("c",m_c);
        image.setVar("d",m_d);
        image.setVar("e",m_e);
        double hfov=SrcPanoImage::calcHFOV(image.getProjection(),image.getExifFocalLength(),image.getExifCropFactor(),image.getSize());
        image.setHFOV(hfov);
        pano.addImage(image);
        std::set<std::string> imgopt;
        if(i==0)
        {
            if(XRCCTRL(*this,"lenscal_opt_a",wxCheckBox)->GetValue())
                imgopt.insert("a");
            if(XRCCTRL(*this,"lenscal_opt_b",wxCheckBox)->GetValue())
                imgopt.insert("b");
            if(XRCCTRL(*this,"lenscal_opt_c",wxCheckBox)->GetValue())
                imgopt.insert("c");
            if(XRCCTRL(*this,"lenscal_opt_de",wxCheckBox)->GetValue())
            {
                imgopt.insert("d");
                imgopt.insert("e");
            }
        };
        optvec.push_back(imgopt);
        //now generate control points from lines
        HuginLines::Lines lines=m_images[i]->GetLines();
        for(unsigned j=0;j<lines.size();j++)
        {
            if(lines[j].status==HuginLines::valid_line)
            {
                HuginBase::CPVector cpv=GetControlPoints(lines[j],i,line_number,cps_per_line);
                for(unsigned int k=0;k<cpv.size();k++)
                    pano.addCtrlPoint(cpv[k]);
                line_number++;
            };
        };
    };
    //assign all images the same lens number
    StandardImageVariableGroups variable_groups(pano);
    ImageVariableGroup & lenses = variable_groups.getLenses();
    if(pano.getNrOfImages()>1)
    {
        for(unsigned int i=1;i<pano.getNrOfImages();i++)
        {
            SrcPanoImage img=pano.getSrcImage(i);
            lenses.switchParts(i,lenses.getPartNumber(0));
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_ExposureValue, i);
            img.setExposureValue(0);
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceRed, i);
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceBlue, i);
            img.setWhiteBalanceRed(1);
            img.setWhiteBalanceBlue(1);
            pano.setSrcImage(i, img);
        };
    };
    //set default exposure value
    PanoramaOptions opts = pano.getOptions();
    opts.outputExposureValue = 0;
    opts.setProjection(HuginBase::PanoramaOptions::RECTILINEAR);
    pano.setOptions(opts);

    pano.setOptimizeVector(optvec);
    return pano;
};

void LensCalFrame::Optimize()
{
    SetStatusText(_("Optimizing lens distortion parameters..."));
    Panorama pano=GetPanorama();
    HuginBase::PTools::optimize(pano);

    const SrcPanoImage img=pano.getImage(0);
    m_a=img.getVar("a");
    m_b=img.getVar("b");
    m_c=img.getVar("c");
    m_d=img.getVar("d");
    m_e=img.getVar("e");
    XRCCTRL(*this,"lenscal_a",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_a,5));
    XRCCTRL(*this,"lenscal_b",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_b,5));
    XRCCTRL(*this,"lenscal_c",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_c,5));
    XRCCTRL(*this,"lenscal_d",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_d,3));
    XRCCTRL(*this,"lenscal_e",wxTextCtrl)->SetValue(hugin_utils::doubleTowxString(m_e,3));
    m_preview->SetLensDistortions(m_a,m_b,m_c,m_d,m_e);
    SetStatusText(_("Finished"));
};

void LensCalFrame::SaveLensToIni()
{
    wxFileDialog dlg(this,
                        _("Save lens parameters file"),
                        wxConfigBase::Get()->Read(wxT("/lensPath"),wxT("")), wxT(""),
                        _("Lens Project Files (*.ini)|*.ini|All files (*)|*"),
                        wxFD_SAVE, wxDefaultPosition);
    dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/lensPath"),wxT("")));
    if (dlg.ShowModal() == wxID_OK)
    {
        wxFileName filename(dlg.GetPath());
        if(!filename.HasExt())
            filename.SetExt(wxT("ini"));
        wxConfig::Get()->Write(wxT("/lensPath"), dlg.GetDirectory());  // remember for later
        if (filename.FileExists())
        {
            int d = wxMessageBox(wxString::Format(_("File %s exists. Overwrite?"), filename.GetFullPath().c_str()),
                                 _("Save project"), wxYES_NO | wxICON_QUESTION);
            if (d != wxYES)
            {
                return;
            }
        }
        Panorama pano=GetPanorama();
        SaveLensParameters(filename.GetFullPath(),&pano,0);
    }
};

void LensCalFrame::OnSaveLens(wxCommandEvent &e)
{
    if(!ReadInputs(true,false,true))
    {
        wxMessageBox(_("There are invalid values in the input boxes.\nPlease check your inputs."),_("Warning"),wxOK | wxICON_INFORMATION, this);
        return;
    }
    unsigned int count=0;
    for(unsigned int i=0;i<m_images.size();i++)
        count+=m_images[i]->GetNrOfValidLines();
    if(count==0)
    {
        wxMessageBox(_("There are no detected lines.\nPlease run \"Find lines\" and \"Optimize\" before saving the lens data. If there are no lines found, change the parameters."),_("Warning"),wxOK  | wxICON_INFORMATION, this);
        return;
    };

    wxArrayString choices;
    choices.push_back(_("Save lens parameters to ini file"));
    choices.push_back(_("Save lens parameters to lensfun database"));
    wxSingleChoiceDialog save_dlg(this,_("Saving lens data"),_("Save lens"),choices);
    if(save_dlg.ShowModal()==wxID_OK)
    {
        if(save_dlg.GetSelection()==0)
        {
            SaveLensToIni();
        }
        else
        {
            Panorama pano=GetPanorama();
            SaveLensParameters(this,pano.getImage(0),false);
        };
    };
};

void LensCalFrame::OnSaveProject(wxCommandEvent &e)
{
    if(!ReadInputs(true,false,true))
    {
        wxMessageBox(_("There are invalid values in the input boxes.\nPlease check your inputs."),_("Warning"),wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxFileDialog dlg(this,_("Save project file"),wxEmptyString,wxEmptyString,
                     _("Project files (*.pto)|*.pto|All files (*)|*"), wxFD_SAVE, wxDefaultPosition);
    dlg.SetDirectory(wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")));
    if (dlg.ShowModal() == wxID_OK)
    {
        wxConfig::Get()->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
        wxFileName filename(dlg.GetPath());
        if(!filename.HasExt())
            filename.SetExt(wxT("pto"));
        if (filename.FileExists())
        {
            int d = wxMessageBox(wxString::Format(_("File %s exists. Overwrite?"), filename.GetFullPath().c_str()),
                                 _("Save project"), wxYES_NO | wxICON_QUESTION);
            if (d != wxYES)
            {
                return;
            }
        }
        Panorama pano=GetPanorama();
        std::string path = getPathPrefix(std::string(filename.GetFullPath().mb_str(HUGIN_CONV_FILENAME)));
        std::ofstream script(filename.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
        script.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
        PT::UIntSet all;
        fill_set(all, 0, pano.getNrOfImages()-1);
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false, path);
        script.close();
    };
};

void LensCalFrame::OnImageSelected(wxCommandEvent &e)
{
    bool selected=m_images_list->GetSelection()!=wxNOT_FOUND;
    XRCCTRL(*this,"lenscal_remove_image",wxButton)->Enable(selected);
    if(selected)
    {
        m_preview->SetImage(m_images[m_images_list->GetSelection()],m_images_list->GetSelection());
    }
    else
    {
        m_preview->SetEmptyImage();
    };
};

void LensCalFrame::OnSelectPreviewContent(wxCommandEvent &e)
{
    m_preview->SetMode((LensCalImageCtrl::LensCalPreviewMode)e.GetSelection());
    XRCCTRL(*this,"lenscal_refresh",wxButton)->Enable(m_preview->GetMode()==LensCalImageCtrl::mode_corrected);
};

void LensCalFrame::OnShowLines(wxCommandEvent &e)
{
    m_preview->SetShowLines(XRCCTRL(*this,"lenscal_show_lines",wxCheckBox)->GetValue());
    m_preview->Refresh(true);
};

void LensCalFrame::OnReset(wxCommandEvent &e)
{
    m_edge_scale=DEFAULT_LENSCAL_SCALE;
    m_edge_threshold=DEFAULT_LENSCAL_THRESHOLD;
    m_resize_dimension=DEFAULT_RESIZE_DIMENSION;
    m_minlinelength=DEFAULT_MINLINELENGTH;
    ParametersToDisplay();
};

void LensCalFrame::OnRefresh(wxCommandEvent &e)
{
    if(!ReadInputs(true,false,true))
    {
        wxMessageBox(_("There are invalid values in the input boxes.\nPlease check your inputs."),_("Warning"),wxOK | wxICON_INFORMATION, this);
        return;
    }
    m_preview->SetLens(m_projection,m_focallength,m_cropfactor);
    m_preview->SetLensDistortions(m_a,m_b,m_c,m_d,m_e);
};

