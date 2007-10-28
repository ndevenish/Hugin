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

#include "Matrix3.h"

inline Matrix3 getIdentity()
{
    Matrix3 tmp;
    tmp.SetIdentity();
    return tmp;
}
Matrix3 Matrix3::Identity = getIdentity();


/** default constructor : initialise to zero */
Matrix3::Matrix3() {
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            m[i][j] = 0.0;
}

/** copy constructor */
Matrix3::Matrix3(const Matrix3& ot)
{
    (*this) = ot; // call copy operator
}

/** Set the identity matrix */
void Matrix3::SetIdentity()
{
    m[0][0] = 1; m[0][1] = 0;  m[0][2] = 0;
    m[1][0] = 0; m[1][1] = 1;  m[1][2] = 0;
    m[2][0] = 0; m[2][1] = 0;  m[2][2] = 1;
}

/** Set the matrice to rotation using yaw, pitch, roll angle */
void Matrix3::SetRotation( double Yaw, double Pitch, double Roll )
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

#if 0
/** Set the matrice to rotation using yaw, pitch, roll angle, panotools style,
 *  copied from Panotools-Script by Bruno Postle
 */
Matrix3::void SetRotationPT( double Yaw, double Pitch, double Roll )
{
    double  SR  = sin(Roll),
    SP  = sin(Pitch),
    SY  = sin(Yaw),
    CR  = cos(Roll),
    CP  = cos(Pitch),
    CY  = cos(Yaw);

    m[0][0] = CP * CY;
    m[0][1] = CP * SY;
    m[0][2] = -SP;

    m[1][0] = SR * SP * CY - CR * SY;
    m[1][1] = SR * SP * SY + CR * CY;
    m[1][2] = SR * CP;

    m[2][0] = CR * SP * CY + SR * SY;
    m[2][1] = CR * SP * SY - CY * SR;
    m[2][2] = CR * CP;
}
#endif 


/** set rotation in panotools style, 
 *  code adapted from Panotools-Script by Bruno Postle
 */
void Matrix3::SetRotationPT( double yaw, double pitch, double roll )
{
    double cosr = cos (roll);
    double sinr = sin (roll);
    double cosp = cos (pitch);
    double sinp = sin (0 - pitch);
    double cosy = cos (yaw);
    double siny = sin (0 - yaw);

    Matrix3 rollm;

    /*
    rollm[0][0] = new Math::Matrix ([        1,       0,       0 ],
            [        0,   cosr,-1*sinr ],
            [        0,   sinr,   cosr ]);
    */

    rollm.m[0][0] = 1.0;      rollm.m[0][1] = 0.0;      rollm.m[0][2] = 0.0;
    rollm.m[1][0] = 0.0;      rollm.m[1][1] = cosr;     rollm.m[1][2] = -sinr;
    rollm.m[2][0] = 0.0;      rollm.m[2][1] = sinr;     rollm.m[2][2] = cosr;

    /*
my pitchm = new Math::Matrix ([    cosp,       0,   sinp ],
                                    [        0,       1,       0 ],
                                    [ -1*sinp,       0,   cosp ]);
    */
    
    Matrix3 pitchm;
    pitchm.m[0][0] = cosp;   pitchm.m[0][1] =  0.0;  pitchm.m[0][2] = sinp;
    pitchm.m[1][0] =  0.0;   pitchm.m[1][1] =    1;  pitchm.m[1][2] = 0.0;
    pitchm.m[2][0] = -sinp;  pitchm.m[2][1] =  0.0;  pitchm.m[2][2] = cosp;

    /*
my yawm   = new Math::Matrix ([    cosy,-1*siny,       0 ],
                                    [    siny,   cosy,       0 ],
                                    [        0,       0,       1 ]);
    */
    Matrix3 yawm;
    yawm.m[0][0] = cosy;   yawm.m[0][1] = -siny;   yawm.m[0][2] = 0.0;
    yawm.m[1][0] = siny;   yawm.m[1][1] =  cosy;   yawm.m[1][2] = 0.0;
    yawm.m[2][0] = 0.0;    yawm.m[2][1] =   0.0;   yawm.m[2][2] = 1.0;


    *this = yawm * pitchm * rollm;
}

/** GetRotation in panotools style. */
void Matrix3::GetRotationPT( double & Yaw, double & Pitch, double & Roll )
{
    /*
    my $matrix = shift;
    my $roll = atan2 ($matrix->[2]->[1], $matrix->[2]->[2]);
    my $pitch = -1 * asin (-1 * $matrix->[2]->[0]);
    my $yaw = atan2 (-1 * $matrix->[1]->[0], $matrix->[0]->[0]);
    return ($roll, $pitch, $yaw);

    */
    Roll = atan2 (m[2][1], m[2][2]);
    Pitch = - asin (- m[2][0]);
    Yaw = atan2 (- m[1][0], m[0][0]);
}

/** set the matrice to rotation around X */
void Matrix3::SetRotationX( double a )
{
    m[0][0] = 1.0;		m[0][1] = 0.0; 		m[0][2] = 0.0;
    m[1][0] = 0.0; 		m[1][1] = cos(a); 	m[1][2] = sin(a);
    m[2][0] = 0.0;		m[2][1] =-m[1][2];	m[2][2] = m[1][1];
}

void Matrix3::SetRotationY( double a )
{
    m[0][0] = cos(a); 	m[0][1] = 0.0;		m[0][2] =-sin(a);
    m[1][0] = 0.0; 		m[1][1] = 1.0;		m[1][2] = 0.0;
    m[2][0] =-m[0][2];	m[2][1] = 0.0;		m[2][2] = m[0][0];
}

void Matrix3::SetRotationZ( double a )
{
    m[0][0] = cos(a); 	m[0][1] = sin(a); 	m[0][2] = 0.0;
    m[1][0] =-m[0][1];	m[1][1] = m[0][0]; 	m[1][2] = 0.0;
    m[2][0] = 0.0;		m[2][1] = 0.0;		m[2][2] = 1.0;
}

/** copy operator */
Matrix3& Matrix3::operator= (const Matrix3& ot)
{
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            m[i][j] = ot.m[i][j];
    return *this;
}

/** multiplication with another matrix */
Matrix3 Matrix3::operator*(const Matrix3& ot) const
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
void Matrix3::operator/=(double s)
{
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            m[i][j] /= s;
}

/** operator *= */
void Matrix3::operator*=(double s)
{
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            m[i][j] *= s;
}

/** operator *= */
void Matrix3::operator*=(Matrix3 ot)
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

/** return inverse if it exists, otherwise identity */
Matrix3 Matrix3::Inverse() const
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

void Matrix3::Print(std::ostream & o) const
{
    o << "[ " << m[0][0] << "\t" << m[0][1] << "\t" <<  m[0][2] << std::endl
      << "  " << m[1][0] << "\t" << m[1][1] << "\t" <<  m[1][2] << std::endl
      << "  " << m[2][0] << "\t" << m[2][1] << "\t" <<  m[2][2] << std::endl;
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
