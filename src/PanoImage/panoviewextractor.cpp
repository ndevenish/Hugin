#include "wx/wx.h"
#include "wx/image.h"
#include "wx/timer.h"
#include "panorama.h"
#include "filter.h"
#include "panoviewextractor.h"

void PV_ExtractStill(TrformStr *TrPtr);
void PrintError (char *fmt, ...);
int SetUpAtan();
int SetUpSqrt();
int SetUpMweights();


//=================================================================================================

PanoViewpoint::PanoViewpoint(
		const int &q,
		const double &h,
		const double &y,
		const double &p,
		const double &r)
{
	SetQuality(q);
	SetHfov(h);
	SetYaw(y);
	SetPitch(p);
	SetRoll(r);
}



PanoViewpoint::PanoViewpoint( const PanoViewpoint &v )
{
	SetViewpoint(v);
}




int PanoViewpoint::GetQuality(void) const
{
	return quality;
}



double PanoViewpoint::GetHfov(void) const
{
	return hfov;
}



double PanoViewpoint::GetYaw(void) const
{
	return yaw;
}



double PanoViewpoint::GetPitch(void) const
{
	return pitch;
}



double PanoViewpoint::GetRoll(void) const
{
	return roll;
}


PanoViewpoint PanoViewpoint::GetViewpoint(void)
{
	return PanoViewpoint(quality, hfov, yaw, pitch, roll);
}



void PanoViewpoint::SetViewpoint ( const PanoViewpoint &v )
{
	quality = v.quality;
	hfov = v.hfov;
	yaw = v.yaw;
	pitch = v.pitch;
	roll = v.roll;
}



void PanoViewpoint::SetQuality ( const int &q)
{
	quality = q;
	if ( quality < 0) quality = 0;
	if ( quality > 100 ) quality = 100;
}



void PanoViewpoint::SetHfov ( const double &h)
{
	hfov = h;
	if ( hfov < 10 ) hfov = 30;
	if ( hfov > 150 ) hfov = 100;
}



void PanoViewpoint::SetYaw ( const double &y)
{
	yaw = y;
	NORM_ANGLE(yaw);
}



void PanoViewpoint::SetPitch ( const double &p)
{
	pitch = p;
	if ( pitch > 90 ) pitch = 90;
	if ( pitch < -90 ) pitch = -90;
}



void PanoViewpoint::SetRoll ( const double &r)
{
	roll = r;
	NORM_ANGLE(roll);
}



int PanoViewpoint::operator==( const PanoViewpoint &right) const
{
	return ( quality == right.quality && hfov == right.hfov && yaw == right.yaw && pitch == right.pitch && roll == right.roll );
}

int PanoViewpoint::operator!=( const PanoViewpoint &right) const
{
	return !operator==(right);
}

//=================================================================================================

PanoViewExtractor::PanoViewExtractor()
{
	// Partially init our source pano (the rest will be handled when a pano is set)
	src.bitsPerPixel = 24;
	src.dataformat = _RGB;
	src.format = _equirectangular;
	src.hfov = 360;
	src.yaw = 0;
	src.pitch = 0;
	src.roll = 0;
	src.name[0] = '\0';    // no image name
	src.data = new unsigned char *;
	if ( src.data == NULL )
	{
		PrintError("Error allocating source data pointer in extraction constructor. Aborting.");
		exit(1);
	}
	*src.data = NULL;

	// Partially init our transformation destination (the rest will be handled when a view is extracted)
	dst.bitsPerPixel = 24;
	dst.dataformat = _RGB;
	dst.format = _panorama;
	dst.name[0] = '\0';    // no image name
	dst.data = new unsigned char *;
	if ( dst.data == NULL )
	{
		PrintError("Error allocating view transformation pointer. Aborted.");
		exit(1);
	}
	*dst.data = NULL;
	
	// Set our transformation structure
	tr.src = &src;
	tr.dest = &dst;

	// Init our pano library
   SetUpAtan();
   SetUpSqrt();
   SetUpMweights();
}


void PanoViewExtractor::GetView( wxImage &s, wxImage &d, const PanoViewpoint &v )
{
	// Adjust destination Image structure
	dst.hfov = v.GetHfov();
	dst.yaw = v.GetYaw();
	dst.pitch = v.GetPitch();
	dst.roll = v.GetRoll();
	dst.width = d.GetWidth();
	dst.height = d.GetHeight();
	dst.bytesPerLine = dst.width * dst.bitsPerPixel/8;
	dst.dataSize = dst.bytesPerLine * dst.height;
	*dst.data = d.GetData();

	src.width = s.GetWidth();
	src.height = s.GetHeight();
	src.bytesPerLine = src.width * src.bitsPerPixel/8;
	src.dataSize = src.bytesPerLine * src.height;
	*src.data = s.GetData();

	// Set up interpolator
   switch ( v.GetQuality() )
   {
   	case 0:
	  		tr.interpolator = _nn;
   		break;
   	default:
   		tr.interpolator = _bilinear;
   }

	// Perform the extraction
	PV_ExtractStill(&tr);
}

