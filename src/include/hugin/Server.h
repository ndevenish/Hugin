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

/** @file  Server.h
 *
 *  @brief implementation of Server Class
 *
 *  @author Guillermo Rodriguez Garcia <guille@iies.es>
 *
 *  $Id$
 */

#ifndef _SERVER_H
#define _SERVER_H

// ==========================================================================
// declarations
// ==========================================================================

// --------------------------------------------------------------------------
// headers
// --------------------------------------------------------------------------

#if defined(__GNUG__) && !defined(__APPLE__)
#  pragma implementation "server.cpp"
#  pragma interface "server.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#include <wx/timer.h>

#ifdef __BORLANDC__
#  pragma hdrstop
#endif

// for all others, include the necessary headers
#ifndef WX_PRECOMP
#  include "wx/wx.h"
#endif

#include "wx/socket.h"
#include "wx/window.h"
#include "common/utils.h"
#include "PT/PanoramaMemento.h"

// --------------------------------------------------------------------------
// resources
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// classes
// --------------------------------------------------------------------------

// Define a new frame type: this is going to be our main frame
class Server : public wxWindow
{
public:
  Server();
  ~Server();

  // event handlers (these functions should _not_ be virtual)
  void OnServerEvent(wxSocketEvent& event);
  void OnSocketEvent(wxSocketEvent& event);
  void OnServerTimer( wxTimerEvent& event );

  void SendFilename( wxString filename );
  bool Connected( void );

  /** panorama projection format to send to panoviewer */
  PT::PanoramaOptions::ProjectionFormat  projectionFormat;
  int height;
  int width;
  bool resetView;
  bool showGrid;

  // convenience functions
  void UpdateStatusBar();

private:
  wxSocketBase   *m_sock;
  wxSocketServer *m_server;
  bool            Start (void);
  int             port;
  bool            m_busy;
  int             m_numClients;
  wxTimer         server_timer;
  // the image name yet to send to panoviewer
  wxString        send;

  // any class wishing to process wxWindows events must use this macro
  DECLARE_EVENT_TABLE()
};

// --------------------------------------------------------------------------
// constants
// --------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
  // id for sockets
  SERVER_ID = 1000,
  SOCKET_ID,
  TIMER_ID
};

#endif // _SERVER_H
