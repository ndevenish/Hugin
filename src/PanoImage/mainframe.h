#ifndef _MAINFRAME_H
#define _MAINFRAME_H


#include "wx/wx.h"
#include "wx/image.h"

#include "panoviewer.h"

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
            int& argc, char *argv[]);
    ~MyFrame();

    void OnFullScreen( wxMenuEvent &event);
    void OnLoadFile ( wxMenuEvent &event );
    void OnQuit ( wxMenuEvent &event );
    void OnPref ( wxMenuEvent &event );
    /**  This function takes the hugin call via wxSocket an loads a new image
     */
    void ShowFile ( wxString fn );
private:
	PanoViewer *pano1;
	wxImage control;
	bool isFullScreen;
	wxFileDialog *load, *save;
	wxDialog *pref;
	wxSlider *qslider;
	wxSlider *mslider;

	DECLARE_EVENT_TABLE()
};

enum
{
	LOAD,
	SAVE,
	QUIT,

	FULLSCREEN,
	PREF
};

// MainFrame
extern MyFrame * frame;

#endif // _MAINFRAME_H

