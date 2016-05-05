// -*- c-basic-offset: 4 -*-

/** @file FindPanoDialog.cpp
 *
 *	@brief implementation of FindPanoDialog class
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FindPanoDialog.h"
#include "base_wx/wxPlatform.h"
#include "panoinc.h"
#include "panodata/OptimizerSwitches.h"
#include "PTBatcherGUI.h"
#include "hugin_utils/alphanum.h"
#include "hugin/config_defaults.h"
#include "wx/mstream.h"
#include "exiv2/exiv2.hpp"
#include "exiv2/preview.hpp"
#ifdef _WIN32
#include <CommCtrl.h>
#endif
#include "base_wx/LensTools.h"
#include "panodata/StandardImageVariableGroups.h"

enum
{
    ID_REMOVE_IMAGE = wxID_HIGHEST + 300,
    ID_SPLIT_PANOS = wxID_HIGHEST + 301
};

BEGIN_EVENT_TABLE(FindPanoDialog,wxDialog)
    EVT_BUTTON(XRCID("find_pano_close"), FindPanoDialog::OnButtonClose)
    EVT_BUTTON(XRCID("find_pano_select_dir"), FindPanoDialog::OnButtonChoose)
    EVT_BUTTON(XRCID("find_pano_start_stop"), FindPanoDialog::OnButtonStart)
    EVT_BUTTON(XRCID("find_pano_add_queue"), FindPanoDialog::OnButtonSend)
    EVT_LISTBOX(XRCID("find_pano_list"), FindPanoDialog::OnSelectPossiblePano)
    EVT_LIST_ITEM_RIGHT_CLICK(XRCID("find_pano_selected_thumbslist"), FindPanoDialog::OnListItemRightClick)
    EVT_MENU(ID_REMOVE_IMAGE, FindPanoDialog::OnRemoveImage)
    EVT_MENU(ID_SPLIT_PANOS, FindPanoDialog::OnSplitPanos)
    EVT_CLOSE(FindPanoDialog::OnClose)
END_EVENT_TABLE()

bool SortFilename::operator()(const HuginBase::SrcPanoImage* img1, const HuginBase::SrcPanoImage* img2)
{
    return doj::alphanum_comp(img1->getFilename(),img2->getFilename())<0;
};

// thumbnail size currently set to 80x80
#define THUMBSIZE 80

FindPanoDialog::FindPanoDialog(BatchFrame* batchframe, wxString xrcPrefix)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this,batchframe,wxT("find_pano_dialog"));

#ifdef __WXMSW__
    wxIcon myIcon(xrcPrefix+ wxT("data/ptbatcher.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(xrcPrefix + wxT("data/ptbatcher.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    m_batchframe=batchframe;
    m_isRunning=false;
    m_stopped=false;

    m_button_start=XRCCTRL(*this,"find_pano_start_stop",wxButton);
    m_button_choose=XRCCTRL(*this,"find_pano_select_dir",wxButton);
    m_button_send=XRCCTRL(*this,"find_pano_add_queue",wxButton);
    m_button_close=XRCCTRL(*this,"find_pano_close",wxButton);
    m_textctrl_dir=XRCCTRL(*this,"find_pano_dir",wxTextCtrl);
#if wxCHECK_VERSION(2,9,3)
    m_textctrl_dir->AutoCompleteDirectories();
#endif
    m_cb_subdir=XRCCTRL(*this,"find_pano_subdir",wxCheckBox);
    m_statustext=XRCCTRL(*this,"find_pano_label",wxStaticText);
    m_list_pano=XRCCTRL(*this,"find_pano_list",wxCheckListBox);
    m_ch_naming=XRCCTRL(*this,"find_pano_naming",wxChoice);
    m_cb_createLinks=XRCCTRL(*this,"find_pano_create_links",wxCheckBox);
    m_cb_loadDistortion=XRCCTRL(*this,"find_pano_load_distortion",wxCheckBox);
    m_cb_loadVignetting=XRCCTRL(*this,"find_pano_load_vignetting",wxCheckBox);
    m_sc_minNumberImages=XRCCTRL(*this, "find_pano_min_number_images", wxSpinCtrl);
    m_sc_maxTimeDiff=XRCCTRL(*this, "find_pano_max_time_diff", wxSpinCtrl);
    m_ch_blender = XRCCTRL(*this, "find_pano_default_blender", wxChoice);
    FillBlenderList(m_ch_blender);

    //set parameters
    wxConfigBase* config = wxConfigBase::Get();
    // restore position and size
    int dx,dy;
    wxDisplaySize(&dx,&dy);
    bool maximized = config->Read(wxT("/FindPanoDialog/maximized"), 0l) != 0;
    if (maximized)
    {
        this->Maximize();
    }
    else
    {
        //size
        int w = config->Read(wxT("/FindPanoDialog/width"),-1l);
        int h = config->Read(wxT("/FindPanoDialog/height"),-1l);
        if (w > 0 && w <= dx)
        {
            this->SetClientSize(w,h);
        }
        else
        {
            this->Fit();
        }
        //position
        int x = config->Read(wxT("/FindPanoDialog/positionX"),-1l);
        int y = config->Read(wxT("/FindPanoDialog/positionY"),-1l);
        if ( y >= 0 && x >= 0 && x < dx && y < dy)
        {
            this->Move(x, y);
        }
        else
        {
            this->Move(0, 44);
        }
    }
    long splitterPos = config->Read(wxT("/FindPanoDialog/splitterPos"), -1l);
    if (splitterPos != -1)
    {
        XRCCTRL(*this, "find_pano_splitter", wxSplitterWindow)->SetSashPosition(splitterPos);
    };
    wxString path=config->Read(wxT("/FindPanoDialog/actualPath"),wxEmptyString);
    if(!path.IsEmpty())
    {
        m_textctrl_dir->SetValue(path);
    }
    bool val;
    config->Read(wxT("/FindPanoDialog/includeSubDirs"),&val,false);
    m_cb_subdir->SetValue(val);
    long i=config->Read(wxT("/FindPanoDialog/Naming"),0l);
    m_ch_naming->SetSelection(i);
    config->Read(wxT("/FindPanoDialog/linkStacks"),&val,true);
    m_cb_createLinks->SetValue(val);
    config->Read(wxT("/FindPanoDialog/loadDistortion"),&val,false);
    m_cb_loadDistortion->SetValue(val);
    config->Read(wxT("/FindPanoDialog/loadVignetting"),&val,false);
    m_cb_loadVignetting->SetValue(val);
    i=config->Read(wxT("/FindPanoDialog/MinNumberImages"), 2l);
    m_sc_minNumberImages->SetValue(i);
    i=config->Read(wxT("/FindPanoDialog/MaxTimeDiff"), 30l);
    m_sc_maxTimeDiff->SetValue(i);
    i = config->Read(wxT("/FindPanoDialog/DefaultBlender"), static_cast<long>(HuginBase::PanoramaOptions::ENBLEND_BLEND));
    SelectListValue(m_ch_blender, i);
    m_button_send->Disable();
    m_thumbs = new wxImageList(THUMBSIZE, THUMBSIZE, true, 0);
    m_thumbsList = XRCCTRL(*this, "find_pano_selected_thumbslist", wxListCtrl);
    m_thumbsList->SetImageList(m_thumbs, wxIMAGE_LIST_NORMAL);
#ifdef _WIN32
    // default image spacing is too big, wxWidgets does not provide direct 
    // access to the spacing, so using the direct API function
    ListView_SetIconSpacing(m_thumbsList->GetHandle(), THUMBSIZE + 20, THUMBSIZE + 20);
#endif
};

FindPanoDialog::~FindPanoDialog()
{
    wxConfigBase* config=wxConfigBase::Get();
    if(!this->IsMaximized())
    {
        wxSize sz = this->GetClientSize();
        config->Write(wxT("/FindPanoDialog/width"), sz.GetWidth());
        config->Write(wxT("/FindPanoDialog/height"), sz.GetHeight());
        wxPoint ps = this->GetPosition();
        config->Write(wxT("/FindPanoDialog/positionX"), ps.x);
        config->Write(wxT("/FindPanoDialog/positionY"), ps.y);
        config->Write(wxT("/FindPanoDialog/maximized"), 0);
    }
    else
    {
        config->Write(wxT("/FindPanoDialog/maximized"), 1l);
    };
    config->Write(wxT("/FindPanoDialog/splitterPos"), XRCCTRL(*this, "find_pano_splitter", wxSplitterWindow)->GetSashPosition());
    config->Write(wxT("/FindPanoDialog/actualPath"),m_textctrl_dir->GetValue());
    config->Write(wxT("/FindPanoDialog/includeSubDirs"),m_cb_subdir->GetValue());
    config->Write(wxT("/FindPanoDialog/Naming"),m_ch_naming->GetSelection());
    config->Write(wxT("/FindPanoDialog/linkStacks"),m_cb_createLinks->GetValue());
    config->Write(wxT("/FindPanoDialog/loadDistortion"),m_cb_loadDistortion->GetValue());
    config->Write(wxT("/FindPanoDialog/loadVignetting"),m_cb_loadDistortion->GetValue());
    config->Write(wxT("/FindPanoDialog/MinNumberImages"), m_sc_minNumberImages->GetValue());
    config->Write(wxT("/FindPanoDialog/MaxTimeDiff"), m_sc_maxTimeDiff->GetValue());
    config->Write(wxT("/FindPanoDialog/DefaultBlender"), static_cast<long>(GetSelectedValue(m_ch_blender)));
    CleanUpPanolist();
    delete m_thumbs;
};

void FindPanoDialog::CleanUpPanolist()
{
    if(m_panos.size()>0)
    {
        while(!m_panos.empty())
        {
            delete m_panos.back();
            m_panos.pop_back();
        };
    };
};

//prevent closing window when running detection
void FindPanoDialog::OnClose(wxCloseEvent& e)
{
    if(e.CanVeto() && m_isRunning)
    {
        wxBell();
        e.Veto();
    }
    else
    {
        e.Skip();
    };
};

void FindPanoDialog::OnButtonClose(wxCommandEvent& e)
{
    if(m_panos.size()>0)
    {
        if(wxMessageBox(_("The list contains possibly unprocessed panoramas.\nIf you close the dialog, you will lose them.\nContinue anyway?"),
                        _("Question"),wxYES_NO|wxICON_QUESTION,this)==wxNO)
        {
            return;
        };
    };
    this->Close();
};

void FindPanoDialog::OnButtonChoose(wxCommandEvent& e)
{
    wxDirDialog dlg(this, _("Specify a directory to search for projects in"),
                    m_textctrl_dir->GetValue(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal()==wxID_OK)
    {
        m_textctrl_dir->SetValue(dlg.GetPath());
    };
};

void FindPanoDialog::OnButtonStart(wxCommandEvent& e)
{
    if(m_isRunning)
    {
        //stop detection
        m_stopped=true;
        m_button_start->SetLabel(_("Accepted"));
    }
    else
    {
        //start detection
        m_start_dir=m_textctrl_dir->GetValue();
        if(wxDir::Exists(m_start_dir))
        {
            if(m_panos.size()>0)
            {
                if(wxMessageBox(_("The list contains still not yet processed panoramas.\nIf you continue, they will be disregarded.\nDo you still want to continue?"),
                                _("Question"),wxYES_NO|wxICON_QUESTION,this)==wxNO)
                {
                    return;
                };
            };
            m_isRunning=true;
            m_stopped=false;
            //deactivate TIFF warning message boxes
            m_oldtiffwarning=TIFFSetWarningHandler(NULL);
            m_button_start->SetLabel(_("Stop"));
            CleanUpPanolist();
            m_list_pano->Clear();
            wxCommandEvent dummy;
            OnSelectPossiblePano(dummy);
            EnableButtons(false);
            SearchInDir(m_start_dir,m_cb_subdir->GetValue(), m_cb_loadDistortion->GetValue(), m_cb_loadVignetting->GetValue(), 
                m_sc_minNumberImages->GetValue(), m_sc_maxTimeDiff->GetValue());
        }
        else
        {
            wxMessageBox(wxString::Format(_("Directory %s does not exist.\nPlease give an existing directory."),m_start_dir.c_str()),
                         _("Warning"),wxOK | wxICON_EXCLAMATION,this);
        };
    };
}

void FindPanoDialog::OnButtonSend(wxCommandEvent& e)
{
    if(m_panos.size()==0)
    {
        return;
    }
    unsigned int nr=0;
    for(unsigned int i=0; i<m_list_pano->GetCount(); i++)
    {
        if(m_list_pano->IsChecked(i))
        {
            nr++;
        };
    };
    if(nr==0)
    {
        wxMessageBox(_("You have selected no possible panorama.\nPlease select at least one panorama and try again."),_("Warning"),wxOK|wxICON_EXCLAMATION,this);
        return;
    }
    bool failed=false;
    bool createLinks=m_cb_createLinks->GetValue();
    for(unsigned int i=0; i<m_list_pano->GetCount(); i++)
    {
        if(m_list_pano->IsChecked(i))
        {
            wxString filename=m_panos[i]->GeneratePanorama((PossiblePano::NamingConvention)(m_ch_naming->GetSelection()),createLinks, 
                static_cast<HuginBase::PanoramaOptions::BlendingMechanism>(GetSelectedValue(m_ch_blender)));
            if(!filename.IsEmpty())
            {
                m_batchframe->AddToList(filename,Project::DETECTING);
            }
            else
            {
                failed=true;
            };
        };
    };
    if(failed)
    {
        wxMessageBox(_("Not all project files could be written successfully.\nMaybe you have no write permission for these directories or your disc is full."),_("Error"),wxOK,this);
    };
    this->Close();
};

void FindPanoDialog::EnableButtons(const bool state)
{
    m_textctrl_dir->Enable(state);
    m_button_choose->Enable(state);
    m_cb_subdir->Enable(state);
    m_ch_naming->Enable(state);
    m_cb_createLinks->Enable(state);
    m_button_close->Enable(state);
    m_button_send->Enable(state);
};

void FindPanoDialog::OnSelectPossiblePano(wxCommandEvent &e)
{
    int selected = m_list_pano->GetSelection();
    if (selected != wxNOT_FOUND)
    {
        XRCCTRL(*this, "find_pano_selected_cam", wxStaticText)->SetLabel(m_panos[selected]->GetCameraName());
        XRCCTRL(*this, "find_pano_selected_lens", wxStaticText)->SetLabel(m_panos[selected]->GetLensName());
        XRCCTRL(*this, "find_pano_selected_focallength", wxStaticText)->SetLabel(m_panos[selected]->GetFocalLength());
        XRCCTRL(*this, "find_pano_selected_date_time", wxStaticText)->SetLabel(m_panos[selected]->GetStartString() + wxT(" (")+ m_panos[selected]->GetDuration() + wxT(")"));
        m_panos[selected]->PopulateListCtrl(m_thumbsList, m_thumbs);
    }
    else
    {
        XRCCTRL(*this, "find_pano_selected_cam", wxStaticText)->SetLabel(wxEmptyString);
        XRCCTRL(*this, "find_pano_selected_lens", wxStaticText)->SetLabel(wxEmptyString);
        XRCCTRL(*this, "find_pano_selected_focallength", wxStaticText)->SetLabel(wxEmptyString);
        XRCCTRL(*this, "find_pano_selected_date_time", wxStaticText)->SetLabel(wxEmptyString);
        m_thumbsList->DeleteAllItems();
        m_thumbs->RemoveAll();
    };
};

void FindPanoDialog::OnListItemRightClick(wxListEvent &e)
{
    // build menu
    wxMenu contextMenu;
    contextMenu.Append(ID_REMOVE_IMAGE, _("Remove image from project"));
    const int selectedPano = m_list_pano->GetSelection();
    long imageIndex = -1;
    imageIndex = m_thumbsList->GetNextItem(imageIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if(imageIndex > 1 && imageIndex <= static_cast<long>(m_panos[selectedPano]->GetImageCount()) - 2)
    {
        contextMenu.Append(ID_SPLIT_PANOS, _("Split here into two panoramas"));
    }
    // show popup menu
    PopupMenu(&contextMenu);
};

void FindPanoDialog::OnRemoveImage(wxCommandEvent &e)
{
    const int selectedPano = m_list_pano->GetSelection();
    if (selectedPano != wxNOT_FOUND)
    {
        if (m_panos[selectedPano]->GetImageCount() < 3)
        {
            // handle special case if pano contains after removing only 1 image
            wxMessageDialog message(this, _("Removing the image from the panorama will also remove the panorama from the list, because it contains then only one image. Do you want to remove the panorama?"),
#ifdef __WXMSW__
                _("PTBatcherGUI"),
#else
                wxT(""),
#endif
                wxYES_NO | wxICON_INFORMATION);
#if wxCHECK_VERSION(3,0,0)
            message.SetYesNoLabels(_("Remove image and panorama"), _("Keep panorama"));
#endif
            if (message.ShowModal() == wxID_YES)
            {
                // remove the pano
                delete m_panos[selectedPano];
                m_panos.erase(m_panos.begin() + selectedPano);
                // update the list of possible panos
                m_list_pano->Delete(selectedPano);
                // clear the labels
                wxCommandEvent dummy;
                OnSelectPossiblePano(dummy);
            };
        }
        else
        {
            long imageIndex = -1;
            imageIndex = m_thumbsList->GetNextItem(imageIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            if (imageIndex != wxNOT_FOUND)
            {
                // remove image from possible pano
                m_panos[selectedPano]->RemoveImage(imageIndex);
                // now remove from the wxListCtrl
                m_thumbsList->DeleteItem(imageIndex);
                // update the pano list
                m_list_pano->SetString(selectedPano, m_panos[selectedPano]->GetItemString(m_start_dir));
                // update the labels above
                wxCommandEvent dummy;
                OnSelectPossiblePano(dummy);
            };
        };
    };
};

void FindPanoDialog::OnSplitPanos(wxCommandEvent &e)
{
    const int selectedPano = m_list_pano->GetSelection();
    if (selectedPano != wxNOT_FOUND)
    {
        long imageIndex = -1;
        imageIndex = m_thumbsList->GetNextItem(imageIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (imageIndex != wxNOT_FOUND)
        {
            if (imageIndex <= 1 || imageIndex > static_cast<long>(m_panos[selectedPano]->GetImageCount()) - 2)
            {
                wxMessageBox(_("The panorama can't be split at this position because one subpanorama would contain only one image."),
#ifdef __WXMSW__
                    wxT("PTBatcherGUI"),
#else
                    wxEmptyString,
#endif
                    wxOK | wxICON_EXCLAMATION);
            }
            else
            {
                // do split
                PossiblePano* newSubPano = m_panos[selectedPano]->SplitPano(imageIndex);
                if (newSubPano->GetImageCount() > 0)
                {
                    // insert new pano into internal list
                    m_panos.insert(m_panos.begin() + selectedPano + 1, newSubPano);
                    // update pano list
                    m_list_pano->SetString(selectedPano, m_panos[selectedPano]->GetItemString(m_start_dir));
                    int newItem = m_list_pano->Insert(m_panos[selectedPano + 1]->GetItemString(m_start_dir), selectedPano + 1);
                    m_list_pano->Check(newItem, true);
                    // update display
                    wxCommandEvent dummy;
                    OnSelectPossiblePano(dummy);
                }
                else
                {
                    wxBell();
                    delete newSubPano;
                };
            };
        };
    };
}

int SortWxFilenames(const wxString& s1,const wxString& s2)
{
    return doj::alphanum_comp(std::string(s1.mb_str(wxConvLocal)),std::string(s2.mb_str(wxConvLocal)));
};

void FindPanoDialog::SearchInDir(wxString dirstring, const bool includeSubdir, const bool loadDistortion, const bool loadVignetting, const size_t minNumberImages, const size_t maxTimeDiff)
{
    std::vector<PossiblePano*> newPanos;
    wxTimeSpan max_diff(0, 0, maxTimeDiff, 0); 
    wxString filename;
    wxArrayString fileList;
    wxDir::GetAllFiles(dirstring,&fileList,wxEmptyString,wxDIR_FILES|wxDIR_HIDDEN);
    fileList.Sort(SortWxFilenames);
    //map for caching projection information to prevent reading from database for each image
    for(size_t j=0; j<fileList.size() && !m_stopped; j++)
    {
        m_statustext->SetLabel(wxString::Format(_("Reading file %s"),fileList[j].c_str()));
        wxFileName file(fileList[j]);
        file.MakeAbsolute();
        wxString ext=file.GetExt();
        if(ext.CmpNoCase(wxT("jpg"))==0 || ext.CmpNoCase(wxT("jpeg"))==0 ||
                ext.CmpNoCase(wxT("tif"))==0 || ext.CmpNoCase(wxT("tiff"))==0)
        {
            std::string filenamestr(file.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
            HuginBase::SrcPanoImage* img = new HuginBase::SrcPanoImage;
            img->setFilename(filenamestr);
            img->readEXIF();
            // check for black/white images, if so skip
            const HuginBase::FileMetaData& metadata = img->getFileMetadata();
            HuginBase::FileMetaData::const_iterator it = metadata.find("pixeltype");
            if (it != metadata.end())
            {
                if (it->second == "BILEVEL")
                { 
                    wxGetApp().Yield(true);
                    continue;
                };
            };
            img->applyEXIFValues();
            if(!img->getExifMake().empty() && !img->getExifModel().empty() && 
                img->getExifFocalLength()!=0 && img->getCropFactor()!=0)
            {
                img->readProjectionFromDB();
                if(loadDistortion)
                {
                    img->readDistortionFromDB();
                };
                if(loadVignetting)
                {
                    img->readVignettingFromDB();
                };
                bool found=false;
                for(unsigned int i=0; i<newPanos.size() && !m_stopped && !found; i++)
                {
                    //compare with all other image groups
                    if(newPanos[i]->BelongsTo(img,max_diff))
                    {
                        newPanos[i]->AddSrcPanoImage(img);
                        found=true;
                    };
                    if(i%10==0)
                    {
                        wxGetApp().Yield(true);
                    };
                };
                if(!found)
                {
                    PossiblePano* newPano=new PossiblePano();
                    newPano->AddSrcPanoImage(img);
                    newPanos.push_back(newPano);
                };
            }
            else
            {
                //could not read exif infos, disregard this image
                delete img;
            };
        };
        //allow processing events
        wxGetApp().Yield(true);
    };
    if(!m_stopped && newPanos.size()>0)
    {
        for(size_t i=0; i<newPanos.size(); i++)
        {
            if(newPanos[i]->GetImageCount()>=minNumberImages)
            {
                m_panos.push_back(newPanos[i]);
                int newItem=m_list_pano->Append(m_panos[m_panos.size()-1]->GetItemString(m_start_dir));
                m_list_pano->Check(newItem,true);
            }
            else
            {
                delete newPanos[i];
            };
        };
    };

    if(includeSubdir && !m_stopped)
    {
        //now we go into all directories
        wxDir dir(dirstring);
        bool cont=dir.GetFirst(&filename,wxEmptyString,wxDIR_DIRS);
        while(cont && !m_stopped)
        {
            SearchInDir(dir.GetName()+wxFileName::GetPathSeparator()+filename,includeSubdir, loadDistortion, loadVignetting, minNumberImages, maxTimeDiff);
            cont=dir.GetNext(&filename);
        }
    };
    if(m_start_dir.Cmp(dirstring)==0)
    {
        m_stopped=false;
        m_isRunning=false;
        m_button_start->SetLabel(_("Start"));
        EnableButtons(true);
        //enable send button if at least one panorama found
        m_button_send->Enable(m_panos.size()>0);
        if(m_panos.size()>0)
        {
            m_statustext->SetLabel(wxString::Format(_("Found %d possible panoramas."), static_cast<int>(m_panos.size())));
        }
        else
        {
            m_statustext->SetLabel(_("No possible panoramas found."));
        };
        TIFFSetWarningHandler(m_oldtiffwarning);
    };
};

PossiblePano::~PossiblePano()
{
    if(!m_images.empty())
    {
        for(ImageSet::reverse_iterator it=m_images.rbegin(); it!=m_images.rend(); ++it)
        {
            delete (*it);
        }
    };
};

bool PossiblePano::BelongsTo(HuginBase::SrcPanoImage* img, const wxTimeSpan max_time_diff)
{
    if(m_make.compare(img->getExifMake())!=0)
    {
        return false;
    }
    if(m_camera.compare(img->getExifModel())!=0)
    {
        return false;
    }
    if(m_lens.compare(img->getExifLens())!=0)
    {
        return false;
    }
    if(fabs(m_focallength-img->getExifFocalLength())>0.01)
    {
        return false;
    }
    if(m_size!=img->getSize())
    {
        return false;
    }
    if(!GetDateTime(img).IsBetween(m_dt_start-max_time_diff,m_dt_end+max_time_diff))
    {
        return false;
    };
    return true;
};

const wxDateTime PossiblePano::GetDateTime(const HuginBase::SrcPanoImage* img)
{
    struct tm exifdatetime;
    if(img->getExifDateTime(&exifdatetime)==0)
    {
        return wxDateTime(exifdatetime);
    }
    else
    {
        wxFileName file(wxString(img->getFilename().c_str(),HUGIN_CONV_FILENAME));
        return file.GetModificationTime();
    };
};

void PossiblePano::AddSrcPanoImage(HuginBase::SrcPanoImage* img)
{
    if(m_images.empty())
    {
        //fill all values from first image
        m_make=img->getExifMake();
        m_camera=img->getExifModel();
        m_lens=img->getExifLens();
        m_focallength=img->getExifFocalLength();
        m_size=img->getSize();
        m_dt_start=GetDateTime(img);
        m_dt_end=m_dt_start;
    }
    else
    {
        wxDateTime dt=GetDateTime(img);
        if(dt.IsEarlierThan(m_dt_start))
        {
            m_dt_start=dt;
        }
        if(dt.IsLaterThan(m_dt_end))
        {
            m_dt_end=dt;
        };
    };
    m_images.insert(img);
};

const wxString PossiblePano::GetFilestring(const wxString BasePath, const bool stripExtension) const
{
    ImageSet::const_iterator it=m_images.begin();
    wxFileName f1(wxString((*it)->getFilename().c_str(),HUGIN_CONV_FILENAME));
    f1.MakeRelativeTo(BasePath);
    ImageSet::const_reverse_iterator rit=m_images.rbegin();
    wxFileName f2(wxString((*rit)->getFilename().c_str(),HUGIN_CONV_FILENAME));
    if(stripExtension)
    {
        return f1.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR)+f1.GetName()+wxT("-")+f2.GetName();
    }
    else
    {
        return f1.GetFullPath()+wxT(" - ")+f2.GetFullName();
    };
};

const wxString PossiblePano::GetItemString(const wxString BasePath) const
{
    return wxString::Format(_("%d images: %s"), static_cast<int>(m_images.size()), GetFilestring(BasePath).c_str());
};

bool PossiblePano::GetNewProjectFilename(NamingConvention nc,const wxString basePath, wxFileName& projectFile)
{
    wxString mask;
    unsigned int i=1;
    projectFile.SetPath(basePath);
    projectFile.SetName(wxT("pano"));
    projectFile.SetExt(wxT("pto"));
    if(!projectFile.IsDirWritable())
    {
        return false;
    };
    switch(nc)
    {
        case NAMING_PANO:
            mask=wxT("panorama%d");
            break;
        case NAMING_FIRST_LAST:
            mask=GetFilestring(basePath,true);
            projectFile.SetName(mask);
            if(!projectFile.FileExists())
            {
                return true;
            };
            mask=mask+wxT("_%d");
            break;
        case NAMING_FOLDER:
            {
                wxArrayString folders=projectFile.GetDirs();
                if(folders.GetCount()==0)
                {
                    return false;
                }
                mask=folders.Last();
                projectFile.SetName(mask);
                if(!projectFile.FileExists())
                {
                    return true;
                }
                mask=mask+wxT("_%d");
            }
            break;
        case NAMING_TEMPLATE:
            {
                HuginBase::Panorama tempPano;
                tempPano.addImage(**m_images.begin());
                tempPano.addImage(**m_images.rbegin());
                wxFileName newProject(getDefaultProjectName(tempPano));
                mask=newProject.GetName();
                projectFile.SetName(mask);
                if(!projectFile.FileExists())
                {
                    return true;
                }
                mask=mask+wxT("_%d");
            };
            break;
        default:
            mask=wxT("panorama%d");
    };

    projectFile.SetName(wxString::Format(mask,i));
    while(projectFile.FileExists())
    {
        i++;
        projectFile.SetName(wxString::Format(mask,i));
        //security fall through
        if(i>1000)
        {
            return false;
        };
    }
    return true;
};

wxString PossiblePano::GeneratePanorama(NamingConvention nc, bool createLinks, HuginBase::PanoramaOptions::BlendingMechanism defaultBlender)
{
    if(m_images.empty())
    {
        return wxEmptyString;
    };
    ImageSet::const_iterator it=m_images.begin();
    wxFileName firstFile(wxString((*it)->getFilename().c_str(),HUGIN_CONV_FILENAME));
    firstFile.MakeAbsolute();
    wxFileName projectFile;
    if(!GetNewProjectFilename(nc,firstFile.GetPath(),projectFile))
    {
        return wxEmptyString;
    };
    //generate panorama
    HuginBase::Panorama pano;
    for(ImageSet::iterator it=m_images.begin(); it!=m_images.end(); ++it)
    {
        pano.addImage(*(*it));
    };
    //assign all images the same lens number
    HuginBase::StandardImageVariableGroups variable_groups(pano);
    HuginBase::ImageVariableGroup& lenses = variable_groups.getLenses();
    if(pano.getNrOfImages()>1)
    {
        double redBalanceAnchor=pano.getImage(pano.getOptions().colorReferenceImage).getExifRedBalance();
        double blueBalanceAnchor=pano.getImage(pano.getOptions().colorReferenceImage).getExifBlueBalance();
        if(fabs(redBalanceAnchor)<1e-2)
        {
            redBalanceAnchor=1;
        };
        if(fabs(blueBalanceAnchor)<1e-2)
        {
            blueBalanceAnchor=1;
        };
        for(unsigned int i=1; i<pano.getNrOfImages(); i++)
        {
            HuginBase::SrcPanoImage img = pano.getSrcImage(i);
            double ev=img.getExposureValue();
            lenses.switchParts(i,lenses.getPartNumber(0));
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_ExposureValue, i);
            img.setExposureValue(ev);
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceRed, i);
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceBlue, i);
            img.setWhiteBalanceRed(img.getExifRedBalance()/redBalanceAnchor);
            img.setWhiteBalanceBlue(img.getExifBlueBalance()/blueBalanceAnchor);
            pano.setSrcImage(i, img);
        };
    };
    if (pano.hasPossibleStacks())
    {
        pano.linkPossibleStacks(createLinks);
    };
    // Setup pano with options from preferences
    HuginBase::PanoramaOptions opts = pano.getOptions();
    //set default exposure value
    opts.outputExposureValue = pano.getImage(0).getExposureValue();
    wxConfigBase* config = wxConfigBase::Get();
    opts.quality = config->Read(wxT("/output/jpeg_quality"),HUGIN_JPEG_QUALITY);
    switch(config->Read(wxT("/output/tiff_compression"), HUGIN_TIFF_COMPRESSION))
    {
        case 0:
        default:
            opts.outputImageTypeCompression = "NONE";
            opts.tiffCompression = "NONE";
            break;
        case 1:
            opts.outputImageTypeCompression = "PACKBITS";
            opts.tiffCompression = "PACKBITS";
            break;
        case 2:
            opts.outputImageTypeCompression = "LZW";
            opts.tiffCompression = "LZW";
            break;
        case 3:
            opts.outputImageTypeCompression = "DEFLATE";
            opts.tiffCompression = "DEFLATE";
            break;
    }
    switch (config->Read(wxT("/output/ldr_format"), HUGIN_LDR_OUTPUT_FORMAT))
    {
        case 1:
            opts.outputImageType ="jpg";
            break;
        case 2:
            opts.outputImageType ="png";
            break;
        case 3:
            opts.outputImageType ="exr";
            break;
        default:
        case 0:
            opts.outputImageType ="tif";
            break;
    }
    opts.outputFormat = HuginBase::PanoramaOptions::TIFF_m;
    opts.blendMode = defaultBlender;
    opts.enblendOptions = config->Read(wxT("Enblend/Args"),wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
    opts.enfuseOptions = config->Read(wxT("Enfuse/Args"),wxT(HUGIN_ENFUSE_ARGS)).mb_str(wxConvLocal);
    opts.interpolator = (vigra_ext::Interpolator)config->Read(wxT("Nona/Interpolator"),HUGIN_NONA_INTERPOLATOR);
    opts.tiff_saveROI = config->Read(wxT("Nona/CroppedImages"),HUGIN_NONA_CROPPEDIMAGES)!=0;
    opts.hdrMergeMode = HuginBase::PanoramaOptions::HDRMERGE_AVERAGE;
    opts.hdrmergeOptions = HUGIN_HDRMERGE_ARGS;
    opts.verdandiOptions = config->Read(wxT("/VerdandiDefaultArgs"), wxEmptyString).mb_str(wxConvLocal);
    pano.setOptions(opts);
    // set optimizer switches
    pano.setOptimizerSwitch(HuginBase::OPT_POSITION);
    pano.setPhotometricOptimizerSwitch(HuginBase::OPT_EXPOSURE | HuginBase::OPT_VIGNETTING | HuginBase::OPT_RESPONSE);

    std::ofstream script(projectFile.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
    script.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
    if(!script.good())
    {
        return wxEmptyString;
    };
    HuginBase::UIntSet all;
    fill_set(all, 0, pano.getNrOfImages()-1);
    try
    {
        std::string Pathprefix(projectFile.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR).mb_str(HUGIN_CONV_FILENAME));
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false, Pathprefix);
    }
    catch (...)
    {
        return wxEmptyString;
    };
    script.close();
    return projectFile.GetFullPath();
};

wxString PossiblePano::GetCameraName()
{
    return wxString(m_camera.c_str(), wxConvLocal);
}

wxString PossiblePano::GetLensName()
{
    return wxString(m_lens.c_str(), wxConvLocal);
};

wxString PossiblePano::GetFocalLength()
{
    return wxString::Format(wxT("%0.1f mm"), m_focallength);
};

wxString PossiblePano::GetStartString()
{
    return m_dt_start.Format();
};

wxString PossiblePano::GetDuration()
{
    wxTimeSpan diff = m_dt_end.Subtract(m_dt_start);
    if (diff.GetSeconds() > 60)
    {
        return diff.Format(_("%M:%S min"));
    }
    else
    {
        return diff.Format(_("%S s"));
    };
};

void PossiblePano::PopulateListCtrl(wxListCtrl* list, wxImageList* thumbs)
{
    list->DeleteAllItems();
    thumbs->RemoveAll();
    wxBusyCursor cursor;
    for (ImageSet::iterator it = m_images.begin(); it != m_images.end(); ++it)
    {
        Exiv2::Image::AutoPtr image;
        bool opened = false;
        try
        {
            image = Exiv2::ImageFactory::open((*it)->getFilename().c_str());
            opened = true;
        }
        catch (...)
        {
            std::cerr << __FILE__ << " " << __LINE__ << " Error opening file" << std::endl;
        }
        int index = -1;
        if (opened)
        {
            image->readMetadata();
            // read all thumbnails
            Exiv2::PreviewManager previews(*image);
            Exiv2::PreviewPropertiesList lists = previews.getPreviewProperties();
            if (!lists.empty())
            {
                // select a preview with matching size
                int previewIndex = 0;
                while (previewIndex < lists.size() - 1 && lists[previewIndex].width_ < THUMBSIZE && lists[previewIndex].height_ < THUMBSIZE)
                {
                    ++previewIndex;
                };
                // load preview image to wxImage
                wxImage rawImage;
                Exiv2::PreviewImage previewImage = previews.getPreviewImage(lists[previewIndex]);
                wxMemoryInputStream stream(previewImage.pData(), previewImage.size());
                rawImage.LoadFile(stream, wxString(previewImage.mimeType().c_str(), wxConvLocal), -1);
                int x = 0;
                int y = 0;
                if (previewImage.width() > previewImage.height())
                {
                    //landscape format
                    int newHeight = THUMBSIZE*previewImage.height() / previewImage.width();
                    rawImage.Rescale(THUMBSIZE, newHeight);
                    x = 0;
                    y = (THUMBSIZE - newHeight) / 2;
                }
                else
                {
                    //portrait format
                    int newWidth = THUMBSIZE*previewImage.width() / previewImage.height();
                    rawImage.Rescale(newWidth, THUMBSIZE);
                    x = (THUMBSIZE - newWidth) / 2;
                    y = 0;
                }
                // create final bitmap with centered thumbnail
                wxBitmap bitmap(THUMBSIZE, THUMBSIZE);
                wxMemoryDC dc(bitmap);
                dc.SetBackground(list->GetBackgroundColour());
                dc.Clear();
                dc.DrawBitmap(rawImage, x, y);
                dc.SelectObject(wxNullBitmap);
                // create mask bitmap
                wxImage mask(THUMBSIZE, THUMBSIZE);
                mask.SetRGB(wxRect(0, 0, THUMBSIZE, THUMBSIZE), 0, 0, 0);
                mask.SetRGB(wxRect(x, y, THUMBSIZE - 2 * x, THUMBSIZE - 2 * y), 255, 255, 255);
                // add to wxImageList
                index = thumbs->Add(bitmap, wxBitmap(mask, 1));
            };
        };
        // create item in thumb list
        wxFileName fn(wxString((*it)->getFilename().c_str(), HUGIN_CONV_FILENAME));
        list->InsertItem(list->GetItemCount(), fn.GetFullName(), index);
    };
};

void PossiblePano::RemoveImage(const unsigned int index)
{
    // remove image with given index
    if (index < m_images.size())
    {
        ImageSet::iterator item = m_images.begin();
        std::advance(item, index);
        delete *item;
        m_images.erase(item);
        //update the internal times
        UpdateDateTimes();
    };
}

PossiblePano* PossiblePano::SplitPano(const unsigned int index)
{
    PossiblePano* newPano = new PossiblePano();
    if (index < m_images.size())
    {
        // now move all images to right pano
        ImageSet allImages = m_images;
        m_images.clear();
        ImageSet::iterator img = allImages.begin();
        while (m_images.size() < index && img != allImages.end())
        {
            m_images.insert(*img);
            ++img;
        };
        while (img != allImages.end())
        {
            newPano->AddSrcPanoImage(*img);
            ++img;
        }
        UpdateDateTimes();
    };
    return newPano;
}

void PossiblePano::UpdateDateTimes()
{
    // update internal stored start and end time
    m_dt_start = GetDateTime(*m_images.begin());
    m_dt_end = m_dt_start;
    for (auto& img : m_images)
    {
        wxDateTime dt = GetDateTime(img);
        if (dt.IsEarlierThan(m_dt_start))
        {
            m_dt_start = dt;
        }
        if (dt.IsLaterThan(m_dt_end))
        {
            m_dt_end = dt;
        };
    };
}
