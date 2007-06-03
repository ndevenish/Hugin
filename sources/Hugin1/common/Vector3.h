// -*- c-basic-offset: 4 -*-
/** @file Vector3.h
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
 *
 *  $Id: Vector3.h 1769 2006-12-20 00:42:16Z dangelo $
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

#ifndef _VECTOR3_H_
#define _VECTOR3_H_

// small number below which we consider that it's zero
#define EPSILON  0.0000001

/** general : Vector3 is a class for handling 3D Vectors manipulation.
 *
 * We made a choose to store only a 3 dimensions vectors to speed
 * up system when we change of view point. A general 3D transformation
 * could be placed in a 4x4 matrix, but some of the coefficients are
 * always null. So that's waste of time.
 *
 */
class Vector3
{
public:
	/** x,y,z coordinates, 0 at the initialisation */
	double	x, y, z;

public:
	
	/** default constructor */
	Vector3(): x(0), y(0), z(0) {}
		
	/** constructor with initialisation */
	Vector3(double a, double b, double c): x(a), y(b), z(c) {}

	/** copy contructor */
	Vector3(const Vector3& v) { x = v.x; y = v.y; z = v.z; }
	
	/** copy operator */
	inline Vector3& operator= (const Vector3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	/** set */
	void Set(double a, double b, double c) { x=a; y=b; z=c; }

	/** comparison : equality */
	inline bool operator== (const Vector3& v) const
	{
		return (v.x==x && v.y==y && v.z==z);
	}

	/** comparison : not equal */
	inline bool operator!= (const Vector3& v ) const
	{
		return !(v == *this);
	}

	/** comparison : zero */
	bool IsZero() const
	{
		return ((x==0.f) && (y==0.f) && (z==0.f));
	}

	/** comparison : nearly zero */
	bool IsNearlyZero() const
	{
		return ( (fabs(x)<EPSILON) && (fabs(y)<EPSILON) && (fabs(z)<EPSILON) );
	}
	
	/** comparison : nearly equal */
	bool IsNearlyEqual(const Vector3& v) const
	{
		return ( (fabs(x-v.x)<EPSILON) && (fabs(y-v.y)<EPSILON) && (fabs(z-v.z)<EPSILON) );
	}

	/** operator * */
	friend Vector3 operator*( double Scale, const Vector3 & v )
	{
		return Vector3( v.x * Scale, v.y * Scale, v.z * Scale );
	}

	/** operator + */
	inline Vector3 operator+( const Vector3& v ) const
	{
		return Vector3( x + v.x, y + v.y, z + v.z );
	}

	/** operator - */
	inline Vector3 operator-( const Vector3& v ) const
	{
		return Vector3( x - v.x, y - v.y, z - v.z );
	}

	/** operator *(double) */
	inline Vector3 operator*( double Scale ) const
	{
		return Vector3( x * Scale, y * Scale, z * Scale );
	}

	/** operator /(double) */
	Vector3 operator/( double Scale ) const
	{
		double invScale = 1.f/Scale;
		return Vector3( x * invScale, y * invScale, z * invScale );
	}

	/** Unary minus */
	inline Vector3 operator-() const
	{
		return Vector3( -x, -y, -z );
	}

	/** operator += */
	inline Vector3 operator+=( const Vector3& v )
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	/** operator -= */
	inline Vector3 operator-=( const Vector3& v )
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	/** double multiply */
	inline Vector3 operator*=( double Scale )
	{
		x *= Scale;
		y *= Scale;
		z *= Scale;
		return *this;
	}

	/** double divide */
	Vector3 operator/=( double Scale )
	{
		double invScale = 1.f/Scale;
		x *= invScale;
		y *= invScale;
		z *= invScale;
		return *this;
	}

	/** euclidien norm */
	double Norm() const
	{
		return sqrt( x*x + y*y + z*z );
	}

	/** squared norm */
	double NormSquared() const
	{
		return x*x + y*y + z*z;
	}

	/** cross product */
	inline Vector3 Cross( const Vector3& v ) const
	{
		return Vector3( v.z*y - v.y*z, v.x*z - v.z*x, v.y*x - v.x*y);
	}

	/** dot product */
	inline double Dot( const Vector3& v ) const
	{
		return x*v.x + y*v.y + z*v.z;
	}

	/** Normalize */
	bool Normalize()
	{
		double SquareSum = x*x + y*y + z*z;
		if( SquareSum >= EPSILON )
		{
			double invNorm = 1.f/sqrt(SquareSum);
			x *= invNorm;
			y *= invNorm;
			z *= invNorm;
			return true;
		}
		return false;
	}
	
	/** return a normalized vector */
	Vector3 GetNormalized() const
	{
		Vector3 result(*this);
		double SquareSum = x*x + y*y + z*z;
		if( SquareSum >= EPSILON )
		{
			double invNorm = 1.f/sqrt(SquareSum);
			result.x *= invNorm;
			result.y *= invNorm;
			result.z *= invNorm;
		}
		return result;
	}
};

inline std::ostream & operator<<(std::ostream & s, const Vector3 & v)
{
    s << "[ " << v.x << ", " << v.y << ", " << v.z << " ]";
    return s;
}

#endif
