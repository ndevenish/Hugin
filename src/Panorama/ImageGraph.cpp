// -*- c-basic-offset: 4 -*-

/** @file ImageGraph.cpp
 *
 *  @brief implementation of ImageGraph Class
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

#include <PT/ImageGraph.h>


using namespace PT;
using namespace boost;

typedef property_map<CPGraph, vertex_index_t>::type CPGraphIndexMap;
void PT::createCPGraph(Panorama & pano, CPGraph & graph)
{
    // clear old graph
    graph.clear();

    // add all verticies to the graph
    int nImg = pano.getNrOfImages();
    for (int i = 0; i < nImg; i++) {
        add_vertex(graph);
    }

    // insert all control points into the graph
    const CPVector & cps = pano.getCtrlPoints();
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it) {
        // probably very inefficient
        graph_traits<CPGraph>::adjacency_iterator ai;
        graph_traits<CPGraph>::adjacency_iterator ai_end;

        CPGraphIndexMap index = get(vertex_index, graph);
        bool found=false;
        for (tie(ai, ai_end) = adjacent_vertices(it->image1Nr, graph);
             ai != ai_end; ++ai)
        {
            if (index[*ai] == it->image2Nr) found = true;
        }
        if (!found) {
            add_edge(it->image1Nr, it->image2Nr,graph);
        }

    }
}
