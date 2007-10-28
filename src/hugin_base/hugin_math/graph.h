// -*- c-basic-offset: 4 -*-
/** @file graph.h
 *
 *  Some graph functions needed.. Should be replaced with bgl,
 *  the graph library from BOOST
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _HUGIN_MATH_GRAPH_H
#define _HUGIN_MATH_GRAPH_H

#include <vector>
#include <set>
#include <queue>

#include <hugin_utils/stl_utils.h>


namespace hugin_utils
{


    // some typedefs for graphs represented with stl stuff.
    typedef std::vector<int> AdjList;
    typedef std::vector<AdjList> AdjListGraph;

    /** find subgraphs
     *
     *  Actually, we could just use the BOOST graph library instead
     *  of hacking our own graph functions.
     */
    void findSubGraphs(AdjListGraph & graph,
                       std::vector<int> & subgraphStart);

        
    struct GraphEdge
    {
        GraphEdge(int a, int b)
            {
                // insert in order to be able to compare easily
                if (a < b) {
                    n1 = a;
                    n2 = b;
                } else {
                    n1 = b;
                    n2 = a;
                }
            };
        bool operator==(const GraphEdge & o) const
            { return ( n1 == o.n1 && n2 == o.n2); }
        bool operator<(const GraphEdge & o) const
            {
                return n1 < o.n1 || (!(o.n1 < n1) && n2 < o.n2);
            }
        int n1;
        int n2;
    };
        
    /** traverse a graph, and run visitor on every vertice - vertice edge
     *  encounterd
     */
    template<class FUNCTOR>
    void traverseEdges(const AdjListGraph & graph,
                       int startNode,
                       FUNCTOR & visitor)
    {
        DEBUG_DEBUG("start: " << startNode);
        // keep track of visited nodes
        std::set<GraphEdge>visited;

        // queue of nodes that need to be visited
        std::queue<int> nextNodes;
        nextNodes.push(startNode);
        // as long as we can traverse further
        while(nextNodes.size() != 0) {
            // current node
            int cNode = nextNodes.front();
            nextNodes.pop();
            DEBUG_DEBUG("current node: " << cNode);
            // visit neighbours
            for (AdjList::const_iterator it = graph[cNode].begin();
                 it != graph[cNode].end(); ++it)
            {
                GraphEdge e(cNode,*it);
                if (! set_contains(visited, e)) {
                    // we have found edge that hasn't been visited
                    DEBUG_DEBUG("visiting link" << cNode << " to " << *it);
                    visited.insert(e);
                    visitor(cNode,*it);
                    // examine neighbours, add to the queue.
                    nextNodes.push(*it);
                } else {
                    DEBUG_DEBUG("link already visited: " << cNode << " to " << *it);
                }
            }
        }
    }

        
    /** traverse graph vertices
     */
    template<class FUNCTOR>
    void traverseVertices(const AdjListGraph & graph,
                                int start,
                                FUNCTOR & visitor)
    {
        // keep track of visited nodes
        std::set<int>visited;

        // queue of nodes that need to be visited
        std::queue<int> nextNodes;
        nextNodes.push(start);
        // as long as we can traverse further
        while(nextNodes.size() != 0) {
            // current node
            int cNode = nextNodes.front();
            nextNodes.pop();
            // we have visited this node
            visitor(cNode);
            visited.insert(cNode);
            // visit neighbours
            for (AdjList::const_iterator it = graph[cNode].begin();
                 it != graph[cNode].end(); ++it)
            {
              if (! set_contains(visited, *it)) {
                    // examine neighbours, add to the subgraph and queue.
                    nextNodes.push(*it);
                }
            }
        }
    }

    /** removes vertices from the set */
    struct RemoveVisitor
    {
        RemoveVisitor(std::set<int> & vertices)
            : m_vert(vertices)
            { };
        void operator()(int vert1)
            {
                m_vert.erase(vert1);
            }
        std::set<int> & m_vert;
    };


    /** remember/track all visited vertices */
    struct TrackVisitor
    {
        void operator()(int vert1)
            {
                m_vert.insert(vert1);
            }
        std::set<int> &  getTraversed()
            {
                return m_vert;
            }
        std::set<int> m_vert;
    };


} // namespace



#endif // _H
