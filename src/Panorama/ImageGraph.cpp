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
#include <PT/ImageTransforms.h>
#include <vigra_ext/ROI.h>

using namespace PT;
using namespace boost;
using namespace vigra;
using namespace vigra_ext;
using namespace std;

typedef property_map<CPGraph, vertex_index_t>::type CPGraphIndexMap;

void PT::createCPGraph(const Panorama & pano, CPGraph & graph)
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


typedef property_map<OverlapGraph, vertex_index_t>::type OverlayGraphIndexMap;


/** count pixels that are > 0 in both images */
struct OverlapSizeCounter
{
    OverlapSizeCounter()
	: count(0)
    { }
    
    template<typename PIXEL>
    void operator()(PIXEL const & img1, PIXEL const & img2)
    {
	if (img1 > 0 && img2 > 0) {
	    count++;
	}
    }

    int getCount()
    { 
	return count; 
    }

    int count;
};

/* not needed so far, probably still buggy

void PT::createOverlapGraph(const Panorama & pano, OverlapGraph & graph)
{
    // clear old graph
    graph.clear();

    // add all verticies to the graph
    unsigned int nImg = pano.getNrOfImages();
    for (unsigned int i = 0; i < nImg; i++) {
        add_vertex(graph);
    }

    PanoramaOptions opts = pano.getOptions();
    // small area, for alpha mask overlap analysis.
    opts.width = 500;
    // find intersecting regions, on a small version of the panorama.
    std::vector< PT::RemappedPanoImage<vigra::BRGBImage, vigra::BImage> > rimg(nImg, PT::RemappedPanoImage<vigra::BRGBImage, vigra::BImage>(pano) );

    for (unsigned int imgNr = 0; imgNr < nImg ; imgNr++) {
	// calculate alpha channel
	rimg[imgNr].setPanoImage(imgNr, opts);
	rimg[imgNr].calcAlpha();
    }

    ROI<Diff2D> overlap;
    // intersect ROI's & masks of all images
    for (unsigned int i1 = 0; i1 < nImg ; i1++) {
	for (unsigned int i2 = i1; i2 < nImg ; i2++) {
	    if ( rimg[i1].getROI().intersect(rimg[i2].getROI(), overlap))
	    {
		OverlapSizeCounter counter;
		inspectTwoImages(overlap.apply(rimg[i1].getAlpha(),
					       rimg[i1].getROI()),
				 overlap.apply(make_pair(rimg[i2].getAlpha().first, rimg[i2].getAlpha().third),
					       rimg[i2].getROI()),
				 counter);
		if (counter.getCount() > 0) {
		    OverlapGraph::Edge e = add_edge(i1, i2, graph);
		    // todo: save number of overlapping pixels.
		    property_map<OverlapGraph, edge_weight_t>::type w
			= get(edge_weight, g);
		    put(w, e, counter.getCount());
		}
	    }
        }
    }
}

*/
