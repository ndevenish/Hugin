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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _MATRIX3_H_
#define _MATRIX3_H_

/** general : Matrix3 is a class for handling 3x3 Matrix manipulation.
 *
 * We do not use 4x4 matrix for view point changement as the calculus could be inefficent
 * (some of the coefficients are null, m14 = m24 = m34 = 0 et m44 = 1.0 always).
 */

class Matrix3
{
public:
	/** we define the Matrix3 as 3 colums of 3 rows */
	double m[3][3];

	static Matrix3	Identity;
	
public:
	/** default constructor : initialise to zero */
	Matrix3() { }
	
	/** copy constructor */
	Matrix3(const Matrix3& ot)
	{
		(*this) = ot; // call copy operator 
	}

	/** Set the identity matrix */
	void SetIdentity()
	{
		m[0][0] = 1; m[0][1] = 0;  m[0][2] = 0;
		m[1][0] = 0; m[1][1] = 1;  m[1][2] = 0;
		m[2][0] = 0; m[2][1] = 0;  m[2][2] = 1;
	}

	/** Set the matrice to rotation using yaw, pitch, roll angle */
	void SetRotation( double Yaw, double Pitch, double Roll )
	{
		double	SR	= sin(Roll),
				SP	= sin(Pitch),
				SY	= sin(Yaw),
				CR	= cos(Roll),
				CP	= cos(Pitch),
				CY	= cos(Yaw);

		m[0][0]	= CP * CY;
		m[0][1]	= CP * SY;
		m[0][2]	= SP;
		
		m[1][0]	= SR * SP * CY - CR * SY;
		m[1][1]	= SR * SP * SY + CR * CY;
		m[1][2]	= - SR * CP;
		
		m[2][0]	= -( CR * SP * CY + SR * SY );
		m[2][1]	= CY * SR - CR * SP * SY;
		m[2][2]	= CR * CP;
	}

	/** set the matrice to rotation around X */
	void SetRotationX( double a )
	{
		m[0][0] = 1.0;		m[0][1] = 0.0; 		m[0][2] = 0.0;
		m[1][0] = 0.0; 		m[1][1] = cos(a); 	m[1][2] = sin(a);
		m[2][0] = 0.0;		m[2][1] =-m[1][2];	m[2][2] = m[1][1];
	}

	void SetRotationY( double a )
	{
		m[0][0] = cos(a); 	m[0][1] = 0.0;		m[0][2] =-sin(a);
		m[1][0] = 0.0; 		m[1][1] = 1.0;		m[1][2] = 0.0;
		m[2][0] =-m[0][2];	m[2][1] = 0.0;		m[2][2] = m[0][0];
	}
	
	void SetRotationZ( double a )
	{
		m[0][0] = cos(a); 	m[0][1] = sin(a); 	m[0][2] = 0.0;
		m[1][0] =-m[0][1];	m[1][1] = m[0][0]; 	m[1][2] = 0.0;
		m[2][0] = 0.0;		m[2][1] = 0.0;		m[2][2] = 1.0;
	}

	/** copy operator */
	Matrix3& operator= (const Matrix3& ot)
	{
		for (int i=0; i<3; i++)
			for (int j=0; j<3; j++)
				m[i][j] = ot.m[i][j];
		return *this;
	}
	
	/** multiplication with another matrix */ 
	Matrix3 operator*(const Matrix3& ot) const
	{
		Matrix3	Result;
		Result.m[0][0] = m[0][0] * ot.m[0][0] + m[0][1] * ot.m[1][0] + m[0][2] * ot.m[2][0];
		Result.m[0][1] = m[0][0] * ot.m[0][1] + m[0][1] * ot.m[1][1] + m[0][2] * ot.m[2][1];
		Result.m[0][2] = m[0][0] * ot.m[0][2] + m[0][1] * ot.m[1][2] + m[0][2] * ot.m[2][2];
		
		Result.m[1][0] = m[1][0] * ot.m[0][0] + m[1][1] * ot.m[1][0] + m[1][2] * ot.m[2][0];
		Result.m[1][1] = m[1][0] * ot.m[0][1] + m[1][1] * ot.m[1][1] + m[1][2] * ot.m[2][1];
		Result.m[1][2] = m[1][0] * ot.m[0][2] + m[1][1] * ot.m[1][2] + m[1][2] * ot.m[2][2];
		
		Result.m[2][0] = m[2][0] * ot.m[0][0] + m[2][1] * ot.m[1][0] + m[2][2] * ot.m[2][0];
		Result.m[2][1] = m[2][0] * ot.m[0][1] + m[2][1] * ot.m[1][1] + m[2][2] * ot.m[2][1];
		Result.m[2][2] = m[2][0] * ot.m[0][2] + m[2][1] * ot.m[1][2] + m[2][2] * ot.m[2][2];
		return Result;
	}

	/** operator *= */
	void operator*=(Matrix3 ot)
	{
		Matrix3 Result;
		Result.m[0][0] = m[0][0] * ot.m[0][0] + m[0][1] * ot.m[1][0] + m[0][2] * ot.m[2][0];
		Result.m[0][1] = m[0][0] * ot.m[0][1] + m[0][1] * ot.m[1][1] + m[0][2] * ot.m[2][1];
		Result.m[0][2] = m[0][0] * ot.m[0][2] + m[0][1] * ot.m[1][2] + m[0][2] * ot.m[2][2];
		
		Result.m[1][0] = m[1][0] * ot.m[0][0] + m[1][1] * ot.m[1][0] + m[1][2] * ot.m[2][0];
		Result.m[1][1] = m[1][0] * ot.m[0][1] + m[1][1] * ot.m[1][1] + m[1][2] * ot.m[2][1];
		Result.m[1][2] = m[1][0] * ot.m[0][2] + m[1][1] * ot.m[1][2] + m[1][2] * ot.m[2][2];
		
		Result.m[2][0] = m[2][0] * ot.m[0][0] + m[2][1] * ot.m[1][0] + m[2][2] * ot.m[2][0];
		Result.m[2][1] = m[2][0] * ot.m[0][1] + m[2][1] * ot.m[1][1] + m[2][2] * ot.m[2][1];
		Result.m[2][2] = m[2][0] * ot.m[0][2] + m[2][1] * ot.m[1][2] + m[2][2] * ot.m[2][2];
		*this = Result;
	}

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
		double result = 0.0;
		result  = m[0][0] * ( m[1][1] * m[2][2] - m[2][1] * m[1][2] );
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
	Matrix3 Inverse() const
	{
		Matrix3 Result;
		double Det = Determinant();

		if (Det == 0.0f)
			return Matrix3::Identity;

		double	invDet = 1.0f / Det;

		Result.m[0][0] =  invDet * ( m[1][1] * m[2][2] - m[2][1] * m[1][2] );
		Result.m[0][1] = -invDet * ( m[0][1] * m[2][2] - m[2][1] * m[0][2] );
		Result.m[0][2] =  invDet * ( m[0][1] * m[1][2] - m[1][1] * m[0][2] );
		
		Result.m[1][0] = -invDet * ( m[1][0] * m[2][2] - m[2][0] * m[1][2] );
		Result.m[1][1] =  invDet * ( m[0][0] * m[2][2] - m[2][0] * m[0][2] );
		Result.m[1][2] = -invDet * ( m[0][0] * m[1][2] - m[1][0] * m[0][2] );
		
		Result.m[2][0] =  invDet * ( m[1][0] * m[2][1] - m[2][0] * m[1][1] );
		Result.m[2][1] = -invDet * ( m[0][0] * m[2][1] - m[2][0] * m[0][1] );
		Result.m[2][2] =  invDet * ( m[0][0] * m[1][1] - m[1][0] * m[0][1] );
		return Result;
	}

	

};

/** return the rotation matrix around vector U : checked */
/*Matrix3 GetRotationAroundU(const Vector3& U, double Angle)
{
	// is debugged and optimized
	Vector3 u = U.Unit();
	if (u.Abs()<0.01)
		return Matrix3(1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0);
	double cs, ss, ux2, uy2, uz2, uxy, uxz, uyz;
	
	cs = cos(Angle);
	ss = sin(Angle);
	ux2 = u.x*u.x;
	uy2 = u.y*u.y;
	uz2 = u.z*u.z;
	uxy = u.x*u.y;
	uxz = u.x*u.z;
	uyz = u.y*u.z;
	return Matrix3( ux2 + cs*(1-ux2),
				 uxy*(1-cs) - u.z*ss,
				 uxz*(1-cs) + u.y*ss,
				 uxy*(1-cs) + u.z*ss,
				 uy2 + cs*(1-uy2),
				 uyz*(1-cs)-u.x*ss,
				 uxz*(1-cs)-u.y*ss,
				 uyz*(1-cs)+u.x*ss,
				 uz2 + cs*(1-uz2) );
}

Matrix3 GetRotationAroundU(const Vector3& da)
{
	return GetRotationAroundU(da, da.Abs());
}*/

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
#endif