// -*- c-basic-offset: 4 -*-
/** @file ImageGraph.h
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

#ifndef _IMAGEGRAPH_H
#define _IMAGEGRAPH_H

#include <PT/Panorama.h>
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

/** create a control point graph structure, with links representing one or
 *  more control points
 *
 */
void createCPGraph(Panorama & pano, CPGraph & graph);


} // namespace

#endif // _IMAGEGRAPH_H
