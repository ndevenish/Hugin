// -*- c-basic-offset: 4 -*-
/** @file algo4sift.hxx
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
 *
 *  $Id$
 *
 *  This program is licenced under the SIFT feature detection licence,
 *  see LICENCE_SIFT document
 *
 */

#include<vigra/basicimage.hxx>
#include<vigra/resizeimage.hxx>


// =======================================================================
// New Templates Algorithme for SIFT implementation
//
//
//
//

static inline float sqr(float a)
{
    return a*a;
}

// =======================================================================
// Resize Half
//   half the size with no interpolation
// =======================================================================
template <class Image>
void ResizeHalf(const Image & in, Image & out)
{
    // image size at half
    int newwidth = (in.width() + 1) / 2;
    int newheight = (in.height() + 1) / 2;

    // resize result image to appropriate size
    out.resize(newwidth, newheight);
    resizeImageNoInterpolation( srcImageRange(in), destImageRange(out) );
}

// =======================================================================
// Resize Double Linear
//   double the size with linear interpolation
// =======================================================================
template <class Image>
void ResizeDoubleLinear( const Image & in, Image & out)
{
    // image size at double
    int newwidth = 2*in.width() + 1;
    int newheight = 2*in.height() + 1;

    // resize result image to appropriate size
    out.resize(newwidth, newheight);
    resizeImageLinearInterpolation( srcImageRange(in), destImageRange(out) );
}

// =======================================================================
// Gaussian blur of the picture
//    double : sigma
// =======================================================================
template <class Image>
void GaussianBlur( double sigma, Image & out )
{
    vigra::Kernel1D<double> GaussCoef;
    GaussCoef.initGaussian( sigma );
    vigra::FImage tmp( out.width(), out.height());
    vigra::separableConvolveX( srcImageRange(out), destImage(tmp), kernel1d( GaussCoef ) );
    separableConvolveY( srcImageRange(tmp), destImage(out), kernel1d( GaussCoef ) );
}

// =======================================================================
// ImageAbsDiff
//   calculate fabs(img1-img2)
// =======================================================================
//using namespace vigra::functor;
/*
template <class Image>
void ImageAbsDiff( const Image & in1, const Image & in2, Image & out)
{
	vigra_precondition( (in1.width() == in2.width()) && (in1.height() == in2.height()),
				"ImageAbsDiff: Images should have same size.");

	// adjust output size
	out.resize(in1.width(), in1.height());
	// substract
	combineTwoImages( srcImageRange(in1), srcImage(in2), destImage(out), std::minus<float>() );
    // fabs
	//transformImage( srcImageRange(out), destImage(out), fabs(Arg1()) );
}*/

// =======================================================================
// Retrieve gradiant and orientation of every pixels
//
// =======================================================================
template <class Image>
void GradiantOrientationImages( const Image & in, Image & gradiant, Image & orientation )
{
    gradiant.resize( in.width(), in.height() );
    orientation.resize( in.width(), in.height() );

    for (int x=1; x<in.width()-1; x++)
        for (int y=1; y<in.height()-1; y++)
        {
            gradiant(x, y) = sqrt( sqr( in(x+1, y) - in(x-1, y) ) + sqr( in(x, y+1) - in(x, y-1) ) );
            orientation(x, y) = atan2( in(x, y+1) - in(x, y-1), in(x+1, y)-in(x-1, y) ); // [-pi, pi [
        }
}
