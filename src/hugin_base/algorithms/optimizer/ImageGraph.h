// -*- c-basic-offset: 4 -*-
/** @file hugin_base/algorithms/optimizer/ImageGraph.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>, T. Modes
 *
 */
/*  This is free software; you can redistribute it and/or
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

#ifndef _PANODATA_IMAGEGRAPH_H
#define _PANODATA_IMAGEGRAPH_H

#include <hugin_shared.h>

#include <panodata/PanoramaData.h>
#include <algorithms/basic/CalculateOverlap.h>

namespace HuginGraph
{

/** abstract base functor for breadth first search in ImageGraph */
class IMPEX BreadthFirstSearchVisitor
{
public:
    virtual void Visit(const size_t vertex, const HuginBase::UIntSet& visitedNeighbors, const HuginBase::UIntSet& unvisitedNeighbors) = 0;
};

/** class to work with images graphs created from a HuginBase::Panorama class
*  it creates a graph based on control points and linked images positions
*  and provides function to work with it */
class IMPEX ImageGraph
{
public:
    /** stores adjacency list for graph */
    typedef std::vector<HuginBase::UIntSet> GraphList;
    /** stores the components of the graph */
    typedef std::vector<HuginBase::UIntSet> Components;
    /** constructor, build internal representation of graph */
    ImageGraph(const HuginBase::PanoramaData& pano, bool ignoreLinkedPosition = false);
    /** constructor, build graph from overlap */
    ImageGraph(const HuginBase::CalculateImageOverlap& overlap);
    /** find all connected components
    *  @returns number of components
    *  if you want to know, if all images are connected
    *  use IsConnected() instead */
    Components GetComponents();
    /** check if all images are connected
    *  @returns true, if all images are connected, otherwise false
    *  it uses an optimized depth first search and breaks out if a
    *  a second components is found. If you need all components
    *  use GetComponents() instead */
    bool IsConnected();
    /** visit all images via a breadth first search algorithm
    *  for each visited images, the functor visitor is called with
    *  HuginBase::UIntSet which contains all neighbors in the graph
    *  @param forceAllComponents if true all images are visited, if false only the images
    *  connected with startImg are visited */
    void VisitAllImages(const size_t startImg, bool forceAllComponents, BreadthFirstSearchVisitor* visitor);
private:
    GraphList m_graph;
}; // class ImageGraph

} // namespace HuginGraph

#endif // _PANODATA_IMAGEGRAPH_H
