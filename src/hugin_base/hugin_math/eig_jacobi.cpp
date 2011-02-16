// -*- c-basic-offset: 4 -*-
/** @file eig_jacobi.cpp
 *
 *  @author mike_ess@yahoo.com
 *
 *  Function comes from http://mywebpages.comcast.net/mike_ess/
 *
 *  $Id$
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "eig_jacobi.h"


//void sortd( int a, double * b, int * c);  /* black box sorting routine */

namespace hugin_utils
{

    void sortd(int length, double * a, int *ind )
    {

        int i, ii, ij, j, m, m1, n2;
        double t;
        
        for ( i = 0 ; i < length; i++ ) ind[ i ] = i;
        m = 1;
        n2 = length / 2;
        m = 2 * m;
        while ( m <= n2 ) m = 2 * m;
        m = m - 1;
    three:;
        m1 = m + 1;
        for ( j = m1-1 ; j < length; j++ ) {
            t = a[ ind[ j ] ];
            ij = ind[ j ];
            i = j - m;
            ii = ind[ i ];
    four:;
            if ( t > a[ ii ] ) {
                ind[ i+m ] = ii;
                i = i - m;
                if ( i >= 0 ) {
                    ii = ind[ i ];
                    goto four;
                }
            }
            ind[ i+m ] = ij;
        }
        m = m / 2;
        if ( m > 0 ) goto three;
        return;
    }


    /* the line below is a marker for where sed will insert global
       parameters like MAX      - size of matrix
                       MAXSWEEP - MAXIMUN number of sweeps
                       EPSILON  - tolerance to deem offdiagonal elements zero
                       choice of test matrix
    */
    //INSERTS

    void eig_jacobi( int n, double a[3][3], double v[3][3], double *d,int* ind,int* maxsweep,int* maxannil,double* epsilon)
    {
    /* Implements jacobi eigenvalue/vector algorithm on a symmetric matrix
       stored as a 2 dimensional matrix a[n][n] and computes the eigenvectors
       in another globally allocated matrix v[n][n].
    intput:
       n        - size of matrix problem
    outputs:
        v        - eigenvector matrix
        d[MAX]   - a vector of unsorted eigenvalues
       ind[MAX] - a vector of indicies sorting d[] into descending order
       maxanil  - number of rotations applied
    inputs/outputs
        a        - input matrix (the input is changed)
        maxsweep - on input max number of sweeps
                - on output actual number of sweeps
       epsilon  - on input tolerance to consider offdiagonal elements as zero
                - on output sum of offdiagonal elements
    */

      int i, j, k, l, p, q, sweep, annil;
      double alpha, beta, c, s, mu1, mu2, mu3;
      double pp, qq, pq, offdiag;
      double t1, t2;


    /* get off diagonal contribution */
      if( n < 1 ) {
        printf( "In jacobi(), size of matrix = %d\n", n );
      }

    /* compute initial offdiagonal sum */
      offdiag = 0.0;
      for( k = 0; k < n; k++ ) {
        for( l = k+1; l < n; l++ ) {
          offdiag = offdiag + a[k][l] * a[k][l];
        }
      }
    /* compute tolerance for completion */
      mu1 = sqrt( offdiag ) / n ;
      mu2 = mu1;
    /* initiallize eigenvector matrix as identity matrix */
      for( i = 0; i < n; i++ ) {
        d[ i ] = a[ i ][ i ];
        for( j = 0; j < n; j++ ) {
          if( i == j ) {
            v[ i ][ j ] = 1.0;
          } else {
            v[ i ][ j ] = 0.0;
          }
        }
      }
      annil = 0;
      sweep = 0; 
    /* test if matrix is initially diagonal */
      if( mu1 <= ( *epsilon * mu1 ) ) goto done;
    /* major loop of sweeps */
      for( sweep = 1; sweep < *maxsweep; sweep++ ) {
    /* sweep */
        for( p = 0; p < n; p++ ) {
          for( q = p+1; q < n; q++ ) {
            if( fabs( a[ p ][ q ] ) > mu2 ) {
              annil++;
    /* calculate rotation to zero out a[ i ][ j ] */
              alpha = 0.5 * ( d[ p ] - d[ q ] );
              beta  = sqrt( a[ p ][ q ] * a[ p ][ q ] + alpha * alpha );
              c = sqrt( 0.5 +  fabs( alpha ) / ( 2.0 * beta ) );
              if( alpha == 0.0 ) {
                s = c;
              } else {
                s = - ( alpha * a[ p ][ q ] ) / ( 2.0 * beta * fabs( alpha ) * c ); 
              }
    /* apply rotation */
              pp = d[ p ];
              pq = a[ p ][ q ];
              qq = d[ q ];
              d[ p ] = c * c * pp + s * s * qq - 2.0 * s * c * pq;
              d[ q ] = s * s * pp + c * c * qq + 2.0 * s * c * pq;
              a[ p ][ q ] = ( c * c - s * s ) * pq + s * c * ( pp - qq );
    /* update columns */
              for( k = 0; k < p; k++ ) {
                t1 = a[ k ][ p ];
                t2 = a[ k ][ q ];
                a[ k ][ p ] = c * t1 - s * t2;
                a[ k ][ q ] = c * t2 + s * t1;
              }
    /* update triangular area */
              for( k = p+1; k < q; k++ ) {
                t1 = a[ p ][ k ];
                t2 = a[ k ][ q ];
                a[ p ][ k ] = c * t1 - s * t2;
                a[ k ][ q ] = c * t2 + s * t1;
              }
    /* update rows */
              for( k = q+1; k < n; k++ ) {
                t1 = a[ p ][ k ];
                t2 = a[ q ][ k ];
                a[ p ][ k ] = c * t1 - s * t2;
                a[ q ][ k ] = c * t2 + s * t1;
              }
    /* update matrix of eigenvectors */
              for( k = 0; k < n; k++ ) {
                t1 = v[ p ][ k ];
                t2 = v[ q ][ k ];
                v[ p ][ k ] = c * t1 - s * t2;
                v[ q ][ k ] = s * t1 + c * t2;
              }
            } /* abs( a[ p ][ q ] ) > mu2 */ 
          } /* p */
        } /* q */
    /* test for small off diagonal contribution */
        offdiag = 0.0;
        for( k = 0; k < n; k++ ) {
          for( l = k+1; l < n; l++ ) {
            offdiag = offdiag + a[k][l] * a[k][l];
          }
        }
        mu3 = sqrt( offdiag ) / n;
    /* has offdiagonal sum gotten larger ? if so there's a problem
         may be not positive definite ? 
    */
        if( mu2 < mu3 ) {
          printf( "Offdiagonal sum is increasing muold= %f munow= %f\n", mu2, mu3 );
          exit( -1 );
        } else {
          mu2 = mu3;
        }
    /* has offdiagonal sum gone below input goal ? */
        if( mu2 <= ( *epsilon * mu1 ) ) goto done;
      } /* sweep */
    done:;
      *maxsweep = sweep;
      *maxannil = annil;
      *epsilon  = offdiag;
      sortd( n, d, ind ); 
    }

} // namespace
