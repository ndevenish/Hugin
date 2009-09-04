/* test_lensFunc.cpp 15 Aug 2009
copyright (C) 2009 Thomas K Sharpless <tksharpless@gmail.com>
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdio.h>
//#include <tchar.h>
#include <math.h>

#include "lensFunc.h"

int main(int argc, char * argv[] )
{
	lensFunc::camera_kind cam = lensFunc::digicam;
	int wid = 3600, 
		hgt = 2400;
	double hres = 160,
		   vres = 160;

	lensFunc::lens_kind lens = lensFunc::equal_angle;
	double flmm = 16;
	double croprad = 1800;

	printf("Testing lensFunc...\n");

	int nperr = 0;

	lensFunc 
		LF ( cam, wid, hgt, hres, vres,
			 lens, flmm, croprad );
	const char * perr = LF.errMsg();
	if( perr ) {
		printf("lensFunc: %s\n", perr );
		++nperr;
	}
	printf("camera %s wid %d hgt %d hpix/mm %.1f vpix/mm %.1f\n",
		LF.camDesc(), wid, hgt, hres, vres );

	int n = (wid + 1)/2;
	double tol = 1.e-8;
	printf("\nInternal round-trip tests (%d pnts, tolerance %g)...\n",
		   n, tol );
  for( int lt = 1; lt < 8; lt++ ){
	  printf("\n");
	  lens = lensFunc::lens_kind( lt );
	  perr = LF.setCamLens( cam, wid, hgt, hres, vres,
							lens, flmm, croprad 
						  );
	printf("lens %s  FL %.1f mm, crop radius %.1f pixels\n",
		LF.lensDesc(), flmm, croprad );
	if( perr ) {
		printf("lensFunc: %s\n", perr );
		++nperr;
		continue;
	}
	int nnerr1 = 0, nnerr2 = 0,
		nnerr3 = 0, nnerr4 = 0;

	double x, y, x1, y1, x2, y2;
	double u, v;
	double xyz[3];
	double ar = double(hgt)/ double(wid);
	double dx = 1.0 / (n-1);
	double trmax = LF.test_Tr_max(),
		   tamax = LF.test_Ta_max(),
		   srmax = LF.test_Sr_max(),
		   samax = LF.test_Sa_max();
	printf("trmax %g  tamax %g  srmax %g  samax %g \n",
		    trmax, tamax, srmax, samax );
	double avgr = 0, avga = 0,
			ssqr = 0, ssqa = 0;
	for(int i = 0; i < n; i++ ){
		x = i * dx;
		if( fabs(LF.test_Trar( x * trmax )) > tol )
			++nnerr1;
		if( fabs(LF.test_Tara( x * tamax )) > tol )
			++nnerr2;

		double d = LF.test_Srar( x * srmax );
		if( fabs( d ) > tol ) ++nnerr3;
		avgr += d;
		ssqr += d * d;

		d = LF.test_Sara( x * samax );
		if( fabs( d ) > tol ) ++nnerr4;
		avga += d;
		ssqa += d * d;
	}
	avga /= n;
	ssqa = sqrt( ssqa / n - avga * avga );
	avgr /= n;
	ssqr = sqrt( ssqr / n - avgr * avgr );

	printf("%d fwd trig errors\n", nnerr1 );
	printf("%d rev trig errors\n", nnerr2 );
	printf("%d fwd spline errors\n", nnerr3 );
	printf("  avg %g  s.d. %g\n", avgr, ssqr );
	printf("%d rev spline errors\n", nnerr4 );
 	printf("  avg %g  s.d. %g\n", avga, ssqa );

  }

	printf("\n%d program errors\n", nperr );

#if 0
	printf("Random round-trip test (%d pnts, tolerance %g)...\n",
		   n, tol );
	for(int i = 0; i < n; i++ ){
		x = i; y = x * ar;
		if( LF.sphere_photo( x, y, &u, &v ) == 0 ) 
			++nperr;
		if( LF.photo_sphere( u, v, &x1, &y1 ) == 0 ) 
			++nperr;
		if( fabs( x1 - x ) > tol
		 || fabs( x1 - x ) > tol )
			++nnerr1;

		if( LF.cart3d_photo( x, y, xyz ) == 0 )
			++nperr;
		if( LF.photo_cart3d( xyz, &x2, &y2 ) == 0 )
			++nperr;
		if( fabs( x2 - x ) > tol
		 || fabs( x2 - x ) > tol )
			++nnerr2;

	}

	printf("%d program errors\n", nperr );
	printf("%d 2d numeric errors\n", nnerr1 );
	printf("%d 3d numeric errors\n", nnerr2 );
#endif

	return 0;
}

