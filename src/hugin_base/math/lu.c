/* lu.c  -  PA = LU factorisation with pivoting
 * $Id: lu.c 1612 2006-06-01 19:04:35Z dangelo $
 * Copyright (C) 2000
 *    Antoine Lefebvre <antoine.lefebvre@polymtl.ca>
 *
 *
 * Licensed under the GPLv2
 */
   
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/*
  
   This algorithm will compute an LU factorisation on the augmented
   matrix (A) passed in arguments.

   This algorithm assumed the element on the diagonal of the lower
   triangular matrix set to 1.
  
   In order to save memory space, every coefficient of both matrix
   L and U are written in the original matrix overwriting initial
   values of A. 

*/

int math_lu_solve(double *matrix, double *solution, int neq)
{
  int i, j, k;
  
  int    idx;   /* index of the larger pivot */
  double big;       /* the larger pivot found */
  double tmp = 0.0;
  int itmp;
  
  int    *P;        /* keep memory of permutation (column permutation) */
  double *y;
    
  P = (int *) calloc (neq, sizeof(int));
  y = (double *) calloc (neq, sizeof(double));

  for (i = 0; i < neq; i++)
  {
    solution[i]  = 0; /* reset the solution vector */
    P[i] = i;         /* initialize permutation vector */
  }
    
  /* LU Factorisation */

  for (i = 0; i < neq - 1; i++) /* line */
  {
    for (j = i; j < neq; j++) /* column */
    {  
      tmp = 0.0;
      for (k = 0; k < i; k++)
        tmp += matrix[i + neq*P[k]] * matrix[k + neq*P[j]];

      matrix[i + neq*P[j]] = matrix[i + neq*P[j]] - tmp; 
    }

    /* find the larger pivot and interchange the columns */
    big = 0.0;
    idx = i;
    for (j = i; j < neq; j++)
    {
      if (big < fabs(matrix[i + neq*P[j]])) /* we found a larger pivot */
      {
        idx = j;
        big = fabs(matrix[i + neq*P[j]]);
      }
    }
    /* check if we have to interchange the lines */
    if (idx != i)
    {
      itmp   = P[i];
      P[i]   = P[idx];
      P[idx] = itmp;
    }

    if (matrix[i + neq*P[i]] == 0.0)
    {
      //printf("LU: matrix is singular, no unique solution.\n");
      free (y);
      free (P);
      return 0;
    }
    
    for (j = i+1; j < neq; j++)
    {
      tmp = 0.0;
      for (k = 0; k < i; k++)
        tmp += matrix[j + neq*P[k]] * matrix[k + neq*P[i]];

      matrix[j + neq*P[i]] = (matrix[j + neq*P[i]] - tmp)/matrix[i + neq*P[i]];
    }
  }

  i = neq - 1;  
  tmp = 0.0;
  for (k = 0; k < neq-1; k++)
    tmp += matrix[i + neq*P[k]] * matrix[k + neq*P[i]];

  matrix[i + neq*P[i]] = matrix[i + neq*P[i]] - tmp;

  
  /* End LU-Factorisation */
    
  /* substitution  for y    Ly = b*/
  for (i = 0; i < neq; i++)
  {
    tmp = 0.0;
    for (j = 0; j < i; j++)
      tmp += matrix[i + neq*P[j]] * y[j];
    
    y[i] = matrix[i + neq*neq] - tmp;
  }
  
  /* substitution for x   Ux = y*/
  for (i = neq - 1; i >=0; i--)
  {
    if (matrix[i + neq*P[i]] == 0.0)
    {
      //printf("LU: No unique solution exist.\n");
      free (y);
      free (P);
      return 0;
    }
    
    tmp = 0.0;
    for (j = i; j < neq; j++)
      tmp += matrix[i + neq*P[j]] * solution[P[j]];
    
    solution[P[i]] = (y[i] - tmp)/matrix[i + neq*P[i]];    
  }
     
  free (y);
  free (P);
  return 1;      
}

/* This function will divide each row of the matrix
 * by the highest element of this row.
 * This kind of scaling could improve precision while
 * solving some difficult matrix.
 */
int NUM_matscale(double *matrix, int neq)
{
  int i; /* line */
  int j; /* column */

  double val;
  double tmp;
  
  for (i = 0; i < neq; i++)
  {
    val = 0;
    /* find the highest value */
    for (j = 0; j < neq; j++)
    {
      tmp = fabs(matrix[i + neq*j]);
      val = (tmp > val) ? tmp : val;
    }

    /* divide element of the line by this value
     * including the right side
     * if the max value is defferent than zero
     */
    if (val != 0.0)
    {
      for (j = 0; j < neq+1; j++)
      matrix[i + neq*j] = matrix[i + neq*j]/val;
    }
  }
  return 0;

}
