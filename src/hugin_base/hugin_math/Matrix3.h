// -*- c-basic-offset: 4 -*-
/** @file Matrix3.h
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _HUGIN_MATH_MATRIX3_H_
#define _HUGIN_MATH_MATRIX3_H_

#include <hugin_shared.h>
#include <math.h>
#include <hugin_math/Vector3.h>


/** general : Matrix3 is a class for handling 3x3 Matrix manipulation.
 *
 * We do not use 4x4 matrix for view point changement as the calculus could be inefficent
 * (some of the coefficients are null, m14 = m24 = m34 = 0 et m44 = 1.0 always).
 */
class IMPEX Matrix3
{
public:
	/** we define the Matrix3 as 3 colums of 3 rows */
	double m[3][3];

	static Matrix3 Identity;
	
public:
	/** default constructor : initialise to zero */
    Matrix3();
	
	/** copy constructor */
	Matrix3(const Matrix3& ot);

	/** Set the identity matrix */
	void SetIdentity();

	/** Set the matrice to rotation using yaw, pitch, roll angle */
	void SetRotation( double Yaw, double Pitch, double Roll );

#if 0
    /** Set the matrice to rotation using yaw, pitch, roll angle, panotools style,
     *  copied from Panotools-Script by Bruno Postle
     */
    void SetRotationPT( double Yaw, double Pitch, double Roll );
#endif 

    
    // [Ippei note]: Why the hell is this a method of general Matrix3 class?
    //  Should be subclassed or externally provided
    //  eg. static Matrix3 RotationMatrixPanoCommand::makeRotationMatrixPT(double yaw, double pitch, double roll)

    /** set rotation in panotools style, 
     *  code adapted from Panotools-Script by Bruno Postle
     */
    void SetRotationPT( double yaw, double pitch, double roll );

    /** GetRotation in panotools style. */
    void GetRotationPT( double & Yaw, double & Pitch, double & Roll );

    
	/** set the matrice to rotation around X */
	void SetRotationX( double a );

	void SetRotationY( double a );
	
	void SetRotationZ( double a );

	/** copy operator */
	Matrix3& operator= (const Matrix3& ot);
	
	/** multiplication with another matrix */
	Matrix3 operator*(const Matrix3& ot) const;

    /** operator *= */
    void operator/=(double s);

    /** operator *= */
    void operator*=(double s);

	/** operator *= */
	void operator*=(Matrix3 ot);

	/** comparison */
	inline bool operator==(Matrix3& ot) const
	{
		for(int i=0; i<3; i++)
			for(int j=0; j<3; j++)
				if(m[i][j] != ot.m[i][j])
					return false;
		return true;
	}

	/** comparison */
	inline bool operator!=(Matrix3& ot) const
	{
		return !(*this == ot);
	}

	/** retrieves transpose */
	inline Matrix3 Transpose()
	{
		Matrix3	Result;
		Result.m[0][0] = m[0][0];
		Result.m[0][1] = m[1][0];
		Result.m[0][2] = m[2][0];
		Result.m[1][0] = m[0][1];
		Result.m[1][1] = m[1][1];
		Result.m[1][2] = m[2][1];
		Result.m[2][0] = m[0][2];
		Result.m[2][1] = m[1][2];
		Result.m[2][2] = m[2][2];
		return Result;
	}

	/** get the determinant */
	inline double Determinant() const
	{
		double result =  m[0][0] * ( m[1][1] * m[2][2] - m[2][1] * m[1][2] );
		result -= m[1][0] * ( m[0][1] * m[2][2] - m[2][1] * m[0][2] );
		result += m[2][0] * ( m[0][1] * m[1][2] - m[1][1] * m[0][2] );
		return result;
	}
	
	/** transforms a vector */
	inline Vector3 TransformVector(const Vector3 &V) const
	{
		Vector3 Result;
		Result.x = V.x * m[0][0] + V.y * m[1][0] + V.z * m[2][0];
		Result.y = V.x * m[0][1] + V.y * m[1][1] + V.z * m[2][1];
		Result.z = V.x * m[0][2] + V.y * m[1][2] + V.z * m[2][2];
		return Result;
	}

	/** return inverse if it exists, otherwise identity */
	Matrix3 Inverse() const;
    
    ///
    void Print(std::ostream & o) const;

};

/** return the rotation matrix around vector U : checked */
inline Matrix3 GetRotationAroundU(const Vector3& U, double Angle)
{
	// is debugged and optimized
	Vector3 u = U.GetNormalized();
    if (u.Norm()<0.01) {
        Matrix3 r;
        r.SetIdentity();
        return r;
    }
	double cs, ss, ux2, uy2, uz2, uxy, uxz, uyz;
	
	cs = cos(Angle);
	ss = sin(Angle);
	ux2 = u.x*u.x;
	uy2 = u.y*u.y;
	uz2 = u.z*u.z;
	uxy = u.x*u.y;
	uxz = u.x*u.z;
	uyz = u.y*u.z;
    Matrix3 m;
    m.m[0][0] = ux2 + cs*(1-ux2);
    m.m[1][0] = uxy*(1-cs) - u.z*ss;
    m.m[2][0] = uxz*(1-cs) + u.y*ss;
    m.m[0][1] = uxy*(1-cs) + u.z*ss;
    m.m[1][1] = uy2 + cs*(1-uy2);
    m.m[2][1] = uyz*(1-cs)-u.x*ss;
    m.m[0][2] = uxz*(1-cs)-u.y*ss;
    m.m[1][2] = uyz*(1-cs)+u.x*ss;
    m.m[2][2] = uz2 + cs*(1-uz2);
    return m;
    /*
	return Matrix3( ux2 + cs*(1-ux2),
				 uxy*(1-cs) - u.z*ss,
				 uxz*(1-cs) + u.y*ss,
				 uxy*(1-cs) + u.z*ss,
				 uy2 + cs*(1-uy2),
				 uyz*(1-cs)-u.x*ss,
				 uxz*(1-cs)-u.y*ss,
				 uyz*(1-cs)+u.x*ss,
				 uz2 + cs*(1-uz2) );
    */
}

inline Matrix3 GetRotationAroundU(const Vector3& da)
{
	return GetRotationAroundU(da, da.Norm());
}

/*// return the rotation matrix around X
Matrix3 GetRotationX(double Ang)
{
	double a = cos(Ang);
	double b = sin(Ang);
	return Matrix3(
			1.0,	0.0,		0.0,
			0.0,	a,			b,
			0.0,    -b,			a );
}

//return the rotation matrix around Y
Matrix3 GetRotationY(double Ang)
{
	double a = cos(Ang);
	double b = -sin(Ang);
	return Matrix3(
			a,		0.0,	b,
			0.0,	1.0,	0.0,
			-b,		0.0,	a );
}

//return the rotation matrix around Z
Matrix3 GetRotationZ(double Ang)
{
	double a = cos(Ang);
	double b = sin(Ang);
	return Matrix3(
			a,		b,		0.0,
			-b,		a,		0.0,
			0.0,	0.0,	1.0);
}
*/

inline std::ostream & operator<<(std::ostream & s, const Matrix3 & m)
{
    m.Print(s);
    return s;
}

#endif // _H
