#ifndef _MAINFRAME_H
#define _MAINFRAME_H


//#include "wx/wx.h"
#include "wx/frame.h"
#include "wx/image.h"
#include "wx/filename.h"
#include "panoviewer.h"


class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
            int& argc, char *argv[]);
    ~MyFrame();

    void OnFullScreen( wxCommandEvent &event);
    void OnLoadFile ( wxCommandEvent &event );
    void OnQuit ( wxCommandEvent &event );
    void OnPref ( wxCommandEvent &event );
    /**  This function takes the hugin call via wxSocket and loads a new image.
     */
    void ShowFile ( wxFileName fn );
    /**  This function takes the hugin call via wxSocket and
     *   sets the projection.
     */
    void setProjectionFormat ( ProjectionFormat projectionFormat );
    /**  This function takes the hugin call via wxSocket and
     *    sets the view width.
     */
    void setViewWidth ( int width );
    /**  This function takes the hugin call via wxSocket and
     *   sets the view heigth.
     */
    void setViewHeight ( int height );
    /**  This function takes the hugin call via wxSocket and sets the grid.
     */
    void setGrid ( bool showGrid );
    void OnGrid ( wxCommandEvent &event );
    void OnView ( wxCommandEvent &event );

    /** the pano panel */
    PanoViewer *pano;

private:
        ProjectionFormat projectionFormat;
        int width;
        int height;
        bool resetView;
        bool showGrid;
	wxImage control;
	bool isFullScreen;
	wxFileDialog *load, *save;
	wxDialog *pref;
	wxSlider *qslider;
	wxSlider *mslider;
        wxFileName filename;

	DECLARE_EVENT_TABLE()
};

enum
{
	LOAD,
	SAVE,
	QUIT,

	FULLSCREEN,
	PREF,
        GRID,
        VIEW
};



// MainFrame
extern MyFrame * frame;

#endif // _MAINFRAME_H

