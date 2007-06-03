// -*- c-basic-offset: 4 -*-
/** @file ImageGraph.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: ImageGraph.h 1763 2006-12-17 21:11:57Z dangelo $
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

#ifndef _IMAGEGRAPH_H
#define _IMAGEGRAPH_H

#include <PT/Panorama.h>

#ifdef __APPLE__
// In the case boost got error with macro "check()", uncomment following two lines.
#include <AssertMacros.h>
#undef check
// Ref. http://lists.boost.org/MailArchives/boost-users/msg06726.php
#endif

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
namespace PT
{

/** graph of images, connected with control points.
 *
 *  verticies: images, links: controlpoints
 *
 */
typedef boost::adjacency_list<boost::vecS, boost::vecS,
                              boost::undirectedS,
                              boost::property<boost::vertex_color_t, boost::default_color_type> > CPGraph;

/** components in a control point graph */
typedef std::vector< std::set<unsigned> > CPComponents;

/** create a control point graph structure, with links representing one or
 *  more control points
 *
 */
void createCPGraph(const Panorama & pano, CPGraph & graph);

int findCPComponents(const CPGraph & graph, 
                     CPComponents & comp);

typedef boost::property<boost::edge_weight_t, float> OverlapEdgeProperty;

/** A graph that contains all image as nodes (vertexes), overlaps are
 *  given by edges.
 *
 *  vertex property (color): indicates if the image has been stitched
 *  into the panorama
 *
 *  edge property (float): amount of overlap
 */
typedef boost::adjacency_list<boost::vecS, boost::vecS,
                              boost::undirectedS,
                              boost::property<boost::vertex_color_t, boost::default_color_type>,
                              OverlapEdgeProperty> OverlapGraph;

/** create a graph with all overlaps, and a suitable blend order.
 *
 *
 */
void createOverlapGraph(const Panorama & pano, OverlapGraph & graph);


} // namespace

#endif // _IMAGEGRAPH_H
