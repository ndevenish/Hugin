#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

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

	pano1 = new PanoViewer(this, -1);

	// Load our control
/*   wxImage p;
   p.LoadFile("control.bmp");*/
   pano1->SetControl(menuView); // pano1->SetControl(p,menuView);
   pano1->SetResolution(300);

   isFullScreen = FALSE;

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
    if ( argc > 1 ) {
      DEBUG_INFO ( "argc = " << argc )
      wxString m_argv ( argv[1] );
      double d;
      if ( m_argv.IsAscii() )
        ShowFile( argv[1] );
      DEBUG_INFO ( _("open") <<" "<< m_argv )
      if (argc > 2) {
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
    }
    
}

MyFrame::~MyFrame ()
{
}

void MyFrame::OnFullScreen ( wxMenuEvent &event )
{
	if ( !isFullScreen )
	{
		pano1->ShowControl();
		ShowFullScreen(TRUE);
		isFullScreen = TRUE;
	} else {
		pano1->ShowControl(FALSE);
		ShowFullScreen(FALSE);
		isFullScreen = FALSE;
	}
}

void MyFrame::OnLoadFile ( wxMenuEvent &event )
{
    DEBUG_INFO ( load->GetWildcard() )
	if ( load->ShowModal() == wxID_OK )
	{
		wxImage p;
		p.LoadFile(load->GetPath());
		pano1->SetPano(p);
	}
}

void MyFrame::OnQuit ( wxMenuEvent &event )
{
	Close();
}

void MyFrame::OnPref ( wxMenuEvent &event )
{
	pref->ShowModal();
	pano1->SetResolution( qslider->GetValue() );
	pano1->SetMouseFactor( mslider->GetValue() );
}

void MyFrame::ShowFile ( wxString fn )
{
    wxImage p;
    if ( p.LoadFile(fn) ) {
      pano1->SetPano(p); 
      DEBUG_INFO ( " " << fn ) }
    else 
      DEBUG_INFO ( "failed " << fn )
}

