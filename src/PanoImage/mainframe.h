#ifndef _MAINFRAME_H
#define _MAINFRAME_H


//#include "wx/wx.h"
#include "wx/frame.h"
#include "wx/image.h"

#include "panoviewer.h"

/** Projection of final panorama
 */
typedef enum ProjectionFormat { RECTILINEAR = 0,
                                CYLINDRICAL = 1,
                                EQUIRECTANGULAR = 2
                              } ProjectionFormat;



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
    /**  This function takes the hugin call via wxSocket and loads a new image.
     */
    void ShowFile ( wxString fn );
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

    /** the pano panel */
    PanoViewer *pano;

private:
        ProjectionFormat projectionFormat;
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

