// -*- c-basic-offset: 4 -*-

/** @file ImageVariableDialog.cpp
 *
 *  @brief implementation of dialog to edit image variables
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

#include "hugin/ImageVariableDialog.h"
#include "base_wx/wxPlatform.h"
#include "panoinc.h"
#include "photometric/ResponseTransform.h"
#include <map>
#include <string>

#include "hugin/huginApp.h"
#include "CommandHistory.h"

using namespace HuginBase;

BEGIN_EVENT_TABLE(ImageVariableDialog,wxDialog)
    EVT_BUTTON(wxID_OK, ImageVariableDialog::OnOk)
    EVT_BUTTON(wxID_HELP, ImageVariableDialog::OnHelp)
    EVT_BUTTON(XRCID("image_show_distortion_graph"), ImageVariableDialog::OnShowDistortionGraph)
    EVT_BUTTON(XRCID("image_show_vignetting_graph"), ImageVariableDialog::OnShowVignettingGraph)
    EVT_BUTTON(XRCID("image_show_response_graph"), ImageVariableDialog::OnShowResponseGraph)
END_EVENT_TABLE()

ImageVariableDialog::ImageVariableDialog(wxWindow *parent, PT::Panorama* pano, HuginBase::UIntSet imgs)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("image_variables_dialog"));

#ifdef __WXMSW__
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(huginApp::Get()->GetXRCPath() + wxT("data/hugin.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);
    wxConfigBase * cfg = wxConfigBase::Get();
    //position
    int x = cfg->Read(wxT("/ImageVariablesDialog/positionX"),-1l);
    int y = cfg->Read(wxT("/ImageVariablesDialog/positionY"),-1l);
    if ( y >= 0 && x >= 0) 
    {
        this->Move(x, y);
    } 
    else 
    {
        this->Move(0, 44);
    };
    m_pano=pano;
    m_images=imgs;
    m_popup=NULL;
    InitValues();
};

ImageVariableDialog::~ImageVariableDialog()
{
    wxConfigBase * cfg = wxConfigBase::Get();
    wxPoint ps = this->GetPosition();
    cfg->Write(wxT("/ImageVariablesDialog/positionX"), ps.x);
    cfg->Write(wxT("/ImageVariablesDialog/positionY"), ps.y);
    cfg->Flush();
};

void ImageVariableDialog::SelectTab(size_t i)
{
    XRCCTRL(*this, "image_variable_notebook", wxNotebook)->SetSelection(i);
};

wxTextCtrl* GetImageVariableControl(const wxWindow* parent, const char* varname)
{
    return wxStaticCast(
                parent->FindWindow(
                    wxXmlResource::GetXRCID(
                        wxString(wxT("image_variable_")).append(wxString(varname, wxConvLocal)).c_str()
                    )
                ), wxTextCtrl
           );
};

void ImageVariableDialog::InitValues()
{
    if(m_images.size()==0)
    {
        return;
    };
    BaseSrcPanoImage::ResponseType responseType= m_pano->getImage(*m_images.begin()).getResponseType();
    bool identical=true;

    for(UIntSet::const_iterator it=m_images.begin();it!=m_images.end() && identical;it++)
    {
        identical=(responseType==m_pano->getImage(*it).getResponseType());
    };
    if(identical)
    {
        XRCCTRL(*this, "image_variable_responseType", wxChoice)->SetSelection(responseType);
    };

    int degDigits = wxConfigBase::Get()->Read(wxT("/General/DegreeFractionalDigitsEdit"),3);
    int pixelDigits = wxConfigBase::Get()->Read(wxT("/General/PixelFractionalDigitsEdit"),2);
    int distDigitsEdit = wxConfigBase::Get()->Read(wxT("/General/DistortionFractionalDigitsEdit"),5);
    
    VariableMapVector imgVarVector=m_pano->getVariables();

    for (const char** varname = m_varNames; *varname != 0; ++varname)
    {
        // update parameters
        int ndigits = distDigitsEdit;
        if (strcmp(*varname, "y") == 0 || strcmp(*varname, "p") == 0 ||
            strcmp(*varname, "r") == 0 || strcmp(*varname, "TrX") == 0 ||
            strcmp(*varname, "TrY") == 0 || strcmp(*varname, "TrZ") == 0 )
        {
            ndigits=degDigits;
        };
        if (strcmp(*varname, "v") == 0 || strcmp(*varname, "d") == 0 ||
            strcmp(*varname, "e") == 0 )
        {
            ndigits = pixelDigits;
        }
        double val=const_map_get(imgVarVector[*m_images.begin()],*varname).getValue();
        bool identical=true;
        for(UIntSet::const_iterator it=m_images.begin();it!=m_images.end() && identical;it++)
        {
            identical=(val==const_map_get(imgVarVector[*it],*varname).getValue());
        };
        if(identical)
        {
            GetImageVariableControl(this, *varname)->SetValue(hugin_utils::doubleTowxString(val,ndigits));
        };
    };
};

void ImageVariableDialog::SetGuiLevel(GuiLevel newLevel)
{
    // translation
    XRCCTRL(*this, "image_variable_text_translation", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_translation_x", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_translation_y", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_translation_z", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_TrX", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_TrY", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_TrZ", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
    // shear
    XRCCTRL(*this, "image_variable_text_shear", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_shear_g", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_text_shear_t", wxStaticText)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_g", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
    XRCCTRL(*this, "image_variable_t", wxTextCtrl)->Show(newLevel==GUI_EXPERT);
};

bool ImageVariableDialog::ApplyNewVariables()
{
    std::vector<PT::PanoCommand*> commands;
    VariableMap varMap;
    for (const char** varname = m_varNames; *varname != 0; ++varname)
    {
        wxString s=GetImageVariableControl(this, *varname)->GetValue();
        if(!s.empty())
        {
            double val;
            if(str2double(s,val))
            {
                if(strcmp(*varname, "v")==0)
                {
                    switch(m_pano->getImage(*m_images.begin()).getProjection())
                    {
                        case SrcPanoImage::RECTILINEAR:
                            if(val>179)
                            {
                                val=179;
                            };
                            break;
                        case SrcPanoImage::FISHEYE_ORTHOGRAPHIC:
                            if(val>190)
                            {
                                if(wxMessageBox(
                                    wxString::Format(_("You have given a field of view of %.2f degrees.\n But the orthographic projection is limited to a field of view of 180 degress.\nDo you want still use that high value?"), val),
#ifdef __WXMSW__
                                    _("Hugin"),
#else
                                    wxT(""),
#endif
                                    wxICON_EXCLAMATION | wxYES_NO)==wxNO)
                                {
                                    return false;
                                };
                            };
                            break;
                        default:
                            break;
                    };
                };
                varMap.insert(std::make_pair(std::string(*varname), Variable(std::string(*varname), val)));
            }
            else
            {
                wxLogError(_("Value must be numeric."));
                return false;
            };
        };
    };
    int sel=XRCCTRL(*this, "image_variable_responseType", wxChoice)->GetSelection();
    if(sel!=wxNOT_FOUND)
    {
        std::vector<SrcPanoImage> SrcImgs;
        for (UIntSet::const_iterator it=m_images.begin(); it!=m_images.end(); it++)
        {
            HuginBase::SrcPanoImage img=m_pano->getSrcImage(*it);
            img.setResponseType((SrcPanoImage::ResponseType)sel);
            SrcImgs.push_back(img);
        }
        commands.push_back(new PT::UpdateSrcImagesCmd( *m_pano, m_images, SrcImgs ));
    }
    if(varMap.size()>0)
    {
        for(UIntSet::const_iterator it=m_images.begin();it!=m_images.end();it++)
        {
            commands.push_back(
                new PT::UpdateImageVariablesCmd(*m_pano,*it,varMap)
                );
        };
    };
    if(commands.size()>0)
    {
        GlobalCmdHist::getInstance().addCommand(new PT::CombinedPanoCommand(*m_pano, commands));
        return true;
    }
    else
    {
        return false;
    };
};

void ImageVariableDialog::OnOk(wxCommandEvent & e)
{
    if(ApplyNewVariables())
    {
        e.Skip();
    };
};

void ImageVariableDialog::OnHelp(wxCommandEvent & e)
{
    // open help on appropriate page
    switch(XRCCTRL(*this, "image_variable_notebook", wxNotebook)->GetSelection())
    {
        //lens parameters
        case 1:
            MainFrame::Get()->DisplayHelp(wxT("/Lens_correction_model.html"));
            break;
        case 2:
            MainFrame::Get()->DisplayHelp(wxT("/Vignetting.html"));
            break;
        case 3:
            MainFrame::Get()->DisplayHelp(wxT("/Camera_response_curve.html"));
            break;
        default:
            MainFrame::Get()->DisplayHelp(wxT("/Image_positioning_model.html"));
            break;
    };
};

const char *ImageVariableDialog::m_varNames[] = { "y", "p", "r", "TrX", "TrY", "TrZ", 
                                  "v", "a", "b", "c", "d", "e", "g", "t",
                                  "Eev", "Er", "Eb", 
                                  "Vb", "Vc", "Vd", "Vx", "Vy",
                                  "Ra", "Rb", "Rc", "Rd", "Re", 0};

IMPLEMENT_CLASS(GraphPopupWindow, wxPopupTransientWindow)

BEGIN_EVENT_TABLE(GraphPopupWindow, wxPopupTransientWindow)
EVT_LEFT_DOWN(GraphPopupWindow::OnLeftDown)
END_EVENT_TABLE()

GraphPopupWindow::GraphPopupWindow(wxWindow* parent, wxBitmap bitmap) : wxPopupTransientWindow(parent)
{
    wxPanel* panel=new wxPanel(this);
    m_bitmapControl=new wxStaticBitmap(panel, wxID_ANY, bitmap);
    panel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(GraphPopupWindow::OnLeftDown), NULL, this);
    m_bitmapControl->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(GraphPopupWindow::OnLeftDown), NULL, this);
    m_bitmapControl->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(GraphPopupWindow::OnRightDown), NULL, this);
    wxBoxSizer* topsizer=new wxBoxSizer(wxHORIZONTAL);
    topsizer->Add(m_bitmapControl);
    panel->SetSizer(topsizer);
    topsizer->Fit(panel);
    topsizer->Fit(this);
};

void GraphPopupWindow::OnLeftDown(wxMouseEvent &e)
{
    Dismiss();
};

void GraphPopupWindow::OnRightDown(wxMouseEvent &e)
{
    wxConfigBase* config=wxConfigBase::Get();
    wxFileDialog dlg(this,
                        _("Save graph"),
                        config->Read(wxT("/actualPath"),wxT("")), wxT(""),
                        _("Bitmap (*.bmp)|*.bmp|PNG-File (*.png)|*.png"),
                        wxFD_SAVE, wxDefaultPosition);
    dlg.SetDirectory(config->Read(wxT("/actualPath"),wxT("")));
    dlg.SetFilterIndex(config->Read(wxT("/lastImageTypeIndex"), 0l));
    if (dlg.ShowModal() == wxID_OK)
    {
        config->Write(wxT("/actualPath"), dlg.GetDirectory());  // remember for later
        wxFileName filename(dlg.GetPath());
        int imageType=dlg.GetFilterIndex();
        config->Write(wxT("/lastImageTypeIndex"), imageType);
        if(!filename.HasExt())
        {
            switch(imageType)
            {
                case 1:
                    filename.SetExt(wxT("png"));
                    break;
                case 0:
                default:
                    filename.SetExt(wxT("bmp"));
                    break;
            };
        };
        if (filename.FileExists())
        {
            int d = wxMessageBox(wxString::Format(_("File %s exists. Overwrite?"), filename.GetFullPath().c_str()),
                                 _("Save image"), wxYES_NO | wxICON_QUESTION);
            if (d != wxYES)
            {
                return;
            }
        }
        switch(imageType)
        {
            case 1:
                m_bitmapControl->GetBitmap().SaveFile(filename.GetFullPath(), wxBITMAP_TYPE_PNG);
                break;
            case 0:
            default:
                m_bitmapControl->GetBitmap().SaveFile(filename.GetFullPath(), wxBITMAP_TYPE_BMP);
                break;
        };
    };
};

/**help class to draw charts */
class Graph
{
public:
    /** constructors, set size and background colour of resulting bitmap */
    Graph(int graphWidth, int graphHeight, wxColour backgroundColour)
    {
        m_width=graphWidth;
        m_height=graphHeight;
        //create bitmap
        m_bitmap=new wxBitmap(m_width, m_height);
        m_dc.SelectObject(*m_bitmap);
        m_dc.SetBackground(wxBrush(backgroundColour));
        m_dc.Clear();
        //draw border
        m_dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)));
        m_dc.SetBrush(*wxTRANSPARENT_BRUSH);
        m_dc.DrawRectangle(0, 0, m_width, m_height);
        SetChartArea(0,0, m_width, m_height);
        SetChartDisplay(0,0,1,1);
    };

    /** destructor */
    ~Graph()
    {
        m_dc.SelectObject(wxNullBitmap);
    };

    /** set where to draw the chart on the bitmap */
    void SetChartArea(int left, int top, int right, int bottom)
    {
        m_dc.DestroyClippingRegion();
        m_left=left;
        m_top=top;
        m_right=right;
        m_bottom=bottom;
        m_dc.SetClippingRegion(m_left-1, m_top-1, m_right-m_left+2, m_bottom-m_top+2);
    };

    /** set the real dimension of the chart */
    void SetChartDisplay(double xmin, double ymin, double xmax, double ymax)
    {
        m_xmin=xmin;
        m_ymin=ymin;
        m_xmax=xmax;
        m_ymax=ymax;
    };

    /** draws the grid with linesX lines in x-direction and linexY lines in y-direction */
    void DrawGrid(size_t linesX, size_t linesY)
    {
        m_dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)));
        for(size_t i=0; i<linesX; i++)
        {
            int x=TransformX((double)i*(m_xmax-m_xmin)/(linesX-1)+m_xmin);
            m_dc.DrawLine(x, m_top, x, m_bottom);
        };
        for(size_t i=0; i<linesY; i++)
        {
            int y=TransformY((double)i*(m_ymax-m_ymin)/(linesY-1)+m_ymin);
            m_dc.DrawLine(m_left, y, m_right, y);
        };
    };

    /** draws a line with the coordinates given in points */
    void DrawLine(std::vector<hugin_utils::FDiff2D> points, wxColour colour, int penWidth=1)
    {
        if(points.size()<2)
        {
            return;
        };
        wxPoint *polygonPoints=new wxPoint[points.size()];
        for(size_t i=0; i<points.size(); i++)
        {
            polygonPoints[i].x=TransformX(points[i].x);
            polygonPoints[i].y=TransformY(points[i].y);
        };
        m_dc.SetPen(wxPen(colour, penWidth));
        m_dc.DrawLines(points.size(), polygonPoints);
        delete []polygonPoints;
    };

    const wxBitmap GetGraph() const { return *m_bitmap; };
private:
    //helper function to transform coordinates from real world to bitmap
    int TransformX(float x)
    {
        return (x-m_xmin)/(m_xmax-m_xmin)*(m_right-m_left)+m_left;
    };
    int TransformY(float y)
    {
        return m_bottom-(y-m_ymin)/(m_ymax-m_ymin)*(m_bottom-m_top);
    };
    // area to be drawn
    double m_xmin, m_xmax, m_ymin, m_ymax;
    // size of canvas
    int m_width, m_height;
    // chart area
    int m_left, m_top, m_right, m_bottom;
    // bitmap
    wxBitmap* m_bitmap;
    wxMemoryDC m_dc;
};

void ImageVariableDialog::OnShowDistortionGraph(wxCommandEvent & e)
{
    wxString stringa=GetImageVariableControl(this, "a")->GetValue();
    wxString stringb=GetImageVariableControl(this, "b")->GetValue();
    wxString stringc=GetImageVariableControl(this, "c")->GetValue();
    if(stringa.empty() || stringb.empty() || stringc.empty())
    {
        wxBell();
        return;
    };
    std::vector<double> radialDist(4 ,0);
    if(!str2double(stringa, radialDist[0]) || !str2double(stringb, radialDist[1]) || !str2double(stringc, radialDist[2]))
    {
        wxBell();
        return;
    };
    radialDist[3] = 1 - radialDist[0] - radialDist[1] - radialDist[2];

    //create transformation
    SrcPanoImage srcImage;
#define NRPOINTS 100
    srcImage.setSize(vigra::Size2D(2*NRPOINTS, 2*NRPOINTS));
    srcImage.setRadialDistortion(radialDist);
    PanoramaOptions opts;
    opts.setHFOV(srcImage.getHFOV());
    opts.setProjection(PanoramaOptions::RECTILINEAR);
    opts.setWidth(2*NRPOINTS);
    opts.setHeight(2*NRPOINTS);
    HuginBase::PTools::Transform transform;
    transform.createTransform(srcImage, opts);

    //draw graph
    delete m_popup;
    Graph graph(300, 200, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    graph.SetChartArea(10, 10, 290, 190);
    graph.SetChartDisplay(0, -0.1, 1, 0.1);
    graph.DrawGrid(6, 7);
    std::vector<hugin_utils::FDiff2D> points;
    for(size_t i=0; i<NRPOINTS; i++)
    {
        double x,y;
        if(transform.transformImgCoord(x, y, NRPOINTS, NRPOINTS-i))
        {
            points.push_back(hugin_utils::FDiff2D(double(i)/NRPOINTS,(NRPOINTS-i-y)/(NRPOINTS)));
        };
    };
    graph.DrawLine(points, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), 2);

    //show popup
    m_popup = new GraphPopupWindow(this, graph.GetGraph());
    wxWindow *button = (wxWindow*) e.GetEventObject();
    wxPoint pos=button->ClientToScreen(wxPoint(0,0));
    m_popup->Position(pos, button->GetSize());
    m_popup->Popup();
};

void ImageVariableDialog::OnShowVignettingGraph(wxCommandEvent & e)
{
    wxString stringVb=GetImageVariableControl(this, "Vb")->GetValue();
    wxString stringVc=GetImageVariableControl(this, "Vc")->GetValue();
    wxString stringVd=GetImageVariableControl(this, "Vd")->GetValue();
    if(stringVb.empty() || stringVc.empty() || stringVd.empty())
    {
        wxBell();
        return;
    };
    std::vector<double> vigCorr(4,0);
    vigCorr[0]=1.0;
    if(!str2double(stringVb, vigCorr[1]) || !str2double(stringVc, vigCorr[2]) || !str2double(stringVd,  vigCorr[3]))
    {
        wxBell();
        return;
    };
    delete m_popup;
    Graph graph(300, 200, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    graph.SetChartArea(10, 10, 290, 190);
    graph.SetChartDisplay(0, 0.8, 1, 1);
    graph.DrawGrid(6, 6);

    //create ResponseTransform with vignetting only
    SrcPanoImage srcImage;
    srcImage.setRadialVigCorrCoeff(vigCorr);
    srcImage.setSize(vigra::Size2D(2*NRPOINTS, 2*NRPOINTS));
    Photometric::ResponseTransform<double> transform(srcImage);
    transform.enforceMonotonicity();

    //now calc vignetting curve
    std::vector<hugin_utils::FDiff2D> points;
#define NRPOINTS 100
    for(size_t i=0; i<=NRPOINTS; i++)
    {
        points.push_back(hugin_utils::FDiff2D(i/double(NRPOINTS),transform(1.0, hugin_utils::FDiff2D(NRPOINTS-i, NRPOINTS-i))));
    };
    graph.DrawLine(points, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), 2);

    //display popup
    m_popup = new GraphPopupWindow(this, graph.GetGraph());
    wxWindow *button = (wxWindow*) e.GetEventObject();
    wxPoint pos=button->ClientToScreen(wxPoint(0,0));
    m_popup->Position(pos, button->GetSize());
    m_popup->Popup();
};

void ImageVariableDialog::OnShowResponseGraph(wxCommandEvent & e)
{
    SrcPanoImage::ResponseType responseType=(SrcPanoImage::ResponseType)XRCCTRL(*this, "image_variable_responseType", wxChoice)->GetSelection();
    wxString stringRa=GetImageVariableControl(this, "Ra")->GetValue();
    wxString stringRb=GetImageVariableControl(this, "Rb")->GetValue();
    wxString stringRc=GetImageVariableControl(this, "Rc")->GetValue();
    wxString stringRd=GetImageVariableControl(this, "Rd")->GetValue();
    wxString stringRe=GetImageVariableControl(this, "Re")->GetValue();
    if(stringRa.empty() || stringRb.empty() || stringRc.empty() || stringRd.empty() || stringRe.empty())
    {
        wxBell();
        return;
    };
    double Ra, Rb, Rc, Rd, Re;
    if(!str2double(stringRa, Ra) || !str2double(stringRb, Rb) || !str2double(stringRc, Rc) ||
        !str2double(stringRd, Rd) || !str2double(stringRe, Re))
    {
        wxBell();
        return;
    };
    delete m_popup;
    Graph graph(300, 200, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    graph.SetChartArea(10, 10, 290, 190);
    graph.SetChartDisplay(0, 0, 1, 1);
    graph.DrawGrid(6, 6);
    switch(responseType)
    {
        case SrcPanoImage::RESPONSE_EMOR:
            {
                //draw standard EMOR curve
                std::vector<float> emor(5, 0.0);
                std::vector<double> outLutStd;
                vigra_ext::EMoR::createEMoRLUT(emor, outLutStd);
                vigra_ext::enforceMonotonicity(outLutStd);
                graph.SetChartDisplay(0, 0, outLutStd.size()-1.0, 1.0);
                std::vector<hugin_utils::FDiff2D> points;
                for(size_t i=0; i<outLutStd.size(); i++)
                {
                    points.push_back(hugin_utils::FDiff2D(i, outLutStd[i]));
                };
                graph.DrawLine(points, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), 1);
                outLutStd.clear();
                points.clear();
                // now draw our curve
                emor[0]=Ra;
                emor[1]=Rb;
                emor[2]=Rc;
                emor[3]=Rd;
                emor[4]=Re;
                std::vector<double> outLut;
                vigra_ext::EMoR::createEMoRLUT(emor, outLut);
                vigra_ext::enforceMonotonicity(outLut);
                graph.SetChartDisplay(0, 0, outLut.size()-1.0, 1.0);
                for(size_t i=0; i<outLut.size(); i++)
                {
                    points.push_back(hugin_utils::FDiff2D(i, outLut[i]));
                };
                graph.DrawLine(points, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), 2);
            };
            break;
        case SrcPanoImage::RESPONSE_LINEAR:
        default:
            {
                std::vector<hugin_utils::FDiff2D> points;
                points.push_back(hugin_utils::FDiff2D(0, 0));
                points.push_back(hugin_utils::FDiff2D(1, 1));
                graph.DrawLine(points, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), 2);
            };
            break;
    };
    //show popup
    m_popup = new GraphPopupWindow(this, graph.GetGraph());
    wxWindow *button = (wxWindow*) e.GetEventObject();
    wxPoint pos=button->ClientToScreen(wxPoint(0,0));
    m_popup->Position(pos, button->GetSize());
    m_popup->Popup();
};
