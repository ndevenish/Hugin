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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "FindPanoDialog.h"
#include "base_wx/wxPlatform.h"
#include "panoinc.h"
#include "PTBatcherGUI.h"
#include "hugin_utils/alphanum.hpp"

BEGIN_EVENT_TABLE(FindPanoDialog,wxDialog)
EVT_BUTTON(XRCID("find_pano_close"), FindPanoDialog::OnButtonClose)
EVT_BUTTON(XRCID("find_pano_select_dir"), FindPanoDialog::OnButtonChoose)
EVT_BUTTON(XRCID("find_pano_start_stop"), FindPanoDialog::OnButtonStart)
EVT_BUTTON(XRCID("find_pano_add_queue"), FindPanoDialog::OnButtonSend)
EVT_CLOSE(FindPanoDialog::OnClose)
END_EVENT_TABLE()

bool SortFilename::operator()(const SrcPanoImage* img1, const SrcPanoImage* img2)
{
    return doj::alphanum_comp<std::string>(img1->getFilename(),img2->getFilename())<0;
};

FindPanoDialog::FindPanoDialog(BatchFrame *batchframe, wxString xrcPrefix)
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
    m_cb_subdir=XRCCTRL(*this,"find_pano_subdir",wxCheckBox);
    m_statustext=XRCCTRL(*this,"find_pano_label",wxStaticText);
    m_list_pano=XRCCTRL(*this,"find_pano_list",wxCheckListBox);
    m_cb_naming=XRCCTRL(*this,"find_pano_naming",wxChoice);

    //set parameters
    wxConfigBase * config = wxConfigBase::Get();
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
    wxString path=config->Read(wxT("/FindPanoDialog/actualPath"),wxEmptyString);
    if(!path.IsEmpty())
        m_textctrl_dir->SetValue(path);
    bool val;
    config->Read(wxT("/FindPanoDialog/includeSubDirs"),&val,false);
    m_cb_subdir->SetValue(val);
    long i=config->Read(wxT("/FindPanoDialog/Naming"),0l);
    m_cb_naming->SetSelection(i);
    m_button_send->Disable();
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
    config->Write(wxT("/FindPanoDialog/actualPath"),m_textctrl_dir->GetValue());
    config->Write(wxT("/FindPanoDialog/includeSubDirs"),m_cb_subdir->GetValue());
    config->Write(wxT("/FindPanoDialog/Naming"),m_cb_naming->GetSelection());
    CleanUpPanolist();
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
void FindPanoDialog::OnClose(wxCloseEvent &e)
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

void FindPanoDialog::OnButtonClose(wxCommandEvent &e)
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

void FindPanoDialog::OnButtonChoose(wxCommandEvent &e)
{
    wxDirDialog dlg(this, _("Specify a directory to search for projects in"),
                     m_textctrl_dir->GetValue(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal()==wxID_OK)
    {
        m_textctrl_dir->SetValue(dlg.GetPath());
    };
};

void FindPanoDialog::OnButtonStart(wxCommandEvent &e)
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
            EnableButtons(false);
            SearchInDir(m_start_dir,m_cb_subdir->GetValue());
        }
        else
        {
            wxMessageBox(wxString::Format(_("Directory %s does not exist.\nPlease give an existing directory."),m_start_dir.c_str()),
                _("Warning"),wxOK | wxICON_EXCLAMATION,this);
        };
    };
}

void FindPanoDialog::OnButtonSend(wxCommandEvent &e)
{
    if(m_panos.size()==0)
        return;
    unsigned int nr=0;
    for(unsigned int i=0;i<m_list_pano->GetCount();i++)
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
    for(unsigned int i=0;i<m_list_pano->GetCount();i++)
    {
        if(m_list_pano->IsChecked(i))
        {
            wxString filename=m_panos[i]->GeneratePanorama((PossiblePano::NamingConvention)(m_cb_naming->GetSelection()));
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
        wxMessageBox(_("Not all project files could be written successfully.\nMaybe you have no write permission for these directories or your hard disc is full."),_("Error"),wxOK,this);
    };
    this->Close();
};

void FindPanoDialog::EnableButtons(const bool state)
{
    m_textctrl_dir->Enable(state);
    m_button_choose->Enable(state);
    m_cb_subdir->Enable(state);
    m_cb_naming->Enable(state);
    m_button_close->Enable(state);
    m_button_send->Enable(state);
};

void FindPanoDialog::SearchInDir(wxString dirstring, bool includeSubdir)
{
    wxDir dir(dirstring);
    std::vector<PossiblePano*> newPanos;
    wxTimeSpan max_diff(0,0,30,0); //max. 30 s difference between images
    wxString filename;
    bool cont=dir.GetFirst(&filename,wxEmptyString,wxDIR_FILES|wxDIR_HIDDEN);
    while(cont && !m_stopped)
    {
        m_statustext->SetLabel(wxString::Format(_("Reading file %s"),filename.c_str()));
        wxFileName file(dir.GetName(),filename);
        file.MakeAbsolute();
        wxString ext=file.GetExt();
        if(ext.CmpNoCase(wxT("jpg"))==0 || ext.CmpNoCase(wxT("jpeg"))==0 ||
        ext.CmpNoCase(wxT("tif"))==0 || ext.CmpNoCase(wxT("tiff"))==0)
        {
            std::string filenamestr(file.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
            SrcPanoImage *img=new SrcPanoImage(filenamestr);
            // this must be called before the first call to newPanos
            // otherwise the first/second image is not processed in the right way
            // But when running in debug mode it works; seems to be a kind of
            // race conditions
            wxGetApp().Yield(true);
            if(img->hasEXIFread())
            {
                bool found=false;
                for(unsigned int i=0;i<newPanos.size() && !m_stopped && !found;i++)
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
                    PossiblePano *newPano=new PossiblePano();
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
        cont=dir.GetNext(&filename);
    }
    if(!m_stopped && newPanos.size()>0)
    {
        for(size_t i=0;i<newPanos.size();i++)
        {
            if(newPanos[i]->GetImageCount()>1)
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
        cont=dir.GetFirst(&filename,wxEmptyString,wxDIR_DIRS);
        while(cont && !m_stopped)
        {
            SearchInDir(dir.GetName()+wxFileName::GetPathSeparator()+filename,includeSubdir);
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
            m_statustext->SetLabel(wxString::Format(_("Found %d possible panoramas."),m_panos.size()));
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
    if(m_images.size()>0)
    {
        for(ImageSet::reverse_iterator it=m_images.rbegin();it!=m_images.rend();it++)
        {
            delete (*it);
        }
    };
};

bool PossiblePano::BelongsTo(SrcPanoImage *img, const wxTimeSpan max_time_diff)
{
    if(m_make.compare(img->getExifMake())!=0)
        return false;
    if(m_camera.compare(img->getExifModel())!=0)
        return false;
    if(fabs(m_focallength-img->getExifFocalLength())>0.01)
        return false;
    if(m_size!=img->getSize())
        return false;
    if(!GetDateTime(img).IsBetween(m_dt_start-max_time_diff,m_dt_end+max_time_diff))
    {
        return false;
    };
    return true;
};

const wxDateTime PossiblePano::GetDateTime(const SrcPanoImage* img)
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

void PossiblePano::AddSrcPanoImage(HuginBase::SrcPanoImage *img)
{
    if(m_images.size()==0)
    {
        //fill all values from first image
        m_make=img->getExifMake();
        m_camera=img->getExifModel();
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
    return wxString::Format(_("%d images: %s"),m_images.size(),GetFilestring(BasePath).c_str());
};

bool PossiblePano::GetNewProjectFilename(NamingConvention nc,const wxString basePath, wxFileName &projectFile)
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

wxString PossiblePano::GeneratePanorama(NamingConvention nc)
{
    if(m_images.size()==0)
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
    for(ImageSet::iterator it=m_images.begin();it!=m_images.end();it++)
    {
        pano.addImage(*(*it));
    };
    //assign all images the same lens number
    HuginBase::StandardImageVariableGroups variable_groups(pano);
    HuginBase::ImageVariableGroup & lenses = variable_groups.getLenses();
    if(pano.getNrOfImages()>1)
    {
        for(unsigned int i=1;i<pano.getNrOfImages();i++)
        {
            SrcPanoImage img=pano.getSrcImage(i);
            double ev=img.getExposureValue();
            lenses.switchParts(i,lenses.getPartNumber(0));
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_ExposureValue, i);
            img.setExposureValue(ev);
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceRed, i);
            lenses.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_WhiteBalanceBlue, i);
            img.setWhiteBalanceRed(1);
            img.setWhiteBalanceBlue(1);
            pano.setSrcImage(i, img);
        };
    };
    //set default exposure value
    PanoramaOptions opts = pano.getOptions();
    opts.outputExposureValue = pano.getImage(0).getExposureValue();
    pano.setOptions(opts);

    std::ofstream script(projectFile.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
    script.exceptions ( std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit );
    if(!script.good())
    {
        return wxEmptyString;
    };
    PT::UIntSet all;
    fill_set(all, 0, pano.getNrOfImages()-1);
    try
    {
        std::string Pathprefix(projectFile.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR).mb_str(HUGIN_CONV_FILENAME));
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false, Pathprefix);
    }
    catch (std::exception & e) 
    {
        return wxEmptyString;
    };
    script.close();
    return projectFile.GetFullPath();
};

