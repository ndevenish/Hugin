#ifndef _PANOVIEWERAPP_H
#define _PANOVIEWERAPP_H

#include "wx/wx.h"
#include "wx/image.h"

#include "panoviewer.h"
#include "client.h"


class MyApp: public wxApp
{
public:
	virtual bool OnInit();
	virtual int OnExit();

        Client * client;

};

IMPLEMENT_APP(MyApp)

#endif // _PANOVIEWERAPP_H
