#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include "wx/filename.h"

#include "mainframe.h"
#include "client.h"
#include "utils.h"

MyFrame * frame;
extern Client * m_client;


BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_MENU(FULLSCREEN, MyFrame::OnFullScreen)
	EVT_MENU(LOAD, MyFrame::OnLoadFile)
	EVT_MENU(QUIT, MyFrame::OnQuit)
	EVT_MENU(PREF, MyFrame::OnPref)
	EVT_MENU(GRID, MyFrame::OnGrid)
	EVT_MENU(VIEW, MyFrame::OnView)
END_EVENT_TABLE()



MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, int& argc, char **argv)
       : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
    wxMenu *menuFile = new wxMenu("", wxMENU_TEAROFF);
    wxMenu *menuView = new wxMenu("", wxMENU_TEAROFF);

    menuFile->Append(LOAD, "&Load...", "Load a 360x180 spherical panorama");
    menuFile->Append(SAVE, "&Save as...", "Save a 360x180 spherical panorama");
    menuFile->AppendSeparator();
    menuFile->Append(QUIT, "&Quit", "Quit this program");

    menuView->Append(FULLSCREEN, "&Full screen", "Toggle full screen mode");
    menuView->Append(PREF, "&Preferences", "Adjust various panorama viewing preferences");
    menuView->Append(GRID, "&Grid", "Show an grid");
    menuView->Append(VIEW, "&Reset view", "look at 0° , 0°");

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuView, "&View");

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

#if wxUSE_STATUSBAR
    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText("Welcome to the wxWindows panorama viewer");
#endif // wxUSE_STATUSBAR

   wxInitAllImageHandlers();

	pano = new PanoViewer(this, -1);

	// Load our control
/*   wxImage p;
   p.LoadFile("control.bmp");*/
   pano->SetControl(menuView); // pano1->SetControl(p,menuView);
   pano->SetResolution(300);

   isFullScreen = FALSE;
   projectionFormat = EQUIRECTANGULAR;   // enum ProjectionFormat
   width = 360;                          // moveable angle
   height = 180;                         // moveable angle
   resetView = TRUE;                     // look at the origin of the pano 0°,0°
   showGrid = FALSE;                     // grid for justifieing in hugin

   load = new wxFileDialog(this, "Load a 360x180 panorama...");
   load->SetWildcard("JPEG files (*.jpg)|*.jpg|BMP files (*.bmp)|*.bmp|GIF files (*.gif)|*.gif|PNG files (*.png)|*.png|All files|*.*");
   load->SetStyle(wxOPEN);

   pref = new wxDialog(this, -1, "Set panorama preferences", wxPoint(-1,-1), wxSize(320,200));
   new wxStaticText(pref, -1, "Image quality/Frame rate speed:", wxPoint(0,0));
   qslider = new wxSlider(pref, -1, 300, 100, 1000, wxPoint(10, 25), wxSize(300, 25) );
   new wxStaticText(pref, -1, "Speed", wxPoint(10,50));
   new wxStaticText(pref, -1, "Quality", wxPoint(250, 50));
   new wxStaticText(pref, -1, "Mouse sensitivity:", wxPoint(0, 80));
   mslider = new wxSlider(pref, -1, 750, 30, 10000, wxPoint(10,105), wxSize(300, 25) );
   new wxStaticText(pref, -1, "Low", wxPoint(10, 130));
   new wxStaticText(pref, -1, "High", wxPoint(270, 130));

    m_client = new Client();
    // Do something with the arguments - statically
    // FIXME Let panoviewer accept different mixed arguments.
    if ( argc > 1 ) {
      DEBUG_INFO ( "argc = " << argc )
      wxString m_argv ( argv[1] );
      double d;
      if ( m_argv.IsAscii() )
        filename = argv[1];
        ShowFile( filename );  // arg [1] : filename to load
      DEBUG_INFO ( _("open") <<" "<< m_argv )
      if (argc > 2) {  // arg [2] : port to listen for panoviewer client
        m_argv = argv[2];
        if (m_argv.IsNumber()) {
          if ( m_argv.ToDouble(&d) ) {
            DEBUG_INFO ( m_argv << " " << ((int)d) )
            m_client->Start ((int)d);
          } else {
            DEBUG_INFO ( m_argv )
            m_client->Start (3000);
          }
        }
      }
      if (argc > 3) {
        m_argv = argv[3]; //  arg [3] : ProjectionFormat
        if (m_argv.IsNumber()) {
          if ( m_argv.ToDouble(&d) ) {
            DEBUG_INFO ( m_argv << " " << ((int)d) )
            projectionFormat = (ProjectionFormat)d;
            setProjectionFormat(projectionFormat);
          } else {
            DEBUG_INFO ( "arg[3] " << _("failed: ") << m_argv )
            projectionFormat = EQUIRECTANGULAR;
          }
        }
      }
      if (argc > 4) {
        m_argv = argv[4]; //  arg [4] : width
        if (m_argv.IsNumber()) {
          if ( m_argv.ToDouble(&d) ) {
            DEBUG_INFO ( m_argv << " " << ((int)d) )
            width = (int)d;
          } else {
            DEBUG_INFO ( "arg[4] " << _("failed: ") << m_argv )
            width = 360;
          }
        }
      }
      if (argc > 5) {
        m_argv = argv[5]; //  arg [5] : height
        if (m_argv.IsNumber()) {
          if ( m_argv.ToDouble(&d) ) {
            DEBUG_INFO ( m_argv << " " << ((int)d) )
            height = (int)d;
          } else {
            DEBUG_INFO ( "arg[5] " << _("failed: ") << m_argv )
            height = 180;
          }
        }
      }
      if (argc > 6) {
        m_argv = argv[6]; //  arg [6] : resetView
        if (m_argv.IsNumber()) {
          if ( m_argv.ToDouble(&d) ) {
            DEBUG_INFO ( m_argv << " " << ((bool)d) )
            resetView = (bool)d;
          } else {
            DEBUG_INFO ( "arg[6] " << _("failed: ") << m_argv )
            resetView = 1;
          }
        }
      }
      if (argc > 7) {
        m_argv = argv[7]; //  arg [7] : showGrid
        if (m_argv.IsNumber()) {
          if ( m_argv.ToDouble(&d) ) {
            DEBUG_INFO ( m_argv << " " << ((bool)d) )
            showGrid = (bool)d;
          } else {
            DEBUG_INFO ( "arg[7] " << _("failed: ") << m_argv )
            showGrid = 0;
          }
        }
      }
    }
    
}

MyFrame::~MyFrame ()
{
}

void MyFrame::OnFullScreen ( wxMenuEvent &event )
{
	if ( !isFullScreen )
	{
		pano->ShowControl();
		ShowFullScreen(TRUE);
		isFullScreen = TRUE;
	} else {
		pano->ShowControl(FALSE);
		ShowFullScreen(FALSE);
		isFullScreen = FALSE;
	}
}

void MyFrame::OnLoadFile ( wxMenuEvent &event )
{
    DEBUG_INFO ( load->GetWildcard() )
	if ( load->ShowModal() == wxID_OK )
	{
                width = 360;    // We expect an equirectangular pano.
                height = 180;
		wxImage p;
		p.LoadFile(load->GetPath());
                filename = load->GetPath();
		pano->SetPano(p);
	}
}

void MyFrame::OnView ( wxMenuEvent &event )
{
    PanoViewpoint vp;
    vp.SetYawLimit ( width );
    vp.SetPitchLimit ( height );
    pano->setView( vp );
    DEBUG_INFO ( "width = " << width << " height = " << height )
}

void MyFrame::OnGrid ( wxMenuEvent &event )
{
    DEBUG_INFO ( "showGrid " << showGrid )
	if ( showGrid )
	{
          showGrid = FALSE;
	} else {
          showGrid = TRUE;
        }
    ShowFile ( filename );
}

void MyFrame::OnQuit ( wxMenuEvent &event )
{
	Close();
}

void MyFrame::OnPref ( wxMenuEvent &event )
{
	pref->ShowModal();
	pano->SetResolution( qslider->GetValue() );
	pano->SetMouseFactor( mslider->GetValue() );
}

void MyFrame::ShowFile ( wxFileName file )
{
    wxImage p;
    if ( file.FileExists() ) {
      if ( p.LoadFile(file.GetFullPath()) ) {
        pano->showGrid(showGrid);
        if ( showGrid ) {
          wxBitmap bmp = p.ConvertToBitmap();
          wxMemoryDC mdc;
          mdc.SelectObject(bmp);
          mdc.BeginDrawing();
          double v_lines = 36.0;   // TODO work for the other projections
          double h_lines = 18.0;
          for ( double i = 0.0 ; i < v_lines ; i++ ) {
            mdc.CrossHair((int)((double)p.GetWidth()/2
                              + (double)p.GetWidth()/v_lines * i),
                          (int)((double)p.GetHeight()/2
                              + (double)p.GetHeight()/h_lines * i));
            mdc.CrossHair((int)((double)p.GetWidth()/2
                              + (double)p.GetWidth()/v_lines * -i),
                          (int)((double)p.GetHeight()/2
                              + (double)p.GetHeight()/h_lines * -i));
          }
          mdc.EndDrawing();
          p = bmp;
        }
        pano->SetPano(p); 
        DEBUG_INFO ( " " << file.GetFullPath() ) }
      else 
        DEBUG_INFO ( _("failed: ") << file.GetFullPath() )
    } else {
      DEBUG_INFO ( _("failed: ") << file.GetFullPath() << _(" dont exist") )
    }
}

void MyFrame::setProjectionFormat ( ProjectionFormat f )
{
    PanoViewpoint vp (pano->getView());
    vp.SetFormat ( f );
    vp.SetYawLimit ( width );
    vp.SetPitchLimit ( height );
    pano->setView( vp );
    DEBUG_INFO ( _("projectionFormat ") << projectionFormat)
}

void MyFrame::setViewWidth ( int width )
{
    DEBUG_INFO ( _("TODO") << _(" setViewWidth ") << width)
}

void MyFrame::setViewHeight ( int height )
{
    DEBUG_INFO ( _("TODO") << _(" setViewHeigth ") << height)
}

void MyFrame::setGrid ( bool yes )
{
    showGrid = yes;
    ShowFile ( filename );
    DEBUG_INFO ( _(" setGrid ") << showGrid )
}


