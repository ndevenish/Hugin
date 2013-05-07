// -*- c-basic-offset: 4 -*-

/** @file PanoOutputDialog.cpp
 *
 *	@brief implementation of PanoOutputDialog class
 *
 *  @author T. Modes
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

#include "hugin/PanoOutputDialog.h"
#include "base_wx/wxPlatform.h"
#include "panoinc.h"

#include "hugin/huginApp.h"
#include "base_wx/platform.h"
#include "hugin/config_defaults.h"

BEGIN_EVENT_TABLE(PanoOutputDialog,wxDialog)
    EVT_BUTTON(wxID_OK, PanoOutputDialog::OnOk)
    EVT_CHECKBOX(XRCID("output_normal"), PanoOutputDialog::OnOutputChanged)
    EVT_CHECKBOX(XRCID("output_fused_blended"), PanoOutputDialog::OnOutputChanged)
    EVT_CHECKBOX(XRCID("output_blended_fused"), PanoOutputDialog::OnOutputChanged)
    EVT_CHECKBOX(XRCID("output_hdr"), PanoOutputDialog::OnOutputChanged)
    EVT_CHOICE(XRCID("output_ldr_format"), PanoOutputDialog::OnLDRFormatChanged)
    EVT_CHOICE(XRCID("output_hdr_format"), PanoOutputDialog::OnHDRFormatChanged)
    EVT_SPINCTRL(XRCID("output_width"), PanoOutputDialog::OnWidthChanged)
    EVT_SPINCTRL(XRCID("output_height"), PanoOutputDialog::OnHeightChanged)
END_EVENT_TABLE()

PanoOutputDialog::PanoOutputDialog(wxWindow *parent, PT::Panorama& pano, GuiLevel guiLevel) : m_pano(pano), m_aspect(0)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("pano_output_dialog"));

#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    //set parameters
    wxConfigBase * cfg = wxConfigBase::Get();
    //position
    int x = cfg->Read(wxT("/PanoOutputDialog/positionX"),-1l);
    int y = cfg->Read(wxT("/PanoOutputDialog/positionY"),-1l);
    if ( y >= 0 && x >= 0) 
    {
        this->Move(x, y);
    } 
    else 
    {
        this->Move(0, 44);
    };
    // get number of stacks and exposure layers
    m_guiLevel=guiLevel;
    m_stacks=getHDRStacks(m_pano, m_pano.getActiveImages(), m_pano.getOptions());
    m_exposureLayers=getExposureLayers(m_pano, m_pano.getActiveImages(), m_pano.getOptions());
    // set initial width
    long opt_width=m_pano.calcOptimalWidth();
    m_newOpt=m_pano.getOptions();
    double sizeFactor = HUGIN_ASS_PANO_DOWNSIZE_FACTOR;
    wxConfigBase* config = wxConfigBase::Get();
    config->Read(wxT("/Assistant/panoDownsizeFactor"), &sizeFactor, HUGIN_ASS_PANO_DOWNSIZE_FACTOR);
    m_newOpt.setWidth(hugin_utils::floori(sizeFactor*opt_width), true);
    m_initalWidth=m_newOpt.getWidth();
    m_initalROIWidth=m_newOpt.getROI().width();
    m_aspect=(double)m_newOpt.getROI().height()/m_newOpt.getROI().width();
    m_edit_width=XRCCTRL(*this, "output_width", wxSpinCtrl);
    m_edit_height=XRCCTRL(*this, "output_height", wxSpinCtrl);
    m_edit_width->SetValue(m_newOpt.getROI().width());
    m_edit_height->SetValue(m_newOpt.getROI().height());

    //LDR output format, as in preferences set
    int i = config->Read(wxT("/output/jpeg_quality"),HUGIN_JPEG_QUALITY);
    XRCCTRL(*this, "output_jpeg_quality", wxSpinCtrl)->SetValue(i);
    i=config->Read(wxT("/output/tiff_compression"), HUGIN_TIFF_COMPRESSION);
    XRCCTRL(*this, "output_tiff_compression", wxChoice)->SetSelection(i);
    i=config->Read(wxT("/output/ldr_format"), HUGIN_LDR_OUTPUT_FORMAT);
    XRCCTRL(*this, "output_ldr_format", wxChoice)->SetSelection(i);
    //HDR output format, as in project given
    if (m_newOpt.outputImageTypeHDR == "exr")
    {
        XRCCTRL(*this, "output_hdr_format", wxChoice)->SetSelection(0);
        XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->SetSelection(2);
    }
    else
    {
        XRCCTRL(*this, "output_hdr_format", wxChoice)->SetSelection(1);
        if (m_newOpt.outputImageTypeHDRCompression  == "PACKBITS")
        {
            XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->SetSelection(1);
        }
        else
        {
            if (m_newOpt.outputImageTypeHDRCompression == "LZW")
            {
                XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->SetSelection(2);
            }
            else
            {
                if (m_newOpt.outputImageTypeHDRCompression  == "DEFLATE")
                {
                    XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->SetSelection(3);
                }
                else
                {
                    XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->SetSelection(0);
                };
            };
        };
    };
    EnableOutputOptions();
    wxCommandEvent dummy;
    OnOutputChanged(dummy);
    OnLDRFormatChanged(dummy);
    OnHDRFormatChanged(dummy);
};

PanoOutputDialog::~PanoOutputDialog()
{
    wxConfigBase * cfg = wxConfigBase::Get();
    wxPoint ps = this->GetPosition();
    cfg->Write(wxT("/PanoOutputDialog/positionX"), ps.x);
    cfg->Write(wxT("/PanoOutputDialog/positionY"), ps.y);
    cfg->Flush();
};

void PanoOutputDialog::EnableOutputOptions()
{
    // check, if hdr images
    wxFileName file1(wxString(m_pano.getImage(0).getFilename().c_str(), HUGIN_CONV_FILENAME));
    wxString ext1=file1.GetExt().Lower();
    if(ext1==wxT(".hdr") || ext1==wxT(".exr"))
    {
        XRCCTRL(*this, "output_hdr", wxCheckBox)->SetValue(true);
        XRCCTRL(*this, "output_hdr", wxCheckBox)->Enable(true);
        XRCCTRL(*this, "output_hdr_bitmap", wxCheckBox)->Enable(true);
        return;
    }
    //hide hdr controls for simple interface
    if(m_guiLevel==GUI_SIMPLE)
    {
        XRCCTRL(*this, "output_hdr", wxCheckBox)->Hide();
        XRCCTRL(*this, "output_hdr_bitmap", wxCheckBox)->Hide();
        XRCCTRL(*this, "output_hdr_format_label", wxStaticText)->Hide();
        XRCCTRL(*this, "output_hdr_format", wxChoice)->Hide();
        XRCCTRL(*this, "output_hdr_compression_label", wxStaticText)->Hide();
        XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->Hide();
        Layout();
        GetSizer()->Fit(this);
    };
    //single image or normal panorama, enable only normal output
    if(m_pano.getNrOfImages()==1 || m_stacks.size() >= 0.8 * m_pano.getNrOfImages())
    {
        XRCCTRL(*this, "output_normal", wxCheckBox)->SetValue(true);
        XRCCTRL(*this, "output_normal", wxCheckBox)->Enable(true);
        XRCCTRL(*this, "output_normal_bitmap", wxCheckBox)->Enable(true);
        if(m_pano.getNrOfImages()==1 || m_stacks.size()==m_pano.getNrOfImages())
        {
            return;
        };
    };
    XRCCTRL(*this, "output_fused_blended", wxCheckBox)->Enable(true);
    XRCCTRL(*this, "output_fused_blended_bitmap", wxCheckBox)->Enable(true);
    XRCCTRL(*this, "output_blended_fused", wxCheckBox)->Enable(true);
    XRCCTRL(*this, "output_blended_fused_bitmap", wxCheckBox)->Enable(true);
    if(m_guiLevel!=GUI_SIMPLE)
    {
        XRCCTRL(*this, "output_hdr", wxCheckBox)->Enable(true);
        XRCCTRL(*this, "output_hdr_bitmap", wxCheckBox)->Enable(true);
    };
    if(m_pano.getNrOfImages() % m_stacks.size() == 0)
    {
        XRCCTRL(*this, "output_fused_blended", wxCheckBox)->SetValue(true);
    }
    else
    {
        if(m_exposureLayers.size()==1)
        {
            XRCCTRL(*this, "output_normal", wxCheckBox)->SetValue(true);
        }
        else
        {
            XRCCTRL(*this, "output_blended_fused", wxCheckBox)->SetValue(true);
        };
    };
};

void PanoOutputDialog::OnOk(wxCommandEvent & e)
{
    bool output_normal=XRCCTRL(*this, "output_normal", wxCheckBox)->GetValue();
    bool output_fused_blended=XRCCTRL(*this, "output_fused_blended", wxCheckBox)->GetValue();
    bool output_blended_fused=XRCCTRL(*this, "output_blended_fused", wxCheckBox)->GetValue();
    bool output_hdr=XRCCTRL(*this, "output_hdr", wxCheckBox)->GetValue();
    bool keep_intermediate=XRCCTRL(*this, "output_keep_intermediate", wxCheckBox)->GetValue();
    //normal output
    m_newOpt.outputLDRBlended=output_normal;
    m_newOpt.outputLDRLayers=output_normal && keep_intermediate;
    //fused stacks, then blended
    m_newOpt.outputLDRExposureBlended=output_fused_blended;
    m_newOpt.outputLDRExposureRemapped=(output_fused_blended || output_blended_fused) && keep_intermediate;
    m_newOpt.outputLDRStacks=output_fused_blended && keep_intermediate;
    // blended exposure layers, then fused
    m_newOpt.outputLDRExposureLayersFused=output_blended_fused;
    m_newOpt.outputLDRExposureLayers=output_blended_fused && keep_intermediate;
    // HDR output
    m_newOpt.outputHDRBlended=output_hdr;
    m_newOpt.outputHDRLayers=output_hdr && keep_intermediate;
    m_newOpt.outputHDRStacks=output_hdr && keep_intermediate;
    // read compression
    if(output_normal || output_fused_blended || output_blended_fused)
    {
        if(m_newOpt.outputImageType=="jpg")
        {
            m_newOpt.quality=XRCCTRL(*this, "output_jpeg_quality", wxSpinCtrl)->GetValue();
        }
        else
        {
            if(m_newOpt.outputImageType=="tif")
            {
                switch(XRCCTRL(*this, "output_tiff_compression", wxChoice)->GetSelection())
                {
                    case 0:
                    default:
                        m_newOpt.outputImageTypeCompression = "NONE";
                        m_newOpt.tiffCompression = "NONE";
                        break;
                    case 1:
                        m_newOpt.outputImageTypeCompression = "PACKBITS";
                        m_newOpt.tiffCompression = "PACKBITS";
                        break;
                    case 2:
                        m_newOpt.outputImageTypeCompression = "LZW";
                        m_newOpt.tiffCompression = "LZW";
                        break;
                    case 3:
                        m_newOpt.outputImageTypeCompression = "DEFLATE";
                        m_newOpt.tiffCompression = "DEFLATE";
                        break;
                };
            };
        };
    };
    //HDR compression
    if(output_hdr)
    {
        if(m_newOpt.outputImageTypeHDR=="tif")
        {
            switch(XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->GetSelection())
            {
                case 0:
                default:
                    m_newOpt.outputImageTypeHDRCompression = "NONE";
                    break;
                case 1:
                    m_newOpt.outputImageTypeHDRCompression = "PACKBITS";
                    break;
                case 2:
                    m_newOpt.outputImageTypeHDRCompression = "LZW";
                    break;
                case 3:
                    m_newOpt.outputImageTypeHDRCompression = "DEFLATE";
                    break;
            };
        };
    }
    // canvas size
    double scale=m_edit_width->GetValue()/m_initalROIWidth;
    m_newOpt.setWidth(hugin_utils::roundi(m_initalWidth*scale), true);
    //some checks to prevent some rounding errors, only for cropped outputs
    if(m_initalROIWidth < m_initalWidth)
    {
        if(m_newOpt.getROI().width()<m_edit_width->GetValue() || m_newOpt.getROI().height()<m_edit_height->GetValue())
        {
            m_newOpt.setWidth(m_newOpt.getWidth()+1, true);
        };
        vigra::Rect2D roi=m_newOpt.getROI();
        if(roi.width()>m_edit_width->GetValue() || roi.height()>m_edit_height->GetValue())
        {
            roi.setSize(m_edit_width->GetValue(), m_edit_height->GetValue());
            m_newOpt.setROI(roi);
        };
    };
    //send Ok
    EndModal(wxID_OK);
};

void PanoOutputDialog::OnOutputChanged(wxCommandEvent & e)
{
    bool output_normal=XRCCTRL(*this, "output_normal", wxCheckBox)->GetValue();
    bool output_fused_blended=XRCCTRL(*this, "output_fused_blended", wxCheckBox)->GetValue();
    bool output_blended_fused=XRCCTRL(*this, "output_blended_fused", wxCheckBox)->GetValue();
    bool output_hdr=XRCCTRL(*this, "output_hdr", wxCheckBox)->GetValue();
    //enable Ok only if at least one option is enabled
    XRCCTRL(*this, "wxID_OK", wxButton)->Enable(output_normal || output_fused_blended || output_blended_fused || output_hdr);
    XRCCTRL(*this, "output_ldr_format_label", wxStaticText)->Enable(output_normal || output_fused_blended || output_blended_fused);
    XRCCTRL(*this, "output_ldr_format", wxChoice)->Enable(output_normal || output_fused_blended || output_blended_fused);
    XRCCTRL(*this, "output_ldr_compression_label", wxStaticText)->Enable(output_normal || output_fused_blended || output_blended_fused);
    XRCCTRL(*this, "output_jpeg_quality", wxSpinCtrl)->Enable(output_normal || output_fused_blended || output_blended_fused);
    XRCCTRL(*this, "output_tiff_compression", wxChoice)->Enable(output_normal || output_fused_blended || output_blended_fused);
    XRCCTRL(*this, "output_hdr_format_label", wxStaticText)->Enable(output_hdr);
    XRCCTRL(*this, "output_hdr_format", wxChoice)->Enable(output_hdr);
    XRCCTRL(*this, "output_hdr_compression_label", wxStaticText)->Enable(output_hdr);
    XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->Enable(output_hdr);
    GetSizer()->Layout();
};

void PanoOutputDialog::OnLDRFormatChanged(wxCommandEvent & e)
{
    int sel = XRCCTRL(*this, "output_ldr_format", wxChoice)->GetSelection();
    switch (sel)
    {
        case 1:
            m_newOpt.outputImageType ="jpg";
            XRCCTRL(*this, "output_ldr_compression_label", wxStaticText)->Show();
            XRCCTRL(*this, "output_ldr_compression_label", wxStaticText)->SetLabel(_("Quality:"));
            XRCCTRL(*this, "output_jpeg_quality", wxSpinCtrl)->Show();
            XRCCTRL(*this, "output_tiff_compression", wxChoice)->Hide();
            break;
        case 2:
            m_newOpt.outputImageType ="png";
            XRCCTRL(*this, "output_ldr_compression_label", wxStaticText)->Hide();
            XRCCTRL(*this, "output_jpeg_quality", wxSpinCtrl)->Hide();
            XRCCTRL(*this, "output_tiff_compression", wxChoice)->Hide();
            break;
        default:
        case 0:
            m_newOpt.outputImageType ="tif";
            XRCCTRL(*this, "output_ldr_compression_label", wxStaticText)->Show();
            XRCCTRL(*this, "output_ldr_compression_label", wxStaticText)->SetLabel(_("Compression:"));
            XRCCTRL(*this, "output_jpeg_quality", wxSpinCtrl)->Hide();
            XRCCTRL(*this, "output_tiff_compression", wxChoice)->Show();
            break;
    };
    GetSizer()->Layout();
};


void PanoOutputDialog::OnHDRFormatChanged(wxCommandEvent & e)
{
    int sel = XRCCTRL(*this, "output_hdr_format", wxChoice)->GetSelection();
    switch (sel)
    {
        case 1:
            m_newOpt.outputImageTypeHDR="tif";
            XRCCTRL(*this, "output_hdr_compression_label", wxStaticText)->Show();
            XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->Show();
            break;
        case 0:
        default:
            m_newOpt.outputImageTypeHDR="exr";
            XRCCTRL(*this, "output_hdr_compression_label", wxStaticText)->Hide();
            XRCCTRL(*this, "output_hdr_tiff_compression", wxChoice)->Hide();
            break;
    };
    GetSizer()->Layout();
};

void PanoOutputDialog::OnWidthChanged(wxSpinEvent & e)
{
    if(m_aspect>0)
    {
        m_edit_height->SetValue(m_edit_width->GetValue()*m_aspect);
    };
};

void PanoOutputDialog::OnHeightChanged(wxSpinEvent & e)
{
    if(m_aspect>0)
    {
        m_edit_width->SetValue(m_edit_height->GetValue()/m_aspect);
    };
};
