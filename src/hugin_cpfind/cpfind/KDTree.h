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
* <http://www.gnu.org/licenses/>.
*/

#ifndef __kdtree_h
#define __kdtree_h

#include <vector>
#include <list>
#include <functional>  // for std::greater

#include <localfeatures/BoundedSet.h>

// implementation of a kdtree is based on this book
// A. Moore, An introductory tutorial on kd-trees,
// tech. report Technical Report No. 209, Computer Laboratory,
// University of Cambridge, Robotics Institute, Carnegie Mellon University, 1991.

namespace KDTreeSpace
{

template <class KE>					class BestMatch;
template <class KE, class TYPE>		class HyperRectangle;
template <class KE, class VTYPE>	class QueueEntry;
template <class KE, class VTYPE>	class KDTree;

template <class VTYPE>
class KDTreeElemInterface
{
    virtual VTYPE& getVectorElem (int iPos) const = 0;  // access to the vector elements.
};

/******************************************************************************
* BestMatch
******************************************************************************/
template <class KE>
class BestMatch
{
public:
    BestMatch(const KE* iMatch, double iDistance) : _distance(iDistance), _match(iMatch) {}
    double	_distance;			// square distance from target
    const KE* 	_match;
};

/******************************************************************************
* HyperRectangle
******************************************************************************/
template <class KE, class TYPE>
class HyperRectangle
{
public:
    HyperRectangle();
    explicit HyperRectangle(int iDim);
    HyperRectangle(HyperRectangle& iOther);
    bool split(HyperRectangle& oLeft, HyperRectangle& oRight, int iSplitDim, TYPE iSplitVal);
    double calcSqDistance (const KE& iTarget);
    bool hasHyperSphereIntersect(const KE& iTarget, double iSqDistance);
    void display();
    bool isTargetIn (const KE& iTarget);

    int _dim;
    std::vector<TYPE> _leftTop, _rightBottom;
};

/******************************************************************************
* QueueEntry
******************************************************************************/
template <class KE, class VTYPE>
class QueueEntry
{
public:
    QueueEntry(HyperRectangle<KE, VTYPE> & iHR, KDTree<KE, VTYPE> * iKDTree, double iDistance) :
        _dist(iDistance), _HR(iHR), _kdTree(iKDTree) {}
    double _dist;					// the distance from target to HyperRectangle
    HyperRectangle<KE, VTYPE> &	_HR;	// reference to the hyperrectangle
    KDTree<KE, VTYPE> *	_kdTree;

    // operator < to be put in a regular set.
    //bool operator < (const QueueEntry<KE> & iOther) { return (_dist < iOther._dist); }

};

/******************************************************************************
* KDTree
******************************************************************************/
template <class KE, class VTYPE>
class KDTree
{
public:
    typedef typename std::vector<KE>										ItemVector_t;
    typedef typename std::vector<KE>::const_iterator						ItemVectorIt_t;
    typedef typename std::vector<const KE*>								ItemPtrVector_t;
    typedef typename std::vector<const KE*>::const_iterator				ItemPtrVectorIt_t;
    typedef typename std::set<BestMatch<KE>, std::greater<BestMatch<KE> > >	BestMatchSet_t;
    typedef lfeat::bounded_set<BestMatch<KE>, std::greater<BestMatch<KE> > >		BestMatchLimitedSet_t;
    typedef typename std::list<QueueEntry<KE, VTYPE> >						QueueEntryList_t;

    // constructor for the root.
    KDTree(const ItemVector_t& iElemsList, int iDimensions);

    // recursive construct.
    KDTree(const ItemPtrVector_t& iElemsPtrList, int iDimensions);

    // Destructor
    ~KDTree();

    BestMatchSet_t		getNearestNeighboursBBF(	const KE& iTarget,
            int iNbBestMatches,
            int iNbSearchSteps);

    // calc the square distance between 2 entries.
    double				calcSqDist(const KE* i1, const KE* i2);

private:
    void				init(const ItemPtrVector_t& iElemsPtrList);			// prepare the structure.
    ItemPtrVectorIt_t	choosePivot(const ItemPtrVector_t& iElemsPtrList);		// choose the pivot point
    void				recurseNearestNeighboursBBF(const KE& iTarget,
            HyperRectangle<KE, VTYPE> & iHR,
            BestMatchLimitedSet_t& ioBestMatches,
            QueueEntryList_t& ioSearchQueue,
            int& ioRemainingUnqueues);
    // dimension of the vectors
    int					_dims;

    // The element stored on this leaf
    const KE* 			_pivot;
    int					_splitDim;

    // The left and the right kd-subtree.
    KDTree<KE, VTYPE> *	 		_leftKD;
    KDTree<KE, VTYPE> *			_rightKD;

};

} // namespace KDTree

#endif
