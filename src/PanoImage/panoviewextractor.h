#ifndef PANO_VEW_EXTRACTOR_HEADER
#define PANO_VEW_EXTRACTOR_HEADER

#include "wx/wx.h"
#include "wx/image.h"
#include "wx/thread.h"
#include "panorama.h"
#include "filter.h"
#include "utils.h"
#include "formats.h"


// A viewpoint is a view from a certain angle at a certain quality
class PanoViewpoint;
class PanoViewpoint
{
public:
	PanoViewpoint(
		const int &q = 50,
                const ProjectionFormat &f = EQUIRECTANGULAR,
		const double &h = 70,
		const double &y = 0,
		const double &p = 0,
		const double &r = 0,
		const double &y_l = 360,
		const double &p_l = 180);
	PanoViewpoint( const PanoViewpoint &v )
	{
          SetViewpoint(v);
	}
	int GetQuality(void) const
	{
	  return quality;
	}
	double GetFormat(void) const
	{
	  return format;
	}
	double GetHfov(void) const
	{
	  return hfov;
	}
	double GetYaw(void) const
	{
	  return yaw;
	}
	double GetYawLimit(void) const
	{
	  return yaw_limit;
	}
	double GetPitch(void) const
	{
	  return pitch;
	}
	double GetPitchLimit(void) const
	{
	  return pitch_limit;
	}
	double GetRoll(void) const
	{
	  return roll;
	}
	PanoViewpoint GetViewpoint(void)
	{
	  return PanoViewpoint(quality, format, hfov, yaw, pitch, roll,
                               yaw_limit, pitch_limit);
	}

	void SetViewpoint ( const PanoViewpoint &v )
	{
	  quality = v.quality;
          format = v.format;
	  hfov = v.hfov;
	  yaw = v.yaw;
	  pitch = v.pitch;
	  roll = v.roll;
	  yaw_limit = v.yaw_limit;
	  pitch_limit = v.pitch_limit;
	}
	void SetQuality ( const int &q = 50 )
	{
	  quality = q;
	  if ( quality < 0) quality = 0;
	  if ( quality > 100 ) quality = 100;
	}
	void SetFormat ( const ProjectionFormat &f = EQUIRECTANGULAR )
	{
	  format = f;
	}
	void SetHfov ( const double &h = 70 )
	{
	  hfov = h;
	  if ( hfov < 10 ) hfov = 14;
	  if ( hfov > 150 ) hfov = 140;
	}
	void SetYaw ( const double &y = 0 )
	{
	  yaw = y;
	  NORM_ANGLE(yaw);
	  if ( yaw < -yaw_limit/2.0 ) yaw = -yaw_limit/2.0;
	  if ( yaw > +yaw_limit/2.0 ) yaw = +yaw_limit/2.0;
	}
	void SetYawLimit ( const double &y_l = 360 )
	{
	  yaw_limit = y_l;
          SetYaw ( GetYaw () );
	}
	void SetPitch ( const double &p = 0 )
	{
	  pitch = p;
//          DEBUG_INFO ("pitch= "<< pitch <<" pitch_limit = "<< pitch_limit )
	  if ( pitch < -pitch_limit/2.0 ) pitch = -pitch_limit/2.0;
	  if ( pitch > +pitch_limit/2.0 ) pitch = +pitch_limit/2.0;
	}
	void SetPitchLimit ( const double &p_l = 180 )
	{
	  pitch_limit = p_l;
          SetPitch ( GetPitch() );
	}
	void SetRoll ( const double &r = 0 )
	{
	  roll = r;
	  NORM_ANGLE(roll);
	}
	
	int operator==( const PanoViewpoint &right) const
	{
	  return ( quality == right.quality && format == right.format && hfov == right.hfov && yaw == right.yaw && pitch == right.pitch && roll == right.roll && yaw_limit == right.yaw_limit && pitch_limit == right.pitch_limit );
	}
	int operator!=( const PanoViewpoint &right) const
	{
	  return !operator==(right);
	}

private:
	int quality;
        ProjectionFormat format;
	double hfov, yaw, pitch, roll;
        double yaw_limit, pitch_limit;  // They contain the visible angle.
        bool zero_pitch, zero_roll;     // They decide to let the viepoint
                                        // glide back to zero or not.
};


class ExtractionBroker
{
public:
	void GetView ( wxImage &s, wxImage &d, const PanoViewpoint &v = PanoViewpoint() );
};


class PanoViewExtractor
{
public:
	PanoViewExtractor();

	// Service functions

	void GetView ( wxImage &s, wxImage &d, const PanoViewpoint &v = PanoViewpoint() );
private:
	Image src, dst;
	TrformStr tr;
};

#endif /* PANO_VEW_EXTRACTOR_HEADER */
