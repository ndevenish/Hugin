/////////////////////////////////////////////////////////////////////////////
// Name:        server.cpp
// Purpose:     Server for wxSocket demo
// Author:      Guillermo Rodriguez Garcia <guille@iies.es>
// Modified by:
// Created:     1999/09/19
// RCS-ID:      $Id$
// Copyright:   (c) 1999 Guillermo Rodriguez Garcia
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/** @file  Server.cpp
 *
 *  @brief implementation of Server Class
 *
 *  @author Guillermo Rodriguez Garcia <guille@iies.es>
 *
 *  $Id$
 */

#include "hugin/Server.h"
#include "hugin/huginApp.h"

// --------------------------------------------------------------------------
// event tables and other macros for wxWindows
// --------------------------------------------------------------------------

BEGIN_EVENT_TABLE(Server, wxWindow)
  EVT_SOCKET(SERVER_ID,  Server::OnServerEvent)
  EVT_SOCKET(SOCKET_ID,  Server::OnSocketEvent)
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

Server::Server()
{
  DEBUG_INFO ( "" )

  // Create the address - defaults to localhost:0 initially
  wxIPV4address addr;
  addr.Service(3000);

  // Create the socket
  m_server = new wxSocketServer(addr);
  DEBUG_INFO ( "" )

  // We use Ok() here to see if the server is really listening
  if (! m_server->Ok())
  {
  }
  else
  {
//    m_text->AppendText(_("Server listening.\n\n"));
  }

  DEBUG_INFO ( "" )
  // Setup the event handler and subscribe to connection events
  m_server->SetEventHandler(*this, SERVER_ID);
  m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
  m_server->Notify(TRUE);

  m_busy = FALSE;
  connected = TRUE;
  m_numClients = 0;
  DEBUG_INFO ( "" )
  UpdateStatusBar();
  DEBUG_INFO ( "" )
}

Server::~Server()
{
  DEBUG_INFO ( "" )
  // No delayed deletion here, as the frame is dying anyway
  delete m_server;
}

// event handlers


void Server::SendFilename( wxString filename )
{
  DEBUG_INFO ( "" )
  const wxChar *buf1;
  wxChar       *buf2;
  unsigned char len;

  // Disable socket menu entries (exception: Close Session)
  m_busy = TRUE;
  UpdateStatusBar();

  if ( m_numClients > 0 ) {
  } else {
    return;
  }
  DEBUG_INFO ( "" )

  if ( connected == FALSE ) {
    if ( m_sock->Ok() )
    {
//    m_text->AppendText(_("New client connection accepted\n\n"));
      DEBUG_INFO ( "connected: " << m_sock << "Ok? " << m_sock->Ok() )
    }
    else
    {
//    m_text->AppendText(_("Error: couldn't accept a new connection\n\n"));
      DEBUG_INFO ( "no conection" )
      return;
    };
  } else {
    connected = FALSE;
    DEBUG_INFO ( "connected: " << connected )
    return;
  }
  // Tell the other which test we are running
  unsigned char c = 0xBE;
  DEBUG_INFO ( "" )
  m_sock->Write(&c, 1);
  DEBUG_INFO ( "" )

  m_sock->SetFlags(wxSOCKET_WAITALL);
  DEBUG_INFO ( "" )

// ---------------------------------------------------------------------
  buf1 = filename;
// ---------------------------------------------------------------------
  len  = (wxStrlen(buf1) + 1) * sizeof(wxChar);
  buf2 = new wxChar[wxStrlen(buf1) + 1];

  DEBUG_INFO ( "" )
  m_sock->Write(&len, 1);
  m_sock->Write(buf1, len);

  DEBUG_INFO ( "" )
  m_sock->Read(buf2, len);

  DEBUG_INFO ( "" )
  if (memcmp(buf1, buf2, len) != 0)
  {
//    m_text->AppendText(_("failed!\n"));
//    m_text->AppendText(_("Test 1 failed !\n"));
  }
  else
  {
//    m_text->AppendText(_("done\n"));
//    m_text->AppendText(_("Test 1 passed !\n"));
  }

  DEBUG_INFO ( "" )
  delete[] buf2;
  m_busy = FALSE;
  DEBUG_INFO ( "" )
  UpdateStatusBar();
  DEBUG_INFO ( ": " << filename )
}

void Server::OnServerEvent(wxSocketEvent& event)
{
  DEBUG_INFO ( "connected " << connected)
  wxString s = _("OnServerEvent: ");
  wxSocketBase *sock;

  switch(event.GetSocketEvent())
  {
    case wxSOCKET_CONNECTION : s.Append(_("wxSOCKET_CONNECTION\n"));
                               connected = TRUE; break;
    default                  : s.Append(_("Unexpected event !\n")); break;
  }

  // Accept new connection if there is one in the pending
  // connections queue, else exit. We use Accept(FALSE) for
  // non-blocking accept (although if we got here, there
  // should ALWAYS be a pending connection).

  sock = m_server->Accept(FALSE);

  DEBUG_INFO ( "connected " << connected)
  if ( connected )//sock->Ok() )
  {
    // connection test
    unsigned char handshake;
    DEBUG_INFO ( "" )
    sock->SetTimeout(2); 
    sock->Read(&handshake, 1);
    DEBUG_INFO ( "sock->Ok " << sock->Ok() )

    m_sock->SetFlags(wxSOCKET_WAITALL);
    DEBUG_INFO ( "" )

    if ( handshake == 0xBF ) {
      handshake = 0xBE;
      sock->Write(&handshake, 1);
      DEBUG_INFO ( "handshake" )
    } else {
      DEBUG_INFO ( "no handshake" )
      sock->Close(); 
    };
//    m_text->AppendText(_("New client connection accepted\n\n"));
  }
  else
  {
//    m_text->AppendText(_("Error: couldn't accept a new connection\n\n"));
    DEBUG_INFO ( "do no handshake" )
//    sock->Close();
    return;
  }

  sock->SetEventHandler(*this, SOCKET_ID);
  sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
  sock->Notify(TRUE);

  m_sock = sock;
  m_numClients++;
  UpdateStatusBar();
  DEBUG_INFO ( "" )
}

void Server::OnSocketEvent(wxSocketEvent& event)
{
  DEBUG_INFO ( "" )
  wxString s = _("OnSocketEvent: ");
//  wxSocketBase *sock = event.GetSocket();

  // First, print a message
  switch(event.GetSocketEvent())
  {
    case wxSOCKET_INPUT : s.Append(_("wxSOCKET_INPUT\n")); break;
    case wxSOCKET_LOST  : s.Append(_("wxSOCKET_LOST\n")); connected=TRUE; break;
    case wxSOCKET_CONNECTION : s.Append(_("wxSOCKET_CONNECTION\n")); break;
    default             : s.Append(_("Unexpected event !\n")); break;
  }

//  m_text->AppendText(s);

  UpdateStatusBar();
}

// convenience functions

void Server::UpdateStatusBar()
{
  DEBUG_INFO ( "" )
  wxString s;
  DEBUG_INFO ( "" )
  s.Printf(_("%d clients connected"), m_numClients);
  DEBUG_INFO ( "m_numClients= " << m_numClients )
//  frame->SetStatusText(s, 1);
  DEBUG_INFO ( "" )
}
