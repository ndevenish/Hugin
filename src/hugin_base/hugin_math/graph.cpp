// -*- c-basic-offset: 4 -*-

/** @file graph.cpp
 *
 *  @brief graph utilities
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "graph.h"


using namespace hugin_utils;


/** find subgraphs (separate panoramas)
 *
 *  Actually, we could just use the BOOST graph library instead
 *  of hacking our own graph functions.
 */
void hugin_utils::findSubGraphs(AdjListGraph & graph,
                          std::vector<int> & subgraphStart)
{
    int nImages = (int) graph.size();
    // nodes that have not been visited
    std::set<int> unseen;
    // fill with all images
    for (int i=0; i<nImages; i++) unseen.insert(i);

    while(unseen.size() != 0 ) {
        // first remaining node determines the next graph
        int root = (int) *(unseen.begin());
        subgraphStart.push_back(root);
        // visit all nodes of this subgraph
        RemoveVisitor rmv(unseen);
        traverseVertices(graph, root, rmv);
    }
}
