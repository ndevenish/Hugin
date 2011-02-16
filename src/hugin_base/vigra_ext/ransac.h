// -*- c-basic-offset: 4 -*-
/** @file ransac.h
 *
 *  Generic implementation of the RanSaC algorithm.
 *
 *  @author Ziv Yaniv
 *
 *   Some minor changes by Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _RANSAC_H_
#define _RANSAC_H_

#include <set>
#include <vector>
#include <stdlib.h>
#include <cstring>
#include <math.h>
#include <ctime>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

//#include "ParameterEsitmator.h"

//#define DEBUG_RANSAC

/**
 * This class implements the Random Sample Consensus (RanSac) framework,
 * a framework for robust parameter estimation.
 * Given data containing outliers we estimate the model parameters using sub-sets of
 * the original data:
 * 1. Choose the minimal subset from the data for computing the exact model parameters.
 * 2. See how much of the input data agrees with the computed parameters.
 * 3. Goto step 1. This can be done up to (m choose N) times, where m is the number of
 *    data objects required for an exact estimate and N is the total number of data objects.
 * 4. Take the largest subset of objects which agreed on the parameters and compute a
 *    least squares fit using them.
 * 
 * This is based on:
 * Fischler M.A., Bolles R.C., 
 * ``Random Sample Consensus: A Paradigm for Model Fitting with Applications to Image Analysis and Automated Cartography'', 
 * Communications of the ACM, Vol. 24(6), 1981.
 *
 * Hartely R., Zisserman A., "Multiple View Geometry in Computer Vision"
 *
 * The class template parameters are T - objects used for the parameter estimation 
 *                                      (e.g. Point2D in line estimation, 
 *                                            std::pair<Point2D,Point2D> in homography estimation).
 *                                   S - type of parameters (e.g. std::vector<double>).                          
 *
 * Author: Ziv Yaniv
 *
 * Small modifications by Pablo d'Angelo:
 *  * allow arbitrary parameters, not just vector<S>
 */
class Ransac {

public:
	/**
	 * Estimate the model parameters using the RanSac framework.
	 * @param parameters A vector which will contain the estimated parameters.
	 *                   If there is an error in the input then this vector will be empty.
	 *                   Errors are: 1. Less data objects than required for an exact fit.
	 *                               2. The given data is in a singular configuration (e.g. trying to fit a circle
	 *                                  to a set of colinear points).
	 * @param paramEstimator An object which can estimate the desired parameters using either an exact fit or a 
	 *                       least squares fit.
	 * @param data The input from which the parameters will be estimated.
	 * @param numForEstimate The number of data objects required for an exact fit.
	 * @param desiredProbabilityForNoOutliers The probability that at least one of the selected subsets doesn't contains an
	 *                                        outlier.
	 * @param maximalOutlierPercentage The maximal expected percentage of outliers.
	 * @return Array with inliers
	 */
        template<class Estimator, class S, class T>
	static std::vector<const T*> compute(S & parameters,
					     std::vector<int> & inliers,
					     const Estimator & paramEstimator ,
					     const std::vector<T> &data, 
					     double desiredProbabilityForNoOutliers,
					     double maximalOutlierPercentage);


	/**
	 * Estimate the model parameters using the maximal consensus set by going over ALL possible
	 * subsets (brute force approach).
	 * Given: n -  data.size()
	 *        k - numForEstimate
	 * We go over all n choose k subsets       n!
	 *                                     ------------
	 *                                      (n-k)! * k!
	 * @param parameters A vector which will contain the estimated parameters.
	 *                   If there is an error in the input then this vector will be empty.
	 *                   Errors are: 1. Less data objects than required for an exact fit.
	 *                               2. The given data is in a singular configuration (e.g. trying to fit a circle
	 *                                  to a set of colinear points).
	 * @param paramEstimator An object which can estimate the desired parameters using either an exact fit or a 
	 *                       least squares fit.
	 * @param data The input from which the parameters will be estimated.
	 * @param numForEstimate The number of data objects required for an exact fit.
	 * @return Array with inliers
	 *
	 * NOTE: This method should be used only when n choose k is small (i.e. k or (n-k) are approximatly equal to n)
	 *
	 */
        template<class Estimator, class S, class T>
        static std::vector<const T*> compute(S &parameters, 
					     const Estimator & paramEstimator ,
					     const std::vector<T> &data);
	
private:

    /**
     * Compute n choose m  [ n!/(m!*(n-m)!)]
     */
    static unsigned int choose(unsigned int n, unsigned int m);

    template<class Estimator, class T>
    static void computeAllChoices(const Estimator & paramEstimator,
                                  const std::vector<T> &data,
                                  int numForEstimate,
                                  short *bestVotes, short *curVotes,
                                  int &numVotesForBest, int startIndex,
                                  int n, int k, int arrIndex, int *arr);


    template<class Estimator, class T, class S>
    static void estimate(const Estimator & paramEstimator, const std::vector<T> &data,
                         int numForEstimate,
                         short *bestVotes, short *curVotes,
                         int &numVotesForBest, int *arr);

    class SubSetIndexComparator 
    {
    private:
        int m_length;
        public:
            SubSetIndexComparator(int arrayLength) : m_length(arrayLength)
            {}

        bool operator()(const int *arr1, const int *arr2) const 
        {
            for(int i=0; i<m_length; i++)
                if(arr1[i] < arr2[i])
                    return true;
                return false;			
        }
    };

};


/*******************************RanSac Implementation*************************/

template<class Estimator, class S, class T>
std::vector<const T *> Ransac::compute(S &parameters,
				       std::vector<int> & inliers,
				       const Estimator & paramEstimator,
				       const std::vector<T> &data,
				       double desiredProbabilityForNoOutliers,
				       double maximalOutlierPercentage)
{
    unsigned int numDataObjects = (int) data.size();
    unsigned int numForEstimate = paramEstimator.numForEstimate();
    //there are less data objects than the minimum required for an exact fit, or
    //all the data is outliers?
    if(numDataObjects < numForEstimate || maximalOutlierPercentage>=1.0) 
        return std::vector<const T*>();

    std::vector<const T *> exactEstimateData;
    std::vector<const T *> leastSquaresEstimateData;
    S exactEstimateParameters;
    int i, j, k, l, numVotesForBest, numVotesForCur, maxIndex, numTries;
    short *bestVotes = new short[numDataObjects]; //one if data[i] agrees with the best model, otherwise zero
    short *curVotes = new short[numDataObjects];  //one if data[i] agrees with the current model, otherwise zero
    short *notChosen = new short[numDataObjects]; //not zero if data[i] is NOT chosen for computing the exact fit, otherwise zero
    SubSetIndexComparator subSetIndexComparator(numForEstimate);
    std::set<int *, SubSetIndexComparator > chosenSubSets(subSetIndexComparator);
    int *curSubSetIndexes;
    double outlierPercentage = maximalOutlierPercentage;
    double numerator = log(1.0-desiredProbabilityForNoOutliers);
    double denominator = log(1- pow((double)(1.0-maximalOutlierPercentage), (double)(numForEstimate)));
    int allTries = choose(numDataObjects,numForEstimate);

    //parameters.clear();


    numVotesForBest = 0; //initalize with 0 so that the first computation which gives any type of fit will be set to best
    
    // intialize random generator
    boost::mt19937 rng;
    // start with a different seed every time.
    rng.seed(static_cast<unsigned int>(std::time(0)));
    // randomly sample points.
    maxIndex = numDataObjects-1;
    boost::uniform_int<> distribIndex(0, maxIndex);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
                             randIndex(rng, distribIndex);  // glues randomness with mapping

//    srand((unsigned)time(NULL)); //seed random number generator
    numTries = (int)(numerator/denominator + 0.5);

    //there are cases when the probablistic number of tries is greater than all possible sub-sets
    numTries = numTries<allTries ? numTries : allTries;

    for(i=0; i<numTries; i++) {
        //randomly select data for exact model fit ('numForEstimate' objects).
        memset(notChosen,'1',numDataObjects*sizeof(short));
        curSubSetIndexes = new int[numForEstimate];

        exactEstimateData.clear();

        maxIndex = numDataObjects-1; 
        for(l=0; l<(int)numForEstimate; l++) {
            //selectedIndex is in [0,maxIndex]
            unsigned int selectedIndex = randIndex();
//            unsigned int selectedIndex = (unsigned int)(((float)rand()/(float)RAND_MAX)*maxIndex + 0.5);
            for(j=-1,k=0; k<(int)numDataObjects && j<(int)selectedIndex; k++) {
                if(notChosen[k])
                    j++;
            }
            k--;
            exactEstimateData.push_back(&(data[k]));
            notChosen[k] = 0;
            maxIndex--;
        }
        //get the indexes of the chosen objects so we can check that this sub-set hasn't been
        //chosen already
        for(l=0, j=0; j<(int)numDataObjects; j++) {
            if(!notChosen[j]) {
                curSubSetIndexes[l] = j+1;
                l++;
            }
        }

        //check that the sub set just chosen is unique
        std::pair< std::set<int *, SubSetIndexComparator >::iterator, bool > res = chosenSubSets.insert(curSubSetIndexes);

        if(res.second == true) { //first time we chose this sub set
		                 //use the selected data for an exact model parameter fit
            if (!paramEstimator.estimate(exactEstimateData,exactEstimateParameters))
                //selected data is a singular configuration (e.g. three colinear points for 
                //a circle fit)
                continue;
            //see how many agree on this estimate
            numVotesForCur = 0;
            memset(curVotes,'\0',numDataObjects*sizeof(short));
            for(j=0; j<(int)numDataObjects; j++) {
                if(paramEstimator.agree(exactEstimateParameters, data[j])) {
                    curVotes[j] = 1;
                    numVotesForCur++;
                }
            }
	    // debug output
	    #ifdef DEBUG_RANSAC
	    std::cerr << "RANSAC iter " << i << ": inliers: " << numVotesForCur << " parameters:";
	    for (int jj=0; jj < exactEstimateParameters.size(); jj++)
		std::cerr << " " << exactEstimateParameters[jj];
	    std::cerr << std::endl;
	    #endif

            if(numVotesForCur > numVotesForBest) {
                numVotesForBest = numVotesForCur;
                memcpy(bestVotes,curVotes, numDataObjects*sizeof(short));
		parameters = exactEstimateParameters;
            }
	    /*
            //update the estimate of outliers and the number of iterations we need
            outlierPercentage = 1 - (double)numVotesForCur/(double)numDataObjects;
            if(outlierPercentage < maximalOutlierPercentage) {
                maximalOutlierPercentage = outlierPercentage;
                denominator = log(1- pow((double)(1.0-maximalOutlierPercentage), (double)(numForEstimate)));
                numTries = (int)(numerator/denominator + 0.5);
                //there are cases when the probablistic number of tries is greater than all possible sub-sets
                numTries = numTries<allTries ? numTries : allTries;
            }
	    */
        }
        else {  //this sub set already appeared, don't count this iteration
            delete [] curSubSetIndexes;
            i--;
        }
    }

    //release the memory
    std::set<int *, SubSetIndexComparator >::iterator it = chosenSubSets.begin();
    std::set<int *, SubSetIndexComparator >::iterator chosenSubSetsEnd = chosenSubSets.end();
    while(it!=chosenSubSetsEnd) {
	delete [] (*it);
        it++;
    }
    chosenSubSets.clear();

    //compute the least squares estimate using the largest sub set
    if(numVotesForBest > 0) {
        for(j=0; j<(int)numDataObjects; j++) {
            if(bestVotes[j]) {
                leastSquaresEstimateData.push_back(&(data[j]));
		inliers.push_back(j);
	    }
        }
        paramEstimator.leastSquaresEstimate(leastSquaresEstimateData,parameters);
    }
    delete [] bestVotes;
    delete [] curVotes;
    delete [] notChosen;

    return leastSquaresEstimateData;
}
/*****************************************************************************/
template<class Estimator, class S, class T>
std::vector<const T*> Ransac::compute(S &parameters,
                    const Estimator & paramEstimator,
                    const std::vector<T> &data)
{
    unsigned int numForEstimate = paramEstimator.numForEstimate();
    std::vector<T *> leastSquaresEstimateData;
    int numDataObjects = data.size();
    int numVotesForBest = 0;
    int *arr = new int[numForEstimate];
    short *curVotes = new short[numDataObjects];  //one if data[i] agrees with the current model, otherwise zero
    short *bestVotes = new short[numDataObjects];  //one if data[i] agrees with the best model, otherwise zero
	
    //parameters.clear();

    //there are less data objects than the minimum required for an exact fit
    if(numDataObjects < numForEstimate) 
        return 0;

    computeAllChoices(paramEstimator,data,numForEstimate,
                      bestVotes, curVotes, numVotesForBest, 0, data.size(), numForEstimate, 0, arr);

    //compute the least squares estimate using the largest sub set
    if(numVotesForBest > 0) {
        for(int j=0; j<numDataObjects; j++) {
            if(bestVotes[j])
                leastSquaresEstimateData.push_back(&(data[j]));
        }
        paramEstimator.leastSquaresEstimate(leastSquaresEstimateData,parameters);
    }

    delete [] arr;
    delete [] bestVotes;
    delete [] curVotes;	

    return leastSquaresEstimateData;
}
/*****************************************************************************/
template<class Estimator, class T>
void Ransac::computeAllChoices(const Estimator &paramEstimator, const std::vector<T> &data,
                                int numForEstimate,short *bestVotes, short *curVotes,
                                int &numVotesForBest, int startIndex, int n, int k,
                                int arrIndex, int *arr)
{
	              //we have a new choice of indexes
  if(k==0) {
		estimate(paramEstimator, data, numForEstimate, bestVotes, curVotes, numVotesForBest, arr);
    return;
  }

	       //continue to recursivly generate the choice of indexes
  int endIndex = n-k;
  for(int i=startIndex; i<=endIndex; i++) {
    arr[arrIndex] = i;
    computeAllChoices(paramEstimator, data, numForEstimate, bestVotes, curVotes, numVotesForBest,
			                i+1, n, k-1, arrIndex+1, arr);
  }

}
/*****************************************************************************/
template<class Estimator, class T, class S>
void Ransac::estimate(const Estimator & paramEstimator, const std::vector<T> &data,
                      int numForEstimate, short *bestVotes, short *curVotes,
                      int &numVotesForBest, int *arr)
{
	std::vector<T *> exactEstimateData;
	std::vector<S> exactEstimateParameters;
	int numDataObjects;
	int numVotesForCur;//initalize with -1 so that the first computation will be set to best
	int j;

	numDataObjects = data.size();
	memset(curVotes,'\0',numDataObjects*sizeof(short));
	numVotesForCur=0;

	for(j=0; j<numForEstimate; j++)
		exactEstimateData.push_back(&(data[arr[j]]));
	paramEstimator.estimate(exactEstimateData,exactEstimateParameters);
	                     //singular data configuration
	if(exactEstimateParameters.size()==0)
		return;

	for(j=0; j<numDataObjects; j++) {
		if(paramEstimator.agree(exactEstimateParameters, data[j])) {
			curVotes[j] = 1;
			numVotesForCur++;
		}
	}
	if(numVotesForCur > numVotesForBest) {
		numVotesForBest = numVotesForCur;
		memcpy(bestVotes,curVotes, numDataObjects*sizeof(short));
	}
}
/*****************************************************************************/
inline unsigned int Ransac::choose(unsigned int n, unsigned int m)
{
	unsigned int denominatorEnd, numeratorStart, numerator,denominator; 
	if((n-m) > m) {
		numeratorStart = n-m+1;
		denominatorEnd = m;
	}
	else {
		numeratorStart = m+1;
		denominatorEnd = n-m;
	}
	
	unsigned int i;
	for(i=numeratorStart, numerator=1; i<=n; i++)
		numerator*=i;
	for(i=1, denominator=1; i<=denominatorEnd; i++)
		denominator*=i;
	return numerator/denominator;

}


#endif //_RANSAC_H_
