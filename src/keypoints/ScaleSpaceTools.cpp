#include ".\scalespace.h"

void OrientationVector::AddOrientation( float rad_angle, float value )
{
	// check boundaries of entries
	//assert( rad_angle>=-PI );
	//assert( rad_angle<= PI );

	float angle = rad_angle;

	// [-pi ; pi] -> [0 ; 2pi]
	angle = rad_angle + PI;

	// [0 ; 2pi] -> [0 ; 36]
	angle = ( angle * ORIENTARRAY ) / (2.0 * PI);

	int index = (int) angle;
	
	// if exactely 2.pi => 0
	if ( index == ORIENTARRAY )
		index = 0;
	//assert ((index >= 0) && (index < OSize));  // [0, 36[

	Orient[index] += value;
}

// smooth the vector
void OrientationVector::Smooth()
{
	float vect[ ORIENTARRAY ];
	int n;

	for( n = 0 ; n < ORIENTARRAY ; n++ ) 
	{
		float val0 = Orient[ (n-1+ORIENTARRAY) % ORIENTARRAY ];
		float val1 = Orient[ (n  +ORIENTARRAY) % ORIENTARRAY ];
		float val2 = Orient[ (n+1+ORIENTARRAY) % ORIENTARRAY ];

		vect[n] = (val0+val1+val2)/3.0f;
	}
	for( n = 0 ; n < ORIENTARRAY ; n++ ) 
		Orient[n] = vect[n];
}

// retrieves the maximum
float OrientationVector::FindMax()
{
	float max = -1e10;
	int n;
	for( n = 0 ; n < ORIENTARRAY ; n++ ) 
		if (Orient[n] > max )
			max = Orient[n];
	return max;
}

// for the feature vector, linear add the value
void FeatureVector::AddValue( float x, float y, float orientation, float value )
{
	// indexes
	//  0 <= x < 4 : [0,4[
	//  0 <= y < 4 : [0,4[
	//  0 <= o < 8 : [0,8[

	// will be lineary added in 8 cases

	int xi = (int)x;
	int yi = (int)y;
	int oi = (int)orientation;

	float magx = x-xi;
	float magy = y-yi;
	float mago = orientation-oi;

	for (int i=0; i<=1; i++)
	{
		if ((i+xi)>3)
			continue;
		
		float mag;
		
		if (i==0)
			mag = magx * value;
		else
			mag = (1.0f - magx) * value;
			
		for (int j=0; j<=1; j++)
		{
			if ((j+yi)>3)
				continue;

			if (j==0)
				mag *= magy;
			else
				mag *= (1.0f - magy);

			for (int k=0; k<=1; k++)
			{
				int oriCur = oi + k;
				if (oriCur > 7)
					oriCur = 0;
				
				if (k==0)
					mag *= mago;
				else
					mag *= (1.0f - mago);

				FVFloat[ 4*8*(i+xi) + 8*(j+yi) + oriCur ] += mag;
			}
		}
	}
}

// normalize the feature Vector
void FeatureVector::Normalize()
{
	int i;
	float sq = 0.0f;
	for (i=0; i<FVSIZE; i++)
		sq += FVFloat[i] * FVFloat[i];
	sq = sqrt(sq);
	for (i=0; i<FVSIZE; i++)
		FVFloat[i] /= sq;
}

// threathold
void FeatureVector::Threshold( float limit )
{
	bool changed = false;
	for (int i=0; i<FVSIZE; i++) 
	{
		if (FVFloat[i] > limit) 
		{
			FVFloat[i] = limit;
			changed = true;
		}
	}
	if (changed)
		Normalize();
}