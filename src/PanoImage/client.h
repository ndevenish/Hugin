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

#ifndef _CLIENT_H
#define _CLIENT_H


// ==========================================================================
// declarations
// ==========================================================================

// --------------------------------------------------------------------------
// headers
// --------------------------------------------------------------------------
/*
#if defined(__GNUG__) && !defined(__APPLE__)
#  pragma implementation "client.cpp"
#  pragma interface "client.cpp"
#endif
*/
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#  pragma hdrstop
#endif

// for all others, include the necessary headers 
#ifndef WX_PRECOMP
#  include "wx/wx.h"
#endif

#include "wx/socket.h"
//#include "wx/url.h"
//#include "wx/wfstream.h"
#include "mainframe.h"

// --------------------------------------------------------------------------
// resources
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// classes
// --------------------------------------------------------------------------

// Define a new frame type: this is going to be an dummy frame
class Client : public wxWindow
{
public:
  Client();
  ~Client();
    bool Start(int port);

  void GetServerData(wxSocketBase *sock);

  // socket event handler
  void OnSocketEvent(wxSocketEvent& event);

  // convenience functions
  void UpdateStatusBar();

private:
  wxSocketClient *m_sock;
  bool            m_busy;
  bool            shaking;

  // any class wishing to process wxWindows events must use this macro
  DECLARE_EVENT_TABLE()
};

// --------------------------------------------------------------------------
// constants
// --------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
  // id for socket
  SOCKET_ID
};

#endif // _CLIENT_H

