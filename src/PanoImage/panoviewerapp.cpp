#include "panoviewerapp.h"
#include "mainframe.h"
#include "utils.h"

bool MyApp::OnInit()
{
	frame = new MyFrame("wxWindows Panorama Viewer, v0.4.2",
	                           wxPoint(50, 50), wxSize(400, 300), argc, argv);

	frame->Show(TRUE);
	//frame->ShowFullScreen(TRUE);
	SetTopWindow(frame);

    // call an wxSocketClient to recieve hugin commands
    client = new Client();

	return TRUE;
}

int MyApp::OnExit()
{
  DEBUG_INFO ( "" )   // close the connections by this
    delete client;
  DEBUG_INFO ( "" )
	return TRUE;
}


