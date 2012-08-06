// -*- c-basic-offset: 4 -*-

/** @file MaskEditorPanel.cpp
 *
 *  @brief implementation of MaskEditorPanel Class
 *
 *  @author Thomas Modes
 *
 *  $Id$
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

// often necessary before panoinc.h
#ifdef __APPLE__
#include "panoinc_WX.h"
#endif

#include "panoinc.h"

// hugin's
#include "base_wx/platform.h"
#include "hugin/MainFrame.h"
#include "hugin/config_defaults.h"
#include "hugin/CommandHistory.h"
#include "hugin/MaskEditorPanel.h"
#include "hugin/MaskLoadDialog.h"
#include <wx/clipbrd.h>

BEGIN_EVENT_TABLE(MaskEditorPanel, wxPanel)
    EVT_LIST_ITEM_SELECTED(XRCID("mask_editor_images_list"), MaskEditorPanel::OnImageSelect)
    EVT_LIST_ITEM_DESELECTED(XRCID("mask_editor_images_list"), MaskEditorPanel::OnImageSelect)
    EVT_LIST_ITEM_SELECTED(XRCID("mask_editor_mask_list"), MaskEditorPanel::OnMaskSelect)
    EVT_LIST_ITEM_DESELECTED(XRCID("mask_editor_mask_list"), MaskEditorPanel::OnMaskSelect)
    EVT_LIST_COL_END_DRAG(XRCID("mask_editor_mask_list"), MaskEditorPanel::OnColumnWidthChange)
    EVT_CHOICE(XRCID("mask_editor_choice_zoom"), MaskEditorPanel::OnZoom)
    EVT_CHOICE(XRCID("mask_editor_choice_masktype"), MaskEditorPanel::OnMaskTypeChange)
    EVT_BUTTON(XRCID("mask_editor_add"), MaskEditorPanel::OnMaskAdd)
    EVT_BUTTON(XRCID("mask_editor_load"), MaskEditorPanel::OnMaskLoad)
    EVT_BUTTON(XRCID("mask_editor_save"), MaskEditorPanel::OnMaskSave)
    EVT_BUTTON(XRCID("mask_editor_copy"), MaskEditorPanel::OnMaskCopy)
    EVT_BUTTON(XRCID("mask_editor_paste"), MaskEditorPanel::OnMaskPaste)
    EVT_BUTTON(XRCID("mask_editor_delete"), MaskEditorPanel::OnMaskDelete)
    EVT_CHECKBOX(XRCID("mask_editor_show_active_masks"), MaskEditorPanel::OnShowActiveMasks)
    EVT_COLOURPICKER_CHANGED(XRCID("mask_editor_colour_polygon_negative"),MaskEditorPanel::OnColourChanged)
    EVT_COLOURPICKER_CHANGED(XRCID("mask_editor_colour_polygon_positive"),MaskEditorPanel::OnColourChanged)
    EVT_COLOURPICKER_CHANGED(XRCID("mask_editor_colour_point_selected"),MaskEditorPanel::OnColourChanged)
    EVT_COLOURPICKER_CHANGED(XRCID("mask_editor_colour_point_unselected"),MaskEditorPanel::OnColourChanged)
END_EVENT_TABLE()

MaskEditorPanel::MaskEditorPanel()
{
    DEBUG_TRACE("**********************");
    m_pano = 0;

}

bool MaskEditorPanel::Create(wxWindow* parent, wxWindowID id,
                    const wxPoint& pos,
                    const wxSize& size,
                    long style,
                    const wxString& name)
{
    DEBUG_TRACE(" Create called *************");
    if (! wxPanel::Create(parent, id, pos, size, style, name))
    {
        return false;
    }

    m_ImageNr=UINT_MAX;
    m_MaskNr=UINT_MAX;
    m_File="";

    wxXmlResource::Get()->LoadPanel(this, wxT("mask_panel"));
    wxPanel * panel = XRCCTRL(*this, "mask_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer(topsizer);

    m_editImg = XRCCTRL(*this, "mask_editor_polygon_editor", MaskImageCtrl);
    assert(m_editImg);
    m_editImg->Init(this);

    // images list
    m_imagesListMask = XRCCTRL(*this, "mask_editor_images_list", ImagesListMask);
    // mask list
    m_maskList = XRCCTRL(*this, "mask_editor_mask_list", wxListCtrl);
    m_maskList->InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 35);
    m_maskList->InsertColumn( 1, _("Mask type"), wxLIST_FORMAT_LEFT, 120);

    //get saved width
    wxConfigBase *config=wxConfigBase::Get();
    for ( int j=0; j < m_maskList->GetColumnCount() ; j++ )
    {
        // -1 is auto
        int width = config->Read(wxString::Format( wxT("/MaskEditorPanel/ColumnWidth%d"), j ), -1);
        if(width != -1)
            m_maskList->SetColumnWidth(j, width);
    }
    bool activeMasks;
    config->Read(wxT("/MaskEditorPanel/ShowActiveMasks"),&activeMasks,false);
    XRCCTRL(*this,"mask_editor_show_active_masks",wxCheckBox)->SetValue(activeMasks);
    m_editImg->setDrawingActiveMasks(activeMasks);

    //load and set colours
    wxColour defaultColour;
    defaultColour.Set(wxT(HUGIN_MASK_COLOUR_POLYGON_NEGATIVE));
    wxColour colour=wxConfigBase::Get()->Read(wxT("/MaskEditorPanel/ColourPolygonNegative"),defaultColour.GetAsString(wxC2S_HTML_SYNTAX));
    XRCCTRL(*this,"mask_editor_colour_polygon_negative",wxColourPickerCtrl)->SetColour(colour);
    m_editImg->SetUserColourPolygonNegative(colour);
    defaultColour.Set(wxT(HUGIN_MASK_COLOUR_POLYGON_POSITIVE));
    colour=wxConfigBase::Get()->Read(wxT("/MaskEditorPanel/ColourPolygonPositive"),defaultColour.GetAsString(wxC2S_HTML_SYNTAX));
    XRCCTRL(*this,"mask_editor_colour_polygon_positive",wxColourPickerCtrl)->SetColour(colour);
    m_editImg->SetUserColourPolygonPositive(colour);
    defaultColour.Set(wxT(HUGIN_MASK_COLOUR_POINT_SELECTED));
    colour=wxConfigBase::Get()->Read(wxT("/MaskEditorPanel/ColourPointSelected"),defaultColour.GetAsString(wxC2S_HTML_SYNTAX));
    XRCCTRL(*this,"mask_editor_colour_point_selected",wxColourPickerCtrl)->SetColour(colour);
    m_editImg->SetUserColourPointSelected(colour);
    defaultColour.Set(wxT(HUGIN_MASK_COLOUR_POINT_UNSELECTED));
    colour=wxConfigBase::Get()->Read(wxT("/MaskEditorPanel/ColourPointUnselected"),defaultColour.GetAsString(wxC2S_HTML_SYNTAX));
    XRCCTRL(*this,"mask_editor_colour_point_unselected",wxColourPickerCtrl)->SetColour(colour);
    m_editImg->SetUserColourPointUnselected(colour);

    // other controls
    m_maskType = XRCCTRL(*this, "mask_editor_choice_masktype", wxChoice);
    // disable some controls
    m_maskType->Disable();
    XRCCTRL(*this, "mask_editor_choice_zoom", wxChoice)->Disable();
    XRCCTRL(*this, "mask_editor_add", wxButton)->Disable();
    XRCCTRL(*this, "mask_editor_load", wxButton)->Disable();
    XRCCTRL(*this, "mask_editor_save", wxButton)->Disable();
    XRCCTRL(*this, "mask_editor_copy", wxButton)->Disable();
    XRCCTRL(*this, "mask_editor_paste", wxButton)->Disable();
    XRCCTRL(*this, "mask_editor_delete", wxButton)->Disable();

    //set shortcuts
    wxAcceleratorEntry entries[2];
    entries[0].Set(wxACCEL_CMD,(int)'C',XRCID("mask_editor_copy"));
    entries[1].Set(wxACCEL_CMD,(int)'V',XRCID("mask_editor_paste"));
    wxAcceleratorTable accel(2, entries);
    SetAcceleratorTable(accel);

    // apply zoom specified in xrc file
    wxCommandEvent dummy;
    dummy.SetInt(XRCCTRL(*this,"mask_editor_choice_zoom",wxChoice)->GetSelection());
    OnZoom(dummy);
    return true;
}

void MaskEditorPanel::Init(PT::Panorama * pano)
{
    m_pano=pano;
    m_imagesListMask->Init(m_pano);
    // observe the panorama
    m_pano->addObserver(this);
}

MaskEditorPanel::~MaskEditorPanel()
{
    wxConfigBase::Get()->Write(wxT("/MaskEditorPanel/ShowActiveMasks"),XRCCTRL(*this,"mask_editor_show_active_masks",wxCheckBox)->GetValue());
    DEBUG_TRACE("dtor");
    m_pano->removeObserver(this);
}

void MaskEditorPanel::setImage(unsigned int imgNr)
{
    DEBUG_TRACE("image " << imgNr);
    bool restoreMaskSelection=(imgNr==m_ImageNr);
    bool updateImage=true;
    m_ImageNr = imgNr;
    HuginBase::MaskPolygonVector masksToDraw;
    if (imgNr == UINT_MAX) 
    {
        m_File = "";
        HuginBase::MaskPolygonVector mask;
        m_currentMasks=mask;
        m_editImg->setCrop(HuginBase::SrcPanoImage::NO_CROP,vigra::Rect2D());
    } 
    else 
    {
        const SrcPanoImage& image=m_pano->getImage(imgNr);
        updateImage=(m_File!=image.getFilename());
        if(updateImage)
            m_File=image.getFilename();
        else
            if(GetRot(imgNr)!=m_editImg->getCurrentRotation())
            {
                updateImage=true;
                m_File=image.getFilename();
            };
        m_currentMasks=image.getMasks();
        masksToDraw=image.getActiveMasks();
        m_editImg->setCrop(image.getCropMode(),image.getCropRect());
    };
    // update mask editor
    if(updateImage)
        m_editImg->setImage(m_File,m_currentMasks,masksToDraw,GetRot(imgNr));
    else
        m_editImg->setNewMasks(m_currentMasks,masksToDraw);
    if(m_currentMasks.size()==0)
        setMask(UINT_MAX);
    // enables or disables controls
    bool enableCtrl=(imgNr<UINT_MAX);
    XRCCTRL(*this, "mask_editor_choice_zoom", wxChoice)->Enable(enableCtrl);
    XRCCTRL(*this, "mask_editor_add", wxButton)->Enable(enableCtrl);
    XRCCTRL(*this, "mask_editor_delete", wxButton)->Enable(enableCtrl && m_MaskNr<UINT_MAX);
    XRCCTRL(*this, "mask_editor_load", wxButton)->Enable(enableCtrl);
    XRCCTRL(*this, "mask_editor_save", wxButton)->Enable(enableCtrl && m_MaskNr<UINT_MAX);
    XRCCTRL(*this, "mask_editor_paste", wxButton)->Enable(enableCtrl);
    XRCCTRL(*this, "mask_editor_copy", wxButton)->Enable(enableCtrl && m_MaskNr<UINT_MAX);
    UpdateMaskList(restoreMaskSelection);
    // FIXME: lets hope that nobody holds references to these images..
    ImageCache::getInstance().softFlush();
}

void MaskEditorPanel::setMask(unsigned int maskNr)
{
    m_MaskNr=maskNr;
    m_maskType->Enable(m_MaskNr<UINT_MAX);
    m_editImg->setActiveMask(m_MaskNr);
    XRCCTRL(*this,"mask_editor_delete", wxButton)->Enable(m_MaskNr<UINT_MAX);
    XRCCTRL(*this, "mask_editor_save", wxButton)->Enable(m_MaskNr<UINT_MAX);
    XRCCTRL(*this, "mask_editor_copy", wxButton)->Enable(m_MaskNr<UINT_MAX);
    if(m_ImageNr<UINT_MAX && m_MaskNr<UINT_MAX)
        m_maskType->SetSelection(m_currentMasks[m_MaskNr].getMaskType());
    else
        m_maskType->SetSelection(0);
};

void MaskEditorPanel::UpdateMask()
{
    if(m_ImageNr<UINT_MAX)
    {
        m_currentMasks=m_editImg->getNewMask();
        GlobalCmdHist::getInstance().addCommand(new PT::UpdateMaskForImgCmd(*m_pano,m_ImageNr,m_currentMasks));
    };
};

void MaskEditorPanel::AddMask()
{
    if(m_ImageNr<UINT_MAX)
    {
        m_currentMasks=m_editImg->getNewMask();
        GlobalCmdHist::getInstance().addCommand(new PT::UpdateMaskForImgCmd(*m_pano,m_ImageNr,m_currentMasks));
        //select added mask
        SelectMask(m_currentMasks.size()-1);
        m_editImg->selectAllMarkers();
    };
};

void MaskEditorPanel::SelectMask(unsigned int newMaskNr)
{
    if(m_ImageNr<UINT_MAX)
        if(newMaskNr<m_currentMasks.size())
            m_maskList->SetItemState(newMaskNr,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
        else
            m_maskList->SetItemState(m_MaskNr,0,wxLIST_STATE_SELECTED);
};

void MaskEditorPanel::panoramaChanged(PT::Panorama &pano)
{
};

void MaskEditorPanel::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    m_imagesListMask->panoramaImagesChanged(pano, changed);
    unsigned int nrImages = pano.getNrOfImages();
    ImageCache::getInstance().softFlush();
    if (nrImages==0)
        setImage(UINT_MAX);
    else
    {
        // select some other image if we deleted the current image
        if ((m_ImageNr<UINT_MAX) && (m_ImageNr >= nrImages))
            setImage(nrImages-1);
        else
            // update changed images
            if(set_contains(changed,m_ImageNr))
            {
                unsigned int countOldMasks=m_currentMasks.size();
                setImage(m_ImageNr);
                if(countOldMasks!=pano.getImage(m_ImageNr).getMasks().size())
                    SelectMask(UINT_MAX);
            };
    };
}

void MaskEditorPanel::OnImageSelect(wxListEvent &e)
{
    UIntSet selected=m_imagesListMask->GetSelected();
    //select no mask
    setMask(UINT_MAX);
    if(selected.size()==0)
        setImage(UINT_MAX);
    else
        setImage(*selected.begin());
};

void MaskEditorPanel::OnMaskSelect(wxListEvent &e)
{
    setMask(GetSelectedMask());
};

void MaskEditorPanel::OnMaskTypeChange(wxCommandEvent &e)
{
    if(m_ImageNr<UINT_MAX && m_MaskNr<UINT_MAX)
    {
        m_currentMasks[m_MaskNr].setMaskType((HuginBase::MaskPolygon::MaskType)e.GetSelection());
        GlobalCmdHist::getInstance().addCommand(new PT::UpdateMaskForImgCmd(*m_pano,m_ImageNr,m_currentMasks));
    };
};

void MaskEditorPanel::OnMaskAdd(wxCommandEvent &e)
{
    if(m_ImageNr<UINT_MAX)
    {
        //deselect current selected mask
        if(m_MaskNr<UINT_MAX)
            m_maskList->SetItemState(m_MaskNr,0,wxLIST_STATE_SELECTED);
        setMask(UINT_MAX);
        MainFrame::Get()->SetStatusText(_("Create a polygon mask by clicking with the left mouse button on image, set the last point with the right mouse button."),0);
        m_editImg->startNewPolygon();
    };
};

void MaskEditorPanel::OnMaskSave(wxCommandEvent &e)
{
    if(m_ImageNr<UINT_MAX && m_MaskNr<UINT_MAX)
    {
        wxFileDialog dlg(this, _("Save mask"),
                wxConfigBase::Get()->Read(wxT("/actualPath"), wxT("")),
                wxT(""), _("Mask files (*.msk)|*.msk|All files (*)|*"), 
                wxFD_SAVE, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) 
        {
            wxString fn = dlg.GetPath();
            if (fn.Right(4) != wxT(".msk")) 
            {
                fn.Append(wxT(".msk"));
            }
            if (wxFile::Exists(fn)) 
            {
                int d = wxMessageBox(wxString::Format(_("File %s exists. Overwrite?"), 
                        fn.c_str()),_("Save mask"), 
                        wxYES_NO | wxICON_QUESTION);
                if (d != wxYES) {
                    return;
                }
            }
            wxFileName filename = fn;
            std::ofstream maskFile(filename.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
            SaveMaskToStream(maskFile);
            maskFile.close();
        };
    }
};

void MaskEditorPanel::SaveMaskToStream(std::ostream& stream)
{
    vigra::Size2D imageSize = m_pano->getImage(m_ImageNr).getSize();
    stream << "# w" << imageSize.width() << " h" << imageSize.height() << std::endl;
    HuginBase::MaskPolygon maskToWrite = m_currentMasks[m_MaskNr];
    maskToWrite.printPolygonLine(stream,m_ImageNr);
};

void MaskEditorPanel::LoadMaskFromStream(std::istream& stream,vigra::Size2D& imageSize,HuginBase::MaskPolygonVector &newMasks)
{
    while (stream.good()) 
    {
        std::string line;
        std::getline(stream,line);
        switch (line[0]) 
        {
            case '#':
            {
                unsigned int w;
                if (getIntParam(w, line, "w"))
                    imageSize.setWidth(w);
                unsigned int h;
                if (getIntParam(h, line, "h"))
                    imageSize.setHeight(h);
                break;
            }
            case 'k':
            {
                HuginBase::MaskPolygon newPolygon;
                //Ignore image number set in mask
                newPolygon.setImgNr(m_ImageNr);
                unsigned int param;
                if (getIntParam(param,line,"t"))
                {
                    newPolygon.setMaskType((HuginBase::MaskPolygon::MaskType)param);
                }
                std::string format;
                if (getPTParam(format,line,"p"))
                {
                    if(newPolygon.parsePolygonString(format)) {
                        newMasks.push_back(newPolygon);
                    } 
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
};

void MaskEditorPanel::OnMaskLoad(wxCommandEvent &e)
{
    if (m_ImageNr<UINT_MAX)
    {
        wxFileDialog dlg(this,_("Load mask"),
                wxConfigBase::Get()->Read(wxT("/actualPath"),wxT("")),
                wxT(""),_("Mask files (*.msk)|*.msk|All files (*)|*"),
                wxFD_OPEN, wxDefaultPosition);
        if (dlg.ShowModal() != wxID_OK)
        {
            MainFrame::Get()->SetStatusText(_("Load mask: cancel"));
            return;
        }
        wxFileName filename(dlg.GetPath());
        std::ifstream in(filename.GetFullPath().mb_str(HUGIN_CONV_FILENAME));
        vigra::Size2D maskImageSize;
        HuginBase::MaskPolygonVector loadedMasks;
        LoadMaskFromStream(in,maskImageSize,loadedMasks);
        in.close();
        if(maskImageSize.area()==0 || loadedMasks.size()==0)
        {
            wxMessageBox(wxString::Format(_("Could not parse mask from file %s."),dlg.GetPath().c_str()),_("Warning"),wxOK | wxICON_EXCLAMATION,this);
            return;
        };
        // compare image size from file with that of current image alert user
        // if different.
        if (maskImageSize != m_pano->getImage(m_ImageNr).getSize()) 
        {
            MaskLoadDialog dlg(this);
            dlg.initValues(m_pano->getImage(m_ImageNr),loadedMasks,maskImageSize);
            if(dlg.ShowModal()!=wxID_OK)
            {
                // abort
                return;
            }
            loadedMasks=dlg.getProcessedMask();
        }
        for(unsigned int i=0;i<loadedMasks.size();i++)
            m_currentMasks.push_back(loadedMasks[i]);
        // Update the pano with the imported masks
        GlobalCmdHist::getInstance().addCommand(new PT::UpdateMaskForImgCmd(*m_pano,m_ImageNr,m_currentMasks));
    }
};

void MaskEditorPanel::OnMaskCopy(wxCommandEvent &e)
{
    if(m_ImageNr<UINT_MAX && m_MaskNr<UINT_MAX)
    {
        std::ostringstream stream;
        SaveMaskToStream(stream);
        if (wxTheClipboard->Open())
        {
            // This data objects are held by the clipboard,
            // so do not delete them in the app.
            wxTheClipboard->SetData(new wxTextDataObject(wxString(stream.str().c_str(),wxConvLocal)));
            wxTheClipboard->Close();
        };
    };
};

void MaskEditorPanel::OnMaskPaste(wxCommandEvent &e)
{
    if(m_ImageNr<UINT_MAX)
    {
        if (wxTheClipboard->Open())
        {
            vigra::Size2D maskImageSize;
            HuginBase::MaskPolygonVector loadedMasks;
            if (wxTheClipboard->IsSupported( wxDF_TEXT ))
            {
                wxTextDataObject data;
                wxTheClipboard->GetData(data);
                std::istringstream stream(std::string(data.GetText().mb_str()));
                LoadMaskFromStream(stream,maskImageSize,loadedMasks);
            }
            wxTheClipboard->Close();
            if(maskImageSize.area()==0 || loadedMasks.size()==0)
            {
                wxBell();
                return;
            };
            // compare image size from file with that of current image alert user
            // if different.
            if (maskImageSize != m_pano->getImage(m_ImageNr).getSize()) 
            {
                MaskLoadDialog dlg(this);
                dlg.initValues(m_pano->getImage(m_ImageNr),loadedMasks,maskImageSize);
                if(dlg.ShowModal()!=wxID_OK)
                {
                    // abort
                    return;
                }
                loadedMasks=dlg.getProcessedMask();
            }
            for(unsigned int i=0;i<loadedMasks.size();i++)
                m_currentMasks.push_back(loadedMasks[i]);
            // Update the pano with the imported masks
            GlobalCmdHist::getInstance().addCommand(new PT::UpdateMaskForImgCmd(*m_pano,m_ImageNr,m_currentMasks));
        };
    };
};

void MaskEditorPanel::OnMaskDelete(wxCommandEvent &e)
{
    if(m_ImageNr<UINT_MAX && m_MaskNr<UINT_MAX)
    {
        HuginBase::MaskPolygonVector editedMasks=m_currentMasks;
        editedMasks.erase(editedMasks.begin()+m_MaskNr);
        //setMask(UINT_MAX);
        GlobalCmdHist::getInstance().addCommand(new PT::UpdateMaskForImgCmd(*m_pano,m_ImageNr,editedMasks));
    };
};

void MaskEditorPanel::OnZoom(wxCommandEvent & e)
{
    double factor;
    switch (e.GetSelection()) 
    {
        case 0:
            factor = 1;
            break;
        case 1:
            // fit to window
            factor = 0;
            break;
        case 2:
            factor = 2;
            break;
        case 3:
            factor = 1.5;
            break;
        case 4:
            factor = 0.75;
            break;
        case 5:
            factor = 0.5;
            break;
        case 6:
            factor = 0.25;
            break;
        default:
            DEBUG_ERROR("unknown scale factor");
            factor = 1;
    }
    m_editImg->setScale(factor);
}

void MaskEditorPanel::OnColourChanged(wxColourPickerEvent &e)
{
    if(e.GetId()==XRCID("mask_editor_colour_polygon_negative"))
    {
        m_editImg->SetUserColourPolygonNegative(e.GetColour());
        wxConfigBase::Get()->Write(wxT("/MaskEditorPanel/ColourPolygonNegative"),e.GetColour().GetAsString(wxC2S_HTML_SYNTAX));
    }
    else 
        if(e.GetId()==XRCID("mask_editor_colour_polygon_positive"))
        {
            m_editImg->SetUserColourPolygonPositive(e.GetColour());
            wxConfigBase::Get()->Write(wxT("/MaskEditorPanel/ColourPolygonPositive"),e.GetColour().GetAsString(wxC2S_HTML_SYNTAX));
        }
        else
            if(e.GetId()==XRCID("mask_editor_colour_point_selected"))
            {
                m_editImg->SetUserColourPointSelected(e.GetColour());
                wxConfigBase::Get()->Write(wxT("/MaskEditorPanel/ColourPointSelected"),e.GetColour().GetAsString(wxC2S_HTML_SYNTAX));
            }
            else
            {
                m_editImg->SetUserColourPointUnselected(e.GetColour());
                wxConfigBase::Get()->Write(wxT("/MaskEditorPanel/ColourPointUnselected"),e.GetColour().GetAsString(wxC2S_HTML_SYNTAX));
            }
    m_editImg->Refresh(true);
};

void MaskEditorPanel::UpdateMaskList(bool restoreSelection)
{
    unsigned int oldSelection=GetSelectedMask();
    m_maskList->Freeze();
    if(m_ImageNr<UINT_MAX)
    {
        if(m_currentMasks.size()>0)
        {
            if(m_maskList->GetItemCount()!=m_currentMasks.size())
            {
                if(m_maskList->GetItemCount()<(int)m_currentMasks.size())
                {
                    //added masks
                    for(int i=m_maskList->GetItemCount();i<(int)m_currentMasks.size();i++)
                        m_maskList->InsertItem(i,wxString::Format(wxT("%d"),i));
                }
                else
                {
                    //deleted masks
                    for(int i=m_maskList->GetItemCount()-1;i>=(int)m_currentMasks.size();i--)
                        m_maskList->DeleteItem(i);
                };
            };
            for(unsigned int i=0;i<m_currentMasks.size();i++)
            {
                m_maskList->SetItem(i,1,m_maskType->GetString(m_currentMasks[i].getMaskType()));
                if(!restoreSelection && i==oldSelection)
                    m_maskList->SetItemState(i,0, wxLIST_STATE_SELECTED);
            };
        }
        else
            m_maskList->DeleteAllItems();
    }
    else
        m_maskList->DeleteAllItems();
    m_maskList->Thaw();
    m_maskType->Enable(m_maskList->GetSelectedItemCount()>0);
}

unsigned int MaskEditorPanel::GetSelectedMask()
{
    for(unsigned int i=0;i<(unsigned int)m_maskList->GetItemCount();i++)
    {
        if(m_maskList->GetItemState(i,wxLIST_STATE_SELECTED) & wxLIST_STATE_SELECTED)
            return i;
    };
    return UINT_MAX;
};

void MaskEditorPanel::OnColumnWidthChange( wxListEvent & e )
{
    int colNum = e.GetColumn();
    wxConfigBase::Get()->Write( wxString::Format(wxT("/MaskEditorPanel/ColumnWidth%d"),colNum), m_maskList->GetColumnWidth(colNum) );
}

MaskImageCtrl::ImageRotation MaskEditorPanel::GetRot(const unsigned int imgNr)
{
    if(imgNr==UINT_MAX)
        return MaskImageCtrl::ROT0;

    double pitch=m_pano->getImage(imgNr).getPitch();
    double roll=m_pano->getImage(imgNr).getRoll();
    
    MaskImageCtrl::ImageRotation rot = MaskImageCtrl::ROT0;
    // normalize roll angle
    while (roll > 360) roll-= 360;
    while (roll < 0) roll += 360;

    while (pitch > 180) pitch -= 360;
    while (pitch < -180) pitch += 360;
    bool headOver = (pitch > 90 || pitch < -90);

    if (roll >= 315 || roll < 45) 
    {
        rot = headOver ? MaskImageCtrl::ROT180 : MaskImageCtrl::ROT0;
    } 
    else 
        if (roll >= 45 && roll < 135) 
        {
            rot = headOver ? MaskImageCtrl::ROT270 : MaskImageCtrl::ROT90;
        }
        else 
            if (roll >= 135 && roll < 225) 
            {
                rot = headOver ? MaskImageCtrl::ROT0 : MaskImageCtrl::ROT180;
            } 
            else 
            {
                rot = headOver ? MaskImageCtrl::ROT90 : MaskImageCtrl::ROT270;
            }
    return rot;
}

void MaskEditorPanel::OnShowActiveMasks(wxCommandEvent &e)
{
    m_editImg->setDrawingActiveMasks(e.IsChecked());
};

IMPLEMENT_DYNAMIC_CLASS(MaskEditorPanel, wxPanel)

MaskEditorPanelXmlHandler::MaskEditorPanelXmlHandler()
                : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *MaskEditorPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, MaskEditorPanel)

    cp->Create(m_parentAsWindow,
                   GetID(),
                   GetPosition(), GetSize(),
                   GetStyle(wxT("style")),
                   GetName());

    SetupWindow(cp);

    return cp;
}

bool MaskEditorPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("MaskEditorPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(MaskEditorPanelXmlHandler, wxXmlResourceHandler)
