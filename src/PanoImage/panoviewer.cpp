#include "wx/wx.h"
#include "wx/image.h"
#include "wx/utils.h"
#include "wx/timer.h"
#include "filter.h"
#include "panoviewextractor.h"
#include "panoviewer.h"
#if defined(__WXGTK__) || defined(__WXX11__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__)
    #include "control_.xpm"
#endif


BEGIN_EVENT_TABLE(PanoViewer, wxPanel)
    EVT_PAINT(PanoViewer::OnPaint)
    EVT_KEY_DOWN(PanoViewer::OnKeyDown)
    EVT_KEY_UP(PanoViewer::OnKeyUp)
    EVT_LEFT_DOWN(PanoViewer::OnMouseLeftDown)
    EVT_LEFT_UP(PanoViewer::OnMouseLeftUp)
    EVT_IDLE(PanoViewer::OnIdle)
    EVT_SIZE(PanoViewer::OnSize)
END_EVENT_TABLE()


PanoViewer::PanoViewer ( wxWindow *parent, wxWindowID id,
             const wxPoint& pos,
             const wxSize& size, long style,
             const wxString& name)
	: wxPanel(parent, id, pos, size, style, name)
{
	// Set our quality and mouse factors
	resolution = 200;
	mf = 1500;

	cw = 0; ch = 0;

   // Init action flags
   isPanning = isZoomingIn = isZoomingOut = FALSE;

	// Hide our dummy control for now
   control.Create(20,20);
   isControlShowing = FALSE;

	// Dummy panorama
   pano = wxImage(200,200);

        // Set control
#if defined(__WXGTK__) || defined(__WXX11__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__)
   wxBitmap c (control_);
   control = c;
#else
   wxImage p;
   p.LoadFile("control.bmp");
   control = p.ConvertToBitmap();
#endif

}


PanoViewer::~PanoViewer()
{
}


void PanoViewer::SetPano ( const wxImage &img )
{
	pano = img;
	vp = PanoViewpoint();
	forceRecalc = TRUE;
	Refresh(FALSE);
}


void PanoViewer::OnKeyDown(wxKeyEvent &event)
{
	if ( event.ShiftDown() ) isZoomingIn = TRUE;
	if ( event.ControlDown() ) isZoomingOut = TRUE;
}


void PanoViewer::OnKeyUp( wxKeyEvent &event )
{
	if ( !event.ShiftDown() ) isZoomingIn = FALSE;
	if ( !event.ControlDown() ) isZoomingOut = FALSE;
}



void PanoViewer::OnIdle ( wxIdleEvent &event )
{
	if ( isZoomingIn ) vp.SetHfov( vp.GetHfov() - 5 );
	if ( isZoomingOut ) vp.SetHfov( vp.GetHfov() + 5 );
	
	if ( isPanning )
	{
		int dx, dy, nmx = 0, nmy = 0;

		// Get our mouse position in absolute coordinates
		wxGetMousePosition(&nmx, &nmy);

		// Calculate the distance in x and y of the mouse from the center
		dx = mx-nmx;
		dy = my-nmy;

		// Adjust the yaw and pitch with respect to the distance
		// If the distance is small, the yaw and pitch will be adjusted by
		// a very small amount, nearly 0.
		vp.SetYaw( vp.GetYaw() - dx * (mf/10000.0) );
		vp.SetPitch( vp.GetPitch() + dy * (mf/10000.0) );
	}

	// Draw the updated image
	if ( isZoomingIn || isZoomingOut || isPanning )
	{
		// Draw a low-quality image
		vp.SetQuality ( 0 );
		Refresh(FALSE);
	} else {
		// Draw only if we have not already settled on high quality
		if ( vp.GetQuality() == 0 )
		{
			vp.SetQuality(1);
			Refresh(FALSE);
		}
	}

	// Request more idle events. No more will be posted until another event causes one
	// unless we request more now.
	event.RequestMore();
}



void PanoViewer::OnMouseLeftDown ( wxMouseEvent &event )
{
	// If the mouse has been clicked outside the control icon region
	if ( !isControlShowing
			||	event.GetX() >= control.GetWidth()
			|| event.GetY() < currentViewBmp.GetHeight() - control.GetHeight()
		)
	{
		// Get our mouse position in absolute coordinates
		mx = my = 0;
		wxGetMousePosition(&mx, &my);
	   CaptureMouse();
		isPanning = TRUE;
	}
}



void PanoViewer::OnMouseLeftUp ( wxMouseEvent & event )
{
	// If we're not panning, we may have clicked on the control icon
	if ( !isPanning )
	{
		if ( isControlShowing
			&& event.GetX() < control.GetWidth()
			&& event.GetY() >= currentViewBmp.GetHeight() - control.GetHeight()
			)
			PopupMenu(cmenu, event.GetX(), event.GetY())
			;
	} else {
		ReleaseMouse();
		isPanning = FALSE;
	}
}


void PanoViewer::OnPaint(wxPaintEvent &event)
{
	static int ow = 0, oh = 0;
	static PanoViewpoint ovp;
	wxPaintDC dc(this);

	GetClientSize(&cw, &ch);
	if ( ow != cw || oh != ch || ovp != vp || forceRecalc )
	{
		if ( cw != 0 && ch != 0 )
		{
			int r;
			wxMemoryDC mdc;

			// Recompute client bitmap and views if size has changed
			if ( ow != cw || oh != ch || forceRecalc)
			{
				currentClientBmp.Create(cw, ch);
				goodView.Create(cw, ch);
				r = resolution;
				if ( cw < r ) r = cw;
				fastView.Create(r, (int)(r * (float)ch/cw ));
			}

			// Get the new view, because size or view angle has changed
			if ( vp.GetQuality() == 0 )
			{
				extractor.GetView(pano, fastView, vp);
				currentViewBmp = fastView.ConvertToBitmap();
			} else {
				extractor.GetView(pano, goodView, vp);
				currentViewBmp = goodView.ConvertToBitmap();
			}

			// Blit the view
			mdc.SelectObject(currentClientBmp);
			mdc.SetUserScale(cw/(double)currentViewBmp.GetWidth(), ch/(double)currentViewBmp.GetHeight() );
			mdc.BeginDrawing();
			mdc.DrawBitmap ( currentViewBmp, 0, 0 );
			if ( isControlShowing )
			{
				mdc.SetUserScale(1, 1);
				mdc.DrawBitmap (control, 0, ch - control.GetHeight() );
			}
			mdc.EndDrawing();
			mdc.SelectObject ( wxNullBitmap );
	
			dc.BeginDrawing();
			dc.DrawBitmap(currentClientBmp, 0,0);
			dc.EndDrawing();

			ow = cw; oh = ch; ovp = vp;
			forceRecalc = FALSE;
		}
	} else {
		dc.BeginDrawing();
		dc.DrawBitmap( currentClientBmp, 0, 0 );
		dc.EndDrawing();
	}
}


void PanoViewer::SetMouseFactor ( const int &f )
{
	mf = f;
	if ( mf < 30 ) mf = 1;
	if ( mf > 10000 ) mf = 10000;
}


void PanoViewer::SetResolution ( const int &r )
{
	resolution = r;
	forceRecalc = TRUE;
	Refresh(FALSE);
}


void PanoViewer::SetControl ( wxMenu *m ) // ( const wxImage &c, wxMenu *m )
{
	cmenu = m;
	Refresh(FALSE);
}


void PanoViewer::ShowControl ( bool show )
{
	isControlShowing = show;
	Refresh(FALSE);
}


void PanoViewer::OnSize( wxSizeEvent &event )
{
	vp.SetQuality(0);
	Refresh(FALSE);
}
