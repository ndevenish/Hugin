#ifndef PANO_VIEWER_HEADER
#define PANO_VIEWER_HEADER

#include "wx/wx.h"
#include "wx/image.h"
#include "wx/timer.h"
#include "panoviewextractor.h"
#include "formats.h"


class PanoViewer: public wxPanel
{
public:
	PanoViewer ( wxWindow *parent, wxWindowID id,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxTAB_TRAVERSAL,
             const wxString& name = wxControlNameStr);
	~PanoViewer();
	void SetPano ( const wxImage &img );
	void setProjection ( ProjectionFormat p );
	void setWidth ( double width );
	void setHeight ( double height );
	PanoViewpoint getView ( void ) {
            return vp;
        }
	void setView ( const PanoViewpoint vp );
	void showGrid ( bool b );

	// Preferences and options
	void SetMouseFactor ( const int &f );
	void SetResolution ( const int &r );
//	void SetControl ( const wxImage &c, wxMenu *m );
	void SetControl ( wxMenu *m );
	void ShowControl ( bool show = TRUE );
	
	// Event handlers
	void OnMouseLeftDown ( wxMouseEvent &event );
	void OnMouseLeftUp ( wxMouseEvent &event );
	void OnPaint(wxPaintEvent &event);
	void OnKeyDown(wxKeyEvent &event);
	void OnKeyUp( wxKeyEvent &event);
	void OnIdle ( wxIdleEvent &event );
	void OnSize ( wxSizeEvent &event );

private:
	void DrawClient( wxDC &dc );
	
	PanoViewExtractor extractor;

	bool isPanning, isZoomingIn, isZoomingOut, isControlShowing, forceRecalc;
	int mx, my;
	PanoViewpoint vp;
        ProjectionFormat projectionFormat;
	int cw, ch;                   // client sizes
	int resolution, mf;
	wxBitmap currentViewBmp, currentClientBmp, control;
	wxImage pano, fastView, goodView;
        bool grid;                    // grid to show ?

        int ow, oh;                   // old client sizes
	wxMenu *cmenu;

	DECLARE_EVENT_TABLE()
};


#endif /* PANO_VIEWER_HEADER */

