#ifndef PANO_VEW_EXTRACTOR_HEADER
#define PANO_VEW_EXTRACTOR_HEADER

#include "wx/wx.h"
#include "wx/image.h"
#include "wx/thread.h"
#include "panorama.h"

// A viewpoint is a view from a certain angle at a certain quality
class PanoViewpoint;
class PanoViewpoint
{
public:
	PanoViewpoint(
		const int &q = 50,
		const double &h = 70,
		const double &y = 0,
		const double &p = 0,
		const double &r = 0);
	PanoViewpoint( const PanoViewpoint &v );

	int GetQuality(void) const;
	double GetHfov(void) const;
	double GetYaw(void) const;
	double GetPitch(void) const;
	double GetRoll(void) const;
	PanoViewpoint GetViewpoint(void);

	void SetViewpoint ( const PanoViewpoint &v );
	void SetQuality ( const int &q = 50 );
	void SetHfov ( const double &h = 70 );
	void SetYaw ( const double &y = 0 );
	void SetPitch ( const double &p = 0 );
	void SetRoll ( const double &r = 0 );
	
	int operator==( const PanoViewpoint &right) const;
	int operator!=( const PanoViewpoint &right) const;
private:
	int quality;
	double hfov, yaw, pitch, roll;
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
