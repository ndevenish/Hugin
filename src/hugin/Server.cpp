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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/filename.h>
#include <wx/config.h>

#include "hugin/Server.h"
#include "hugin/huginApp.h"
#include "hugin/MainFrame.h"

#include <fstream>

// --------------------------------------------------------------------------
// event tables and other macros for wxWindows
// --------------------------------------------------------------------------

BEGIN_EVENT_TABLE(Server, wxWindow)
  EVT_SOCKET (SERVER_ID,  Server::OnServerEvent)
  EVT_SOCKET (SOCKET_ID,  Server::OnSocketEvent)
  EVT_TIMER  (TIMER_ID,   Server::OnServerTimer)
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

Server::Server()  : server_timer(this, TIMER_ID)
{
  DEBUG_INFO ( "" )

  port = 3000;

    projectionFormat = PanoramaOptions::EQUIRECTANGULAR;
    height = 180;
    width = 360;
    resetView = FALSE;
    showGrid = FALSE;

  if ( !Start() ) {
//    wxMessageBox(_("Can't connect to post 3000\nmaybe another hugin is running\n"), _("Alert !"));
  }
  DEBUG_INFO ( "" )

}

Server::~Server()
{
  DEBUG_INFO ( "" )
  // No delayed deletion here, as the frame is dying anyway
  delete m_server;
}

bool Server::Start( void )
{
  bool success (TRUE);

  // Create the address - defaults to localhost:0 initially
  wxIPV4address addr;
  addr.Service(port);

  // Create the socket
  m_server = new wxSocketServer(addr);

  // We use Ok() here to see if the server is really listening
  if (! m_server->Ok())
  {
    send = "start server";
    success = FALSE;
    DEBUG_INFO ( "Server not active: " << m_server->Ok() )
    if ( !server_timer.IsRunning() )
      server_timer.Start(500);  // let's try it further
  }
  else
  {
    server_timer.Stop();
    send = "";        // Leave eventually the timer loop
    DEBUG_INFO ( "Server listening: " << _("Yes") )
  }

  // Setup the event handler and subscribe to connection events
  m_server->SetEventHandler(*this, SERVER_ID);
  m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
  m_server->Notify(TRUE);

  m_busy = FALSE;
  m_numClients = 0;

  return success;
}

bool Server::Connected( void )
{
    if ( m_numClients > 0 ) {
      return TRUE;
    } else {
      return FALSE;
    }
}

void Server::SendFilename( wxString filename )
{
  DEBUG_INFO ( _("start") )
  const wxChar *buf1;
  wxChar       *buf2;
  unsigned char len;

  // Disable socket menu entries (exception: Close Session)
//  m_busy = TRUE;
  UpdateStatusBar();

  DEBUG_INFO ( "" )

    // we send the command only once to panoviewer
    if ( Connected() ) {
      send = "";
    } else {
      // get the global config object
      wxConfigBase* config = wxConfigBase::Get();
      std::string dir = config->Read("startDir","").c_str();
      dir.append( "/../PanoImage/panoviewer" );
      wxFileName pv (dir.c_str());
      char viewer_bin[] = "panoviewer";
      wxString viewer (viewer_bin);
      if ( pv.FileExists() ) // get panoviewer from sourcetree
        viewer = pv.GetFullPath();
      else {
        viewer.Append ( ".exe" );
        pv = viewer;
        if ( pv.FileExists() )
          viewer = pv.GetFullPath();
        else {
          viewer = INSTALL_BIN_DIR ;
          viewer.Append ( (wxString)wxFileName::GetPathSeparator() );
          viewer.Append ( viewer_bin );
          pv = viewer;
          if ( pv.FileExists() ) // get panoviewer from installationtree
            viewer = pv.GetFullPath();
          else {
            viewer.Append ( ".exe" );
            pv = viewer;
            if ( pv.FileExists() )
              viewer = pv.GetFullPath();
            else
              DEBUG_WARN ( "panoviewer not found" )
          }
        }
      }
      viewer.Append (" ");
      viewer.Append (filename);
      viewer.Append (" ");
      wxString p;
      p.sprintf(" %d", port);
      viewer.Append (p);
      DEBUG_INFO ( "port = " << port )
//      viewer.sprintf("panoviewer %s %d", filename, port);
      viewer.Append (" ");
      p=""; p.sprintf(" %d", (int)projectionFormat);
      viewer.Append (p);
      viewer.Append (" ");
      p=""; p.sprintf(" %d", width);
      viewer.Append (p);
      viewer.Append (" ");
      p=""; p.sprintf(" %d", height);
      viewer.Append (p);
      viewer.Append (" ");
      p=""; p.sprintf(" %d", resetView);
      viewer.Append (p);
      viewer.Append (" ");
      p=""; p.sprintf(" %d", showGrid);
      viewer.Append (p);

      DEBUG_INFO ( "command = " << viewer )
      if ( Connected() == false ) {
        wxExecute( viewer, FALSE /* sync */);
      };

//      send = filename;
//      server_timer.Start(100);    // 1 second interval
      return;
    };

  // Tell the other which command we are running
  unsigned char c = 0xCa;   // FILENAME
  m_sock->Write(&c, 1);
  m_sock->SetFlags(wxSOCKET_WAITALL);

// ---------------------------------------------------------------------
  buf1 = filename;      // command to panoviewer
// ---------------------------------------------------------------------
  len  = (wxStrlen(buf1) + 1) * sizeof(wxChar);
  buf2 = new wxChar[(len)];

  m_sock->Write(&len, 1);
  m_sock->Write(buf1, len);
  DEBUG_INFO ( "sent " << buf1 )
  DEBUG_INFO ( m_sock->LastCount() )
  m_sock->Read (buf2, len);
  DEBUG_INFO ( "read " << buf2 )
  DEBUG_INFO ( m_sock->LastCount() )

  if (memcmp(buf1, buf2, len) != 0)
  {
      DEBUG_INFO("test failed; starting timer "<<buf1<<" "<<buf2<<" "<<(int)len)
      send = filename;
      server_timer.Start(100);    // 1 second interval
  } else {
      DEBUG_INFO ( "test passed, sent - " << filename )
      send = "";
      server_timer.Stop();        //
  }

  delete[] buf2;
  m_busy = FALSE;
//      m_sock->Discard();    // cleanup
      if ( m_sock->LastCount() > 0 )
        DEBUG_INFO ( "bytes " << m_sock->LastCount() )
  DEBUG_INFO ( _("end") )
  UpdateStatusBar();
}

// event handlers

void Server::OnServerTimer(wxTimerEvent& event)
{
    if ( send == "start server" ) {
       port++;
       Start();
       return;
    }

    if ( !send.IsEmpty() ) {
      DEBUG_INFO ( _("send") << " " << send )
      if ( Connected() ) {
        SendFilename ( send );
//        server_timer.Stop();      // stop here and let SendFilename decide
      } else {
        ;                         // hope the connection will establish
      }
    } else { // OnServerTimer is only useful with an imagename.
      server_timer.Stop();
    }
    wxString s;
    if ( send.IsEmpty() )
      s = _("Yes");
    else
      s = _("No");
    DEBUG_INFO ( _("send empty? ") << s  )
  s = "";
/*  if ( m_sock->Ok() ) {
    if ( m_sock->Error() )
      s.Append (" error ");
    DEBUG_INFO ( _("") )
    if ( m_sock->IsData() )
      s.Append (" data ");
    DEBUG_INFO ( _("") )
    if ( m_sock->Ok() )
      s.Append (" sockOk ");
  }*/
  DEBUG_INFO ( _("end")  << s  )
}

void Server::OnServerEvent(wxSocketEvent& event)
{
  wxString s = ("now ");

  switch(event.GetSocketEvent())
  {
    case wxSOCKET_CONNECTION : s.Append(_("wxSOCKET_CONNECTION")); break;
    default                  : s.Append(_("Unexpected event !\n")); break;
  }
  DEBUG_INFO ( s )

  // Accept new connection if there is one in the pending
  // connections queue, else exit. We use Accept(FALSE) for
  // non-blocking accept (although if we got here, there
  // should ALWAYS be a pending connection).

  m_sock = m_server->Accept(FALSE);

  if ( m_sock->Ok() )
  {
    // handshake for proofing the client
    unsigned char handshake;
    m_sock->SetTimeout(2);
    m_sock->Read(&handshake, 1);

    m_sock->SetFlags(wxSOCKET_WAITALL);

    if ( handshake == 0xBF ) {  // handshake we await
      handshake = 0xBA;         // handshake signal we send back
      m_sock->Write(&handshake, 1);
      DEBUG_INFO ( "New client connection accepted: send handshake" )
    } else {
      DEBUG_INFO ( "handshake failed - refusing" )
      m_sock->Close();
      UpdateStatusBar();
      return;
    };
  }
  else
  {
    DEBUG_INFO ( "do no handshake" )
    m_sock->Close();
    return;
  }

  m_sock->SetEventHandler(*this, SOCKET_ID);
  m_sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
  m_sock->Notify(TRUE);

      m_sock->Discard();    // cleanup
      if ( m_sock->LastCount() > 0 )
        DEBUG_INFO ( "bytes " << m_sock->LastCount() )
  m_numClients++;
  UpdateStatusBar();
  s = "";
  if ( m_sock->Error() )
    s.Append (" error ");
  if ( m_sock->IsData() )
    s.Append (" data ");
  if ( m_sock->Ok() )
    s.Append (" sockOk ");
  DEBUG_INFO ( _("end") << m_sock->LastCount() << s  )
}

void Server::OnSocketEvent(wxSocketEvent& event)
{
  DEBUG_INFO ( "" )
  wxString s ("now ");

  // We dont revieve anything here.
  // First, print a message
  switch(event.GetSocketEvent())
  {
    case wxSOCKET_INPUT : s.Append(_("wxSOCKET_INPUT\n")); break;
    case wxSOCKET_LOST  : s.Append(_("wxSOCKET_LOST\n"));
                          m_numClients--;
                          break;
    case wxSOCKET_CONNECTION : s.Append(_("wxSOCKET_CONNECTION\n")); break;
    default             : s.Append(_("Unexpected event !\n")); break;
  }
    DEBUG_INFO ( s );

      m_sock->Discard();    // cleanup
      if ( m_sock->LastCount() > 0 )
        DEBUG_INFO ( "bytes " << m_sock->LastCount() )
  UpdateStatusBar();

  s = "";
  if ( m_sock->Error() )
    s.Append (" error ");
  if ( m_sock->IsData() )
    s.Append (" data ");
  if ( m_sock->Ok() )
    s.Append (" sockOk ");
  DEBUG_INFO ( _("end") << m_sock->LastCount() << s  )
}

// convenience functions

void Server::UpdateStatusBar()
{
  wxString s;
//  s.sprintf(_("%d "), m_numClients);
//  s.Append(_("clients"));
  for (int i (0); i < m_numClients; i++)
    s.Append(_("*"));
  frame->SetStatusText(s,1);
}
