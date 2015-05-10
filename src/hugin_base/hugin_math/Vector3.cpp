// -*- c-basic-offset: 4 -*-
/** @file Vector3.h
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

#include <math.h>

#include "Vector3.h"


/** set */
void Vector3::Set(double a, double b, double c) { x=a; y=b; z=c; }

/** comparison : zero */
bool Vector3::IsZero() const
{
    return ((x==0.f) && (y==0.f) && (z==0.f));
}

/** comparison : nearly zero */
bool Vector3::IsNearlyZero() const
{
    return ( (fabs(x)<EPSILON) && (fabs(y)<EPSILON) && (fabs(z)<EPSILON) );
}

/** comparison : nearly equal */
bool Vector3::IsNearlyEqual(const Vector3& v) const
{
    return ( (fabs(x-v.x)<EPSILON) && (fabs(y-v.y)<EPSILON) && (fabs(z-v.z)<EPSILON) );
}

/** operator /(double) */
Vector3 Vector3::operator/( double Scale ) const
{
    double invScale = 1.f/Scale;
    return Vector3( x * invScale, y * invScale, z * invScale );
}

/** double divide */
Vector3 Vector3::operator/=( double Scale )
{
    double invScale = 1.f/Scale;
    x *= invScale;
    y *= invScale;
    z *= invScale;
    return *this;
}

/** euclidien norm */
double Vector3::Norm() const
{
    return sqrt( x*x + y*y + z*z );
}

/** squared norm */
double Vector3::NormSquared() const
{
    return x*x + y*y + z*z;
}

/** Normalize */
bool Vector3::Normalize()
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
Vector3 Vector3::GetNormalized() const
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

