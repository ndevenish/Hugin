// -*- c-basic-offset: 4 -*-

/** @file panosifter.cpp
 *
 *  @brief program to sift images and create control points.
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

#include <config.h>
#include <fstream>
#include <sstream>

#include <stdlib.h>
#include <unistd.h>

#include "common/graph.h"
#include "vigra_ext/PointMatching.h"

#include <panoinc.h>


using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace PT;
using namespace utils;

static void usage(const char * name)
{
    cerr << name << ": create one or more .pto files, from multiple images."<< endl
         << endl
         << "   Does a scouting feature detection on all image combination, with" << endl
         << "   very small images, only does the more expensive matching " << endl
         << "   for the confirmed links."
         << endl
         << "   All the hard work is done by David Lowe's SIFT feature detector" << endl
         << endl
         << "Usage: " << name  << " [options] -o pto_prefix image1 image2 image3 ..." << endl
         << endl
         << "  [options] can be: " << endl
         << "     -o prefix   # prefix of the created hugin project files" << endl
         << "     -n number   # number of features per overlap, default: 20" << endl
         << "     -v number   # HFOV of images, in degrees, Used for images that do" << endl
         << "                   not provide this information in the EXIF header" << endl
         << "     -r number   # downscale factor for scouting. default: 0.05" << endl
         << "                   increase if not images are wrongly categorized." << endl
         << "     -k keypoint_exe # executable to use for keypoint detection" << endl
         << "     -s scale    # downscale images before final sift matching. default: 0.25" << endl
         << "                   while this seems crude, it is needed for speed" << endl
         << "                   and still gives good results" << endl;

}

int main(int argc, char *argv[])
{

    // parse arguments
    const char * optstring = "ho:n:o:v:r:s:k:";
    int c;

    opterr = 0;

    int nFeatures = 20;
    string outputPrefix("pano");
    double scoutScale = 0.05;
    double defaultHFOV = 40.0;
    double scaling = 0.25;
    string keypoints("keypoints");

    while ((c = getopt (argc, argv, optstring)) != -1) {
        switch (c) {
        case 'o':
            outputPrefix = optarg;
            break;
        case 'n':
            nFeatures = atoi(optarg);
            break;
        case 'v':
            defaultHFOV = atof(optarg);
            break;
        case 's':
            scaling = atof(optarg);
            break;
        case 'k':
            keypoints = optarg;
            break;
        case 'r':
            scoutScale = atof(optarg);
            break;
        case '?':
        case 'h':
            usage(argv[0]);
            return 1;
        default:
            abort ();
        }
    }
    unsigned int nImages = argc-optind;
    char **imgNames = argv+optind;

//    utils::CoutProgressDisplay pdisp;
    utils::StreamMultiProgressDisplay pdisp(cout);

    vector<string> fnames;
    for (unsigned int i=0; i<nImages; i++) {
        fnames.push_back(imgNames[i]);
    }

    // scout feature table
    SIFTFeatureTable scoutfTable;
    // feature table
    SIFTFeatureTable ftable;

    cerr << "starting exhaustive matching" << endl;
    // do a low resolution "recon" run, to find connected images
    extractSIFT2(fnames, scoutScale, scoutfTable, scaling, ftable, pdisp, keypoints);

    // matched features [img1][img2][matchnr] = [sqr_dist, feat1, feat2]
    SIFTMatchMatrix matches;

    // connection graph
    AdjListGraph imageGraph;

    cerr << "starting exhaustive matching, low resolution images" << endl;
    // match all images with each other
    MatchFeatures m;
//    RandomSampledMatcher<MatchFeatures> rmatcher(m,10);
    exhaustiveSiftMatching(scoutfTable,
                           m,
                           matches,
                           imageGraph,
                           pdisp);

    // nodes that start a subgraph
    vector<int> subgraphStart;

    // find individual subgraphs. (panoramas)
    findSubGraphs(imageGraph, subgraphStart);
    cerr << "found " << subgraphStart.size() << " panoramas" << endl;

    for (vector<int>::iterator panoit = subgraphStart.begin();
         panoit != subgraphStart.end() ; ++panoit)
    {
        int nPano = panoit - subgraphStart.begin();
        DEBUG_DEBUG("creating pano " << nPano + 1);
        // create a new panorama
        Panorama pano;

        // get list of all images in this pano.
        TrackVisitor getConn;
        traverseVertices(imageGraph, *panoit, getConn);

        // mapping from graph image numbers to pano image numbers
        map<int, int> graph2panoMapping;

        set<int> imgs = getConn.getTraversed();
        unsigned int subPanoSize = imgs.size();
        cerr << "pano " << nPano +1 << " consists of " << subPanoSize << " images";

        // add images to pano
        for (set<int>::iterator pit = imgs.begin(); pit != imgs.end() ; ++pit){
            graph2panoMapping[*pit] = pano.addImageAndLens(fnames[*pit], defaultHFOV);
        }

        // do sift matching for control points
        vigra_ext::SIFTMatchingVisitor sift2Matcher(graph2panoMapping,
//                                                   matcher,
                                                    ftable, 1/scaling, nFeatures,
                                                    pdisp);

        traverseEdges(imageGraph, *panoit, sift2Matcher);

        // get matching features.
        const CPVector cps = sift2Matcher.getCtrlPoints();
        for (CPVector::const_iterator cpit = cps.begin();
             cpit != cps.end(); ++cpit)
        {
            pano.addCtrlPoint(*cpit);
        }

        // TODO: estimate image positions, by computing pairwise
        // homographies. (could also be done by SIFTMatchingVisitor,
        // and stored in the graph edge). Then, it should be used here.

        // set sensible default options
        OptimizeVector optvec(subPanoSize);
        // fill optimize vector, just anchor one image.
        for (unsigned int im=1; im<subPanoSize; im++) {
            optvec[im].insert("y");
            optvec[im].insert("p");
            optvec[im].insert("r");
        }

        // write project
        ostringstream finame;
        finame << outputPrefix << "_" << nPano + 1 << ".pto";
        ofstream of(finame.str().c_str());
        UIntSet simgs = pano.getActiveImages();        
        pano.printOptimizerScript(of, optvec, pano.getOptions(), simgs);
    }
}
