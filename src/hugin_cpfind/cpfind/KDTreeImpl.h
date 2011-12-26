/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "KDTree.h"
#include <limits>
#include <list>
#include <vector>

namespace KDTreeSpace
{

/******************************************************************************
*
*	KDTree()
*
*	Constructor for the tree, use this one to build the tree
*
******************************************************************************/

template <class KE, class VTYPE>
KDTree<KE, VTYPE>::KDTree(const ItemVector_t& iElemsList, int iDimensions) :
    _pivot(NULL), _leftKD(NULL), _rightKD(NULL), _dims(iDimensions)
{
    // disallow to build a tree with 0 elements.
    if (!iElemsList.size())
    {
        return;
    }

    // construct a vector with pointers to the elems lists, and run init on it.
    ItemPtrVector_t aElemsPtrList;
    ItemVectorIt_t aElemsIt = iElemsList.begin();
    ItemVectorIt_t aElemsItEnd = iElemsList.end();
    for(; aElemsIt != aElemsItEnd; ++aElemsIt)
    {
        const KE& aTmp = *aElemsIt;
        aElemsPtrList.push_back(&aTmp);
    }
    init(aElemsPtrList);
}

/******************************************************************************
*
*	KDTree()
*
*	Constructor for the sub KDTrees
*
******************************************************************************/

template <class KE, class VTYPE>
KDTree<KE, VTYPE>::KDTree(const ItemPtrVector_t& iElemsPtrList, int iDimensions) :
    _pivot(NULL), _leftKD(NULL), _rightKD(NULL), _dims(iDimensions)
{
    init(iElemsPtrList);
}

/******************************************************************************
*
*	~KDTree()
*
*	Destructor
*
******************************************************************************/

template <class KE, class VTYPE>
KDTree<KE, VTYPE>::~KDTree()
{
    delete(_leftKD);
    delete(_rightKD);
}

/******************************************************************************
*
*	init()
*
*	Build the local KDTree and recurse to create childs
*
******************************************************************************/

template <class KE, class VTYPE>
void	KDTree<KE, VTYPE>::init(const ItemPtrVector_t& iElemsPtrList)
{
    // choose the pivot element
    typename std::vector<const KE*>::const_iterator aPivotIt = choosePivot(iElemsPtrList);
    _pivot = *aPivotIt;

    // create the left/right vectors.
    ItemPtrVector_t aLeftElems, aRightElems;

    // split the elements set according to the split dimension
    ItemPtrVectorIt_t aElemsIt = iElemsPtrList.begin();
    ItemPtrVectorIt_t aElemsItEnd = iElemsPtrList.end();
    VTYPE aPivotDimValue = _pivot->getVectorElem(_splitDim);
    for(; aElemsIt != aElemsItEnd; ++aElemsIt)
    {
        // skip the pivot element.
        if (aElemsIt == aPivotIt)
        {
            continue;
        }

        if ((*aElemsIt)->getVectorElem(_splitDim) <= aPivotDimValue)
        {
            aLeftElems.push_back(*aElemsIt);
        }
        else
        {
            aRightElems.push_back(*aElemsIt);
        }
    }

    //int aLeftSize = (int)aLeftElems.size();
    //int aRightSize = (int)aRightElems.size();

    // recursively create the sub-KDTrees
    if (aLeftElems.size())
    {
        _leftKD = new KDTree<KE, VTYPE> (aLeftElems, _dims);
    }
    else
    {
        _leftKD = NULL;
    }

    if (aRightElems.size())
    {
        _rightKD = new KDTree<KE, VTYPE> (aRightElems, _dims);
    }
    else
    {
        _rightKD = NULL;
    }

}

/******************************************************************************
*
*	choosePivot()
*
*	Will choose a splitting dimension for the current tree and get closest
*	point
*
******************************************************************************/

template <class KE, class VTYPE>
typename std::vector<const KE*>::const_iterator KDTree<KE, VTYPE>::choosePivot(const ItemPtrVector_t& iElemsPtrList)
{
    // search for minimum and maximum in each dimension
    std::vector<VTYPE> aMinVals(_dims, std::numeric_limits<VTYPE>::max());
    std::vector<VTYPE> aMaxVals(_dims,  - std::numeric_limits<VTYPE>::max());

    ItemPtrVectorIt_t aElemsIt = iElemsPtrList.begin();
    ItemPtrVectorIt_t aElemsEnd = iElemsPtrList.end();
    for(; aElemsIt != aElemsEnd; ++aElemsIt)
    {
        for (int aDim = 0; aDim < _dims; ++aDim)
        {
            VTYPE& aCurValue = (*aElemsIt)->getVectorElem(aDim);
            if (aCurValue < aMinVals[aDim])
            {
                aMinVals[aDim] = aCurValue;
            }
            if (aCurValue > aMaxVals[aDim])
            {
                aMaxVals[aDim] = aCurValue;
            }
        }
    }

    // look for the dimension that has the largest range of values
    _splitDim = 0;
    VTYPE aLargestRangeValue = - std::numeric_limits<VTYPE>::max();
    for (int aDim = 0; aDim < _dims; ++aDim)
    {
        if ( (aMaxVals[aDim] - aMinVals[aDim]) > aLargestRangeValue )
        {
            _splitDim = aDim;
            aLargestRangeValue = aMaxVals[aDim] - aMinVals[aDim];
        }
    }

    // compute the median value in the chosen dimension
    VTYPE aMedian = aLargestRangeValue / 2 + aMinVals[_splitDim];

    // look for the element closest to the median in the chosen dimension
    typename std::vector<const KE*>::const_iterator aClosestElemIt;
    VTYPE aClosestDiff = std::numeric_limits<VTYPE>::max();
    for(aElemsIt = iElemsPtrList.begin(); aElemsIt != aElemsEnd; ++aElemsIt)
    {
        VTYPE aCurValue = fabs((*aElemsIt)->getVectorElem(_splitDim) - aMedian);
        if (aCurValue < aClosestDiff)
        {
            aClosestDiff = aCurValue;
            aClosestElemIt = aElemsIt;
        }
    }

    return aClosestElemIt;
}

/******************************************************************************
*
*	CalcSqDist()
*
*	calc the square distance between 2 entries
*
******************************************************************************/

template <class KE, class VTYPE>
double KDTree<KE, VTYPE>::calcSqDist(const KE* i1, const KE* i2)
{
    double aDist = 0.0;
    for (int n = 0 ; n < _dims ; ++n)
    {
        double aDiff = i1->getVectorElem(n) - i2->getVectorElem(n);
        aDist += aDiff * aDiff;
    }
    return (aDist);
}


template <class KE, class VTYPE>
std::set<BestMatch<KE>, std::greater<BestMatch<KE> > > KDTree<KE, VTYPE>::getNearestNeighboursBBF(const KE& iTarget, int iNbBestMatches, int iNbSearchSteps)
{
    // create a hyper rectangle for whole space
    HyperRectangle<KE, VTYPE> aHR(_dims);

    // create a limited set to hold best matches.
    BestMatchLimitedSet_t aBestMatches(iNbBestMatches);

    // create a limited set to hold the search queue
    std::list<QueueEntry<KE, VTYPE> > aSearchQueue;

    // recursively process the search
    recurseNearestNeighboursBBF(iTarget, aHR, aBestMatches, aSearchQueue, iNbSearchSteps);

    return aBestMatches.getSet();
}

template <class KE, class VTYPE>
void KDTree<KE, VTYPE>::recurseNearestNeighboursBBF(const KE& iTarget,
        HyperRectangle<KE, VTYPE> & iHR,
        BestMatchLimitedSet_t& ioBestMatches,
        QueueEntryList_t& ioSearchQueue,
        int& ioRemainingUnqueues)
{
    // add the node entry to the bestmatches set
    ioBestMatches.insert(BestMatch<KE>(_pivot, calcSqDist(&iTarget, _pivot)));

    //std::set<BestMatch<KE>, std::greater<BestMatch<KE> > >::iterator aBEB;
    //for (aBEB = ioBestMatches.begin(); aBEB!= ioBestMatches.end(); ++aBEB)
    //	cout << "Best Match " << aBEB->_distance << endl;

    // split the space by a plane perpendicular to the current node dimension
    // and going through the node point.
    HyperRectangle<KE, VTYPE> aLeftHR(iHR);
    HyperRectangle<KE, VTYPE> aRightHR(iHR);
    //cout << "split dim :" << _splitDim << " " << _pivot->getVectorElem(_splitDim) << endl;
    //cout << "iHR" << endl;
    //iHR.display();
    if (!iHR.split(aLeftHR, aRightHR, _splitDim, _pivot->getVectorElem(_splitDim)))
    {
        return;
    }
    //cout << "leftHR" << endl;
    //aLeftHR.display();
    //cout << "rightHR" << endl;
    //aRightHR.display();



    // Create some pointers and refs to the nearer and further HyperRectangles and KDTrees.
    KDTree<KE, VTYPE> * aNearKD = _leftKD;
    KDTree<KE, VTYPE> * aFarKD = _rightKD;
    HyperRectangle<KE, VTYPE> & aNearHR = aLeftHR;
    HyperRectangle<KE, VTYPE> & aFarHR = aRightHR;

    // Determine which HR and sub-KD tree is near/far
    // by comparing the Target and pivot values at node dimension
    if (iTarget.getVectorElem(_splitDim) > _pivot->getVectorElem(_splitDim))
    {
        aNearKD = _rightKD;
        aFarKD = _leftKD;
        aNearHR = aRightHR;
        aFarHR = aLeftHR;
    }

    //std::cout << "Near HR, target is in " << aNearHR.isTargetIn(iTarget) << endl;
    //std::cout << "Far HR, target is in " << aFarHR.isTargetIn(iTarget) << endl;


    // add the far HyperRectangle to the queue. The queue is sorted by
    // the distance from target point to the hyperrectangle :
    // "The distance to a bin is defined to be the minimum distance between q and any
    //	point on the bin boundary."
    if (aFarKD)
    {
        ioSearchQueue.push_back(QueueEntry<KE, VTYPE>(aFarHR, aFarKD, aFarHR.calcSqDistance(iTarget)));
    }

    //list<QueueEntry<KE> >::iterator aSQ;
    //for (aSQ = ioSearchQueue.begin(); aSQ!= ioSearchQueue.end(); ++aSQ)
    //	cout << "SQ " << aSQ->_dist << endl;

    // go direcly in depth until a leaf is found.
    if (aNearKD)
    {
        //cout << "Going in depth" << endl;
        aNearKD->recurseNearestNeighboursBBF(iTarget, aNearHR, ioBestMatches, ioSearchQueue, ioRemainingUnqueues);
    }
    // else process the queue
    else if (ioRemainingUnqueues > 0 && ioBestMatches.size() && ioSearchQueue.size())
    {
        //cout << "Going in queue" << endl;

        // decrease the number of 'queue processings'
        ioRemainingUnqueues--;

        // get the (squared) distance of the worse 'best match'. this is the first one in the best list.
        double aHyperSphereRadius = ioBestMatches.begin()->_distance;

        // get the first element out of the queue, then remove it
        typename std::list<QueueEntry<KE, VTYPE> >::iterator aSQ, aSmallestIt;
        double aSmallestDist = std::numeric_limits<double>::max();
        for (aSQ = ioSearchQueue.begin(); aSQ!= ioSearchQueue.end(); ++aSQ)
        {
            if (aSQ->_dist < aSmallestDist)
            {
                aSmallestDist = aSQ->_dist;
                aSmallestIt = aSQ;
            }
        }
        QueueEntry<KE, VTYPE> aQueueElem = *(aSmallestIt);
        ioSearchQueue.erase(aSmallestIt);
        //QueueEntry<KE> aQueueElem = *(ioSearchQueue.begin());
        //ioSearchQueue.erase(ioSearchQueue.begin());

        // if the hypersphere and hyper rectangle intersect, there is maybe a better match in the intersection
        if (aQueueElem._HR.hasHyperSphereIntersect(iTarget, aHyperSphereRadius))
        {
            aQueueElem._kdTree->recurseNearestNeighboursBBF(iTarget, aQueueElem._HR, ioBestMatches, ioSearchQueue, ioRemainingUnqueues);
        }
        else
        {
            //cout << "No intersect" << endl;
        }
    }
}




// default constructor
template <class KE, class TYPE>
HyperRectangle<KE, TYPE>::HyperRectangle() :
    _dim(0)
{

}


// constructor
template <class KE, class TYPE>
HyperRectangle<KE, TYPE>::HyperRectangle(int iDim) :
    _leftTop(std::vector<TYPE>(iDim, -std::numeric_limits<TYPE>::max())),
    _rightBottom(std::vector<TYPE>(iDim, std::numeric_limits<TYPE>::max())),
    _dim(iDim)
{

}

// copy constructor
template <class KE, class TYPE>
HyperRectangle<KE, TYPE>::HyperRectangle(HyperRectangle& iOther) :
    _leftTop(std::vector<TYPE>(iOther._leftTop)),
    _rightBottom(std::vector<TYPE>(iOther._rightBottom)),
    _dim(iOther._dim)
{

}




// split into smaller HyperRectangle
template <class KE, class TYPE>
bool HyperRectangle<KE, TYPE>::split(HyperRectangle& oLeft, HyperRectangle& oRight, int iSplitDim, TYPE iSplitVal)
{
    // check if the split value is in the range for the given dimension
    if (_leftTop[iSplitDim] >= iSplitVal || _rightBottom[iSplitDim] < iSplitVal)
    {
        return false;
    }

    // make copies of the current vector to destination bin
    //oLeft._leftTop = _leftTop;
    //oRight._leftTop = _leftTop;
    //oLeft._rightBottom = _rightBottom;
    //oRight._rightBottom = _rightBottom;
    //oLeft._dim = _dim;
    //oRight._dim = _dim;

    // split
    oLeft._rightBottom[iSplitDim] = iSplitVal;
    oRight._leftTop[iSplitDim] = iSplitVal;

    return true;
}

template <class KE, class TYPE>
double HyperRectangle<KE, TYPE>::calcSqDistance (const KE& iTarget)
{
    double aSqDistance = 0;

    // compute the closest point within hr to the target.
    for (int n = 0 ; n < _dim ; ++n)
    {
        TYPE aTargetVal = iTarget.getVectorElem(n);
        TYPE aHRMin = _leftTop[n];
        TYPE aHRMax = _rightBottom[n];

        double aDimDist = aTargetVal;
        if (aTargetVal <= aHRMin)
        {
            aDimDist = aTargetVal - aHRMin;
        }
        else if (aTargetVal > aHRMin && aTargetVal < aHRMax)
        {
            aDimDist = 0;
        }
        else if (aTargetVal >= aHRMax)
        {
            aDimDist = aTargetVal - aHRMax;
        }
        aSqDistance += aDimDist * aDimDist;
    }

    return aSqDistance;
}

template <class KE, class TYPE>
bool HyperRectangle<KE, TYPE>::hasHyperSphereIntersect(const KE& iTarget, double iSqDistance)
{
    // avoid calculating the square root by using basic math.
    //

    double aDist = calcSqDistance(iTarget);
    //if (aDist > 1.0 && iSqDistance)

    //cout << "hypersphereintersect " << sqrt(aDist) << " " << sqrt(iSqDistance) << endl;

    return (aDist < iSqDistance);
}

template <class KE, class TYPE>
void HyperRectangle<KE, TYPE>::display()
{
    for (int n = 0 ; n < _dim ; ++n)
    {
        if (_leftTop[n] > -std::numeric_limits<TYPE>::max()
                || _rightBottom[n] < std::numeric_limits<TYPE>::max())
        {
            std::cout << "dim[" << n << "] = {" << _leftTop[n] << " , " <<  _rightBottom[n] << "}" << std::endl;
        }
    }
    std::cout << std::endl;
}

template <class KE, class TYPE>
bool HyperRectangle<KE, TYPE>::isTargetIn (const KE& iTarget)
{
    if (iTarget.getVectorSize() != _dim)
    {
        std::cout << "is target in dimension mismatch" << std::endl;
    }

    for (int n = 0 ; n < _dim ; ++n)
    {
        TYPE aDimVal = iTarget.getVectorElem(n);
        if (aDimVal <= _leftTop[n] || aDimVal >= _rightBottom[n])
        {
            return (false);
        }
    }

    return true;
}

template <class KE, class VTYPE>
bool operator < (const QueueEntry<KE, VTYPE> & iA, const QueueEntry<KE, VTYPE> & iB)
{
    return (iA._dist < iB._dist);
}

template <class KE>
bool operator > (const BestMatch<KE> & iA, const BestMatch<KE> & iB)
{
    return (iA._distance > iB._distance);
}

} //namespace KDTree



