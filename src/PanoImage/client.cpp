/////////////////////////////////////////////////////////////////////////////
// Name:        client.h
// Purpose:     Client for wxSocket demo
// Author:      Guillermo Rodriguez Garcia <guille@iies.es>
// Modified by:
// Created:     1999/09/19
// RCS-ID:      $Id$
// Copyright:   (c) 1999 Guillermo Rodriguez Garcia
// Licence:     wxWindows licence
//              adopted for hugin by Kai-Uwe Behrmann 2003
/////////////////////////////////////////////////////////////////////////////

#include "client.h"


// --------------------------------------------------------------------------
// event tables and other macros for wxWindows
// --------------------------------------------------------------------------

BEGIN_EVENT_TABLE(Client, wxFrame)
  EVT_SOCKET(SOCKET_ID,     Client::OnSocketEvent)
END_EVENT_TABLE()

// ==========================================================================
// implementation
// ==========================================================================

// --------------------------------------------------------------------------
// the application class
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// frame
// --------------------------------------------------------------------------

// constructor
Client::Client() 
{
  DEBUG_INFO ( "" )
  connected = FALSE;

  // Create the socket
  m_sock = new wxSocketClient();

  // Setup the event handler and subscribe to most events
  m_sock->SetEventHandler(*this, SOCKET_ID);
  m_sock->SetNotify(wxSOCKET_CONNECTION_FLAG |
                    wxSOCKET_INPUT_FLAG |
                    wxSOCKET_LOST_FLAG);
  m_sock->Notify(TRUE);

  m_busy = FALSE;
  UpdateStatusBar();

  DEBUG_INFO ( "" )
  // direct connect
  wxIPV4address addr;

  // Ask user for server address
  wxString hostname = _("localhost");

  addr.Hostname(hostname);
  addr.Service(3000);

  m_sock->Connect(addr, FALSE);
  m_sock->WaitOnConnect(100);


  DEBUG_INFO ( "" )
  if (m_sock->IsConnected()) {
    connected = TRUE;
  } else {
    m_sock->Close();
    wxMessageBox(_("Can't connect to the specified host"), _("Alert !"));
  }
  
  UpdateStatusBar();
  DEBUG_INFO ( "" )
}

Client::~Client()
{
  DEBUG_INFO ( "" )
  // No delayed deletion here, as the frame is dying anyway
  m_sock->Destroy();
  delete m_sock;
  DEBUG_INFO ( "" )
}

// event handlers
void Client::GetServerData(wxSocketBase *sock)
{ 
  unsigned char len;
  char *buf;

  // Receive data from socket and send it back. We will first
  // get a byte with the buffer size, so we can specify the
  // exact size and use the wxSOCKET_WAITALL flag. Also, we
  // disabled input events so we won't have unwanted reentrance.
  // This way we can avoid the infamous wxSOCKET_BLOCK flag.
  
  sock->SetFlags(wxSOCKET_WAITALL);
  
  // Read the size
  sock->Read(&len, 1);
  buf = new char[len];
  
  // Read the data
  sock->Read(buf, len);

  wxString s = buf;
  frame->SetStatusText(s, 1);

  // Write it back
  sock->Write(buf, len);

  // load file
  frame->ShowFile (buf);

  DEBUG_INFO ( s << buf )
  delete[] buf;
  
} 

void Client::OnSocketEvent(wxSocketEvent& event)
{
  wxString s = _("OnSocketEvent: ");
  wxSocketBase *sock = event.GetSocket();

/*  switch(event.GetSocketEvent())
  {
    case wxSOCKET_INPUT      : s.Append(_("wxSOCKET_INPUT\n")); break;
    case wxSOCKET_LOST       : s.Append(_("wxSOCKET_LOST\n")); break;
    case wxSOCKET_CONNECTION : s.Append(_("wxSOCKET_CONNECTION\n")); break;
    default                  : s.Append(_("Unexpected event !\n")); break;
  }
*/
  // Now we process the event
  switch(event.GetSocketEvent())
  {
    case wxSOCKET_CONNECTION: {
      connected = TRUE;
      sock->Write ( 0xBF, 1 );
      break;
      }
    case wxSOCKET_INPUT:
    {
      // We disable input events, so that the test doesn't trigger
      // wxSocketEvent again.
      sock->SetNotify(wxSOCKET_LOST_FLAG);

      // Which test are we going to run?
      unsigned char c;
      sock->Read(&c, 1);

      switch (c)
      {
        case 0xBE: GetServerData(sock); break;
//        case 0xCE: Test2(sock); break;
//        case 0xDE: Test3(sock); break;
      }

      // Enable input events again.
      sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
      break;
    }
    case wxSOCKET_LOST:
    {
      sock->Destroy();
      connected = FALSE;
      break;
    }
    default: ;
  }

//  UpdateStatusBar();
}

// convenience functions

void Client::UpdateStatusBar()
{
  wxString s ("");

  if (!m_sock->IsConnected())
  {
    s.Printf(_("Not connected"));
  }
  else
  {
    wxIPV4address addr;
//    m_sock->GetPeer(addr);
//    s.Printf(_("%s : %d"), (addr.Hostname()).c_str(), addr.Service());
  }

  frame->SetStatusText(s, 1);
}
