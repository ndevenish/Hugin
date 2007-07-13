// -*- c-basic-offset: 4 -*-
/** @file graph.h
 *
 *  Some graph functions needed.. Should be replaced with bgl,
 *  the graph library from BOOST
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: graph.h 490 2004-01-18 02:07:02Z dangelo $
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

#ifndef _Hgn1_UTILS_GRAPH_H
#define _Hgn1_UTILS_GRAPH_H

#include <hugin_math/graph.h>


namespace utils
{


// some typedefs for graphs represented with stl stuff.
using hugin_utils::AdjList;
using hugin_utils::AdjListGraph;

/** find subgraphs
 *
 *  Actually, we could just use the BOOST graph library instead
 *  of hacking our own graph functions.
 */
using hugin_utils::findSubGraphs;
    
using hugin_utils::GraphEdge;


/** traverse a graph, and run visitor on every vertice - vertice edge
 *  encounterd
 */
using hugin_utils::traverseEdges;
    
/** traverse graph vertices
 */
using hugin_utils::traverseVertices;


/** removes vertices from the set */
using hugin_utils::RemoveVisitor;
    
/** remember/track all visited vertices */
using hugin_utils::TrackVisitor;
    
} // namespace



#endif // _GRAPH_H
