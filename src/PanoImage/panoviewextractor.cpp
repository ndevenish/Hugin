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
                const ProjectionFormat &pr,
		const double &h,
		const double &y,
		const double &p,
		const double &r,
		const double &y_l,
		const double &p_l)
{
	SetQuality(q);
	SetFormat(pr);
	SetHfov(h);
	SetYaw(y);
	SetPitch(p);
	SetRoll(r);
	SetYawLimit(y_l);
	SetPitchLimit(p_l);
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
        switch ((int)v.GetFormat()) {
          case RECTILINEAR:    { dst.format = _orthographic;
                                 src.format = _rectilinear;
                                 break;
                               }
          case CYLINDRICAL:    { dst.format = _spherical_cp;
                                 src.format = _spherical_cp;
                                 break;
                               }
          case EQUIRECTANGULAR:{ dst.format = _panorama;
                                 src.format = _equirectangular;
                                 break;
                               }
        }
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
//        DEBUG_INFO ( "format = "<< src.format <<"/"<< dst.format )

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

