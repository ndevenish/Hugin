// -*- c-basic-offset: 4 -*-

/** @file PTOptimise.cpp
 *
 *  @brief optimisation functions (wrappers around PTOptimizer)
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

//#define PT_CUSTOM_OPT

#include <config.h>
#include <fstream>

#include "PT/PTOptimise.h"
#include "PT/ImageGraph.h"

#include <boost/graph/graphviz.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_utility.hpp>

using namespace std;
using namespace PT;
using namespace PTools;
using namespace boost;
using namespace utils;

namespace PTools {

#ifdef PT_CUSTOM_OPT
    
struct MLOptFuncData
{
    MultiProgressDisplay * progDisp;
    bool terminate;
    int maxIter;
    AlignInfo * g;
};

static MLOptFuncData optdata;


// some functions that needed to be ripped from pano12 dll...

// Angular Distance of Control point "num"
double distSphere( int num ){
	double 		x, y ; 	// Coordinates of control point in panorama
	double		w2, h2;
	int j;
	Image sph;
	int n[2];
	struct 	MakeParams	mp;
	struct  fDesc 		stack[15];
	CoordInfo b[2];
	

	// Get image position in imaginary spherical image
	
	SetImageDefaults( &sph );
	
	sph.width 			= 360;
	sph.height 			= 180;
	sph.format			= _equirectangular;
	sph.hfov			= 360.0;
	
	n[0] = optdata.g->cpt[num].num[0];
	n[1] = optdata.g->cpt[num].num[1];
	
	// Calculate coordinates x/y in panorama

	for(j=0; j<2; j++){
		SetInvMakeParams( stack, &mp, &optdata.g->im[ n[j] ], &sph, 0 );
		
		h2 	= (double)optdata.g->im[ n[j] ].height / 2.0 - 0.5;
		w2	= (double)optdata.g->im[ n[j] ].width  / 2.0 - 0.5;
		
		
		execute_stack( 	(double)optdata.g->cpt[num].x[j] - w2,		// cartesian x-coordinate src
						(double)optdata.g->cpt[num].y[j] - h2,		// cartesian y-coordinate src
						&x, &y, stack);

		x = DEG_TO_RAD( x );
		y = DEG_TO_RAD( y ) + PI/2.0;
		b[j].x[0] =   sin(x) * sin( y );
		b[j].x[1] =   cos( y );
		b[j].x[2] = - cos(x) * sin(y);
	}
	
	return acos( SCALAR_PRODUCT( &b[0], &b[1] ) ) * optdata.g->pano.width / ( 2.0 * PI );
}

// Calculate the distance of Control Point "num" between two images
// in final pano

double distSquared( int num )
{
	double 		x[2], y[2]; 				// Coordinates of control point in panorama
	double		w2, h2;
	int j, n[2];
	double result;

	struct 	MakeParams	mp;
	struct  fDesc 		stack[15];

	

	n[0] = optdata.g->cpt[num].num[0];
	n[1] = optdata.g->cpt[num].num[1];
	
	// Calculate coordinates x/y in panorama

	for(j=0; j<2; j++)
	{
		SetInvMakeParams( stack, &mp, &optdata.g->im[ n[j] ], &optdata.g->pano, 0 );
		
		h2 	= (double)optdata.g->im[ n[j] ].height / 2.0 - 0.5;
		w2	= (double)optdata.g->im[ n[j] ].width  / 2.0 - 0.5;
		

		execute_stack( 	(double)optdata.g->cpt[num].x[j] - w2,		// cartesian x-coordinate src
						(double)optdata.g->cpt[num].y[j] - h2,		// cartesian y-coordinate src
						&x[j], &y[j], stack);
		// test to check if inverse works
#if 0
		{
			double xt, yt;
			struct 	MakeParams	mtest;
			struct  fDesc 		stacktest[15];
			SetMakeParams( stacktest, &mtest, &optdata.g->im[ n[j] ], &optdata.g->pano, 0 );
			execute_stack( 	x[j],		// cartesian x-coordinate src
							y[j],		// cartesian y-coordinate src
						&xt, &yt, stacktest);
			
			printf("x= %lg,	y= %lg,  xb = %lg, yb = %lg \n", optdata.g->cpt[num].x[j], optdata.g->cpt[num].y[j], xt+w2, yt+h2);
			
		}
#endif
	}
	
	
//	printf("Coordinates 0:   %lg:%lg	1:	%lg:%lg\n",x[0] + optdata.g->pano->width/2,y[0]+ optdata.g->pano->height/2, x[1] + optdata.g->pano->width/2,y[1]+ optdata.g->pano->height/2);


	// take care of wrapping and points at edge of panorama
	
	if( optdata.g->pano.hfov == 360.0 )
	{
		double delta = abs( x[0] - x[1] );
		
		if( delta > optdata.g->pano.width / 2 )
		{
			if( x[0] < x[1] )
				x[0] += optdata.g->pano.width;
			else
				x[1] += optdata.g->pano.width;
		}
	}


	switch( optdata.g->cpt[num].type )		// What do we want to optimize?
	{
		case 1:			// x difference
			result = ( x[0] - x[1] ) * ( x[0] - x[1] );
			break;
		case 2:			// y-difference
			result =  ( y[0] - y[1] ) * ( y[0] - y[1] );
			break;
		default:
			result = ( y[0] - y[1] ) * ( y[0] - y[1] ) + ( x[0] - x[1] ) * ( x[0] - x[1] ); // square of distance
			break;
	}
	

	return result;
}

void pt_getXY(int n, double x, double y, double *X, double *Y){
	struct 	MakeParams	mp;
	struct  fDesc 		stack[15];
	double h2,w2;

	SetInvMakeParams( stack, &mp, &optdata.g->im[ n ], &optdata.g->pano, 0 );
	h2 	= (double)optdata.g->im[ n ].height / 2.0 - 0.5;
	w2	= (double)optdata.g->im[ n ].width  / 2.0 - 0.5;


	execute_stack( 	x - w2,	y - h2,	X, Y, stack);
}

// Return distance of 2 lines
// The line through the two farthest apart points is calculated
// Returned is the distance of the other two points
double distLine(int N0, int N1){
	double x[4],y[4], del, delmax, A, B, C, mu, d0, d1;
	int n0, n1, n2, n3, i, k;

	pt_getXY(optdata.g->cpt[N0].num[0], (double)optdata.g->cpt[N0].x[0], (double)optdata.g->cpt[N0].y[0], &x[0], &y[0]);
	pt_getXY(optdata.g->cpt[N0].num[1], (double)optdata.g->cpt[N0].x[1], (double)optdata.g->cpt[N0].y[1], &x[1], &y[1]);
	pt_getXY(optdata.g->cpt[N1].num[0], (double)optdata.g->cpt[N1].x[0], (double)optdata.g->cpt[N1].y[0], &x[2], &y[2]);
	pt_getXY(optdata.g->cpt[N1].num[1], (double)optdata.g->cpt[N1].x[1], (double)optdata.g->cpt[N1].y[1], &x[3], &y[3]);

	delmax = 0.0;
	n0 = 0; n1 = 1;

	for(i=0; i<4; i++){
		for(k=i+1; k<4; k++){
			del = (x[i]-x[k])*(x[i]-x[k])+(y[i]-y[k])*(y[i]-y[k]);
			if(del>delmax){
				n0=i; n1=k; delmax=del;
			}
		}
	}
	if(delmax==0.0) return 0.0;

	for(i=0; i<4; i++){
		if(i!= n0 && i!= n1){
			n2 = i;
			break;
		}
	}
	for(i=0; i<4; i++){
		if(i!= n0 && i!= n1 && i!=n2){
			n3 = i;
		}
	}


	A=y[n1]-y[n0]; B=x[n0]-x[n1]; C=y[n0]*(x[n1]-x[n0])-x[n0]*(y[n1]-y[n0]);

	mu=1.0/sqrt(A*A+B*B);

	d0 = (A*x[n2]+B*y[n2]+C)*mu;
	d1 = (A*x[n3]+B*y[n3]+C)*mu;

	return d0*d0 + d1*d1;

}



// Levenberg-Marquardt function measuring the quality of the fit in fvec[]

// taken from adjust.c of pano tools by H. Dersch.
// removed GUI stuff, and replaced with ProgressDisplay interface
int fcnPano2(int m,int n, double * x, double * fvec, int * iflag)
/*
int fcnPano2(m, n, x, fvec, iflag)
int m;
int n;
double * x;
double * fvec;
int * iflag
*/
{
//#pragma unused(n)
    int i;
    static int numIt;
    double result;

    if( *iflag == -100 ){ // reset
        numIt = 0;
        optdata.progDisp->pushTask(ProgressTask("Optimizing","",0));
//		infoDlg ( _initProgress, "Optimizing Variables" );
        return 0;
    }
    if( *iflag == -99 ){ //
        optdata.progDisp->popTask();
//		infoDlg ( _disposeProgress, "" );
        return 0;
    }


    if( *iflag == 0 ) {
        char message[256];
		
        result = 0.0;
        for( i=0; i < optdata.g->numPts; i++) {
            result += fvec[i] ;
        }
        result = sqrt( result/ (double)optdata.g->numPts );
		
        sprintf( message, "Avg. Error after %d iter: %g pixels", numIt,result);//average);
        numIt += 10;

        optdata.progDisp->setMessage(message);

        // termination criteria?
        if (optdata.maxIter > 0 && numIt > optdata.maxIter)
            *iflag = -1;
        if (optdata.terminate)
            *iflag = -1;
        return 0;
    }

    // Set Parameters

    SetAlignParams( x ) ;
	
    // Calculate distances
	
    result = 0.0;
    for( i=0; i < optdata.g->numPts; i++){
        int j;
        switch(optdata.g->cpt[i].type){
        case 0:
            fvec[i] = distSphere( i );
            break;
        case 1:
        case 2:
            fvec[i] = distSquared( i );
            break;
        default:
            for(j=0; j<optdata.g->numPts; j++){
                if(j!=i && optdata.g->cpt[i].type == optdata.g->cpt[j].type){
                    fvec[i] = distLine(i,j);
                    break;
                }
            }
            break;
        }
        result += fvec[i] ;
    }
    result = result/ (double)optdata.g->numPts;
	
    for( i=optdata.g->numPts; i < m; i++) {
        fvec[i] = result ;
    }
    return 0;
}


# endif

} // namespace

void PTools::optimize(const Panorama & pano,
                      const PT::UIntVector &imgs,
                      const OptimizeVector & optvec,
                      VariableMapVector & vars,
                      CPVector & cps,
                      utils::MultiProgressDisplay & progDisplay,
                      int maxIter)
{
//    VariableMapVector res;
    // setup data structures
    aPrefs    aP;
    OptInfo   opt;

    SetAdjustDefaults(&aP);
    AlignInfoWrap aInfo;
    // copy pano information int libpano data structures
    if (aInfo.setInfo(pano, imgs, vars, cps, optvec)) {
        aInfo.setGlobal();

        opt.numVars 		= aInfo.gl.numParam;
        opt.numData 		= aInfo.gl.numPts;
        opt.SetVarsToX		= SetLMParams;
        opt.SetXToVars		= SetAlignParams;
        opt.fcn			= aInfo.gl.fcn;
        *opt.message		= 0;

        DEBUG_DEBUG("starting optimizer");
        RunLMOptimizer( &opt );
        std::ostringstream oss;
        oss << "optimizing images";
        for (UIntVector::const_iterator it = imgs.begin(); it != imgs.end(); ++it) {
            if (it + 1 != imgs.end()) {
                oss << *it << ",";
            } else {
                oss << *it;
            }
        }
        oss << "\n" << opt.message;
        progDisplay.setMessage(oss.str());
        DEBUG_DEBUG("optimizer finished:" << opt.message);

        vars = aInfo.getVariables();
        cps = aInfo.getCtrlPoints();
    }
}

/** autooptimise the panorama (does local optimisation first) */
PT::VariableMapVector PTools::autoOptimise(const PT::Panorama & pano,
                                           const std::set<std::string> & optvars,
                                           CPVector & cps,
                                           utils::MultiProgressDisplay & progDisp)
{
// DGSW FIXME - Unreferenced
//	unsigned nImg = unsigned(pano.getNrOfImages());
    // build a graph over all overlapping images
    CPGraph graph;
    createCPGraph(pano,graph);

    
#if DEBUG
    {
        ofstream gfile("cp_graph.dot");
        // output doxygen graph
        boost::write_graphviz(gfile, graph);
    }
#endif

    unsigned int startImg = pano.getOptions().optimizeReferenceImage;

    // start a breadth first traversal of the graph, and optimize
    // the links found (every vertex just once.)

    PT::VariableMapVector vars = pano.getVariables();
    PTools::OptimiseVisitor optVisitor(pano, optvars, vars, cps, progDisp);

    boost::queue<boost::graph_traits<CPGraph>::vertex_descriptor> qu;
    progDisp.pushTask(utils::ProgressTask("", "",0));
    boost::breadth_first_search(graph, startImg,
                                color_map(get(vertex_color, graph)).
                                visitor(optVisitor));
    // iterate over all image combinations.

    progDisp.popTask();
    return vars;
}


void test__()
    {
        
    }

#ifdef PT_CUSTOM_OPT
void PTools::stopOptimiser()
{
    optdata.terminate = true;
}
#endif

